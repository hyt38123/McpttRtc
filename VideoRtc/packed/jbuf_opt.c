///////////////////////////////////////////////////////
//writePackettoSub 写包函数
//readPacketfromSub 读包数据函数
//getLostPacket 获取丢包数组函数
//////////////////////////////////////////////////////////

#include "glog.h"
#include "jbuf_opt.h"
#include "utils.h"

//#include <pj/string.h>
//#include <linux/net.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define RTP_BASE_HEADLEN 12


typedef struct
{
	unsigned short uForRead:1;
	unsigned short uResendFlg:3;
	unsigned short uPktLen:12;         /* rtp包大小 */
	unsigned short uPktSeq;            /* 包序列号 */
	pj_ssize_t       uTimeStamp;       /* 收包时间戳*/
	unsigned char  xRtpBuf[0];         /* rtp包数据 */
}_Packet_Store_ST_;


PJ_DEF(pj_ssize_t) GetCurrentMsec()
{
	// pj_time_val now;
	// pj_gettickcount(&now);
	// return now.msec + now.sec * 1000;
	struct timeval tv_cur;
	gettimeofday(&tv_cur, NULL);
	return tv_cur.tv_sec*1000 + tv_cur.tv_usec/1000; 
}


//ringbuffer写,按照环形缓存的方式填充，拷贝数据，改变内部写指针的位置，如果包类
//型没有标记过，如果遇到帧的首包，需要标识出帧的类型
//包头标记中的reset，每次写都需要中心置位==之前本包的reset+1
PJ_DEF(pj_int32_t) writePackettoSub(IBaseRunLib * pRtl, pj_int8_t * pk, pj_int32_t len)
{
	//pjmedia_jbuf_packet_recv_stat(&pRtl->rtpstat, pk, len);
	/* add by j33783 20190724 */
	pjmedia_ringbuffer_write(pRtl->pRingBuffer, pk, len);
	return PJ_SUCCESS;
};

PJ_DEF (pj_status_t) pjmedia_ringbuffer_write(_RingBuffer_ST_* pRingBuffer, unsigned char* pRtpPkt, unsigned uRtpLen)
{
	pj_status_t status = PJ_FALSE;
	unsigned short usSeq = 0;
	 _Packet_Store_ST_ *pStorePkt = NULL;
	 int index = 0;
	pj_ssize_t uCurTS;
	int nSeqGap = 0;
	
	
	if(!pRingBuffer || !pRtpPkt)
	{
		log_error("parameter error, pRingBuffer:%p, pRtpPkt:%p", pRingBuffer, pRtpPkt);
		return status;
	}

	if(uRtpLen <= _rtp_head_length || uRtpLen > (pRingBuffer->uPacketSize - sizeof(_Packet_Store_ST_)))
	{
		log_error("parameter error, uRtpLen:%u", uRtpLen);
		return status;
	}

	usSeq =pj_ntohs(*(unsigned short*)(pRtpPkt+2));
	if(-1 == pRingBuffer->uPreSeq)
	{
		pRingBuffer->uPreSeq = usSeq - 1;
	}

	/* check max rtp seq winodow */
	nSeqGap = (usSeq - pRingBuffer->uPreSeq)%65536;
	if(abs(nSeqGap) > pRingBuffer->uMaxPktWin)
	{
		log_error("current seq:%u, prev seq:%u, gap:%d, is over max rtp seq windows:%d", 
			usSeq, pRingBuffer->uPreSeq, nSeqGap, pRingBuffer->uMaxPktWin);
		pjmedia_ringbuffer_init(pRingBuffer);
		nSeqGap = 1;
	}

	/* check rtp packet recv delay */
	uCurTS = GetCurrentMsec();
	if(0 == pRingBuffer->uPreTime)
	{
		pRingBuffer->uPreTime = uCurTS;
	}

	if((uCurTS - pRingBuffer->uPreTime) > _max_lost_wait_time_unknow)
	{
		log_error("current ts:%u, prev ts:%u is over max rtp wait time:%d", 
			uCurTS, pRingBuffer->uPreTime, pRingBuffer->uMaxPktTime);
		pjmedia_ringbuffer_init(pRingBuffer);
		nSeqGap = 1;
	}

	/* store the rtp packet */
	index = (pRingBuffer->uPrePos + nSeqGap + pRingBuffer->uPacketNum)%pRingBuffer->uPacketNum;
	pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + index*pRingBuffer->uPacketSize);	
	pStorePkt->uForRead = 1;
	pStorePkt->uResendFlg = 0;
    pStorePkt->uPktSeq = usSeq;		
    pStorePkt->uTimeStamp = uCurTS;
	pStorePkt->uPktLen = uRtpLen;	
	pj_memcpy(pStorePkt->xRtpBuf, pRtpPkt, uRtpLen);

	/* update the ringbuffer record */
	status = PJ_SUCCESS;
	if(nSeqGap > 0)
	{
		if(usSeq-pRingBuffer->uPreSeq != 1)
			log_error("rtp seq:%u, len:%d, to buffer pos:%d ---------------------", usSeq, pStorePkt->uPktLen, index);
		else
			log_error("rtp seq:%u, len:%d, to buffer pos:%d", usSeq, pStorePkt->uPktLen, index);
		
		pRingBuffer->uPrePos = index;	
		pRingBuffer->uPreSeq = usSeq;
	}	
	pRingBuffer->uPreTime = uCurTS;
	pRingBuffer->uPacketCount++;
	pRingBuffer->uBufsize += uRtpLen;

	//log_error("rtp seq:%u, len:%d, to buffer pos:%d", usSeq, pStorePkt->uPktLen, index);
	
	return status; 
}

