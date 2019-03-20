//
//  CTestAdp.cpp
//  TestPlayerAuto
//
//  Created by Jun Lin on 25/04/2018.
//  Copyright © 2018 Jun Lin. All rights reserved.
//

#include "CTestAdp.h"
#import <UIKit/UIKit.h>

#define _viewItem    ((__bridge UITextView*)m_viewItem)
#define _viewFunc    ((__bridge UITextView*)m_viewFunc)
#define _viewMsg	((__bridge UITextView*)m_viewMsg)
#define _viewErr    ((__bridge UITextView*)m_viewErr)
#define _viewPlayingTime    ((__bridge UILabel*)m_viewPlayingTime)

CTestAdp::CTestAdp()
:m_viewMsg(NULL)
,m_viewErr(NULL)
,m_viewItem(NULL)
,m_viewFunc(NULL)
,m_viewPlayingTime(NULL)
{
    
}

CTestAdp::~CTestAdp()
{
    
}

void CTestAdp::ShowInfo(QCTEST_InfoItem* pItem)
{
//    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
//
//     }];

    dispatch_sync(dispatch_get_main_queue(), ^{
        UITextView* view = NULL;
        
        if(pItem->nInfoType == QCTEST_INFO_Err)
        {
            view = _viewErr;
        }
        else if(pItem->nInfoType == QCTEST_INFO_Item)
        {
            view = _viewItem;
        }
        else if(pItem->nInfoType == QCTEST_INFO_Func)
        {
            view = _viewFunc;
        }
//        else if(pItem->nInfoType == QCTEST_INFO_Msg)
//        {
//            view = _viewMsg;
//        }
        
        if(view)
        {
            if (strcmp(pItem->pInfoText, "RESET") == 0)
                view.text = @"";
            else
            {
                view.text = [NSString stringWithFormat:@"%s\n%s", [view.text UTF8String], pItem->pInfoText];
                [view scrollRangeToVisible:NSMakeRange(view.text.length, 1)];
            }
        }
    });
}

void CTestAdp::SetPos(long long llPlayingTime, long long llDuration)
{
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSString* strPos = [NSString stringWithFormat:@"%02lld:%02lld:%02lld", (llPlayingTime/1000) / 3600, (llPlayingTime/1000) % 3600 / 60, (llPlayingTime/1000) % 3600 % 60];
        NSString* strDur = [NSString stringWithFormat:@"%02lld:%02lld:%02lld", (llDuration/1000) / 3600, (llDuration/1000) % 3600 / 60, (llDuration/1000) % 3600 % 60];
        _viewPlayingTime.text = [NSString stringWithFormat: @"%@%@%@", strPos, @" / " , strDur];
    });
}
