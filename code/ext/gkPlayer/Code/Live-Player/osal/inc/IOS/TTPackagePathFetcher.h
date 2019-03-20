/**
 * File : TTPackagePathFetcher.h 
 * Description : TTPackagePathFetcher˛
 */

#ifndef __TT_PACKAGE_PATH_FETCHER_H__
#define __TT_PACKAGE_PATH_FETCHER_H__
#import <Foundation/Foundation.h>
#include "GKMacrodef.h"
#include "GKTypedef.h"
@interface TTPackagePathFetcher : NSObject {
@private
}
+ (const TTChar*) DocumentsPath;
+ (const TTChar*) CacheFilePath;
@end
#endif
