
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := fft
LOCAL_SRC_FILES := fft_cpu.c
include $(BUILD_EXECUTABLE)
