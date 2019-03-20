//
//  GKNotifyEnumDef.h
//  acquisition
//
//  Created by on 15/12/11.
//  Copyright © 2015年  All rights reserved.
//

#ifndef GKNotifyEnumDef_h
#define GKNotifyEnumDef_h

enum GKPushNotifyMsg
{
    ENotifyPushNone = 0
    , ENotifyUrlParseError= 1
    , ENotifySokcetConnectFailed = 2
    , ENotifyStreamConnectFailed= 3
    , ENotifyTransferError = 4
	, ENotifyOpenSucess = 5
	, ENotifyServerIP = 6
    , ENotifyOpenStart = 7
    , ENotifyReconn_OpenStart = 8
    , ENotifyReconn_OpenSucess = 9
    , ENotifyReconn_SokcetConnectFailed = 10
    , ENotifyReconn_StreamConnectFailed= 11
    , ENotifyNetBadCondition = 12
    , ENotifyNetReconnectUpToMax = 13
    , ENotifyRecord_FILEOPEN_OK = 14
    , ENotifyRecord_FILEOPEN_FAIL = 15
    , ENotifyRecord_START = 16
    , ENotifyAVPushException = 17
    //, ENotifyResetEncoder= 20
};

#endif /* GKNotifyEnumDef_h */