//内网环境下，默认20毫秒时间可以超时重传了
PJ_DEF (pj_int32_t) getLostPacket(IBaseRunLib * pRtl, void * pMediaCache, int * iCacke, int max, pj_uint32_t timeout, int tmaxout)
{
	/* add by j33783 20190724 */
	return pjmedia_ringbuffer_scan_lost(pRtl->pRingBuffer, iCacke, max);
}

PJ_DEF(pj_int32_t) readPacketfromSub(IBaseRunLib * pRtl, pj_int8_t * pk, pjmedia_format_id   fmt_id)
{
	/* add by j33783 20190724 */
	unsigned int rtplen = 0;
	if(!pRtl)
	{
		log_error("stream->jb_opt is not create.");
		return 0;
	}
	pjmedia_ringbuffer_read(pRtl->pRingBuffer, pk, &rtplen, fmt_id);
	//log_debug("read a packet from ringbuffer, len:%u", rtplen);

	/* add by j33783 20200220 
	if(rtplen)
	{
		pjmedia_jbuf_packet_recv_stat(&pRtl->rtpstat, pk, rtplen);
	}*/
	return rtplen;
};


PJ_DEF(pj_status_t) pjmedia_jbuf_create_opt(/*pj_pool_t *pool, */unsigned packet_size, pj_bool_t resend_support, IBaseRunLib **p_jb)
{
	IBaseRunLib *jb;
	jb = (IBaseRunLib *)malloc(sizeof(IBaseRunLib));
    //jb = PJ_POOL_ZALLOC_T(pool, IBaseRunLib);

	/* add by j33783 20190724 */
	jb->pRingBuffer = pjmedia_ringbuffer_alloc(/*pool,*/ _video_pk_length_, _video_pk_num_);
    jb->pRingBuffer->resend_support = resend_support;
	pjmedia_ringbuffer_init(jb->pRingBuffer);

	pjmedia_jbuf_packet_recv_init(&jb->rtpstat);

	*p_jb = jb;
	return PJ_SUCCESS;
}

PJ_DEF (pj_status_t) pjmedia_jbuf_opt_reset(IBaseRunLib * jb)
{ 
	/* add by j33783 20190724 */
	pjmedia_ringbuffer_init(jb->pRingBuffer);
    return PJ_SUCCESS;
}

PJ_DECL(pj_status_t) pjmedia_jbuf_opt_destroy(IBaseRunLib *jb)
{
	/* add by j33783 20190724 */
	pjmedia_ringbuffer_destory(jb->pRingBuffer);
	free(jb);
	return PJ_SUCCESS;
}

#define _____________JITTER_STAT________________

/* add by j33783 20200219  begin */
PJ_DEF(void)  pjmedia_jbuf_packet_recv_init(_RTP_RECV_STAT_* pStat)
{
	if(!pStat)
	{
		log_error("invalid rtp stat object.");
		return;
	}

	pStat->uStartTS = 0;
	pStat->uCurRecvTS = 0;
	pStat->nStatCycle = 1000;

	pStat->nStartSeq = -1;
	pStat->nPreSeq = -1;
	pStat->uCurSeq = -1;

	pStat->nSizeCnt = 0;
	pStat->nRecvCnt = 0;
	pStat->uLostCnt = 0;	
	pStat->uResendCnt = 0;
	pStat->nFrameCnt = 0;
	
	pStat->uRecordTS = 0;
	pStat->nBitrate = 0;
	pStat->fFrameRate = 0.0;
	pStat->fLostRate = 0.0;
}

