cp ../config_bak/armv7/config.h ../../../
cp ../config_bak/armv7/Application.mk ./
ndk-build
cp ../libs/armeabi-v7a/libqcCodec.so ../../../../../projects/android/qplayerTest/app/libs/armeabi-v7a/