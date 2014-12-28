################################################################################
#
# LE Melodify SDK Android make file
#
# Copyright (c) 2013 - 2014. Little Endian Ltd. All rights reserved.
#
################################################################################

MELODIFY_SDK_PATH := $(call my-dir)/..

################################################################################
# Define the LE Melodify SDK module:
################################################################################

LOCAL_PATH:= $(MELODIFY_SDK_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE            := le_melodify_sdk
LOCAL_EXPORT_C_INCLUDES := $(abspath $(MELODIFY_SDK_PATH)/include) $(abspath $(MELODIFY_SDK_PATH)/include/melodify) $(abspath $(MELODIFY_SDK_PATH)/demo)
LOCAL_EXPORT_CFLAGS     := -Wno-multichar
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES     := libs/development/libSpectrumWorxMelodifySDK_Android_x86-32_SSSE3.a
else
    ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
        LOCAL_ARM_NEON  := true
        LOCAL_SRC_FILES := libs/development/libSpectrumWorxMelodifySDK_Android_ARMv7a_NEON.a
    else
        LOCAL_SRC_FILES := libs/release/libSpectrumWorxMelodifySDK_Android_ARMv6_VFP2.a
    endif
endif

include $(PREBUILT_STATIC_LIBRARY)
