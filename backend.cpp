#include "backend.h"

#ifdef RIF_USE_METAL
#define BACKEND_TYPE RIF_BACKEND_API_METAL
#elif defined RIF_USE_DX12
#define BACKEND_TYPE RIF_BACKEND_API_DIRECTX12
#else
#define BACKEND_TYPE RIF_BACKEND_API_OPENCL
#endif // RIF_USE_METAL

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "RadeonImageFilter/samples/ImageTools/stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "RadeonImageFilter/samples/ImageTools/stb_image.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-c-typedef-for-linkage"
#include "RadeonImageFilter/samples/ImageTools/ImageTools.h"
#pragma clang diagnostic pop

rif_context g_pContext = nullptr;
int g_nDeviceID = -1;
rif_command_queue g_pCommandQueue = nullptr;

FilterDesc* g_pFilterDesc;
rif_image_filter* g_pFilters = nullptr;
rif_image* g_pImages = nullptr;

#define SET_ERROR_AND_RETURN(ret, msg, ...) if (ret != RIF_SUCCESS) { g_strLastBackendError = Format(msg, __VA_ARGS__); return ret; }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template<typename T>
void PremultiplyPixel(RGBA* pDest, T* pSrc)
{
	if constexpr (std::is_base_of<Pixel_Uint8, T>::value)
	{
		if constexpr (std::is_same<PixelGA_Uint8, T>::value)
		{
			if (pSrc->a == 255)
			{
				pDest->a = 255;
				pDest->r = pDest->g = pDest->b = pSrc->g;
			}
			else if (pSrc->a == 0)
			{
				pDest->a = pDest->r = pDest->g = pDest->b = 0;
			}
			else
			{
				pDest->a = pSrc->a;
				
				int nAlpha = pDest->a + (pDest->a >> 7);
				pDest->r = pDest->g = pDest->b = (nAlpha * pSrc->g) >> 8;
			}
		}
		else if constexpr (std::is_same<PixelRGBA_Uint8, T>::value)
		{
			if (pSrc->a == 255)
			{
				pDest->a = 255;
				pDest->r = pSrc->r;
				pDest->g = pSrc->g;
				pDest->b = pSrc->b;
			}
			else if (pSrc->a == 0)
			{
				pDest->a = pDest->r = pDest->g = pDest->b = 0;
			}
			else
			{
				pDest->a = pSrc->a;
				
				int nAlpha = pDest->a + (pDest->a >> 7);
				pDest->r = (nAlpha * pSrc->r) >> 8;
				pDest->g = (nAlpha * pSrc->g) >> 8;
				pDest->b = (nAlpha * pSrc->b) >> 8;
			}
		}
	}
	else if constexpr (std::is_base_of<Pixel_Float, T>::value)
	{
		if constexpr (std::is_same<PixelGA_Float16, T>::value || std::is_same<PixelGA_Float32, T>::value )
		{
			if (pSrc->a == 1.0f)
			{
				pDest->a = 255;
				pDest->r = pDest->g = pDest->b = pSrc->g * 255.0f;
			}
			else if (pSrc->a == 0)
			{
				pDest->a = pDest->r = pDest->g = pDest->b = 0;
			}
			else
			{
				pDest->a = pSrc->a;
				pDest->r = pDest->g = pDest->b = pSrc->g * pSrc->a * 255.0f;
			}
		}
		else if constexpr (std::is_same<PixelRGBA_Float16, T>::value || std::is_same<PixelRGBA_Float32, T>::value)
		{
			if (pSrc->a == 1.0f)
			{
				pDest->a = 255;
				pDest->r = pSrc->r * 255.0f;
				pDest->g = pSrc->g * 255.0f;
				pDest->b = pSrc->b * 255.0f;
			}
			else if (pSrc->a == 0)
			{
				pDest->a = pDest->r = pDest->g = pDest->b = 0;
			}
			else
			{
				pDest->a = pSrc->a;
				pDest->r = pSrc->r * pSrc->a * 255.0f;
			}
		}
	}
}

