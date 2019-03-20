#cp ../config_bak/x86/config.h ../../../
#cp ../config_bak/x86/Application.mk ./
ndk-build
cp ../libs/x86/libqcCodec.so ../../../../../projects/android/qplayerTest/app/libs/x86/
