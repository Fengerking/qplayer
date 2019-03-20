DATE=`date +"%Y-%m-%d--%H-%M" `
PATH_CURR=`pwd`
PATH_INCLUDE=../../include
PATH_QPLAYERENG=QPlayEng/prj/
PATH_TEST_APP=TestCode/
PATH_BIN=../../../bin/ios
PATH_RELEASE_BIN=../../../../qplayer-sdk/ios/bin
PATH_RELEASE_INC=../../../../qplayer-sdk/ios/include
PATH_RELEASE_LIB=../../../../qplayer-sdk/ios/lib
PATH_RELEASE_SMP=../../../../qplayer-sdk/ios/sample
PATH_PILI_QPLAYER_BIN=../../../../plplayerkit/pili-player-ios-kit/PLPlayerKit/PLPlayerKit/QCPlayerSDK/lib


cp $PATH_BIN/libqcPlayEng.a $PATH_PILI_QPLAYER_BIN

echo COPY FINISH!!!