template<typename T>
void RIF_TransferImage(ImageBuffer* pBuffer, void* pRawData, unsigned int dwHeight, unsigned int dwWidth)
{
	T* pTypedData = (T*) pRawData;
			
	for (unsigned int y = 0; y < dwHeight; y++)
	{
		for (unsigned int x = 0; x < dwWidth; x++)
		{
			unsigned int dwPos = y * dwWidth + x;
					
			T* pSrc = pTypedData + dwPos;
			RGBA* pDest = pBuffer->Begin() + dwPos;
					
			if constexpr (std::is_base_of<Pixel_Uint8, T>::value)
			{
				if constexpr (std::is_same<PixelG_Uint8, T>::value)
				{
					pDest->a = 255;
					pDest->r = pDest->g = pDest->b = pSrc->g;
				}
				else if constexpr (std::is_same<PixelGA_Uint8, T>::value)
				{
					PremultiplyPixel(pDest, pSrc);
				}
				else if constexpr (std::is_same<PixelRGB_Uint8, T>::value)
				{
					pDest->a = 255;
					pDest->r = pSrc->r;
					pDest->g = pSrc->g;
					pDest->b = pSrc->b;
				}
				else if constexpr (std::is_same<PixelRGBA_Uint8, T>::value)
				{
					PremultiplyPixel(pDest, pSrc);
				}
			}
			else if constexpr (std::is_base_of<Pixel_Float, T>::value)
			{
				if constexpr (std::is_same<PixelG_Float16, T>::value || std::is_same<PixelG_Float32, T>::value)
				{
					pDest->a = 255;
					pDest->r = pDest->g = pDest->b = pSrc->g * 255.0f;
				}
				else if constexpr (std::is_same<PixelGA_Float16, T>::value || std::is_same<PixelGA_Float32, T>::value)
				{
					PremultiplyPixel(pDest, pSrc);
				}
				else if constexpr (std::is_same<PixelRGB_Float16, T>::value || std::is_same<PixelRGB_Float32, T>::value)
				{
					pDest->a = 255;
					pDest->r = pSrc->r * 255.0f;
					pDest->g = pSrc->g * 255.0f;
					pDest->b = pSrc->b * 255.0f;
				}
				else if constexpr (std::is_same<PixelRGBA_Float16, T>::value || std::is_same<PixelRGBA_Float32, T>::value)
				{
					PremultiplyPixel(pDest, pSrc);
				}
			}
		}
	}
}
#pragma clang diagnostic pop

int RIF_LoadImageIntoBuffer(const String& strFilename, ImageBuffer** ppBuffer, rif_image_desc* pDesc)
{
	rif_image pImage = ImageTools::LoadImage(strFilename.ToStd(), g_pContext);
	if (!pImage)
	{
		g_strLastBackendError = Format(g_strBA_LoadImageFailPattern, strFilename);
		return RIF_ERROR_INVALID_IMAGE;
	}
	
	memset(pDesc, 0, sizeof(rif_image_desc));
	
	size_t retSize;
	int nRet = rifImageGetInfo(pImage, RIF_IMAGE_DESC, sizeof(rif_image_desc), pDesc, &retSize);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetImageInfoFailPattern, strFilename, rifGetErrorCodeString(nRet));
	
	void* pRawData;
    nRet = rifImageMap(pImage, RIF_IMAGE_MAP_READ, &pRawData);
    SET_ERROR_AND_RETURN(nRet, g_strBA_GetImageMapFailPattern, strFilename, rifGetErrorCodeString(nRet));
	
	*ppBuffer = new ImageBuffer(pDesc->image_width, pDesc->image_height);

	switch (pDesc->type)
	{
		case RIF_COMPONENT_TYPE_UINT8:
			switch (pDesc->num_components)
			{
				case 1: RIF_TransferImage<PixelG_Uint8>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 2: RIF_TransferImage<PixelGA_Uint8>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 3: RIF_TransferImage<PixelRGB_Uint8>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 4: RIF_TransferImage<PixelRGBA_Uint8>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
			}
		break;
		
		case RIF_COMPONENT_TYPE_FLOAT16:
			switch (pDesc->num_components)
			{
				case 1: RIF_TransferImage<PixelG_Float16>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 2: RIF_TransferImage<PixelGA_Float16>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 3: RIF_TransferImage<PixelRGB_Float16>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 4: RIF_TransferImage<PixelRGBA_Float16>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
			}
		break;
		
		case RIF_COMPONENT_TYPE_FLOAT32:
			switch (pDesc->num_components)
			{
				case 1: RIF_TransferImage<PixelG_Float32>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 2: RIF_TransferImage<PixelGA_Float32>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 3: RIF_TransferImage<PixelRGB_Float32>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
				case 4: RIF_TransferImage<PixelRGBA_Float32>(*ppBuffer, pRawData, pDesc->image_height, pDesc->image_width); break;
			}
		break;
	}
	
	if (pDesc->num_components % 2)
		(*ppBuffer)->SetKind(IMAGE_OPAQUE);
	else
		(*ppBuffer)->SetKind(IMAGE_ALPHA);
		
	rifImageUnmap(pImage, pRawData);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetImageUnmapFailPattern, strFilename, rifGetErrorCodeString(nRet));
	
	rifObjectDelete(pImage);
	
	return RIF_SUCCESS;
}

