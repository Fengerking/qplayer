#include "GKNetWorkConfig.h"
#include "GKMacrodef.h"
#include "GKTypedef.h"
#include "TTLog.h"
#include <string.h>
#define LOG_SWITCH_OPEN 0
#ifndef __TT_OS_WINDOWS__
extern TTUint32 gProxyHostIP ;
extern TTInt    gProxyHostPort;
extern TTChar*   g_AutherKey ;
extern TTChar*   g_Domain ;
TTChar*   g_HostMetadata = NULL ;
extern TTBool  gUseProxy;
int g_LogOpenFlag = LOG_SWITCH_OPEN;

void ConfigProxyServer(TTUint32 aIP, TTInt Port, const TTChar* aAuthen, TTBool aUseProxy)
{
	LOGI("ConfigProxyServer: gUseProxy %d, aUseProxy %d", gUseProxy, aUseProxy);
    if (aUseProxy)
    {
        gProxyHostIP = aIP;
        gProxyHostPort = Port;
        SAFE_FREE(g_AutherKey);
        g_AutherKey = (TTChar*)malloc(strlen(aAuthen) + 1);
        strcpy(g_AutherKey, aAuthen);
    }
    gUseProxy = aUseProxy;
    SAFE_FREE(g_Domain);    
}

void ConfigProxyServerByDomain(const TTChar* aDomain, TTInt Port, const TTChar* aAuthen, TTBool aUseProxy)
{
    SAFE_FREE(g_Domain);
    if (aUseProxy)
    {
        gProxyHostPort = Port;
        g_Domain = (TTChar*)malloc(strlen(aDomain) + 1);
        strcpy(g_Domain, aDomain);
    }
    gUseProxy = aUseProxy;
}

void setLogOpenSwitch(int aSwitch)
{
    g_LogOpenFlag = aSwitch;
}

void SetHostMetaData(const TTChar* aHostHead)
{
	SAFE_FREE(g_HostMetadata);
	if (aHostHead)
	{
		g_HostMetadata = (TTChar*)malloc(strlen(aHostHead) + 1);
		strcpy(g_HostMetadata, aHostHead);
	} 
}

TTChar* GetHostMetaData()
{
    return g_HostMetadata;
}
#endif

GKNetWorkConfig* GKNetWorkConfig::iNetWorkConfig = NULL;

GKNetWorkConfig* GKNetWorkConfig::getInstance()
{
	if (iNetWorkConfig == NULL)
	{
		iNetWorkConfig = new GKNetWorkConfig();
	}

	return iNetWorkConfig;
}

void GKNetWorkConfig::release()
{
	SAFE_DELETE(iNetWorkConfig);
}

GKActiveNetWorkType GKNetWorkConfig::getActiveNetWorkType()
{	
	return iNetWorkType; 
}

void GKNetWorkConfig::SetActiveNetWorkType(GKActiveNetWorkType aNetWorkType)
{ 
	iNetWorkType = aNetWorkType;
    /*switch (aNetWorkType)
	{
	case EActiveNetWork_2G:
	case EActiveNetWork_WAP:
		iNetWorkType = EActiveNetWork_LOW;
		break;
	case EActiveNetWork_3G:
		iNetWorkType = EActiveNetWork_MIDDLE;
		break;
	case EActiveNetWork_4G:
	case EActiveNetWork_WIFI:
		iNetWorkType = EActiveNetWork_FAST;
		break;
	default:
		iNetWorkType = EActiveNetWork_LOW;
		break;
	}*/
}

GKNetWorkConfig::GKNetWorkConfig() 
: iNetWorkType(EActiveNetWorkGPRS)
{
}
