
#ifndef __PJMEDIA_UTILS_H__
#define __PJMEDIA_UTILS_H__

#include <string.h>
#include <assert.h>
#include "types.h"

#include <arpa/inet.h>
#include <sys/time.h>

#ifdef __GLIBC__
#   define PJ_HAS_BZERO		1
#endif

#define pj_thread_sleep(msec) usleep(msec * 1000)

//#define PJ_INLINE_SPECIFIER	static __inline
//#define PJ_INLINE(type)	  PJ_INLINE_SPECIFIER type


long get_currenttime_us();
long get_timeofday_us(const struct timeval *tval);

void pj_bzero(void *dst, pj_size_t size);
void* pj_memcpy(void *dst, const void *src, pj_size_t size);
pj_uint16_t pj_ntohs(pj_uint16_t netshort);
pj_uint16_t pj_htons(pj_uint16_t netshort);


typedef struct pjmedia_rect_size
{
	unsigned w;
	unsigned h;
}pjmedia_rect_size;

typedef struct pjmedia_vid_codec_param
{
    //pjmedia_format      enc_fmt;        /**< Encoded format	            */
    int      avg_bps;
    int      vid_sizes_change;

    //add by z38123,20200307
    void*    encStream;
	uint8_t* bufferAddr;
    int      orientation;
    uint64_t presentationTimeUs;
    int (*func_frame_output)(void *, uint8_t* , int ); //userdata, buffer, lenght

	pjmedia_rect_size crop_size; /*change video sizes. add by d32700*/

} pjmedia_vid_codec_param;


#endif

