#pragma once

#include <android/log.h>

#define LOGV(...) (__android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__))
#define LOGI(...) (__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define LOGW(...) (__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define LOGE(...) (__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

#ifdef NDEBUG
#define LOGD(...)
#else
#define LOGD(...) (__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))
#endif
