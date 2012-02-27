# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	main.c         \
	modem-control.c \
	utils.c

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libril

ifeq ($(TARGET_ARCH),arm)
LOCAL_SHARED_LIBRARIES += libdl
endif # arm

LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE:= RIL_TEST

include $(BUILD_EXECUTABLE)