int RIF_GetDeviceCount(int* pnDeviceCount)
{
	int nRet = rifGetDeviceCount(BACKEND_TYPE, pnDeviceCount);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetDeviceCountFailPattern, rifGetErrorCodeString(nRet))

	return RIF_SUCCESS;
}

int RIF_GetDeviceName(int nDeviceID, String& strDeviceName)
{
	char anBuffer[BUFFER_SIZE_DEFAULT] = { 0 };
	size_t nActualSize = 0;
	
	int nRet = rifGetDeviceInfo(BACKEND_TYPE, nDeviceID, RIF_DEVICE_NAME, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetDeviceInfoFailPattern, nDeviceID, rifGetErrorCodeString(nRet))
	
	strDeviceName = anBuffer;
	return RIF_SUCCESS;
}

int RIF_AttachLog(int nLevel, const String& strFilename)
{
	if (!strFilename.IsEmpty())
	{
		rif_logger_desc desc;
		
		desc.level = nLevel;
		desc.message.filename = strFilename;
		desc.message.handle = nullptr;
		desc.message.rdbuf = nullptr;
		desc.error.filename = strFilename;
		desc.error.handle = nullptr;
		desc.error.rdbuf = nullptr;
		
		int nRet = rifLoggerAttach(&desc);
		SET_ERROR_AND_RETURN(nRet, g_strBA_AttachLogFailPatter, rifGetErrorCodeString(nRet))
	}
	
	return RIF_SUCCESS;
}

void RIF_DeleteQueue()
{
	if (g_pCommandQueue)
	{
		rifFlushQueue(g_pCommandQueue);
		rifObjectDelete(g_pCommandQueue);
		g_pCommandQueue = nullptr;
	}
}

void RIF_DeleteContext()
{
	g_nDeviceID = -1;
	if (g_pContext)
	{
		rifObjectDelete(g_pContext);
		g_pContext = nullptr;
	}
}

void RIF_DeleteFilters()
{
	if (!g_pFilterDesc || !g_pFilters)
		return;
	
	for (int i = 0; i < g_pFilterDesc->GetCommandCount(); i++)
	{
		if (g_pFilters[i])
			rifObjectDelete(g_pFilters[i]);
	}
	
	delete[] g_pFilters;
	g_pFilters = nullptr;
	g_pFilterDesc = nullptr;
}

void RIF_DeleteFilter(rif_image_filter pFilter)
{
	rifObjectDelete(pFilter);
}