PJ_DEF(void) pjmedia_jbuf_packet_recv_stat(_RTP_RECV_STAT_* pStat, pj_int8_t * pkt, pj_size_t size)
{
	pj_uint16_t usSeq = -1;
	pj_ssize_t uCurTS = GetCurrentMsec();
	pj_ssize_t uOffset = 0;
	pj_int32_t nExpectCnt = 0;

	if(!pStat || !pkt)
	{
		log_error("invalid rtp stat object.");
		return;
	}

	usSeq =pj_ntohs(*(unsigned short*)(pkt+2));

	if(0 == pStat->uStartTS)
	{
		pStat->uStartTS = uCurTS;
	}

	if(-1 == pStat->nStartSeq)
	{
		pStat->nStartSeq = usSeq;
	}

	if(-1 == pStat->nPreSeq)
	{
		pStat->nPreSeq = usSeq;
	}

	//uOffset = uCurTS - pStat->uCurRecvTS;
	pStat->uCurRecvTS = uCurTS;	
	pStat->uCurSeq = usSeq;


	if(pStat->uCurSeq != pStat->nPreSeq)
	{
		if(pStat->uCurSeq > pStat->nPreSeq)
		{
			pStat->uLostCnt +=(pStat->uCurSeq - pStat->nPreSeq - 1);
			pStat->nRecvCnt++;
			if((*(pkt+1))&0x80)
			{
				pStat->nFrameCnt++;
			}
		}
		else if(pStat->uCurSeq < 256 && pStat->nPreSeq > 65200)
		{
			pStat->uLostCnt +=(pStat->uCurSeq+ 65535 - pStat->nPreSeq - 1);
			pStat->nRecvCnt++;
			if((*(pkt+1))&0x80)
			{
				pStat->nFrameCnt++;
			}
		}
		else
		{
			pStat->uResendCnt++;
		}
	}
	else
	{
		pStat->nRecvCnt++;
		if((*(pkt+1))&0x80)
		{
			pStat->nFrameCnt++;
		}
	}
	
	pStat->nSizeCnt += size;

	if(pStat->uCurRecvTS - pStat->uStartTS < pStat->nStatCycle)
	{
		
		 log_info("rtp stat running, start:%d, pre:%d, cur:%d, size:%d, frm:%d, cnt:%d, lost:%d, rs:%d, expired:%u",
		 	pStat->nStartSeq, pStat->nPreSeq, pStat->uCurSeq, pStat->nSizeCnt,   pStat->nFrameCnt,
		 	pStat->nRecvCnt, pStat->uLostCnt, pStat->uResendCnt, pStat->uCurRecvTS - pStat->uStartTS );

	 	if(pStat->uCurSeq >= pStat->nPreSeq || (512 <= abs(pStat->uCurSeq - pStat->nPreSeq)))
 		{
 			pStat->nPreSeq = pStat->uCurSeq;
 		}
	}
	else
	{
		log_info("rtp stat running, start:%d, pre:%d, cur:%d, size:%d, frm:%d, cnt:%d, lost:%d, rs:%d, expired:%u",
		 	pStat->nStartSeq, pStat->nPreSeq, pStat->uCurSeq, pStat->nSizeCnt,   pStat->nFrameCnt,
		 	pStat->nRecvCnt, pStat->uLostCnt, pStat->uResendCnt, pStat->uCurRecvTS - pStat->uStartTS );
	
		 pj_int32_t nBaseSeq = 0;
		 pStat->uRecordTS = pStat->uCurRecvTS;
		 pStat->nBitrate = (pStat->nSizeCnt * 8) /(pStat->uCurRecvTS - pStat->uStartTS);

		if((pStat->uCurSeq < pStat->nPreSeq) && (512 <= abs(pStat->uCurSeq - pStat->nPreSeq)))
		{
			if(pStat->nPreSeq >= pStat->nStartSeq)
			{
				nExpectCnt = (pStat->nPreSeq - pStat->nStartSeq + 1);
			}
			else
			{
				nExpectCnt = (pStat->nPreSeq + 65536 - pStat->nStartSeq) % 65536 + 1;
			}

			nBaseSeq = pStat->nPreSeq;
		}
		else
		{
			if(pStat->uCurSeq >= pStat->nStartSeq)
			{
				nExpectCnt = (pStat->uCurSeq - pStat->nStartSeq + 1);
			}
			else
			{
				nExpectCnt = (pStat->uCurSeq + 65536 - pStat->nStartSeq) % 65536 + 1;
			}

			nBaseSeq = pStat->uCurSeq;
		}

		/* if(pStat->nRecvCnt > nExpectCnt)
		 {
		 	nExpectCnt = pStat->nRecvCnt;
		 }*/
		 pStat->fLostRate = (1.0000 - (float)pStat->nRecvCnt/nExpectCnt);
		 pStat->fFrameRate = ((float)(pStat->nFrameCnt)/((float)(pStat->uCurRecvTS - pStat->uStartTS - uOffset)/1000.0));

		 log_info("rtp stat end, br:%d kpbs, lr:%0.4f, fps:%0.1f, start:%d, end:%d, recv:%d, expect:%d, rs:%d, expired:%u",
		 		 pStat->nBitrate, pStat->fLostRate, pStat->fFrameRate, pStat->nStartSeq, pStat->uCurSeq, 
		 		 pStat->nRecvCnt, nExpectCnt, pStat->uResendCnt, pStat->uCurRecvTS - pStat->uStartTS );

		pStat->uStartTS = pStat->uRecordTS;
		pStat->uCurRecvTS = 0;

		pStat->nStartSeq = (nBaseSeq+1)%65536;
		pStat->nPreSeq = (nBaseSeq+1)%65536;
		pStat->uCurSeq = -1;

		pStat->nSizeCnt = 0;
		pStat->nRecvCnt = 0;
		pStat->uLostCnt = 0;	
		pStat->uResendCnt = 0;
		pStat->nFrameCnt = 0;
	}
}

