#ifndef _RIF_globals_h_
#define _RIF_globals_h_

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

#define LAYOUTFILE <RIF/main.lay>
#include <CtrlCore/lay.h>

#include "RadeonImageFilter/include/RadeonImageFilters.h"

#ifdef RIF_USE_METAL
static const String g_strAPI = "Metal";
#elif defined RIF_USE_DX12
static const String g_strAPI = "DX12";
#else
static const String g_strAPI = "OpenCL";
#endif

#define APP_VER	0.06f
#define DONE_WITH_BACKEND_DELAY 1500
#define SOLID_IMAGE_DIMS 8

#pragma pack(push, 1)
struct Pixel_Uint8 { };
struct Pixel_Float { };

struct PixelRGBA_Uint8 : Pixel_Uint8
{
	byte r;
	byte g;
	byte b;
	byte a;
};

struct PixelRGB_Uint8 : Pixel_Uint8
{
	byte r;
	byte g;
	byte b;
};

struct PixelGA_Uint8 : Pixel_Uint8
{
	byte g;
	byte a;
};

struct PixelG_Uint8 : Pixel_Uint8
{
	byte g;
};

struct PixelRGBA_Float16 : Pixel_Float
{
	__fp16 r;
	__fp16 g;
	__fp16 b;
	__fp16 a;
};

struct PixelRGB_Float16 : Pixel_Float
{
	__fp16 r;
	__fp16 g;
	__fp16 b;
};

struct PixelGA_Float16 : Pixel_Float
{
	__fp16 g;
	__fp16 a;
};

struct PixelG_Float16 : Pixel_Float
{
	__fp16 g;
};

struct PixelRGBA_Float32 : Pixel_Float
{
	float r;
	float g;
	float b;
	float a;
};

struct PixelRGB_Float32 : Pixel_Float
{
	float r;
	float g;
	float b;
};

struct PixelGA_Float32 : Pixel_Float
{
	float g;
	float a;
};

struct PixelG_Float32 : Pixel_Float
{
	float g;
};
#pragma pack(pop)

#define LI_FLAG_SHOW_ALERT		(1<<0)
#define LI_FLAG_REFRESH_LAYOUT	(1<<1)
#define LI_FLAG_SHOW_VIEWER		(1<<2)
#define LI_FLAG_SET_HALF_CW		(1<<3)

enum eGenericResult
{
	GR_NA = -1,
	GR_SUCCESS = 0,
	GR_SOFTFAIL,
	GR_FAIL,
	GR_PROGRESS,
	GR_CANCELLED,
	GR_MAX
};

enum eColorPickerMode
{
	CPM_BACKGROUND = 0,
	CPM_SEPARATOR = 1,
	CPM_MAX
};

enum eBackendInitState
{
	BIS_BACKEND = 0,
	BIS_FILTER_PARAMS = 1,
	BIS_DONE,
	BIS_DONE_AND_CLOSE,
	BIS_MAX
};

static const Vector<String> g_astrComponentTypeStr
{
	"UINT8",
	"FLOAT16",
	"FLOAT32"
};

static const Vector<String> g_astrParameterTypeStr
{
	"float1",
	"float2",
	"float3",
	"float4",
	"float8",
	"float16",
	"uint1",
	"uint2",
	"uint3",
	"uint4",
	"uint8",
	"uint16",
	"int1",
	"int2",
	"int3",
	"int4",
	"int8",
	"int16",
	"image",
	"string",
	"float_array",
	"uint_array",
	"image_array",
	"local_memory"
};

static const Vector<String> g_astrOutPatternFindItems
{
	"%DIR%",
	"%TITLE%",
	"%FILTER%",
	"%INDEX%",
	"%EXT%"
};

String ComposeOutFilename(const String& strFilename, const String& strOutPattern, const char* szFilterSuffix, bool bIndexed);
bool CompareCArray(unsigned int* pArray1, unsigned int* pArray2, int nLen);

