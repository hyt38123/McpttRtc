#ifndef __VID_CODEC_H__
#define __VID_CODEC_H__

#include "types.h"

typedef struct pjmedia_frame
{
    void		    *buf;	    /**< Pointer to buffer.		    */
    int		        size;	    /**< Frame size in bytes.		    */

	int             orientation;

    int             width;
    int             height;
    int             enc_packed_pos;
    pj_bool_t       has_more;

} pjmedia_frame;

int get_h264_package(char* frameBuffer, int frameLen, pjmedia_frame *out_package);
int get_h265_package(char* frameBuffer, int frameLen, pjmedia_frame *out_package);
#endif
