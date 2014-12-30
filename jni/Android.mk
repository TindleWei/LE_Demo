################################################################################
#
# LittleEndianDemoApp Android make file
#
# Copyright (c) 2013 - 2014. Little Endian Ltd. All rights reserved.
#
################################################################################

MY_LOCAL_PATH := $(call my-dir)

ifndef LE_SDK_PATH
    LE_SDK_PATH := $(call my-dir)/..
endif

include $(MY_LOCAL_PATH)/le_melodify_sdk.mk
include $(MY_LOCAL_PATH)/le_audioio.mk
include $(MY_LOCAL_PATH)/le_utility.mk


################################################################################
# Define the demo application module:
################################################################################

LOCAL_PATH := ${MY_LOCAL_PATH}
include $(CLEAR_VARS)

LOCAL_MODULE           := little-effect
LOCAL_SRC_FILES        := processingExample.cpp leaudio.cpp
#LOCAL_CFLAGS           += -std=gnu++11 -fno-exceptions -fno-rtti -Wall -Wno-multichar -Wno-non-template-friend -Wno-unused-local-typedefs -Wno-unknown-warning-option
LOCAL_LDLIBS           += -llog -landroid
LOCAL_STATIC_LIBRARIES := le_melodify_sdk le_audioio le_utility android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
