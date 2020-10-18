#ifndef __UNPACK_ES_RTP_H__
#define __UNPACK_ES_RTP_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RTP_BASE_HEADLEN 12
#define RTP_MAX_HEAD_EXTERN_LEN 12

#define MAX_RTP_PACKET_LEN 1500


/* add by j33783 20190524 */
#define _____________NEW_H264_UNPACK______

#define H264_MIN_PAYLOAD_SIZE 2
#define H264_START_CODE_SIZE   4
#define H264_FRAME_MAX_SIZE    256*1024
typedef enum rtp_frame_pos
{
	EN_FRAME_POS_PARAM     = 0,
	EN_FRAME_POS_START      = 1,  /* 帧起始包 */
	EN_FRAME_POS_MIDDLE    = 2,   /* 帧中间包 */
	EN_FRAME_POS_END          = 3	  /* 帧结束包 */
}rtp_frame_pos, EN_RTP_FRAME_POS;
 
#pragma pack(push,1)

typedef struct rtp_packet_info
{
	unsigned int                  usPayloadLen;                 /* payload 长度*/
	unsigned char*              pPayloadBuf;                   /* payload 数据 */	
	unsigned char                ucPayloadType;               /* rtp payload 值 */
	unsigned short               usSeq;                            /* rtp包序列号 */
	unsigned int                  uSSRC;                            /* rtp包同步源 */
	unsigned int                  uTimeStamp;                   /* rtp包时间戳 */
	rtp_frame_pos               enFramePos;                   /* 该包位于帧的位置 */
	unsigned char                ucNalType;				/* NAL类型 */
	unsigned char                ucFrmStart;                     /* 帧的首包 */
	unsigned char                ucMarker;                        /* 帧结束标记 */
}rtp_packet_info, ST_RTP_PACKET_INFO;

typedef struct pjmedia_vid_frame_buf
{
	unsigned int         		 uFrameLen;
	unsigned char*      	pFrameBuf;
}pjmedia_vid_frame_buf, ST_PJMEDIA_VID_FRAME_BUF;

/* add by j33783 20190527 */
typedef struct pjmedia_vid_rtp_unpack
{
	unsigned int                 nCodecID;                               /* 编码类型H264 or H265 */
	unsigned int         		 uFrameLen; 				       /* 帧长度 */
	unsigned char*     		 pFrameBuf;   				/* 帧数据 */
	int                      		 iFrameType;  				/* 帧类型1:I帧2:P帧3:B帧 */
	unsigned short      		 usRtpCnt;    					/* 帧包含RTP 包数 */
	int			     		 usRtpSSeq;   				/* 帧起始RTP 序列号*/
	int			      		 usRtpESeq;   				/* 帧结束RTP 序列号*/
	int                       	 bVPS;                                      /* GOP内是否有VPS(H265) */
	int                       	 bSPS;                                      /* GOP内是否有SPS */
	int                       	 bPPS;                                      /* GOP内是否有PPS*/
	int                       	 bIDR;                                      /* GOP内是否有IDR*/
	unsigned int                 uTimeStamp;                           /* 当前帧时间戳 */
	int                               bLenom;                                 /* previous packet is endbit of fu-a but not marker */	
	pjmedia_vid_frame_buf  stPreFrame;                            /* 前一个已组好的完整帧 */
}pjmedia_vid_rtp_unpack,ST_PJMEDIA_VID_RTP_UNPACK;

typedef enum unpack_frame_status
{
	EN_UNPACK_FRAME_START    = 0,  			/* 组帧开始，起始包 */
	EN_UNPACK_FRAME_MIDDLE  = 1,				/* 组帧进行中，中间包*/
	EN_UNPACK_FRAME_FINISH   = 2,				/* 组帧结束，结束包*/
	EN_UNPACK_FRAME_FAILURE = 3,                      /* 组帧失败 */
	EN_UNPACK_FRAME_ERROR   = 4                       /* 其他错误*/
} unpack_frame_status, EN_UNPACK_FRAME_STATUS;


pjmedia_vid_rtp_unpack* pjmedia_unpack_alloc_frame();
void pjmedia_unpack_free_frame(pjmedia_vid_rtp_unpack* pUnpack);
void pjmedia_unpack_reset_buf(pjmedia_vid_rtp_unpack* pUnpack);
void pjmedia_unpack_reset_frame(pjmedia_vid_rtp_unpack* pUnpack);
unpack_frame_status pjmedia_unpack_rtp_h264(unsigned char* pRtpPkt, unsigned int uRtpLen,  pjmedia_vid_rtp_unpack* pUnpack);

void test_pjmedia_h264_pack(unsigned char* pRtpPkt, unsigned int uRtpLen);


#pragma pack(pop)
#endif
