#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

#include <jni.h>
#include <android/native_window_jni.h>


#include "utils.h"
#include "glog.h"

#ifdef __cplusplus
	extern "C"{
#endif
	#include "mediacodec.h"
	#include "video_rtc_api.h"
#ifdef __cplusplus
}
#endif


#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#define REG_PATH "com/wy/mptt/medialib/NativeVideoRtc"


JavaVM*	g_javaVM	= NULL;
jclass 	g_mClass	= NULL;

int gWidth = 0, gHeight = 0;
jbyteArray byteArray_notifyRender = NULL;


int gVideoProcess = 0;

typedef struct PROFILE_FRAME {
	char data[1024];
	int  size;
}PROFILE_FRAME;

PROFILE_FRAME gProfile;


uint8_t *yuvI420 = NULL;
FILE *mwFile = NULL;
const char *FILE_PATH = "/sdcard/upper.h264";

int DataInfoCallback( int frameRate, int bitRate) {
	jint result = -1;
	JNIEnv*		menv;
	jobject		mobj;
	jclass		tmpClass;

	if(NULL == g_mClass) {
		MediaCodec_LOGE("g_mClass is null.");
		return result;
	}

	if(g_javaVM)
	{
		result = g_javaVM->AttachCurrentThread( &menv, NULL);
		if(NULL == menv) {
			MediaCodec_LOGE("function: %s, line: %d, GetEnv failed!", __FUNCTION__, __LINE__);
			return g_javaVM->DetachCurrentThread();
		}
	}
	else
	{
		MediaCodec_LOGE("function: %s, line: %d, JavaVM is null!", __FUNCTION__, __LINE__);
		return result;
	}

	if(NULL != g_mClass)
	{
		tmpClass = menv->GetObjectClass(g_mClass);
		if(tmpClass == NULL) {
			MediaCodec_LOGE("function: %s, line: %d, find class error", __FUNCTION__, __LINE__);
			return -1;
		}
		mobj = menv->AllocObject(tmpClass);
		if(NULL == mobj) {
			MediaCodec_LOGE("function: %s, line: %d, find jobj error!", __FUNCTION__, __LINE__);
			return -1;
		}

		jmethodID methodID_func = menv->GetMethodID(tmpClass, "onDataInfoCall", "(II)V");//(Ljava/lang/String;I[B)
		if(methodID_func == NULL) {
			MediaCodec_LOGE("function: %s, line: %d,find method error!", __FUNCTION__, __LINE__);
			return -1;
		}
		else {
			menv->CallVoidMethod(mobj, methodID_func, frameRate, bitRate);
		}
	}

	return 0;
}

int FrameCallback( jbyte*frame, int length) {
	jint result = -1;
	JNIEnv*		menv;
	jobject		mobj;
	jclass		tmpClass;

	if(NULL == g_mClass) {
		log_error("g_mClass is null.");
		return result;
	}

	if(g_javaVM)
	{
		result = g_javaVM->AttachCurrentThread( &menv, NULL);
		if(NULL == menv) {
			log_error("function: %s, line: %d, GetEnv failed!", __FUNCTION__, __LINE__);
			return g_javaVM->DetachCurrentThread();
		}
	}
	else
	{
		log_error("function: %s, line: %d, JavaVM is null!", __FUNCTION__, __LINE__);
		return result;
	}

	if(NULL==byteArray_notifyRender) {
		jclass gbyteArray = (jclass)menv->NewByteArray(length);
		byteArray_notifyRender = (jbyteArray)menv->NewGlobalRef(gbyteArray);
	}
		

	if(NULL != g_mClass)
	{
		tmpClass = menv->GetObjectClass(g_mClass);
		if(tmpClass == NULL) {
			log_error("function: %s, line: %d, find class error", __FUNCTION__, __LINE__);
			return -1;
		}
		mobj = menv->AllocObject(tmpClass);
		if(NULL == mobj) {
			log_error("function: %s, line: %d, find jobj error!", __FUNCTION__, __LINE__);
			return -1;
		}

		jmethodID methodID_func = menv->GetMethodID(tmpClass, "onFrameCall", "([BI)V");//(Ljava/lang/String;I[B)
		if(methodID_func&&byteArray_notifyRender&&frame) {
			//MediaCodec_LOGE("------------------0:%d", length);
			
			menv->SetByteArrayRegion( byteArray_notifyRender, 0, length, frame);
			//MediaCodec_LOGE("------------------2");
			menv->CallVoidMethod(mobj, methodID_func, byteArray_notifyRender, length);
			//MediaCodec_LOGE("------------------3");
		}
		else {
			log_error("function: %s, line: %d,find method error!", __FUNCTION__, __LINE__);
			return -1;
		}
	}

	return 0;
}