PJ_DEF(void) pjmedia_jbuf_packet_recv_report(IBaseRunLib * pRtl, pj_int32_t* pBitRate, float* pLostRate, float* pFrameRate)
{
	pj_ssize_t uCurTS = GetCurrentMsec();
	
	if(!pRtl || !pBitRate || !pLostRate)
	{
		return;
	}

	if(uCurTS - pRtl->rtpstat.uRecordTS > ( 2 * pRtl->rtpstat.nStatCycle))
	{
		*pBitRate = 0;
		*pLostRate = 0.00;
		*pFrameRate = 0.0;
	}
	else
	{
		*pBitRate = pRtl->rtpstat.nBitrate;
		*pLostRate = pRtl->rtpstat.fLostRate;
		*pFrameRate = pRtl->rtpstat.fFrameRate;
	}
}

/* add by j33783 20200219  end */


#define _____________JITTER_BUFFER________________
PJ_DEF(int)  pjmedia_ringbuffer_find_sps(_RingBuffer_ST_* pRingBuffer, int  curReadPos, int curWritePos, unsigned pkt_len, pjmedia_format_id   fmt_id)
{
	_Packet_Store_ST_ *pStorePkt = NULL;
	int i, pos;
	int ucNalType;
	uint8_t *payloadcursor, *rtpcursor;
	unsigned short curReadSeq;
	unsigned int uRtpHdrLen = 0;
    unsigned int uCSRCCnt = 0;
	unsigned int payloadlen = 0;
	int nExtFlag;
	int unsigned short *pusSeq;
	unsigned short usSeq;
	curReadPos = curReadPos % pRingBuffer->uPacketNum;	
	curWritePos = curWritePos % pRingBuffer->uPacketNum;
	curReadSeq = (pRingBuffer->uPreReadSeq+1)%65536;
	

	pos = curWritePos;


	if(curReadPos < curWritePos)
	{
		for(i = curReadPos; i < curWritePos; i++)
		{
			
			pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + i*pRingBuffer->uPacketSize);
			if(pStorePkt->uForRead)
			{
				log_info("find a position, i=%d, curwritepos=%d, uForRead=%d",i, curWritePos, pStorePkt->uForRead);
				rtpcursor = (uint8_t*)(((uint8_t*)pStorePkt) + sizeof(_Packet_Store_ST_));
				pusSeq = (unsigned short*)(rtpcursor+2);
				usSeq = ntohs(*pusSeq);
				uRtpHdrLen = RTP_BASE_HEADLEN;
				uCSRCCnt = rtpcursor[0]&0x0F;
    			nExtFlag = (rtpcursor[0]&0x10)>>4;
				if(uCSRCCnt)
			    {
			         uRtpHdrLen += uCSRCCnt*4;
			    }
				if(nExtFlag)
			    {
			         uRtpHdrLen += (((rtpcursor [uRtpHdrLen + 2] << 8) |rtpcursor [uRtpHdrLen + 3] ) + 1)*4;
			    }
				payloadcursor = rtpcursor+uRtpHdrLen;

				if(fmt_id == PJMEDIA_FORMAT_H264)
				{
					ucNalType = (*payloadcursor) & 0x1F;  
					log_info("H264 usSeq=%d ucNalType = %d", usSeq, ucNalType); 
					if(ucNalType == 7)
					{
						return i;
					}
					else if(ucNalType == 24)
					{
						unsigned char* qStart = payloadcursor + 1;
		                unsigned char* qEnd = payloadcursor + payloadlen;
						unsigned nal_size = 0;
						payloadlen = pkt_len - uRtpHdrLen;
						while(qStart < qEnd)
						{
						    nal_size =  (*qStart << 8) | *(qStart+1);
						    qStart += 2;
						    if(qStart + nal_size > qEnd)
						    {
						         log_error("parse STAP-A packet failure: qStart:%p, qEnd:%p, nal_size=%d", qStart, qEnd, nal_size);
						         return i;
						    }
							ucNalType = (*payloadcursor) & 0x1F;  
							log_info("H264 STAP usSeq=%d ucNalType = %d", usSeq, ucNalType); 
							if(ucNalType == 7)
							{
								return i;
							}
						    qStart += nal_size;
						}
					}
				}
				else if(fmt_id == PJMEDIA_FORMAT_H265)
				{
					ucNalType = (*payloadcursor >> 1) & 0x3F;
					log_info("H265 usSeq=%d ucNalType = %d", usSeq, ucNalType); 
					if( (ucNalType == 33) || (ucNalType == 32) ) //SPS or VPS
					{
						return i;
					}
					else if(ucNalType == 24)
					{
						unsigned char* qStart = payloadcursor + 1;
		                unsigned char* qEnd = payloadcursor + payloadlen;
						unsigned nal_size = 0;
						payloadlen = pkt_len - uRtpHdrLen;
						while(qStart < qEnd)
						{
						    nal_size =  (*qStart << 8) | *(qStart+1);
						    qStart += 2;
						    if(qStart + nal_size > qEnd)
						    {
						         log_error("parse STAP-A packet failure: qStart:%p, qEnd:%p, nal_size=%d", qStart, qEnd, nal_size);
						         return i;
						    }
							ucNalType = (*qStart >> 1) & 0x3F;
							log_info("H265 STAP usSeq=%d ucNalType = %d", usSeq, ucNalType); 
							if( (ucNalType == 33) || (ucNalType == 32) ) //SPS or VPS
							{
								return i;
							}
						    qStart += nal_size;
						}
					}
				}	
				
			}
			else
			{
				continue;
			}
			pStorePkt->uForRead = 0;
		}
	}

	if(curReadPos > curWritePos)
	{
		for(i = curReadPos; i < curWritePos+pRingBuffer->uPacketNum; i++)
		{
			pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + (i%pRingBuffer->uPacketNum)*pRingBuffer->uPacketSize);
			if(pStorePkt->uForRead)
			{
				log_info("find a position, i=%d, curwritepos=%d, uForRead=%d",i%pRingBuffer->uPacketNum, curWritePos, pStorePkt->uForRead);
				rtpcursor = (uint8_t*)(((uint8_t*)pStorePkt) + sizeof(_Packet_Store_ST_));
				pusSeq = (unsigned short*)(rtpcursor+2);
				usSeq = ntohs(*pusSeq);
				uRtpHdrLen = RTP_BASE_HEADLEN;
				uCSRCCnt = rtpcursor[0]&0x0F;
    			nExtFlag   = (rtpcursor[0]&0x10)>>4;
				if(uCSRCCnt)
			    {
			         uRtpHdrLen += uCSRCCnt*4;
			    }
				if(nExtFlag)
			    {
			         uRtpHdrLen += (((rtpcursor [uRtpHdrLen + 2] << 8) |rtpcursor [uRtpHdrLen + 3] ) + 1)*4;
			    }
				payloadcursor = rtpcursor+uRtpHdrLen;

				if(fmt_id == PJMEDIA_FORMAT_H264)
				{
					ucNalType = (*payloadcursor) & 0x1F;  
					log_info("H264 usSeq=%d ucNalType = %d", usSeq, ucNalType); 
					if(ucNalType == 7)
					{
						return i%pRingBuffer->uPacketNum;
					}
					else if(ucNalType == 24)
					{
						unsigned char* qStart = payloadcursor + 1;
						unsigned char* qEnd = payloadcursor + payloadlen;
						unsigned nal_size = 0;
						payloadlen = pkt_len - uRtpHdrLen;
						while(qStart < qEnd)
						{
							nal_size =	(*qStart << 8) | *(qStart+1);
							qStart += 2;
							if(qStart + nal_size > qEnd)
							{
								 log_error("parse STAP-A packet failure: qStart:%p, qEnd:%p, nal_size=%d", qStart, qEnd, nal_size);
								 return i%pRingBuffer->uPacketNum;
							}
							ucNalType = (*payloadcursor) & 0x1F;  
							log_info("H264 STAP usSeq=%d ucNalType = %d", usSeq, ucNalType); 
							if(ucNalType == 7)
							{
								return i%pRingBuffer->uPacketNum;
							}
							qStart += nal_size;
						}
					}
				}
				else if(fmt_id == PJMEDIA_FORMAT_H265)
				{
					ucNalType = (*payloadcursor >> 1) & 0x3F;
					log_info("H265 usSeq=%d ucNalType = %d", usSeq, ucNalType); 
					if( (ucNalType == 33) || (ucNalType == 32) ) //SPS or VPS
					{
						return i%pRingBuffer->uPacketNum;
					}
					else if(ucNalType == 24)
					{
						unsigned char* qStart = payloadcursor + 1;
						unsigned char* qEnd = payloadcursor + payloadlen;
						unsigned nal_size = 0;
						payloadlen = pkt_len - uRtpHdrLen;
						while(qStart < qEnd)
						{
							nal_size =	(*qStart << 8) | *(qStart+1);
							qStart += 2;
							if(qStart + nal_size > qEnd)
							{
								 log_error("parse STAP-A packet failure: qStart:%p, qEnd:%p, nal_size=%d", qStart, qEnd, nal_size);
								 return i%pRingBuffer->uPacketNum;
							}
							ucNalType = (*qStart >> 1) & 0x3F;
							log_info("H265 STAP usSeq=%d ucNalType = %d", usSeq, ucNalType); 
							if( (ucNalType == 33) || (ucNalType == 32) ) //SPS or VPS
							{
								return i%pRingBuffer->uPacketNum;
							}
							qStart += nal_size;
						}
					}
				}
			}
			else
			{
				continue;
			}
			pStorePkt->uForRead = 0;
		}
	}

	
	log_info("scan complete, sps not found, returning current write position minus 1.");
	return curWritePos-1;
}

