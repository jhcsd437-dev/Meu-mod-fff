LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := mod
LOCAL_SRC_FILES := Main.cpp
LOCAL_LDLIBS    := -llog -landroid
include $(BUILD_SHARED_LIBRARY)
