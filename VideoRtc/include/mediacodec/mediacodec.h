#ifndef __MEDIA_CODEC_H__
#define __MEDIA_CODEC_H__


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "NativeCodec.h"

#include <jni.h>
#include <android/log.h>
#include <sys/system_properties.h>

#include "utils.h"

#define MediaCodec_LOGI(...) __android_log_print(ANDROID_LOG_INFO , "MediaCodecEncoder", __VA_ARGS__)
#define MediaCodec_LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "MediaCodecEncoder", __VA_ARGS__)

#define AVC_CODEC  0
#define HEVC_CODEC 1

#define CONFIGURE_FLAG_ENCODE 1
#define CONFIGURE_FLAG_DECODE 0

//for mediacodec reset parameter
#define MEDIACODEC_RESET_NONE 0
#define MEDIACODEC_RESET_BITRATE 1
#define MEDIACODEC_RESET_RESOLUTION 2

//#define ENCODER_ASYNC_THREAD


typedef enum YUV_PIXEL_FORMAT{
	I420,
	NV12,
	NV21
}YUV_PIXEL_FORMAT;

//-------------------encode-------------------------
typedef struct MediaCodecEncoder{
	AMediaCodec* codec;//MediaCodec接口
	
	int32_t width;
	int32_t height;
	int32_t bit_rate;
	int32_t frame_rate;
	int32_t frameIndex;
	uint8_t  codeType;
	uint8_t* sps;
	uint8_t* pps;
	char* startCode;
	int32_t sps_length;
	int32_t pps_length;
	int32_t startCode_length;
	int32_t firstI;

	int32_t reseting; 	
	AMediaFormat* inputFormat;
	int64_t paramModifyTime;
	pthread_mutex_t	 encMutex;



	const char* mime;//解码器输出格式类型
	int32_t colorFormat;//解码器输出媒体颜色格式

	int SDK_INT;//SDK版本
	char* phone_type;//手机型号
	char* hardware;//cpu型号
	int DEBUG;//调试日志开关
	int64_t TIME_OUT;//解码缓存获得超时时间
	int64_t MAX_TIME_OUT;
	//char* MIMETYPE_VIDEO_CODE;
	int profile;
	int level;
	YUV_PIXEL_FORMAT yuv_pixel_format;

	uint8_t* bufferAddr;
	pjmedia_vid_codec_param* codecParam;

#ifdef ENCODER_ASYNC_THREAD
	pthread_t encThreadId;
	pthread_cond_t idleCond;
	int thdRuning;
#endif

}MediaCodecEncoder;

MediaCodecEncoder* mediacodec_encoder_alloc(int isDebug, int width, int height, int frame_rate, int bit_rate, 
											int timeout, YUV_PIXEL_FORMAT yuv_pixel_format, int codecType);
int mediacodec_encoder_free(MediaCodecEncoder* encoder);

int mediacodec_encoder_open(MediaCodecEncoder* encoder);
int mediacodec_encoder_close(MediaCodecEncoder* encoder);
	
int mediacodec_encoder_flush(MediaCodecEncoder* encoder);
int mediacodec_encoder_encode(MediaCodecEncoder* encoder, uint8_t* in, uint64_t pts, int offset, uint8_t* out, int length, int* error_code);

int mediacodec_encoder_restart(MediaCodecEncoder* encoder, AMediaFormat* mediaFormat);
int mediacodec_encoder_modify_bitrate(MediaCodecEncoder* encoder, int bitrate);
int mediacodec_encoder_reset_param(MediaCodecEncoder* encoder, const pjmedia_vid_codec_param *param);

int mediacodec_encoder_input(MediaCodecEncoder* encoder, uint8_t* in, uint64_t pts, int offset, uint8_t* out, int length, int* error_code);
int mediacodec_encoder_output(MediaCodecEncoder* encoder, int offset, uint8_t* out, int* res_code);

int mediacodec_encoder_getConfig_int(MediaCodecEncoder* encoder, char* key);
int mediacodec_encoder_setConfig_int(MediaCodecEncoder* encoder, char* key, int value);
uint64_t mediacodec_encoder_computePresentationTime(MediaCodecEncoder* encoder);
int mediacodec_encoder_ffAvcFindStartcodeInternal(uint8_t* data, int offset, int end);
int mediacodec_encoder_ffAvcFindStartcode(uint8_t* data, int offset, int end);

//-------------------encode-------------------------

//-------------------decode-------------------------
typedef struct MediaCodecDecoder{
	AMediaCodec* codec;//MediaCodec接口
	
	int32_t width;
	int32_t height;
	int32_t stride;//YUV宽度步长
	int32_t sliceHeight;//YUV高度步长
	int32_t crop_left;
	int32_t crop_right;
	int32_t crop_top;
	int32_t crop_bottom;
	const char* mime;//解码器输出格式类型
	int32_t colorFormat;//解码器输出媒体颜色格式

	int SDK_INT;//SDK版本
	char* phone_type;//手机型号
	char* hardware;//cpu型号
	int DEBUG;//调试日志开关
	int64_t TIME_OUT;//解码缓存获得超时时间
	int64_t MAX_TIME_OUT;
	char* MIMETYPE_VIDEO_CODE;
	YUV_PIXEL_FORMAT yuv_pixel_format;

	void* surface;
}MediaCodecDecoder;

MediaCodecDecoder* mediacodec_decoder_alloc(int isDebug, int timeout, YUV_PIXEL_FORMAT yuv_pixel_format, int codecType, void*surface);
MediaCodecDecoder* mediacodec_decoder_alloc1(int isDebug, int timeout, YUV_PIXEL_FORMAT yuv_pixel_format, int codecType);
MediaCodecDecoder* mediacodec_decoder_alloc2(int isDebug, int codecType);
MediaCodecDecoder* mediacodec_decoder_alloc3(int codecType);
MediaCodecDecoder* mediacodec_decoder_alloc4(int codecType, void*surface);
int mediacodec_decoder_free(MediaCodecDecoder* decoder);

int mediacodec_decoder_open(MediaCodecDecoder* decoder);
int mediacodec_decoder_close(MediaCodecDecoder* decoder);

int mediacodec_decoder_flush(MediaCodecDecoder* decoder);
int mediacodec_decoder_decode(MediaCodecDecoder* decoder, uint8_t* in, int offset, uint8_t* out, int length, int* error_code);
int mediacodec_decoder_render(MediaCodecDecoder* decoder, uint8_t* in, int length);
int mediacodec_decoder_getConfig_int(MediaCodecDecoder* decoder, char* key);
int mediacodec_decoder_setConfig_int(MediaCodecDecoder* decoder, char* key, int value);
//-------------------decode-------------------------

//-------------------utils-------------------------
void NV12toYUV420Planar(uint8_t* input, int offset, uint8_t* output, int width, int height);
void NV21toYUV420Planar(uint8_t* input, int offset, uint8_t* output, int width, int height);
void I420toYUV420SemiPlanar(uint8_t* input, int offset, uint8_t* output, int width, int height);
void I420toNV21(uint8_t* input, int offset, uint8_t* output, int width, int height);
void swapNV12toNV21(uint8_t* input, int offset, uint8_t* output, int width, int height);
void CropYUV420SemiPlanar(uint8_t* input, int width, int height, uint8_t* output,
					int crop_left, int crop_right, int crop_top, int crop_bottom);
void CropYUV420Planar(uint8_t* input, int width, int height, uint8_t* output,
					int crop_left, int crop_right, int crop_top, int crop_bottom);
//-------------------utils-------------------------

#endif//head