template<typename T>
int GetVectorIndex(Vector<T>& list, T& val)
{
	for (int i = 0; i < list.GetCount(); i++)
	{
		if (list[i] == val)
			return i;
	}
	
	return -1;
}

static const String g_strMainWindowPattern = "RIF Simple (%s)";
static const String g_strStatusReady = "Ready";
static const String g_strStatusInProgress = "In progress...";
static const String g_strAllPattern = "*.*";
static const String g_strDefaultOutPattern = "%DIR%%TITLE%.%FILTER%%INDEX%%EXT%";
static const String g_strDefaultCachePath = "kernelsCache";
static const String g_strDefaultLogFilename = "backend.log";
static const String g_strDefaultPerfLogFilename = "performance.log";
static const String g_strParameter = "Parameter";
static const String g_strType = "Type";
static const String g_strValue = "Value";
static const String g_strDescription = "Description";
static const String g_strSelectFiles = "Please select files!";
static const String g_strSelectOneFile = "Please select a file!";
static const String g_strNotYetImplemented = "Not yet implemented!";
static const String g_strNoPreviewForFilePattern = "No preview for this file - %s";
static const String g_strFilelistEmpty = "File list is empty!";
static const String g_strFileNotFoundPattern = "File not found - %s";
static const String g_strMenuAddFiles = "Add Files";
static const String g_strMenuAddDir = "Add Dir";
static const String g_strMenuRemoveFiles = "Remove Files";
static const String g_strMenuRemoveAll = "Remove All";
static const String g_strMenuExit = "Exit";
static const String g_strMenuAbout = "About";
static const String g_strMenuFile = "File";
static const String g_strMenuView = "View";
static const String g_strMenuDevice = "Device";
static const String g_strMenuHelp = "Help";
static const String g_strMenuOpenFirst = "Open First";
static const String g_strMenuOpenSecond = "Open Second";
static const String g_strMenuSwap = "Swap";
static const String g_strMenuClearFirst = "Clear First";
static const String g_strMenuClearSecond = "Clear Second";
static const String g_strMenuBGColor = "Background Color";
static const String g_strMenuSepColor = "Separator Color";
static const String g_strMenuSepShow = "Show Separator";
static const String g_strMenuEnlarge = "Enlarge";
static const String g_strSelectDevice = "Select Device";
static const String g_strPreviewStatusPattern = "Dims: (%d, %d), Components: (%d, %s), Ratio: %d%%";
static const String g_strComparePairPattern = "%s ---- %s";
static const String g_strCompareNothing = "(none)";
static const String g_strCompareNothingBoth = "(none) ---- (none)";
static const String g_strInitializingBackend = "Initializing RIF backend, please wait...";
static const String g_strQueryingFilterParams = "Querying filter params, please wait...";
static const String g_strDoneNoErrors = "Done without errors!";
static const String g_strDoneWithErrors = "Done with errors!";
static const String g_strGetDeviceInfoFail = "Unable to get device info!";
static const String g_strFileSelPattern = "Selected Pattern (%s)\t%s\nAll Files (*.*)\t*.*";
static const String g_strFileSelAll = "All Files (*.*)\t*.*";
static const String g_strBatchConvertWindowPattern = "Convert All - %s";
static const String g_strColFilename = "Filename";
static const String g_strColStatus = "Status";
static const String g_strFileStatusInQueue = "In Queue";
static const String g_strFileStatusInProgress = "In Progress";
static const String g_strFileStatusFailed = "Failed: ";
static const String g_strFileStatusDone = "Done";
static const String g_strBackgroundTaskInProgress = "Background task is in progress!";
static const String g_strConversionInProgress = "Conversion is in progress!";
static const String g_strConfigFilename = "config.json";
static const String g_strCustomPrefix = "CUSTOM: ";
static const String g_strEmptyString = "";
static const String g_strCustomFilterParamDescPattern = "[%s] %s";
static const String g_strIgnoreParameterTag = "#";
static const String g_strOutFilenameIndex = "%d";
static const String g_strSrcBufferParam = "srcBuffer";
static const String g_strDstBufferParam = "dstBuffer";
static const String g_strAboutPrettyPattern = "{{1:3-1 RIFSimple||||Version||\1%g\1||Backend||\1%s\1}}";
static const String g_strPerfStatPrettyPattern =
	"{{1:3 Input||\1%s\1||Output||\1%s\1||Filter||\1%s\1||Compile time||\1%s\1||Execution time||\1%s\1}}";

