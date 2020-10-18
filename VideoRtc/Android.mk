LOCAL_PATH := $(call my-dir)

LOCAL_PROJECT_ROOT := $(LOCAL_PATH)#$(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)))

COMMON_PATH	 = include
CODEC_INC  = include/mediacodec
CODEC_SRC = mediacodec
PACKED_SRC = packed

#include $(CLEAR_VARS)
#LOCAL_MODULE := avformat
#LOCAL_SRC_FILES := $(LOCAL_PROJECT_ROOT)/$(FFMPEG_PATH)/android/lib/libavformat.so
#include $(PREBUILT_SHARED_LIBRARY)
#
#include $(CLEAR_VARS)
#LOCAL_MODULE := avcodec
#LOCAL_SRC_FILES := $(LOCAL_PROJECT_ROOT)/$(FFMPEG_PATH)/android/lib/libavcodec.so
#include $(PREBUILT_SHARED_LIBRARY)
#
#include $(CLEAR_VARS)
#LOCAL_MODULE := avutil
#LOCAL_SRC_FILES := $(LOCAL_PROJECT_ROOT)/$(FFMPEG_PATH)/android/lib/libavutil.so
#include $(PREBUILT_SHARED_LIBRARY)
#

include $(CLEAR_VARS)

#LOCAL_CFLAGS := -D__ANDROID__ 
LOCAL_CPPFLAGS 	+= -std=c++11
#LOCAL_CFLAGS += -std=c99

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CFLAGS := -D__ANDROID__  -D__cpusplus -g -mfloat-abi=softfp -mfpu=neon -march=armv7-a -mtune=cortex-a8 -DHAVE_NEON=1
endif
ifeq ($(TARGET_ARCH_ABI),$(filter $(TARGET_ARCH_ABI), armeabi-v7a x86))
LOCAL_ARM_NEON := true
endif

LOCAL_MODULE    := VideoRtc

LOCAL_C_INCLUDES += \
		$(LOCAL_PROJECT_ROOT)/$(COMMON_PATH) \
		$(LOCAL_PROJECT_ROOT)/$(CODEC_INC) \
		$(PACKED_SRC)/pj \
		$(PACKED_SRC)

#LOCAL_SRC_FILES := mediacodec_ndk.c FileDeCodecJni.cpp $(FFMPEG_PATH)/FfmpegContext.cpp $(THREAD_PATH)/gthreadpool.cpp
LOCAL_SRC_FILES :=  NALDecoder.c \
					vid_codec.c \
					vid_port.c \
					vid_stream.c \
					transport_udp.c  \
					packed/packets_list.c \
					packed/jitter_buffer.c \
					packed/callCplus.c \
					packed/dealrtpunpack.c \
					packed/h264_packetizer.c \
					packed/rtp.c \
					packed/rtcp.c \
					packed/pj/os_core_unix.c \
					packed/pj/utils.c \
					packed/pj/glog.c \
					$(CODEC_SRC)/mediacodec_decoder.c \
		   			$(CODEC_SRC)/mediacodec_encoder.c \
		   			$(CODEC_SRC)/mediacodec_utils.c \
					$(CODEC_SRC)/NativeCodec.cpp \
					RtcNative.cpp


#LOCAL_SHARED_LIBRARIES := avformat avcodec avutil swresample


#LOCAL_STATIC_LIBRARIES := libMediaStream
#LOCAL_SHARED_LIBRARIES := libandroid_runtime
LOCAL_STATIC_LIBRARIES := cpufeatures

LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/cpufeatures)
