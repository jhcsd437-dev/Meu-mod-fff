LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := mod
LOCAL_SRC_FILES := Main.cpp
LOCAL_LDLIBS    := -llog -landroid -lGLESv2 -lEGL
LOCAL_CFLAGS    := -fvisibility=hidden -ffunction-sections -fdata-sections
LOCAL_CPPFLAGS  := -std=c++17 -fexceptions -frtti
include $(BUILD_SHARED_LIBRARY)
