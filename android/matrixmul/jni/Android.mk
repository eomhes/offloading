
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := matrixmul
LOCAL_SRC_FILES := matrix_mul_host.c
include $(BUILD_EXECUTABLE)