PJ_DEF(int)  pjmedia_ringbuffer_move_pos(_RingBuffer_ST_* pRingBuffer, int  curReadPos, int curWritePos)
{
	_Packet_Store_ST_ *pStorePkt = NULL;
	int i = 0;
	
	if(curReadPos >= pRingBuffer->uPacketNum || curReadPos < 0 
		|| curWritePos >= pRingBuffer->uPacketNum || curWritePos < 0)
	{
		log_warn("can't find rtp packet, pos from %d to %d", curReadPos, curWritePos);
		return 0;
	}

	if(pRingBuffer->uPreSeq == pRingBuffer->uPreReadPos)
	{
		return curWritePos;
	}

	if(curReadPos < curWritePos)
	{
		for(i = curReadPos; i <= curWritePos; i++)
		{
			pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + i*pRingBuffer->uPacketSize);
			if(pStorePkt->uForRead)
			{
				log_info("find rtp packet, read:%d, write:%d, at pos:%d, seq:%d, len:%d", 
					curReadPos, curWritePos, i, pStorePkt->uPktSeq, pStorePkt->uPktLen);
				pRingBuffer->uPreReadSeq = pStorePkt->uPktSeq - 1;
				return i;
			}
		}
	}
	else
	{
		if(abs(curWritePos+pRingBuffer->uPacketNum - curReadPos) > pRingBuffer->uMaxPktWin)
		{
			for(i = curReadPos; i < curWritePos+pRingBuffer->uPacketNum; i++) /* discard all packet */
			{
				pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + (i%pRingBuffer->uPacketNum)*pRingBuffer->uPacketSize);
				pStorePkt->uForRead = 0;
				log_warn("over max window size, read pos:%d, write pos:%d ", curReadPos, curWritePos);
			}
			return curWritePos;
		}
		
		for(i = curReadPos; i <= curWritePos+pRingBuffer->uPacketNum; i++)
		{
			pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + (i%pRingBuffer->uPacketNum)*pRingBuffer->uPacketSize);
			if(pStorePkt->uForRead)
			{
				log_info("find valid rtp packet, read:%d, write:%d, at pos:%d, seq:%d, len:%d", 
					curReadPos, curWritePos, i%pRingBuffer->uPacketNum, pStorePkt->uPktSeq, pStorePkt->uPktLen);
				pRingBuffer->uPreReadSeq = pStorePkt->uPktSeq - 1;
				return (i%pRingBuffer->uPacketNum);
			}
		}
	}
	
	log_warn("can't find valid rtp packet, pos from %d to %d", curReadPos, curWritePos);
	return curWritePos;
}

