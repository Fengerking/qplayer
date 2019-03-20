BUILD_FFMPEG=0
BUILD_GK_AAC=0
BUILD_GK_264=0
BUILD_OPENSSL=0
BUILD_LIBYUV=0
BUILD_RELEASE_PACKAGE=1

GIT_COMMIT=`git rev-parse --short HEAD`
DATE=`date +"%m-%d.%H-%M-%S" `
PATH_CURR=`pwd`
PATH_INCLUDE=../../include
PATH_QPLAYERENG=QPlayEng/prj/
PATH_GK_AAC_DEC=codec/gk/aacdec/prj/
PATH_GK_H264_DEC=codec/gk/h264dec/prj/
PATH_LIBYUV=../../ext/libyuv/projects/ios/
PATH_TEST_APP=TestCode/
PATH_BIN=../../../bin/ios
PATH_DOC=../../../doc
PATH_SDK_VERSION_FILE=../../mfw/ios/CiOSPlayer.mm

PATH_RELEASE_BIN=../../../../qplayer-sdk/ios/bin
PATH_RELEASE_INC=../../../../qplayer-sdk/ios/include
PATH_RELEASE_LIB=../../../../qplayer-sdk/ios/lib
PATH_RELEASE_SMP=../../../../qplayer-sdk/ios/sample
PATH_RELEASE_DOC=../../../../qplayer-sdk/ios

PATH_RELEASE_BIN_DEV=../../../../qplayer-sdk-dev/ios/bin
PATH_RELEASE_INC_DEV=../../../../qplayer-sdk-dev/ios/include
PATH_RELEASE_LIB_DEV=../../../../qplayer-sdk-dev/ios/lib
PATH_RELEASE_SMP_DEV=../../../../qplayer-sdk-dev/ios/sample
PATH_RELEASE_DOC_DEV=../../../../qplayer-sdk-dev/ios

I386_ARCHS='i386 x86_64'
I386_SDK=iphonesimulator12.1
#VALID_ARCHS='i386 x86_64'
#echo $DATE
#xcodebuild -showsdks

# modify source file
sh updateversion.sh

if [ $BUILD_OPENSSL -eq 1 ]
then
cd $PATH_CURR
cd rtmp
sh build-libssl.sh
cd $PATH_CURR
cp `find rtmp/lib -name *.a` $PATH_BIN
rm `find rtmp/ -name *.gz`
rm `find rtmp/lib -name *.a`
rm -rf rtmp/bin
rm -rf rtmp/src
rm -rf rtmp/include/openssl
else
echo not build OpenSSL
fi
#exit

if [ $BUILD_FFMPEG -eq 1 ]
then
cd $PATH_CURR
cd ../../ext/ffmpeg/QCP/ios
sh build-ffmpeg.sh
#exit
else
echo not build FFMPEG
fi

if [ $BUILD_GK_AAC -eq 1 ]
then
cd $PATH_CURR
cd $PATH_GK_AAC_DEC
xcodebuild clean
xcodebuild -project ./qcAACDec.xcodeproj -target qcAACDec -configuration release ARCHS='arm64 armv7' VALID_ARCHS='arm64 armv7'
xcodebuild -project ./qcAACDec.xcodeproj -target qcAACDec_i386_64 -configuration release -sdk $I386_SDK ARCHS='i386 x86_64' VALID_ARCHS='i386 x86_64'
cd $PATH_CURR
cd $PATH_BIN
lipo -create libqcAACDec.a libqcAACDec_i386_64.a -output libqcAACDec.a
rm libqcAACDec_i386_64.a
rm -r build
else
echo not build GK aac
fi

if [ $BUILD_GK_264 -eq 1 ]
then
cd $PATH_CURR
cd $PATH_GK_H264_DEC
xcodebuild clean
xcodebuild -project ./h264dec.xcodeproj -target h264dec -configuration release ARCHS='arm64 armv7' VALID_ARCHS='arm64 armv7'
xcodebuild -project ./h264dec.xcodeproj -target h264dec_i386_64 -configuration release -sdk $I386_SDK VALID_ARCHS='i386 x86_64'
cd $PATH_CURR
cd $PATH_BIN
lipo -create libqcH264Dec.a libqcH264Dec_i386_64.a -output libqcH264Dec.a
rm libqcH264Dec_i386_64.a
rm -r build
else
echo not build GK h264
fi

if [ $BUILD_LIBYUV -eq 1 ]
then
cd $PATH_CURR
cd $PATH_LIBYUV
xcodebuild clean
xcodebuild -project ./libYUV.xcodeproj -target libYUV -configuration release ARCHS='arm64 armv7 armv7s' VALID_ARCHS='arm64 armv7 armv7s'
xcodebuild -project ./libYUV.xcodeproj -target libYUV_simulator -configuration release -sdk $I386_SDK VALID_ARCHS='i386 x86_64'
cd $PATH_CURR
cd $PATH_BIN
lipo -create libyuv.a libyuv_simulator.a -output libyuv.a
rm libyuv_simulator.a
lipo -info libyuv.a
#exit
else
echo not build LIBYUV
fi

cd $PATH_CURR
cp ../../ext/ffmpeg/QCP/libavutil/avconfig.h ../../ext/ffmpeg/libavutil/


