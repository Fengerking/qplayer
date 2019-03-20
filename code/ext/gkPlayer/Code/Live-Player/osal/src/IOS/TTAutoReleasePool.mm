/**
 * File : TTAutoReleasePool.mm
 * Description : TTAutoReleasePool ˛
 */

#include <Foundation/NSAutoreleasePool.h>
#include "TTAutoReleasePool.h"
#include "GKMacrodef.h"
#include "GKCritical.h"

static NSAutoreleasePool* gAutoReleasePool = NULL;

void TTAutoReleasePool::InitAutoRelesePool()
{
    GKASSERT(gAutoReleasePool == NULL);
    gAutoReleasePool = [[NSAutoreleasePool alloc] init];
}

void TTAutoReleasePool::UninitAutoReleasePool()
{
    GKASSERT(gAutoReleasePool != NULL);
    [gAutoReleasePool release];
    gAutoReleasePool = NULL;
}