PJ_DEF(int)  pjmedia_ringbuffer_scan_lost(_RingBuffer_ST_* pRingBuffer, int * pCache, unsigned nMax)
{
	int nLostGrpCnt = 0;
	int  curReadPos, curWritePos;
	_Packet_Store_ST_ *pStorePkt = NULL;
	unsigned short curReadSeq, curWriteSeq, lostSeq;
	int i = 0;
	int prevLost = 0;
	
	if(!pRingBuffer || !pCache || !nMax)
	{
		log_error("parameter error, pRingBuffer:%p, pCache:%p, nMax:%u", pRingBuffer, pCache, nMax);
		return nLostGrpCnt;
	}

	for(i = 0; i < nMax; i++)
	{
		pCache[i] = 0;
	}
	
	if(-1 == pRingBuffer->uPrePos || -1 == pRingBuffer->uPreReadPos 
		|| -1 == pRingBuffer->uPreSeq || -1 == pRingBuffer->uPreReadSeq 
		||pRingBuffer->uPreReadPos == pRingBuffer->uPrePos)
	{
		return 0;
	}

	curReadPos = (pRingBuffer->uPreReadPos+1) % pRingBuffer->uPacketNum;
	curReadSeq = (pRingBuffer->uPreReadSeq+1)%65536;
	
	curWritePos = (pRingBuffer->uPrePos) % pRingBuffer->uPacketNum;
	curWriteSeq = pRingBuffer->uPreSeq%65536;

	if(curReadPos < curWritePos)
	{
		for(i = curReadPos; i <= curWritePos; i++)
		{
			pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + i*pRingBuffer->uPacketSize);
			if(pStorePkt->uForRead)
			{
				if(prevLost)
				{
					nLostGrpCnt++;					
					prevLost = 0;
					if(nLostGrpCnt >= nMax)
					{
						break;
					}
				}
				continue;
			}
			else
			{
				if(++pStorePkt->uResendFlg >= _max_require_resend) /* over max require resend times */
				{
					continue;
				}
				
				lostSeq = (curReadSeq + (i - curReadPos))%65536;
				if(prevLost)
				{
					pCache[nLostGrpCnt] = (pCache[nLostGrpCnt] & 0xFFFF0000) | lostSeq;
				}
				else
				{
					pCache[nLostGrpCnt] = (lostSeq << 16) | lostSeq;
					prevLost = 1;
				}
			}
			
		}
	}

	if(curReadPos > curWritePos)
	{
		for(i = curReadPos; i <= curWritePos+pRingBuffer->uPacketNum; i++)
		{
			pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + (i%pRingBuffer->uPacketNum)*pRingBuffer->uPacketSize);
			if(pStorePkt->uForRead)
			{
				if(prevLost)
				{
					nLostGrpCnt++;					
					prevLost = 0;
					if(nLostGrpCnt >= nMax)
					{
						break;
					}
				}
				continue;
			}
			else
			{
				if(++pStorePkt->uResendFlg >= _max_require_resend) /* over max require resend times */
				{
					continue;
				}
				
				lostSeq =(curReadSeq + (i - curReadPos))%65536;
				if(prevLost)
				{
					pCache[nLostGrpCnt] = (pCache[nLostGrpCnt] & 0xFFFF0000) | lostSeq;
				}
				else
				{
					pCache[nLostGrpCnt] = (lostSeq << 16) | lostSeq;
					prevLost = 1;
				}
			}
		}
	}

	for(i = 0; i < nLostGrpCnt; i++)
	{
		log_warn("rpos:%d, wpos:%d, grp:[%d], from:%d, to:%d", 
			curReadPos, curWritePos,  i, pCache[i]>>16, pCache[i]&0x0000FFFF);			
	}

	//nLostGrpCnt = 0;

	return nLostGrpCnt;
}

