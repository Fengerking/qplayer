/**
 * File : TTPackagePathFetcher.cpp
 * Created on : 2012-3-21
 * Author : Kevin
 * Copyright : Copyright (c) 2016 GoKu Software Ltd. All rights reserved.
 * Description : TTPackagePathFetcher  µœ÷Œƒº˛
 */
#include "GKTypedef.h"
#include "GKMacrodef.h"
#include <stdlib.h>
#include "TTLog.h"

static TTChar* gCacheFilePath = NULL;

void gSetCacheFilePath(const TTChar* aCacheFilePath)
{
	LOGI("CacheFilePath:%s", aCacheFilePath);
	gCacheFilePath = (TTChar*)aCacheFilePath;
}

const TTChar* gGetCacheFilePath()
{
	return gCacheFilePath == NULL ? "C:\\cache.tmp" : gCacheFilePath;
}

const bool gGetCacheFileEnble()
{
	bool bCanWrite = true;
	FILE* Handle = fopen(gGetCacheFilePath(), "ab+");
	if (Handle == NULL){
		bCanWrite = false;
	} else {
		fclose(Handle);
	}

	return bCanWrite;
}
