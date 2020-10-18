
#include <unistd.h>
#include "vid_port.h"
#include "vid_stream.h"
#include "glog.h"
#include "callCplus.h"
//#include "jbuf_opt.h"
#include "jitter_buffer.h"

// FILE *mwFile = NULL;
// const char *FILE_PATH = "save.h264";

int tount = 0;
static  void  worker_thread_jbuf(void *arg)
{	
    pjmedia_vid_port *port = (pjmedia_vid_port *)arg;
    pjmedia_vid_stream *stream = (pjmedia_vid_stream *)port->useStream;

	int status = 0, pk_len, frame_len, continumDec = 0;
	pj_int8_t rtpPack[PJMEDIA_MAX_MTU] = {0};
    while(!port->jbuf_recv_thread.thread_quit)
	{
		if (continumDec > 2)
		{
			usleep(1000);
		}
	

        pk_len = 0;
	    ringbuffer_read(stream->ringbuf, rtpPack, &pk_len);
		//pk_len = readPacketfromSub(stream->jb_opt, rtpPack, stream->fmt_id);
		if(pk_len <= 0)
		{
			continumDec = 0;
			usleep(10000);
		}
		else
		{
                //log_error("readPacketfromRingbuf lenth:%d seq:%d", pk_len, pj_ntohs((unsigned short)rtpPack[2]));
                frame_len = RTPUnpackH264(stream->rto_to_h264_obj, (char*)rtpPack, pk_len, &stream->rtp_unpack_buf);
                if(frame_len > 0)
                {
                    tount++;
                    log_warn("RTPUnpackH264__ count:%d frame_len:%d\n", tount, frame_len);
                #ifdef __ANDROID__
                    if(port->decoder)
                        mediacodec_decoder_render(port->decoder, (uint8_t*)stream->rtp_unpack_buf, frame_len);
                #endif
                	// if(mwFile)
				    //     fwrite((uint8_t*)stream->rtp_unpack_buf, 1, frame_len, mwFile);
                }
                else if(-1 == frame_len) /* add by j33783 20190805 unpack failure, send FIR to encoder to flush IDR */
                {
                    //send_rtcp_fir(stream);
                    log_error("unpack failure, send FIR to encoder to flush IDR");
                }
        }
    }
    log_error("worker_thread_jbuf thread exit threadid:%d\n", (int)port->jbuf_recv_thread.threadId);
}

int vid_port_start(pjmedia_vid_port *vidPort)
{
    pj_status_t status = 0;
//mwFile = fopen(FILE_PATH, "w");
#ifdef __ANDROID__
    if(vidPort && vidPort->surface) {
        pjmedia_vid_stream *stream = (pjmedia_vid_stream *)vidPort->useStream;
        int codecType = (stream->codecType == H264_HARD_CODEC)?AVC_CODEC:HEVC_CODEC;
        vidPort->decoder = mediacodec_decoder_alloc4(codecType, vidPort->surface);//AVC_CODEC 0, HEVC_CODEC 1
        mediacodec_decoder_open(vidPort->decoder);
    }
#endif

    pj_thread_create("jbuf_recv", (thread_proc *)&worker_thread_jbuf, vidPort, 0, 0, &vidPort->jbuf_recv_thread);
    return status;
}

int vid_port_stop(pjmedia_vid_port *vidPort) 
{
    pj_status_t status = 0;
    vidPort->jbuf_recv_thread.thread_quit = 1;

#ifdef __ANDROID__
    if(vidPort->decoder) {
		mediacodec_decoder_free(vidPort->decoder);
		vidPort->decoder = NULL;
	}
#endif
    // if(mwFile != NULL) {
    //     fclose(mwFile);
    //     mwFile = NULL;
    // }
    return status;
}
