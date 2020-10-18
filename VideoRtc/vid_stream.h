
#ifndef __PJMEDIA_VID_STREAM_H__
#define __PJMEDIA_VID_STREAM_H__


#include "types.h"
#include "rtp.h"
#include "rtcp.h"

#include "vid_port.h"

//#include "jbuf_opt.h"
#include "jitter_buffer.h"
#include "transport_udp.h"

#define VERSION "1.1.1"

//#ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
#define RTCP_NACK 205
#define RTCP_RR   201
#define RTCP_SR   200
#define RTCP_FIR   192 /* add by j33783 20190805 */
#define RTCP_SR_RR_FALG 1
#define RTCP_SDES_FALG (RTCP_SR_RR_FALG << 1)
#define RTCP_NACK_FALG (RTCP_SDES_FALG << 1)
#define RTCP_BYTE_FALG (RTCP_NACK_FALG << 1)
#define RTCP_FIR_FALG (RTCP_BYTE_FALG << 1)   /* add by j33783 20190805 */
#define RTP_PACKET_SEQ_MAP_MAX 17
#define RTCP_RESEND_ARR_MAX_NUM 64  
#define RESEND_HEARTBREAK_TYPE 306
#define RESEND_HEARTBREAK_MAX_LEN 32
#define HEARTBREAK_RTP_PT 127
#define RESEND_BREAKBEART_TIMES 3
#define TERMINAL_TYPE 5
#define RESEND_TIMES_MAX 2
#define RESEND_REQ_BASESEQ_BUFF_LEN 1024
//#endif

#if defined(PJMEDIA_VIDEO_RESEND_OPTIMIZE)
#define RESEND_BUFF_SIZE  1024
#endif

//#ifdef PJMEDIA_VIDEO_JBUF_OPTIMIZE
#define DISCARD_RESEND_NUM_MAX 30
#define DEC_FRAME_ARR_MAX 20
#define FRAME_LEN_MAX 720*1280*3/2
#define DELAY_FRAME_NUM  5
#define DELAY_FRAME_NUM_MAX  10
#define BEGIN_RENDER_FRAME_IDX  3
//#endif

#ifdef HYT_HEVC
#define H265_FRAME_MAX_SIZE  256*1920
#endif


struct pjmedia_vid_stream
{
    pjmedia_format_id       fmt_id;

    unsigned                clock_rate;
    uint64_t                first_pts;
    float                   pts_ratio;
    pjmedia_rtp_session     rtp_session;
    transport_udp           *trans;
    pjmedia_vid_port        vid_port;
    RingBuffer*             ringbuf;          /**< Jitter buffer optimize.            */
    void* rtp_unpack_buf;
    void* rto_to_h264_obj;
    int                     codecType;

    //unsigned		        out_rtcp_pkt_size;
    char		            out_rtcp_pkt[PJMEDIA_MAX_MTU];  /**< Outgoing RTCP packet.	    */
};
typedef struct pjmedia_vid_stream pjmedia_vid_stream;

int packet_and_send_(struct pjmedia_vid_stream*stream, char* frameBuffer, int frameLen);


#ifndef __ANDROID__
int stream_send_test(const char *localAddr, unsigned short localPort, const char*remoteAddr,
	unsigned short remotePort, const char*sendFile, int codecType);
int stream_recv_test(const char *localAddr, short localPort, int codecType);

int stream_send_rtcp_test(const char *localAddr, short localPort, const char*remoteAddr, 
	short remotePort, int codecType);

int rtp_packet_recv_264(struct pjmedia_vid_stream*stream, char* packetBuffer, int packetLen);
void rtp_packet_and_unpack_notnet_test_264(pj_uint8_t *bits, pj_size_t bits_len);
void h264_package_test(pj_uint8_t *bits, pj_size_t bits_len);
void packet_and_send_test(pj_uint8_t *bits, pj_size_t bits_len);
#endif //__ANDROID__


#endif	/* __PJMEDIA_VID_STREAM_H__ */
