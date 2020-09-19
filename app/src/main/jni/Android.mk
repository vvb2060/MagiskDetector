LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE           := vvb2060
LOCAL_CFLAGS           := -Oz
LOCAL_SRC_FILES        := vvb2060.c cpp.cpp
LOCAL_LDLIBS           := -llog
LOCAL_STATIC_LIBRARIES := xposed_detector crypto_static
include $(BUILD_SHARED_LIBRARY)

$(call import-module,prefab/xposeddetector)
$(call import-module,prefab/boringssl)
