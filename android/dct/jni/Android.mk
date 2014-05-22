
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := dct
LOCAL_SRC_FILES := dct_cpu.c
include $(BUILD_EXECUTABLE)
