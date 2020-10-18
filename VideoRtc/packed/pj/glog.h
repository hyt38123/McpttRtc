#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>
#include <string.h>

#ifdef __ANDROID__
    #include <android/log.h>
#endif

#ifndef PJ_DEF
#define PJ_DEF(type)		    type
#endif

#ifndef PJ_LOG_MAX_LEVEL
#  define PJ_LOG_MAX_LEVEL   2
#endif

#ifndef LOG_MAX_SIZE
#  define LOG_MAX_SIZE	    500
#endif

#define filename(x) strrchr(x,'/')?strrchr(x,'/')+1:x

#ifdef __cplusplus
extern "C"{
#endif

void pj_log( const char *sender, int level, const char *format, va_list marker);


#if PJ_LOG_MAX_LEVEL >= 0
#define log_fatal(fmt, ...)    pj_log_0(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define log_fatal(fmt, ...)     
#endif

#if PJ_LOG_MAX_LEVEL >= 1
    #ifdef __ANDROID__
        #define log_error(fmt, ...)    __android_log_print(ANDROID_LOG_ERROR , filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #else
        PJ_DEF(void) pj_log_1(const char *obj, const char *format, ...);
        #define log_error(fmt, ...)    pj_log_1(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #endif
#else
    #define log_error(fmt, ...)     
#endif

#if PJ_LOG_MAX_LEVEL >= 2
    #ifdef __ANDROID__
        #define log_warn(fmt, ...)    __android_log_print(ANDROID_LOG_WARN , filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #else
        PJ_DEF(void) pj_log_2(const char *obj, const char *format, ...);
        #define log_warn(fmt, ...)    pj_log_2(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #endif
#else
    #define log_warn(fmt, ...)     
#endif

#if PJ_LOG_MAX_LEVEL >= 3
    #ifdef __ANDROID__
        #define log_info(fmt, ...)    __android_log_print(ANDROID_LOG_INFO , filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #else
        PJ_DEF(void) pj_log_3(const char *obj, const char *format, ...);
        #define log_info(fmt, ...)    pj_log_3(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #endif
#else
    #define log_info(fmt, ...)     
#endif

#if PJ_LOG_MAX_LEVEL >= 4
    #ifdef __ANDROID__
        #define log_debug(fmt, ...)    __android_log_print(ANDROID_LOG_DEBUG , filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #else
        PJ_DEF(void) pj_log_4(const char *obj, const char *format, ...);
        #define log_debug(fmt, ...)    pj_log_4(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #endif
#else
    #define log_debug(fmt, ...)     
#endif

#if PJ_LOG_MAX_LEVEL >= 5
PJ_DEF(void) pj_log_5(const char *obj, const char *format, ...);
#define log_trace(fmt, ...)    pj_log_5(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define log_trace(fmt, ...)     
#endif

#if PJ_LOG_MAX_LEVEL >= 6
PJ_DEF(void) pj_log_6(const char *obj, const char *format, ...);
#define log_strace(fmt, ...)    pj_log_6(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define log_strace(fmt, ...)     
#endif


// #if PJ_LOG_MAX_LEVEL >= 0
// #define log_fatal(fmt, ...)    pj_log_0(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #else
// #define log_fatal(fmt, ...)     
// #endif

// #if PJ_LOG_MAX_LEVEL >= 1
// #define log_error(fmt, ...)    pj_log_1(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #else
// #define log_error(fmt, ...)     
// #endif

// #if PJ_LOG_MAX_LEVEL >= 2
// #define log_warn(fmt, ...)    pj_log_2(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #else
// #define log_warn(fmt, ...)     
// #endif

// #if PJ_LOG_MAX_LEVEL >= 3
// #define log_info(fmt, ...)    pj_log_3(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #else
// #define log_info(fmt, ...)     
// #endif

// #if PJ_LOG_MAX_LEVEL >= 4
// #define log_debug(fmt, ...)    pj_log_4(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #else
// #define log_debug(fmt, ...)     
// #endif

// #if PJ_LOG_MAX_LEVEL >= 5
// #define log_trace(fmt, ...)    pj_log_5(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #else
// #define log_trace(fmt, ...)     
// #endif

// #if PJ_LOG_MAX_LEVEL >= 6
// #define log_strace(fmt, ...)    pj_log_6(filename(__FILE__), "%s:%d " fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #else
// #define log_strace(fmt, ...)     
// #endif

#ifdef __cplusplus
}
#endif

#endif

