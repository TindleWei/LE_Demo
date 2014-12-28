################################################################################
#
# LittleEndian AudioIO Android make file
#
# Copyright (c) 2012 - 2014. Little Endian Ltd. All rights reserved.
#
################################################################################

ifndef LE_SDK_PATH
    LE_SDK_PATH := $(call my-dir)/..
endif

################################################################################
# Define the LE AudioIO module:
################################################################################

LOCAL_PATH:= $(LE_SDK_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE            := le_audioio
LOCAL_EXPORT_C_INCLUDES := $(abspath $(LE_SDK_PATH)/include)
LOCAL_EXPORT_LDLIBS     += -lOpenSLES
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES     := libs/development/libAudioIO_Android_x86-32_SSSE3.a
else
    ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
        LOCAL_ARM_NEON  := true
        LOCAL_SRC_FILES := libs/development/libAudioIO_Android_ARMv7a_NEON.a
    else
        LOCAL_SRC_FILES := libs/release/libAudioIO_Android_ARMv6_VFP2.a
    endif
endif

include $(PREBUILT_STATIC_LIBRARY)
