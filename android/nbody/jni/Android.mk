
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := nbody
LOCAL_SRC_FILES := nbody_cpu.c
include $(BUILD_EXECUTABLE)