_RingBuffer_ST_* pjmedia_ringbuffer_alloc(/*pj_pool_t *pool,*/ unsigned mtu, unsigned num)
{
	_RingBuffer_ST_* pRingBuffer = NULL;
	pRingBuffer = (_RingBuffer_ST_*)malloc(sizeof(_RingBuffer_ST_));
	//pRingBuffer = (_RingBuffer_ST_*)PJ_POOL_ZALLOC_T(pool, _RingBuffer_ST_);
	if(!pRingBuffer)
	{
		log_error("allocate memory for _RingBuffer_ST_ failure.");
		return NULL;
	}
	
	pRingBuffer->xbuffer = (unsigned char*)malloc(mtu*num);
	//pRingBuffer->xbuffer = pj_pool_alloc(pool, mtu*num);
	if(!pRingBuffer->xbuffer)
	{
		log_error("allocate memory for xbuffer failure.");
		return NULL;
	}	
	
	pj_bzero(pRingBuffer->xbuffer, mtu*num);
	
	return pRingBuffer;
}

PJ_DEF(void)  pjmedia_ringbuffer_destory(_RingBuffer_ST_* pRingBuffer)
{
	//PJ_UNUSED_ARG(pRingBuffer);
	if(pRingBuffer->xbuffer)
		free(pRingBuffer->xbuffer);
	free(pRingBuffer);
	return;
}

PJ_DEF (void) pjmedia_ringbuffer_init(_RingBuffer_ST_* pRingBuffer)
{
	if(!pRingBuffer)
	{
		log_error("pRingBuffer is null.");
		return;
	}
	pRingBuffer->uPacketSize = _video_pk_length_;
	pRingBuffer->uPacketNum = _video_pk_num_;
	pRingBuffer->uMaxPktWin = _max_rtp_seq_windows;

	pRingBuffer->uPreSeq = -1;
	pRingBuffer->uPrePos = -1;
	pRingBuffer->uPreReadPos = -1;
	pRingBuffer->uPreTime = 0;
	pRingBuffer->uBufsize = 0;
	pRingBuffer->uPacketCount = 0;
	pRingBuffer->uPreReadSeq = -1;
	pRingBuffer->uPreReadTS = 0;
	pRingBuffer->uCuPktLost = 0;
	if(pRingBuffer->resend_support)
	{
	    pRingBuffer->uMaxPktTime = _max_lost_wait_time;
	}
	else
	{
	    pRingBuffer->uMaxPktTime = 0;
	}
	pj_bzero(pRingBuffer->xbuffer, _video_pk_length_*_video_pk_num_);
	log_info("finish,resend_support:%d,uMaxPktTime:%d",pRingBuffer->resend_support,pRingBuffer->uMaxPktTime);
}


