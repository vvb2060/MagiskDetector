LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE           := vvb2060
LOCAL_SRC_FILES        := vvb2060.c
LOCAL_LDLIBS           := -llog
include $(BUILD_SHARED_LIBRARY)
