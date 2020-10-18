

#include <sys/time.h>
#include <stdlib.h>
#include "glog.h"
#include "rtp.h"
#include "utils.h"
#include "jitter_buffer.h"


pj_ssize_t getCurrentTimeMs()
{
	struct timeval tv_cur;
	gettimeofday(&tv_cur, NULL);
	return tv_cur.tv_sec*1000 + tv_cur.tv_usec/1000; 
}

pj_status_t ringbuffer_create(unsigned packet_size, pj_bool_t resend_support, RingBuffer **p_jb)
{
	RingBuffer*prb = ringbuffer_alloc( VIDEO_PACKET_LENGTH, MAX_PACKET_NUMBER);
    prb->resend_support = resend_support;
	ringbuffer_init(prb);

	*p_jb = prb;
	return PJ_SUCCESS;
}

RingBuffer* ringbuffer_alloc( unsigned mtu, unsigned packetNum) {
	RingBuffer* pRingBuffer = (RingBuffer*)malloc(sizeof(RingBuffer));
	if(!pRingBuffer)
	{
		log_error("allocate memory for _RingBuffer_ST_ failure.");
		return NULL;
	}
	
	pRingBuffer->xbuffer = (unsigned char*)malloc(mtu*packetNum);
	if(!pRingBuffer->xbuffer)
	{
		log_error("allocate memory for xbuffer failure.");
		return NULL;
	}	
	
	pj_bzero(pRingBuffer->xbuffer, mtu*packetNum);
	
	return pRingBuffer;
}

void ringbuffer_destory(RingBuffer* pRingBuffer)
{
	//PJ_UNUSED_ARG(pRingBuffer);
	if(pRingBuffer->xbuffer)
		free(pRingBuffer->xbuffer);

	free(pRingBuffer);

	return;
}

void ringbuffer_init(RingBuffer* pRingBuffer)
{
	if(!pRingBuffer)
	{
		log_error("pRingBuffer is null.");
		return;
	}
	pRingBuffer->uPacketSize = VIDEO_PACKET_LENGTH;
	pRingBuffer->uPacketNum  = MAX_PACKET_NUMBER;
	pRingBuffer->uMaxPktWin  = MAX_RTP_SEQ_WINDOW;

	pRingBuffer->uPreSeq 		= -1;
	pRingBuffer->uPrePos 		= -1;
	pRingBuffer->uPreReadPos 	= -1;
	pRingBuffer->uPreTime 		= 0;
	pRingBuffer->uBufsizeTotal 	= 0;
	pRingBuffer->uPacketCount 	= 0;
	pRingBuffer->uPreReadSeq 	= -1;
	pRingBuffer->uPreReadTS 	= 0;
	pRingBuffer->uCuPktLost 	= 0;
	if(pRingBuffer->resend_support)
	{
	    pRingBuffer->uMaxPktTime = MAX_LOST_WAIT_TIME_MS;
	}
	else
	{
	    pRingBuffer->uMaxPktTime = 0;
	}
	pj_bzero(pRingBuffer->xbuffer, VIDEO_PACKET_LENGTH*MAX_PACKET_NUMBER);
	log_error("finish,resend_support:%d,uMaxPktTime:%d MAX_PACKET_NUMBER:%d\n", pRingBuffer->resend_support, pRingBuffer->uMaxPktTime, pRingBuffer->uPacketNum);
}