PJ_DEF (pj_status_t) pjmedia_ringbuffer_read(_RingBuffer_ST_* pRingBuffer, unsigned char* pRtpPkt, unsigned* pRtpLen, pjmedia_format_id   fmt_id)
{
	pj_status_t status = PJ_FALSE;
	int  curReadPos = -1;
	_Packet_Store_ST_ *pStorePkt = NULL;
	int offset;
	
	if(!pRingBuffer || !pRtpPkt || !pRtpLen)
	{
		log_error("parameter error, pRingBuffer:%p, pRtpPkt:%p, pRtpLen:%p", pRingBuffer, pRtpPkt, pRtpLen);
		return status;
	}

	if(-1 == pRingBuffer->uPreReadPos)
	{
		if(-1 != pRingBuffer->uPrePos)
		{
			curReadPos = 0;
		}
		else
		{	
			//log_debug("no packet for reading at this time.");
			return status;
		}	
		pRingBuffer->uPreReadTS = GetCurrentMsec();
	}
	else
	{
		curReadPos = (pRingBuffer->uPreReadPos + 1) % pRingBuffer->uPacketNum;
	}

	pStorePkt = (_Packet_Store_ST_ *)(pRingBuffer->xbuffer + curReadPos*pRingBuffer->uPacketSize);

	if(!pStorePkt->uForRead)
	{
		if(pRingBuffer->uPreSeq == pRingBuffer->uPreReadSeq) /* read equal write */
		{
			return status;
		}
		
		if(abs(GetCurrentMsec() - pRingBuffer->uPreReadTS) <= pRingBuffer->uMaxPktTime)
		{
			if(curReadPos < pRingBuffer->uPrePos)
			{
				log_error("read pos:%d, write pos:%d, expect seq:%d readTs:%d maxTime:%d, may be lost", 
				curReadPos, pRingBuffer->uPrePos, (pRingBuffer->uPreReadSeq+1)%65536, pRingBuffer->uPreReadTS, pRingBuffer->uMaxPktTime);
			}
			else
			{
				//log_debug("pos:%d, expect seq:%d, may not be ready", curReadPos, (pRingBuffer->uPreReadSeq+1)%65536);
			}
		}
		else
		{
			log_error("pos:%d, expect seq:%d, wait timeout, skip it", curReadPos, (pRingBuffer->uPreReadSeq+1)%65536);
			pRingBuffer->uCuPktLost++;
			if(pRingBuffer->uCuPktLost >= 3)  /* skip to valid packet pos */
			{
				pRingBuffer->uCuPktLost = 0;
				offset = pjmedia_ringbuffer_find_sps(pRingBuffer, curReadPos, pRingBuffer->uPrePos, pStorePkt->uPktLen, fmt_id);//THIS LINE IS ADDED
				pRingBuffer->uPreReadPos = pjmedia_ringbuffer_move_pos(pRingBuffer, offset, pRingBuffer->uPrePos) - 1;
				//pRingBuffer->uPreReadPos = pjmedia_ringbuffer_move_pos(pRingBuffer, curReadPos, pRingBuffer->uPrePos) - 1;
			}
			else
			{
				if(pRingBuffer->uPreReadPos < pRingBuffer->uPrePos)
				{
					pRingBuffer->uPreReadPos++;
					pRingBuffer->uPreReadSeq++;	
				}
			}

			pRingBuffer->uPreReadTS = GetCurrentMsec();
		}		
		return status;
	}
	
	pj_memcpy(pRtpPkt, pStorePkt->xRtpBuf, pStorePkt->uPktLen);
	*pRtpLen = pStorePkt->uPktLen;


	status = PJ_TRUE;
	pStorePkt->uForRead = 0;
	pStorePkt->uResendFlg = 0;
	pRingBuffer->uPreReadPos = curReadPos;
	pRingBuffer->uPreReadSeq = pStorePkt->uPktSeq;
	pRingBuffer->uPreReadTS = pStorePkt->uTimeStamp;
	pRingBuffer->uCuPktLost = 0;

	log_info("rtp seq:%u, len:%d, from buffer pos:%u", pStorePkt->uPktSeq, pStorePkt->uPktLen, curReadPos);

	return status;
}


/* add by j33783 20200422 */
PJ_DEF(void) pjmedia_jbuf_set_resend_wait_time(IBaseRunLib * pRtl, unsigned int uMaxWaitTimeMS)
{
	if(!pRtl)
	{
		log_error("parameter error, pRtl:%p", pRtl);
		return;
	}

	if(!pRtl->pRingBuffer)
	{
		log_error("parameter error, pRtl->pRingBuffer:%p", pRtl->pRingBuffer);
		return;
	}

	log_info("rtt time:%u", uMaxWaitTimeMS);

	if(uMaxWaitTimeMS + 100 < 300)
	{
		uMaxWaitTimeMS = 300;
	}
	else if(uMaxWaitTimeMS + 100 > 30000)
	{
		uMaxWaitTimeMS = 30000;
	}
	else
	{
		uMaxWaitTimeMS += 100;
	}

	pRtl->pRingBuffer->uMaxPktTime = uMaxWaitTimeMS;

	log_info("set packet wait time:%u", pRtl->pRingBuffer->uMaxPktTime);
	
}
