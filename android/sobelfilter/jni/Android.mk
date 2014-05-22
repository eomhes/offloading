
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := discovery_client

LOCAL_C_INCLUDES := .

LOCAL_LDLIBS := -lpthread

LOCAL_SRC_FILES := ../../src/client.c 