cd $PATH_CURR
cd $PATH_QPLAYERENG
xcodebuild clean
xcodebuild -project ./QPlayEng.xcodeproj -target QPlayEng -configuration release ARCHS='arm64 armv7 armv7s' VALID_ARCHS='arm64 armv7 armv7s'
xcodebuild -project ./QPlayEng.xcodeproj -target QPlayEng_i386_64 -configuration release -sdk $I386_SDK VALID_ARCHS='i386 x86_64'
#xcodebuild -project ./QPlayEng.xcodeproj -target QPlayEng_pure -configuration release ARCHS='arm64 armv7' VALID_ARCHS='arm64 armv7'
#xcodebuild -project ./QPlayEng.xcodeproj -target QPlayEng_i386_64_pure -configuration release -sdk $I386_SDK VALID_ARCHS='i386 x86_64'
if [ $? -eq 0 ]
then
echo Engine build ok!!!
else
echo Engine build failed!!!
cd $PATH_CURR
exit
fi
cd $PATH_CURR
cd $PATH_BIN
lipo -create libqcPlayEng.a libqcPlayEng_i386_64.a -output libqcPlayEng.a
rm libqcPlayEng_i386_64.a
#lipo -create libqcPlayEng_pure.a libqcPlayEng_i386_64_pure.a -output pure/libqcPlayEng.a
#rm libqcPlayEng_i386_64_pure.a
#rm libqcPlayEng_pure.a
#cp libssl.a libcrypto.a libavcodec.a libavformat.a libavutil.a libswresample.a pure/
rm -r build

cd $PATH_CURR
cd $PATH_TEST_APP
xcodebuild clean
xcodebuild
xcodebuild archive -scheme TestCode -archivePath ./build/Release-iphoneos/TestCode.xcarchive
xcodebuild -exportArchive -archivePath ./build/Release-iphoneos/TestCode.xcarchive -exportPath ${PATH_CURR}"/"${PATH_BIN} -exportOptionsPlist ./TestCode/export.plist
rm -r build
cd $PATH_CURR
mv $PATH_BIN/TestCode.ipa $PATH_BIN/corePlayer.ipa

# verify simulator lib
cd $PATH_CURR
cd $PATH_TEST_APP
xcodebuild clean
xcodebuild -project ./TestCode.xcodeproj -target TestCode -configuration release -sdk $I386_SDK VALID_ARCHS='i386 x86_64'
if [ $? -eq 0 ]
then
rm -r build
echo Simulator app build ok!!!
else
rm -r build
echo Simulator app build failed!!!
cd $PATH_CURR
exit
fi

#package release
if [ $BUILD_RELEASE_PACKAGE -eq 1 ]
then
cd $PATH_CURR
cp $PATH_BIN/*.a $PATH_RELEASE_LIB
cp $PATH_INCLUDE/qcPlayer.h $PATH_RELEASE_INC
cp $PATH_INCLUDE/qcDef.h $PATH_RELEASE_INC
cp $PATH_INCLUDE/qcType.h $PATH_RELEASE_INC
cp $PATH_INCLUDE/qcErr.h $PATH_RELEASE_INC
cp $PATH_INCLUDE/qcMsg.h $PATH_RELEASE_INC
cp $PATH_INCLUDE/qcData.h $PATH_RELEASE_INC
cp $PATH_DOC/qcPlayerSDK_User_iOS.docx $PATH_RELEASE_DOC

cp $PATH_BIN/*.a $PATH_RELEASE_LIB_DEV
cp $PATH_INCLUDE/qcPlayer.h $PATH_RELEASE_INC_DEV
cp $PATH_INCLUDE/qcDef.h $PATH_RELEASE_INC_DEV
cp $PATH_INCLUDE/qcType.h $PATH_RELEASE_INC_DEV
cp $PATH_INCLUDE/qcErr.h $PATH_RELEASE_INC_DEV
cp $PATH_INCLUDE/qcMsg.h $PATH_RELEASE_INC_DEV
cp $PATH_INCLUDE/qcData.h $PATH_RELEASE_INC_DEV
cp $PATH_DOC/qcPlayerSDK_User_iOS.docx $PATH_RELEASE_DOC_DEV

cd $PATH_CURR
rm -rf ~/tmp1
mkdir ~/tmp1
cp -rf $PATH_TEST_APP ~/tmp1
find ~/tmp1  -name "*.git" -o -name "*.bat" | xargs -n1 rm -f -r
cp -rf ~/tmp1/ $PATH_RELEASE_SMP
cp -rf ~/tmp1/ $PATH_RELEASE_SMP_DEV
rm -rf ~/tmp1

cd $PATH_CURR
cd $PATH_RELEASE_SMP
xcodebuild clean
xcodebuild archive -scheme TestCode -archivePath ./build/Release-iphoneos/TestCode.xcarchive
xcodebuild -exportArchive -archivePath ./build/Release-iphoneos/TestCode.xcarchive -exportPath ${PATH_CURR}"/"${PATH_RELEASE_BIN} -exportOptionsPlist ./TestCode/export.plist
rm -r build
cd $PATH_CURR
cp $PATH_RELEASE_BIN/TestCode.ipa $PATH_RELEASE_BIN/corePlayer.ipa
mv $PATH_RELEASE_BIN/TestCode.ipa $PATH_RELEASE_BIN_DEV/corePlayer.ipa

cd $PATH_CURR
lipo -info $PATH_RELEASE_LIB/libqcPlayEng.a
lipo -info $PATH_RELEASE_LIB_DEV/libqcPlayEng.a

else
echo ------------- not prepare release packge ------------------
cd $PATH_CURR
lipo -info $PATH_BIN/libqcPlayEng.a
fi

rm $PATH_BIN/*.plist $PATH_BIN/*.log
#end of package release

# for temp build
cd $PATH_CURR
rm $PATH_BIN/*.zip
zip -r $PATH_BIN/libqcPlayEng.$DATE.$GIT_COMMIT.zip $PATH_BIN/libqcPlayEng.a
cp $PATH_BIN/libqcPlayEng.$DATE.$GIT_COMMIT.zip ~/Temp/

echo BUILD FINISH!!!
