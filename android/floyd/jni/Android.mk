
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := floyd
LOCAL_SRC_FILES := floyd_cpu.c
include $(BUILD_EXECUTABLE)
