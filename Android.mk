LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  dirtycow.c \
  dcow.c 

LOCAL_MODULE := dirtycow
LOCAL_LDFLAGS   += -llog
LOCAL_CFLAGS    += -DDEBUG
LOCAL_CFLAGS    += -fPIE
LOCAL_LDFLAGS   += -fPIE -pie

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := app_process
LOCAL_SRC_FILES := \
	dirtycow.c \
	app_process.c
LOCAL_CFLAGS += -DDEBUG
LOCAL_CFLAGS    += -fPIE
LOCAL_LDFLAGS   += -fPIE -pie
LOCAL_LDFLAGS   += -llog

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := sysutils
LOCAL_SRC_FILES := \
	dirtycow.c \
	libsysutils.c
LOCAL_CFLAGS += -DDEBUG
LOCAL_CFLAGS    += -fPIE
#LOCAL_LDFLAGS   += -fPIE -pie
LOCAL_LDFLAGS   += -fPIE
LOCAL_LDFLAGS   += -llog

include $(BUILD_SHARED_LIBRARY)

