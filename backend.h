#ifndef _RIF_backend_h_
#define _RIF_backend_h_

#include "RIF.h"

#define BUFFER_SIZE_DEFAULT 1024

int RIF_LoadImageIntoBuffer(const String& strFilename, ImageBuffer** ppBuffer, rif_image_desc* pDesc);
int RIF_AttachLog(int nLevel, const String& strFilename);
int RIF_GetDeviceCount(int* pnDeviceCount);
int RIF_GetDeviceName(int nDeviceID, String& strDeviceName);
int RIF_CreateObjects(int nDeviceID, const String& strCachePath, FilterDesc* pFilterDesc);
void RIF_DeleteObjects();
void RIF_DeleteFilter(rif_image_filter pFilter);
void RIF_DeleteFilters();
int RIF_FillCustomFilterParameters(int nDeviceID, const String& strCachePath, FilterDesc* pFilterDesc, VectorMap<String, String>& mapParams,
	Vector<String>& aCommandAliases, rif_image_filter* ppFilter);
int RIF_FillBaseFilterParameters(int nDeviceID, const String& strCachePath, FilterDesc* pFilterDesc, VectorMap<String, String>& mapDefaults,
	rif_image_filter* ppFilter);

int SetParameters_Simple(int nIndex);
int FilterExecute_Full(const String& strFilename, const String& strOutFilename, rif_performance_statistic* pPerformanceStat);
int FilterExecute_Simple(const String& strFilename, const String& strOutFilename, rif_performance_statistic* pPerformanceStat);
int CreateImage_1x(rif_image_desc* pDesc, rif_image* ppImage);
int CreateImage_2x(rif_image_desc* pDesc, rif_image* ppImage);

static SetFilterParams_type g_aSetFilterParamsFuncs[FPF_MAX] = { SetParameters_Simple };
static ExecuteFilter_type g_aExecuteFilterFuncs[FEF_MAX] = { FilterExecute_Full, FilterExecute_Simple };
static CreateImage_type g_aCreateImageFuncs[CIF_MAX] = { CreateImage_1x, CreateImage_2x };

#endif
