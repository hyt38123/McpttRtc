#ifndef __PJMEDIA_JBUF_OPT_H__
#define __PJMEDIA_JBUF_OPT_H__

#include "types.h"
//#include <pj/pool.h>
//#include <pjmedia/format.h>


#define _slow_rtp_head_flag				0xcccc			//慢包标记
#define _lost_rtp_head_flag				0xff00			//丢包标记
#define _max_rtp_seq_value				65535
#define _max_rtp_seq_windows			640
#define _max_lost_wait_time_unknow		1000			//150毫秒 未知终端类型的最大等待时间
#define _video_pk_length_               		1500
#define _video_pk_num_               			2048	//ringbuffer package size

#define _max_new_flow_wait_time			1500			//1500毫秒
#define _max_packet_length_in_rtp_ 		1500
#define _rtp_head_length 					12
#define _max_resend_wait_time			60                       //default  20
#define _max_lost_wait_time 				100                     //最大等待重传超时时间 //500ms
#define _max_require_resend                      3

#define _____________JITTER_BUFFER________________
typedef struct
{
	unsigned int   uPacketSize;         /* 单packet最大大小 */
	unsigned int   uPacketNum;          /* 最大缓存Packet数量 */
	unsigned int   uMaxPktWin;          /* 最大packet守候窗口大小 */
	unsigned int   uMaxPktTime;         /* 最大packet等待窗口时间 */
	unsigned int   uPacketCount;        /* ringbuffer接收packet总数 */
	
	int				uPreSeq;               /* 前一个rtp包的seq */
	int				uPrePos;               /* 前一个rtp包写位置 */	
	pj_ssize_t      uPreTime;              /* 前一个rtp写入时间 */

	int			uPreReadPos;       /* 前一个rtp包读取位置 */
	int			uPreReadSeq;       /* 前一个rtp包读取系列号 */
	pj_ssize_t	uPreReadTS;        /* 前一个rtp包读取时间戳 */

	int                uCuPktLost;           /* 等待连续丢包数 */
	
	unsigned int   uBufsize;
	unsigned char* xbuffer;
	int resend_support; /*for support resend*/
}_RingBuffer_ST_;



PJ_DECL(_RingBuffer_ST_*) pjmedia_ringbuffer_alloc(/*pj_pool_t *pool,*/ unsigned mtu, unsigned num);
PJ_DECL(void)  pjmedia_ringbuffer_destory(_RingBuffer_ST_* pRingBuffer);
PJ_DECL(void) pjmedia_ringbuffer_init(_RingBuffer_ST_* pRingBuffer);
PJ_DECL (pj_status_t) pjmedia_ringbuffer_write(_RingBuffer_ST_* pRingBuffer, unsigned char* pRtpPkt, unsigned uRtpLen);
PJ_DECL (pj_status_t) pjmedia_ringbuffer_read(_RingBuffer_ST_* pRingBuffer, unsigned char* pRtpPkt, unsigned* pRtpLen, pjmedia_format_id   fmt_id);


#define _________________________________________

/* add by j33783 20200219  begin */
typedef struct
{
	pj_ssize_t uStartTS;    /* 统计周期开始时间*/
	pj_ssize_t uCurRecvTS;  /* 统计周期当前时间*/
	pj_int32_t nStatCycle;  /* 统计周期单位ms */

	pj_int32_t nStartSeq;   /* 统计周期内起始rtp 序列号 */
	pj_uint16_t nPreSeq;    /* 统计周期内前一个rtp 序列号 */
	pj_uint16_t uCurSeq;    /* 统计周期内当前rtp 序列号 */

	pj_int32_t nSizeCnt;    /* 统计周期内收包大小*/
	pj_int32_t nRecvCnt;
	pj_int32_t nFrameCnt;   /* 统计周期内收到的帧数*/
	pj_int32_t uLostCnt;    /* 统计周期内统计周期内丢包数统计 */
	pj_uint16_t uResendCnt; /* 统计周期内统计周期内重传数统计 */

	pj_ssize_t uRecordTS;   /* 统计时间戳*/
	pj_int32_t nBitrate;    /* 统计收包大小*/
	float fFrameRate;       /* 统计周期内收到的帧数*/
	float      fLostRate;   /* 统计丢包率*/
}_RTP_RECV_STAT_;
/* add by j33783 20200219  end */


typedef struct _IBaseRunLib
{
    //BASE_MEDIA_CACHE_CH catch_info; 
    //const void *jbuf;
    _RingBuffer_ST_ * pRingBuffer; 	/* add by j33783 20190724 */
    _RTP_RECV_STAT_ rtpstat;    		/* add by j33783 20200219 */
 	
}IBaseRunLib;


PJ_DECL(pj_status_t) pjmedia_jbuf_create_opt(/*pj_pool_t *pool,  */unsigned pk_size, pj_bool_t resend_support, IBaseRunLib **p_jb);
PJ_DEF(pj_int32_t)  readPacketfromSub(IBaseRunLib * pRtl, pj_int8_t * pk, pjmedia_format_id fmt_id);
PJ_DEF(pj_int32_t)  writePackettoSub(IBaseRunLib * pRtl, pj_int8_t * pk, pj_int32_t len);
PJ_DECL(pj_status_t) pjmedia_jbuf_opt_destroy(IBaseRunLib *jb);
PJ_DEF (pj_int32_t) getLostPacket(IBaseRunLib * pRtl, void * pMediaCache, int * iCacke, int max, pj_uint32_t timeout, int tmaxout);
PJ_DEF(int)  pjmedia_ringbuffer_scan_lost(_RingBuffer_ST_* pRingBuffer, int * pCache, unsigned nMax);

//PJ_DEF(pj_ssize_t) GetCurrentMsec(void);
PJ_DEF (pj_status_t) pjmedia_jbuf_opt_reset(IBaseRunLib * pRtl);

PJ_DECL(void) pjmedia_jbuf_set_resend_wait_time(IBaseRunLib * pRtl, unsigned int uMaxWaitTimeMS);

/* add by j33783 20200219  begin */
PJ_DECL(void) pjmedia_jbuf_packet_recv_init(_RTP_RECV_STAT_* pStat);
PJ_DECL(void) pjmedia_jbuf_packet_recv_stat(_RTP_RECV_STAT_* pStat, pj_int8_t * pkt,  pj_size_t size);
PJ_DECL(void) pjmedia_jbuf_packet_recv_report(IBaseRunLib * pRtl, pj_int32_t* pBitRate, float* pLostRate, float* pFrameRate);
/* add by j33783 20200219  end */

#endif /*__PJMEDIA_JBUF_OPT_H__*/