static const String g_strBA_GetDeviceCountFailPattern = "Unable to get device count! (code = %s)";
static const String g_strBA_GetDeviceInfoFailPattern = "Unable to get device info! (device = %d, code = %s)";
static const String g_strBA_CreateContextFailPattern = "Unable to create context! (code = %s)";
static const String g_strBA_CreateCommandQueueFailPattern = "Unable to create command queue! (code = %s)";
static const String g_strBA_CreateImageFilterFailPattern = "Unable to create image filter! (filter = 0x%X, code = %s)";
static const String g_strBA_SetComputeTypeFailPattern = "Unable to set compute type for filter! (filter = 0x%X, code = %s)";
static const String g_strBA_LoadImageFailPattern = "Unable to load image - %s";
static const String g_strBA_GetImageInfoFailPattern = "Unable to get image info - %s (code = %s)";
static const String g_strBA_GetImageMapFailPattern = "Unable to map image - %s (code = %s)";
static const String g_strBA_GetImageUnmapFailPattern = "Unable to unmap image - %s (code = %s)";
static const String g_strBA_CreateImageFailPattern = "Unable to create image - %s (code = %s)";
static const String g_strBA_SetFilterParamFailPattern = "Unable to set filter parameter! (filter = 0x%X, parameter = %s, type = %s, code = %s)";
static const String g_strBA_AttachFilterToQueueFailPattern = "Unable to attach filter to command queue! (filter = 0x%X, code = %s)";
static const String g_strBA_AttachFiltersToQueueFailPattern = "Unable to attach filter to command queue! (filters = %s, code = %s)";
static const String g_strBA_ExecuteCommandQueueFailPattern = "Unable to execute command queue! (code = %s)";
static const String g_strBA_SaveImageFailPattern = "Unable to save image - %s";
static const String g_strBA_GetFilterInfoFailPattern = "Unable to get filter info! (filter = 0x%X, code = %s)";
static const String g_strBA_GetParamInfoFailPattern = "Unable to get parameter info! (filter = 0x%X, index = 0x%X, code = %s)";
static const String g_strBA_AttachLogFailPatter = "Unable attach log! (code = %s)";

struct AppConfig
{
	struct Logging
	{
		int nLevel;
		String strFilename;
		
		void Jsonize(JsonIO& json)
		{
			json
				("level", nLevel, RIF_LOG_LEVEL_MESSAGE)
				("filename", strFilename, g_strDefaultLogFilename)
			;
		}
	};
	
	struct Filter
	{
		Vector<int> anComputeType;
		Vector<int> anSetParamsFunc;
		int nExecuteFunc;
		Vector<int> anCreateImageFunc;
		String strSuffix;
		Vector<int> anCommands;
		Vector<String> astrCommandAliases;
		String strDescription;
		VectorMap<String, String> mapParams;
		
		void Jsonize(JsonIO& json)
		{
			json
				("compute_types", anComputeType)
				("set_params_funcs", anSetParamsFunc)
				("execute_func", nExecuteFunc)
				("create_image_funcs", anCreateImageFunc)
				("commands", anCommands)
				("command_aliases", astrCommandAliases)
				("suffix", strSuffix)
				("description", strDescription)
				("params", mapParams)
			;
		}
	};
		
	int nAppLanguage;
	bool bQuickStart;
	String strFilePattern;
	String strOutPattern;
	String strKernelsCachePath;
	String strPerfLogFilename;
	Logging sLogging;
	ArrayMap<String, Filter> mapFilters;