int startRecvRender(JNIEnv *env, jobject obj, 
					jstring strLocalAddr, int localPort, jobject surface, int codecType) 
{
	int status = 0;
	jboolean isOk = JNI_FALSE;
	ANativeWindow *pAnw = ANativeWindow_fromSurface(env, surface);
	const char* localAddr = env->GetStringUTFChars(strLocalAddr, &isOk);
	status = vid_stream_create(localAddr, localPort, pAnw, codecType);//local
	if(status>=0)
		vid_stream_start( localAddr, localPort);//remote
	env->ReleaseStringUTFChars(strLocalAddr, localAddr);
	log_info("startRecvRender done.");

	return status;
}

int stopRecvRender(JNIEnv *env, jobject obj) {
	int status = 0;
	vid_stream_stop();
	vid_stream_destroy();
	log_info("stopRecvRender done.");

	return status;
}

int startSendVideo(JNIEnv *env, jobject obj, 
					jstring strLocalAddr, int localPort, jstring strRemoteAddr, int remotePort, int codecType) 
{
	int status = 0;
	memset(&gProfile, 0, sizeof(PROFILE_FRAME));
	jboolean isOk = JNI_FALSE;
	const char* localAddr  = env->GetStringUTFChars(strLocalAddr, &isOk);
	const char* remoteAddr = env->GetStringUTFChars(strRemoteAddr, &isOk);
	status = vid_stream_create(localAddr, localPort, NULL, codecType);//local
	if(status>=0)
		vid_stream_start( remoteAddr, remotePort);//remote

	env->ReleaseStringUTFChars(strLocalAddr, localAddr);
	env->ReleaseStringUTFChars(strRemoteAddr, remoteAddr);

	MediaCodec_LOGE("startSendVideo localAddr:%s localPort:%d remoteAddr:%s remotePort:%d", localAddr, localPort, remoteAddr, remotePort);

	return status;
}

int stopSendVideo(JNIEnv *env, jobject obj) {
	int status = 0;
	vid_stream_stop();
	log_info("vid_stream_stop done.");
	vid_stream_destroy();
	log_info("stopSendVideo done.");
	return status;
}

int sendSampleData(JNIEnv *env, jobject obj, jobject byteBuf, jint offset, jint size, jlong ptus,  jint flags) {
	//jlong dstSize;
	char *data = (char*)env->GetDirectBufferAddress(byteBuf);
	// if(dst == NULL) {
	// }else
	// {
	// 	dstSize = env->GetDirectBufferCapacity(byteBuf);
	// }

	// char*data = (char*)dst;
	// log_info("sendSampleData dst type:%d dstSize:%d", data[4], size);

	if(flags == 2) {
		memcpy(gProfile.data, data, size);
		gProfile.size = size;
	}

	if(gProfile.size>0 && flags==1) {
		packet_and_send(gProfile.data, gProfile.size);
	}
	
	return packet_and_send(data, size);
}

static JNINativeMethod video_method_table[] = {
		{ "startRecvRender", "(Ljava/lang/String;ILandroid/view/Surface;I)I", (void *)startRecvRender },
		{ "stopRecvRender", "()I", (void *)stopRecvRender },

		{ "startSendVideo", "(Ljava/lang/String;ILjava/lang/String;II)I", (void *)startSendVideo },
		{ "stopSendVideo", "()I", (void *)stopSendVideo },
		{ "sendSampleData", "(Ljava/nio/ByteBuffer;IIJI)I", (void *)sendSampleData },
};

int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* methods, int numMethods)
{
    jclass clazz;

    clazz = env->FindClass(className);
    if (clazz == NULL) {
        log_info("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, methods, numMethods) < 0) {
        log_info("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		log_error("GetEnv failed!");
		return result;
	}

	g_javaVM = vm;

	log_info("enc JNI_OnLoad......1");
	registerNativeMethods(env,
			REG_PATH, video_method_table,
			NELEM(video_method_table));
	log_info("enc JNI_OnLoad......2");
	
	return JNI_VERSION_1_4;
}