int ringbuffer_write(RingBuffer* pRingBuffer, unsigned char* pRtpPkt, unsigned uRtpLen)
{
	int status = SEQ_OK;
	unsigned short packSeq = 0;
	Packet_Store_ST_ *pStorePkt = NULL;
	int index = 0;
	pj_ssize_t uCurTS;
	int nSeqGap = 0;
	
	
	if(!pRingBuffer || !pRtpPkt)
	{
		log_error("parameter error, pRingBuffer:%p, pRtpPkt:%p", pRingBuffer, pRtpPkt);
		return SEQ_NORMAL_ERROR;
	}

	if(uRtpLen <= RTP_HEAD_LENGTH || uRtpLen > (pRingBuffer->uPacketSize - sizeof(Packet_Store_ST_)))
	{
		log_error("parameter error, uRtpLen:%u", uRtpLen);
		return SEQ_NORMAL_ERROR;
	}

	packSeq = pj_ntohs(*(unsigned short*)(pRtpPkt+2));

	/* check max rtp seq winodow */
	// nSeqGap = (packSeq - pRingBuffer->uPreSeq)%65536;
	// if(abs(nSeqGap) > pRingBuffer->uMaxPktWin)
	// {
	// 	log_error("current seq:%u, prev seq:%u, gap:%d, is over max rtp seq windows:%d", 
	// 		packSeq, pRingBuffer->uPreSeq, nSeqGap, pRingBuffer->uMaxPktWin);
	// 	pjmedia_ringbuffer_init(pRingBuffer);
	// 	nSeqGap = 1;
	// }

	/* check rtp packet recv delay */
	uCurTS = getCurrentTimeMs();
	if(0 == pRingBuffer->uPreTime)
	{
		pRingBuffer->uPreTime = uCurTS;
	}

	if((uCurTS - pRingBuffer->uPreTime) > MAX_LOST_WAIT_TIME_UNKNOW)
	{
		log_error("current ts:%ld, prev ts:%ld is over max rtp wait time:%u", 
			uCurTS, pRingBuffer->uPreTime, pRingBuffer->uMaxPktTime);
		ringbuffer_init(pRingBuffer);
		nSeqGap = 1;
	}

	/* store the rtp packet */
	index = packSeq%pRingBuffer->uPacketNum;
	pStorePkt = (Packet_Store_ST_*)(pRingBuffer->xbuffer + index*pRingBuffer->uPacketSize);	
	pStorePkt->uForRead 	= 1;
	pStorePkt->uResendFlg 	= 0;
    pStorePkt->uPktSeq 		= packSeq;		
    pStorePkt->uTimeStamp 	= uCurTS;
	pStorePkt->uPktLen 		= uRtpLen;	
	pj_memcpy(pStorePkt->xRtpBuf, pRtpPkt, uRtpLen);

	/* update the ringbuffer record */

	if(packSeq - pRingBuffer->uPreSeq != 1) {
		if(-1!=pRingBuffer->uPreSeq && MAX_RTP_SEQ_VALUE!=(pRingBuffer->uPreSeq-packSeq)) {
			if(packSeq>pRingBuffer->uPreSeq) {
				LostPackets *lp = &pRingBuffer->lostPack;
				lp->bgnSeq = pRingBuffer->uPreSeq + 1;
				lp->endSeq = packSeq - 1;
				lp->packCount = lp->endSeq - lp->bgnSeq +1;
				status = SEQ_DISCONTINUOUS;
				log_error("find discontinuous package diff:%d cur:%d last:%d \n", 
					packSeq-pRingBuffer->uPreSeq-1, packSeq, pRingBuffer->uPreSeq);
			}
			else {
				status = SEQ_DISORDER;
				log_error("find disorder package diff:%d cur:%d last:%d \n", 
					packSeq-pRingBuffer->uPreSeq-1, packSeq, pRingBuffer->uPreSeq);
			}
		}
	}

	pRingBuffer->uPrePos    = index;	
	pRingBuffer->uPreSeq    = packSeq;	//current can read packet seq
	pRingBuffer->uPreTime   = uCurTS;
	pRingBuffer->uBufsizeTotal  += uRtpLen;
    pRingBuffer->uPacketCount++;

	log_error("rtp seq:%u, len:%d, to buffer pos:%d", packSeq, pStorePkt->uPktLen, index);
	
	return status; 
}

pj_status_t ringbuffer_read(RingBuffer* pRingBuffer, unsigned char* pRtpPkt, unsigned* pRtpLen)
{
	pj_status_t status = PJ_FALSE;
	int  curReadSeq = -1, curReadPos = -1, canRead = 0;
	Packet_Store_ST_ *pStorePkt = NULL;
	int offset;
	
	if(!pRingBuffer || !pRtpPkt || !pRtpLen)
	{
		log_error("parameter error, pRingBuffer:%p, pRtpPkt:%p, pRtpLen:%p", pRingBuffer, pRtpPkt, pRtpLen);
		return status;
	}

	if(pRingBuffer->uPreSeq == pRingBuffer->uPreReadSeq) //read equal write
	{
		return status;
	}

	if(-1 != pRingBuffer->uPreReadSeq)
	{
		curReadSeq = pRingBuffer->uPreReadSeq+1;
	}
	else	//first read
	{
		if(-1 != pRingBuffer->uPreSeq)
		{
			curReadSeq = pRingBuffer->uPreSeq;
		}
		else //maybe cannot run here
		{
			return status;
		}
	}

	if(curReadSeq!=-1) {
		curReadPos = curReadSeq%pRingBuffer->uPacketNum;
		pStorePkt = (Packet_Store_ST_ *)(pRingBuffer->xbuffer + curReadPos*pRingBuffer->uPacketSize);
		if( pStorePkt->uForRead )
		{
			canRead = 1;
			pRingBuffer->uPreReadTS = getCurrentTimeMs();//normal read time
		}
		else
		{
			if(((getCurrentTimeMs() - pRingBuffer->uPreReadTS) <= MAX_LOST_WAIT_TIME_MS)) {
				//return
			}
			else
			{
				//calculate lost packate, send fir and find idr frame
				canRead = 1;
				pRingBuffer->uCuPktLost++;
				log_error("lost packet count:%d seq %d\n", pRingBuffer->uCuPktLost, curReadSeq);
			}
		}

		if(canRead) {
			pj_memcpy(pRtpPkt, pStorePkt->xRtpBuf, pStorePkt->uPktLen);
			*pRtpLen = pStorePkt->uPktLen;

			pStorePkt->uForRead 		= 0;
			pStorePkt->uResendFlg 		= 0;

			pRingBuffer->uPreReadPos 	= curReadPos;
			pRingBuffer->uPreReadSeq 	= curReadSeq;
			//pRingBuffer->uPreReadTS 	= pStorePkt->uTimeStamp;

			status = PJ_TRUE;

			log_error("rtp seq:%u, len:%d, from buffer pos:%u", pStorePkt->uPktSeq, pStorePkt->uPktLen, curReadPos);
		}

	}

	return status;
}