	void Jsonize(JsonIO& json)
	{
		json
			("quick_start", bQuickStart, false)
			("app_language", nAppLanguage, 0)
			("file_pattern", strFilePattern, g_strAllPattern)
			("out_pattern", strOutPattern, g_strDefaultOutPattern)
			("perf_log_filename", strPerfLogFilename, g_strDefaultPerfLogFilename)
			("kernels_cache_path", strKernelsCachePath, g_strDefaultCachePath)
			("logging", sLogging)
			("filters", mapFilters)
		;
	}
};

extern AppConfig g_AppConfig;
extern String g_strLastBackendError;

struct FilterDesc;

struct FilterParam
{
	unsigned int m_dwType;
	String m_strName;
	String m_strValue;
	String m_strDefaultValue;
	String m_strDescription;
	unsigned int m_dwFilter;
		
	static unsigned int FilterParamTypeFromString(const String& strType);
	static String StringFromFilterParamType(unsigned int dwType);
	
	String GetTypeStr() { return StringFromFilterParamType(m_dwType); }
	
	FilterParam(unsigned int dwType, const String& strName, String& strValue, const String& strDefaultValue,
		String& strDesc, unsigned int dwFilter)
	{
		m_dwType = dwType;
		m_strName = strName;
		m_strValue = strValue;
		m_strDescription = strDesc;
		m_dwFilter = dwFilter;
	}
	
	FilterParam(const String& strType, const String& strName, const String& strValue, const String& strDefaultValue,
		const String& strDesc, unsigned int dwFilter)
	{
		m_dwType = FilterParamTypeFromString(strType);
		m_strName = strName;
		m_strValue = strValue;
		m_strDescription = strDesc;
		m_dwFilter = dwFilter;
	}
};

enum eFilterSetParamsFunc
{
	FPF_SIMPLE = 0,
	FPF_MAX
};

enum eFilterExecFunc
{
	FEF_FULL = 0,
	FEF_SIMPLE,
	FEF_MAX
};

enum eCreateImageFunc
{
	CIF_1X = 0,
	CIF_2X,
	CIF_MAX
};

typedef int (*SetFilterParams_type)(int nIndex);
typedef int (*ExecuteFilter_type)(const String& strFilename, const String& strOutFilename, rif_performance_statistic* pPerformanceStat);
typedef int (*CreateImage_type)(rif_image_desc* pDesc, rif_image * ppImage);

struct FilterDesc
{
	Vector<unsigned int> m_adwComputeTypes;
	String m_strSuffix;
	String m_strDescription;
	bool m_bJoinable;
	Vector<unsigned int> m_adwCommands;
	Vector<SetFilterParams_type> m_apSetParamsFuncs;
	ExecuteFilter_type m_pExecuteFunc;
	Vector<CreateImage_type> m_apCreateImageFuncs;
	
	VectorMap<String, FilterParam*> m_mapParams;
	
	int GetCommandCount() { return m_adwCommands.GetCount(); };
};

struct RIF_PerfStatWrapper : rif_performance_statistic
{
	void Jsonize(JsonIO& json)
	{
		String strCompileTime = Format("%f", compile_time);
		String strExecutionTime;
		strExecutionTime << execution_time;
		
		json
			("compile_time", strCompileTime)
			("execution_time", strExecutionTime)
		;
	}
	
	String ToQTFSimpleTable(const String& strFilter, const String& strFilename1, const String& strFilename2)
	{
		String strCompileTime = Format("%f", compile_time);
		String strExecutionTime;
		strExecutionTime << execution_time;
		
		return Format(g_strPerfStatPrettyPattern, strFilename1, strFilename2, strFilter, strCompileTime, strExecutionTime);
	}
	
	RIF_PerfStatWrapper()
	{
		compile_time = 0;
		execution_time = 0;
		measure_compile_time = true;
		measure_execution_time = true;
	}
};

#define PLAIN_TEXT(str) "\1" + str + "\1"

#endif
