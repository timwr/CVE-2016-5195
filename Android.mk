LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	dirtycow.c \
	dcow.c

LOCAL_MODULE := dirtycow
LOCAL_LDFLAGS   += -llog
LOCAL_CFLAGS    += -DDEBUG

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := run-as
LOCAL_SRC_FILES := \
	run-as.c

LOCAL_CFLAGS += -DDEBUG
LOCAL_LDFLAGS   += -llog

include $(BUILD_EXECUTABLE)

