
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := hmm
LOCAL_SRC_FILES := hmm_cpu.c
include $(BUILD_EXECUTABLE)