int RIF_CreateContextAndQueue(int nDeviceID, const String& strCachePath)
{
	if (g_nDeviceID != nDeviceID)
	{
		RIF_DeleteQueue();
		RIF_DeleteContext();
		
		int nRet = rifCreateContext(RIF_API_VERSION, BACKEND_TYPE, nDeviceID, strCachePath, &g_pContext);
		SET_ERROR_AND_RETURN(nRet, g_strBA_CreateContextFailPattern, rifGetErrorCodeString(nRet))
		
		nRet = rifContextCreateCommandQueue(g_pContext, &g_pCommandQueue);
		SET_ERROR_AND_RETURN(nRet, g_strBA_CreateCommandQueueFailPattern, rifGetErrorCodeString(nRet))
		
		g_nDeviceID = nDeviceID;
	}
	
	return RIF_SUCCESS;
}

int RIF_CreateImageFilters(FilterDesc* pFilterDesc)
{
	if (pFilterDesc != g_pFilterDesc)
	{
		RIF_DeleteFilters();
		g_pFilterDesc = pFilterDesc;
		
		int nCommandCount = pFilterDesc->GetCommandCount();
		g_pFilters = new rif_image_filter[nCommandCount];
		
		int nRet;
		for (int i = 0; i < nCommandCount; i++)
		{
			g_pFilters[i] = nullptr;
			unsigned int dwFilter = pFilterDesc->m_adwCommands[i];
			
			nRet = rifContextCreateImageFilter(g_pContext, dwFilter, &g_pFilters[i]);
			SET_ERROR_AND_RETURN(nRet, g_strBA_CreateImageFilterFailPattern, (int)dwFilter, rifGetErrorCodeString(nRet))
			
			nRet = rifImageFilterSetComputeType(g_pFilters[i], pFilterDesc->m_adwComputeTypes[i]);
			SET_ERROR_AND_RETURN(nRet, g_strBA_SetComputeTypeFailPattern, (int)dwFilter, rifGetErrorCodeString(nRet))
			
			nRet = pFilterDesc->m_apSetParamsFuncs[i](i);
			if (nRet != RIF_SUCCESS)
				return nRet;
		}
	}
	
	return RIF_SUCCESS;
}

int RIF_CreateObjects(int nDeviceID, const String& strCachePath, FilterDesc* pFilterDesc)
{
	int nRet = RIF_CreateContextAndQueue(nDeviceID, strCachePath);
	if (nRet != RIF_SUCCESS)
		return nRet;
	
	return RIF_CreateImageFilters(pFilterDesc);
}

void RIF_DeleteObjects()
{
	RIF_DeleteFilters();
	RIF_DeleteQueue();
	RIF_DeleteContext();
}

String ParamBufferToString(const char* pBuffer, unsigned int dwType)
{
	String strRet;
	switch (dwType)
	{
		case RIF_PARAMETER_TYPE_FLOAT1:
		{
			float* pValue = *(float**)pBuffer;
			strRet = Format("%f", *pValue);
			break;
		}
		case RIF_PARAMETER_TYPE_UINT1:
		{
			unsigned int* pValue = *(unsigned int**)pBuffer;
			strRet << *pValue;
			break;
		}
		
		case RIF_PARAMETER_TYPE_INT1:
		{
			int* pValue = *(int**)pBuffer;
			strRet << *pValue;
			break;
		}
					
		case RIF_PARAMETER_TYPE_STRING:
		{
			strRet = *(char**)pBuffer;
			break;
		}
	}
	
	return strRet;
}

