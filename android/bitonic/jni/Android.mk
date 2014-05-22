
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := bitonic
LOCAL_SRC_FILES := bitonic_cpu.c
include $(BUILD_EXECUTABLE)
