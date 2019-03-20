//
//  CTestAdp.hpp
//  TestPlayerAuto
//
//  Created by Jun Lin on 25/04/2018.
//  Copyright © 2018 Jun Lin. All rights reserved.
//

#ifndef CTestAdp_hpp
#define CTestAdp_hpp

#include "CTestInst.h"

class CTestAdp
{
public:
    CTestAdp();
    virtual ~CTestAdp();
    
public:
    void ShowInfo(QCTEST_InfoItem* pItem);
    void SetPos(long long llPlayingTime, long long llDuration);

public:
    void*    m_viewItem;
    void*    m_viewFunc;
    void*    m_viewMsg;
    void*    m_viewErr;
    void*	 m_viewPlayingTime;
};

#endif /* CTestAdp_hpp */