int RIF_FillCustomFilterParameters(int nDeviceID, const String& strCachePath, FilterDesc* pFilterDesc, VectorMap<String, 
	String>& mapParams, Vector<String>& astrCommandAliases, rif_image_filter* ppFilter)
{
	int nRet = RIF_CreateContextAndQueue(nDeviceID, strCachePath);
	if (nRet != RIF_SUCCESS)
		return nRet;
	
	char anBuffer[BUFFER_SIZE_DEFAULT] = { 0 };
	size_t nActualSize = 0;
	
	for (int i = 0; i < pFilterDesc->GetCommandCount(); i++)
	{
		rif_image_filter pFilter = nullptr;
		unsigned int dwFilter = pFilterDesc->m_adwCommands[i];
		
		nRet = rifContextCreateImageFilter(g_pContext, dwFilter, &pFilter);
		SET_ERROR_AND_RETURN(nRet, g_strBA_CreateImageFilterFailPattern, (int)dwFilter, rifGetErrorCodeString(nRet))
		
		*ppFilter = pFilter;
		
		nRet = rifImageFilterGetInfo(pFilter, RIF_IMAGE_FILTER_PARAMETER_COUNT, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
		SET_ERROR_AND_RETURN(nRet, g_strBA_GetFilterInfoFailPattern, (int)dwFilter, rifGetErrorCodeString(nRet))
		unsigned int dwParamCount = *(unsigned int*)anBuffer;
	
		for (unsigned int j = 0; j < dwParamCount; j++)
		{
			rifParameterGetInfo(pFilter, j, RIF_PARAMETER_NAME_STRING, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
			SET_ERROR_AND_RETURN(nRet, g_strBA_GetParamInfoFailPattern, (int)dwFilter, (int)j, rifGetErrorCodeString(nRet))
			String strCurName = anBuffer;
			
			for(const KeyValueRef<String, String>& kvrParam : ~mapParams)
			{
				int nDotIndex = kvrParam.key.Find(".");
				if (nDotIndex > -1)
				{
					String strCommandAlias = kvrParam.key.Left(nDotIndex);
			
					int nAliasIndex = GetVectorIndex(astrCommandAliases, strCommandAlias);
					if (nAliasIndex > -1 && pFilterDesc->m_adwCommands[nAliasIndex] == dwFilter)
					{
						String strName = kvrParam.key.Mid(nDotIndex + 1);
						
						if (strName == strCurName)
						{
							rifParameterGetInfo(pFilter, j, RIF_PARAMETER_TYPE, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
							SET_ERROR_AND_RETURN(nRet, g_strBA_GetParamInfoFailPattern, (int)dwFilter, (int)j, rifGetErrorCodeString(nRet))
							unsigned int dwType = *(unsigned int*)anBuffer;
							
							rifParameterGetInfo(pFilter, j, RIF_PARAMETER_DESCRIPTION, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
							SET_ERROR_AND_RETURN(nRet, g_strBA_GetParamInfoFailPattern, (int)dwFilter, (int)j, rifGetErrorCodeString(nRet))
							
							String strDesc = Format(g_strCustomFilterParamDescPattern, strCommandAlias, anBuffer);
							
							FilterParam* pParam = new FilterParam(dwType, strName, kvrParam.value, kvrParam.value,
								strDesc, dwFilter);
								
							pFilterDesc->m_mapParams.Add(kvrParam.key, pParam);
						}
					}
				}
			}
		}
			
		*ppFilter = nullptr;
		rifObjectDelete(pFilter);
	}
	
	return RIF_SUCCESS;
}

int RIF_FillBaseFilterParameters(int nDeviceID, const String& strCachePath, FilterDesc* pFilterDesc,
	VectorMap<String, String>& mapDefaults, rif_image_filter* ppFilter)
{
	int nRet = RIF_CreateContextAndQueue(nDeviceID, strCachePath);
	if (nRet != RIF_SUCCESS)
		return nRet;
	
	rif_image_filter pFilter = nullptr;
	unsigned int dwFilter = pFilterDesc->m_adwCommands[0];
	
	nRet = rifContextCreateImageFilter(g_pContext, dwFilter, &pFilter);
	SET_ERROR_AND_RETURN(nRet, g_strBA_CreateImageFilterFailPattern, (int)dwFilter, rifGetErrorCodeString(nRet))
	
	*ppFilter = pFilter;
	char anBuffer[BUFFER_SIZE_DEFAULT] = { 0 };
	size_t nActualSize = 0;
	
	nRet = rifImageFilterGetInfo(pFilter, RIF_IMAGE_FILTER_PARAMETER_COUNT, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetFilterInfoFailPattern, (int)dwFilter, rifGetErrorCodeString(nRet))
	unsigned int dwParamCount = *(unsigned int*)anBuffer;
	
	nRet = rifImageFilterGetInfo(pFilter, RIF_IMAGE_FILTER_DESCRIPTION, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetFilterInfoFailPattern, (int)dwFilter, rifGetErrorCodeString(nRet))
	pFilterDesc->m_strDescription = anBuffer;
	
	nRet = rifImageFilterGetInfo(pFilter, RIF_IMAGE_FILTER_JOINABLE, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetFilterInfoFailPattern, (int)dwFilter, rifGetErrorCodeString(nRet))
	pFilterDesc->m_bJoinable = *(unsigned int*)anBuffer;
		
	for (unsigned int i = 0; i < dwParamCount; i++)
	{
		rifParameterGetInfo(pFilter, i, RIF_PARAMETER_NAME_STRING, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
		SET_ERROR_AND_RETURN(nRet, g_strBA_GetParamInfoFailPattern, (int)dwFilter, (int)i, rifGetErrorCodeString(nRet))
		String strName = anBuffer;
	
		rifParameterGetInfo(pFilter, i, RIF_PARAMETER_TYPE, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
		SET_ERROR_AND_RETURN(nRet, g_strBA_GetParamInfoFailPattern, (int)dwFilter, (int)i, rifGetErrorCodeString(nRet))
		unsigned int dwType = *(unsigned int*)anBuffer;
		
		rifParameterGetInfo(pFilter, i, RIF_PARAMETER_DESCRIPTION, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
		SET_ERROR_AND_RETURN(nRet, g_strBA_GetParamInfoFailPattern, (int)dwFilter, (int)i, rifGetErrorCodeString(nRet))
		String strDesc = anBuffer;
				
		rifParameterGetInfo(pFilter, i, RIF_PARAMETER_VALUE, BUFFER_SIZE_DEFAULT, anBuffer, &nActualSize);
		SET_ERROR_AND_RETURN(nRet, g_strBA_GetParamInfoFailPattern, (int)dwFilter, (int)i, rifGetErrorCodeString(nRet))
		String strDefaultValue = ParamBufferToString(anBuffer, dwType);
		
		int nIndex = mapDefaults.Find(g_strIgnoreParameterTag + strName);
		if (nIndex > -1)
			continue;
		
		nIndex = mapDefaults.Find(strName);
		String strValue = (nIndex > -1) ? mapDefaults[nIndex] : strDefaultValue;
	
		FilterParam* pParam = new FilterParam(dwType, strName, strValue, strDefaultValue, strDesc, dwFilter);
		pFilterDesc->m_mapParams.Add(strName, pParam);
	}
	
	*ppFilter = nullptr;
	rifObjectDelete(pFilter);
		
	return RIF_SUCCESS;
}

int CreateImage_1x(rif_image_desc* pDesc, rif_image * ppImage)
{
	return rifContextCreateImage(g_pContext, pDesc, nullptr, ppImage);
}

int CreateImage_2x(rif_image_desc* pDesc, rif_image * ppImage)
{
	pDesc->image_width <<= 1;
	pDesc->image_height <<= 1;
	
	return rifContextCreateImage(g_pContext, pDesc, nullptr, ppImage);
}

int CreateImages_Full(const String& strFilename)
{
	int nCommandCount = g_pFilterDesc->GetCommandCount();
	g_pImages = new rif_image[nCommandCount + 1];
	
	g_pImages[0] = ImageTools::LoadImage(strFilename.ToStd(), g_pContext);
	if (!g_pImages[0])
	{
		g_strLastBackendError = Format(g_strBA_LoadImageFailPattern, strFilename);
		return RIF_ERROR_INVALID_IMAGE;
	}
	
	rif_image_desc desc;
	size_t retSize;
	int nRet = rifImageGetInfo(g_pImages[0], RIF_IMAGE_DESC, sizeof(desc), &desc, &retSize);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetImageInfoFailPattern, strFilename, rifGetErrorCodeString(nRet))
	
	for (int i = 1; i < nCommandCount + 1; i++)
	{
		nRet = g_pFilterDesc->m_apCreateImageFuncs[i - 1](&desc, &g_pImages[i]);
		SET_ERROR_AND_RETURN(nRet, g_strBA_CreateImageFailPattern, strFilename, rifGetErrorCodeString(nRet))
	}
	
	return RIF_SUCCESS;
}

int CreateImages_Simple(const String& strFilename)
{
	int nCommandCount = g_pFilterDesc->GetCommandCount();
	g_pImages = new rif_image[2];
	
	g_pImages[0] = ImageTools::LoadImage(strFilename.ToStd(), g_pContext);
	if (!g_pImages[0])
	{
		g_strLastBackendError = Format(g_strBA_LoadImageFailPattern, strFilename);
		return RIF_ERROR_INVALID_IMAGE;
	}
	
	rif_image_desc desc;
	size_t retSize;
	int nRet = rifImageGetInfo(g_pImages[0], RIF_IMAGE_DESC, sizeof(desc), &desc, &retSize);
	SET_ERROR_AND_RETURN(nRet, g_strBA_GetImageInfoFailPattern, strFilename, rifGetErrorCodeString(nRet))
	
	nRet = g_pFilterDesc->m_apCreateImageFuncs[0](&desc, &g_pImages[1]);
	SET_ERROR_AND_RETURN(nRet, g_strBA_CreateImageFailPattern, strFilename, rifGetErrorCodeString(nRet))
	
	return RIF_SUCCESS;
}

int SetParameter(int nIndex, const String& strName, FilterParam* pParam)
{
	int nRet;
	switch (pParam->m_dwType)
	{
		case RIF_PARAMETER_TYPE_FLOAT1: nRet = rifImageFilterSetParameter1f(g_pFilters[nIndex], strName, Atof(pParam->m_strValue)); break;
		case RIF_PARAMETER_TYPE_UINT1: nRet = rifImageFilterSetParameter1u(g_pFilters[nIndex], strName, atol(pParam->m_strValue)); break;
		case RIF_PARAMETER_TYPE_INT1: nRet = rifImageFilterSetParameter1i(g_pFilters[nIndex], strName, atoi(pParam->m_strValue)); break;
					
		case RIF_PARAMETER_TYPE_STRING:
		default: nRet = rifImageFilterSetParameterString(g_pFilters[nIndex], strName, pParam->m_strValue); break;
	}
	
	SET_ERROR_AND_RETURN(nRet, g_strBA_SetFilterParamFailPattern, (int)g_pFilterDesc->m_adwCommands[nIndex], strName,
		pParam->GetTypeStr(), rifGetErrorCodeString(nRet))
	return RIF_SUCCESS;
}

int SetParameters_Simple(int nIndex)
{
	for(const KeyValueRef<String, FilterParam*>& kvrPair : ~g_pFilterDesc->m_mapParams)
	{
		FilterParam* pParam = kvrPair.value;
		if (pParam->m_dwFilter == g_pFilterDesc->m_adwCommands[nIndex])
		{
			if (pParam->m_strValue == pParam->m_strDefaultValue)
				continue;
			
			int nResult = SetParameter(nIndex, pParam->m_strName, pParam);
					
			if (nResult != RIF_SUCCESS)
				return nResult;
		}
	}
			
	return RIF_SUCCESS;
}

void FilterCleanup_Full(int nAttachedFilters)
{
	for (int i = 0; i < nAttachedFilters; i++)
		rifCommandQueueDetachImageFilter(g_pCommandQueue, g_pFilters[i]);
	
	rifFlushQueue(g_pCommandQueue);
	
	for (int i = 0; i < g_pFilterDesc->GetCommandCount() + 1; i++)
	{
		if (g_pImages[i])
			rifObjectDelete(g_pImages[i]);
	}
	
	delete[] g_pImages;
	g_pImages = nullptr;
}

int FilterExecute_Full(const String& strFilename, const String& strOutFilename, rif_performance_statistic* pPerformanceStat)
{
	int nRet = CreateImages_Full(strFilename);
	if (nRet != RIF_SUCCESS)
	{
		FilterCleanup_Full(0);
		return nRet;
	}
	
	int nCommandCount = g_pFilterDesc->GetCommandCount();
	for (int i = 0; i < nCommandCount; i++)
	{
		nRet = rifCommandQueueAttachImageFilter(g_pCommandQueue, g_pFilters[i], g_pImages[i], g_pImages[i + 1]);
		
		if (nRet != RIF_SUCCESS)
		{
			FilterCleanup_Full(i);
			g_strLastBackendError = Format(g_strBA_AttachFilterToQueueFailPattern, (int)g_pFilterDesc->m_adwCommands[i],
				rifGetErrorCodeString(nRet));
			return nRet;
		}
	}
	
	nRet = rifContextExecuteCommandQueue(g_pContext, g_pCommandQueue, nullptr, nullptr, pPerformanceStat);
	if (nRet != RIF_SUCCESS)
	{
		FilterCleanup_Full(nCommandCount);
		g_strLastBackendError = Format(g_strBA_ExecuteCommandQueueFailPattern, rifGetErrorCodeString(nRet));
		return nRet;
	}
	
	for (int i = 1; i < nCommandCount + 1; i++)
	{
		String strFinalOutFilename = Format(strOutFilename, i - 1);
		nRet = ImageTools::ImageSaveToFile(g_pImages[i], strFinalOutFilename);
		if (nRet != RIF_SUCCESS)
		{
			FilterCleanup_Full(nCommandCount);
			g_strLastBackendError = Format(g_strBA_SaveImageFailPattern, strFinalOutFilename);
			return nRet;
		}
	}
	
	FilterCleanup_Full(nCommandCount);
	return RIF_SUCCESS;
}

void FilterCleanup_Simple(int nAttachedFilters)
{
	for (int i = 0; i < nAttachedFilters; i++)
		rifCommandQueueDetachImageFilter(g_pCommandQueue, g_pFilters[i]);
	
	rifFlushQueue(g_pCommandQueue);
	
	if (g_pImages[0])
		rifObjectDelete(g_pImages[0]);
	
	if (g_pImages[1])
		rifObjectDelete(g_pImages[1]);
	
	delete[] g_pImages;
	g_pImages = nullptr;
}

int FilterExecute_Simple(const String& strFilename, const String& strOutFilename, rif_performance_statistic* pPerformanceStat)
{
	int nRet = CreateImages_Simple(strFilename);
	if (nRet != RIF_SUCCESS)
	{
		FilterCleanup_Simple(0);
		return nRet;
	}
	
	int anIndices[] = { 0, 1, 0 };
	int nCommandCount = g_pFilterDesc->GetCommandCount();
	for (int i = 0; i < nCommandCount; i++)
	{
		int nIndex = i % 2;
		nRet = rifCommandQueueAttachImageFilter(g_pCommandQueue, g_pFilters[i], g_pImages[nIndex], g_pImages[anIndices[nIndex + 1]]);
		
		if (nRet != RIF_SUCCESS)
		{
			FilterCleanup_Simple(i);
			g_strLastBackendError = Format(g_strBA_AttachFilterToQueueFailPattern, (int)g_pFilterDesc->m_adwCommands[i],
				rifGetErrorCodeString(nRet));
			return nRet;
		}
	}
	
	nRet = rifContextExecuteCommandQueue(g_pContext, g_pCommandQueue, nullptr, nullptr, pPerformanceStat);
	if (nRet != RIF_SUCCESS)
	{
		FilterCleanup_Simple(nCommandCount);
		g_strLastBackendError = Format(g_strBA_ExecuteCommandQueueFailPattern, rifGetErrorCodeString(nRet));
		return nRet;
	}
	
	nRet = ImageTools::ImageSaveToFile(g_pImages[nCommandCount % 2], strOutFilename);
	if (nRet != RIF_SUCCESS)
	{
		FilterCleanup_Simple(nCommandCount);
		g_strLastBackendError = Format(g_strBA_SaveImageFailPattern, strOutFilename);
		return nRet;
	}
	
	FilterCleanup_Simple(nCommandCount);
	return RIF_SUCCESS;
}
