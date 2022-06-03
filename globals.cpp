#include "globals.h"

AppConfig g_AppConfig;
String g_strLastBackendError;

unsigned int FilterParam::FilterParamTypeFromString(const String& strType)
{
	for (int i = 0; i < g_astrParameterTypeStr.GetCount(); i++)
	{
		if (g_astrParameterTypeStr[i] == strType)
			return i;
	}
	
	return RIF_PARAMETER_TYPE_STRING;
}

String FilterParam::StringFromFilterParamType(unsigned int nType)
{
	return g_astrParameterTypeStr[nType];
}

String ComposeOutFilename(const String& strFilename, const String& strOutPattern, const char* szFilterSuffix, bool bIndexed)
{
	Vector<String> astrOutPatternReplaceItems
	{
		GetFileDirectory(strFilename),
		GetFileTitle(strFilename),
		szFilterSuffix,
		bIndexed ? g_strOutFilenameIndex : "",
		GetFileExt(strFilename)
	};
	
	return Replace(strOutPattern, g_astrOutPatternFindItems, astrOutPatternReplaceItems);
}
