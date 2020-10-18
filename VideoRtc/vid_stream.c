


#include <time.h>
#include <stdint.h>
#include <stdio.h>



#include "utils.h"
#include "basedef.h"
#include "vid_codec.h"
#include "callCplus.h"

#include "vid_stream.h"
#include "video_rtc_api.h"

#if defined(PJMEDIA_HAS_VIDEO) && (PJMEDIA_HAS_VIDEO != 0)

#define THIS_FILE			"vid_stream.c"
#define ERRLEVEL			1
#define LOGERR_(expr)			stream_perror expr
#define TRC_(expr)			log_trace(expr)
#define SIGNATURE			PJMEDIA_SIG_PORT_VID_STREAM
#endif

/* Tracing jitter buffer operations in a stream session to a CSV file.
 * The trace will contain JB operation timestamp, frame info, RTP info, and
 * the JB state right after the operation.
 */

#ifndef PJMEDIA_VSTREAM_SIZE
#   define PJMEDIA_VSTREAM_SIZE	1000
#endif

#ifndef PJMEDIA_VSTREAM_INC
#   define PJMEDIA_VSTREAM_INC	1000
#endif

typedef enum {
	_SOFT_CODING_SOFT_DECODING_,
	_SOFT_CODING_HARD_DECODING_,
	_HARD_CODING_SOFT_DECODING_,
	_HARD_CODING_HARD_DECODING_
}pjsua_call_hardware_e;

/*add for h264 and h265 define by 180732700, 20191113*/
typedef enum {
	AVC_NAL_SINGLE_NAL_MIN	= 1,
	AVC_NAL_UNIT_IDR = 5,
    AVC_NAL_UNIT_SEI = 6,
	AVC_NAL_UNIT_SPS = 7,
	AVC_NAL_UNIT_PPS = 8,
    AVC_NAL_SINGLE_NAL_MAX	= 23,
    AVC_NAL_STAP_A		= 24,
    AVC_NAL_FU_TYPE_A	= 28,
}avc_nalu_type_e;

typedef enum {
	HEVC_NAL_UNIT_IDR = 19,
	HEVC_NAL_UNIT_VPS = 32,	//32
	HEVC_NAL_UNIT_SPS = 33,	//33
	HEVC_NAL_UNIT_PPS = 34,	//34
	HEVC_NAL_FU_TYPE  = 49,
}hevc_nalu_type_e;
/*end for h264 and h265 define by 180732700, 20191113*/

/* add by 180732700 for call type 20200423*/
typedef enum {
    _PNAS_CALL_TYPE_FULL_DUPLEX_VOICE_SINGLE_CALL = 1,
	_PNAS_CALL_TYPE_HALF_DUPLEX_SINGLE_VOICE_CALL = 2,
	_PNAS_CALL_TYPE_VOICE_GROUP_CALL = 3,
	_PNAS_CALL_TYPE_ENV_LISTENING_CALL = 4,
	_PNAS_CALL_TYPE_FULL_DUPLEX_VOICE_VIDEO_SINGLE_CALL = 10,
	_PNAS_CALL_TYPE_SAME_VOICE_VIDEO_SOURCE_GROUP_CALL = 11,
	_PNAS_CALL_TYPE_DIFFERENT_VOICE_VIDEO_SOURCE_GROUP_CALL = 12,
	_PNAS_CALL_TYPE_VIDEO_PUSH_CALL = 13,
	_PNAS_CALL_TYPE_VIDEO_UP_PULL_CALL = 14,
	_PNAS_CALL_TYPE_VIDEO_DOWN_PULL_CALL = 15,
	_PNAS_CALL_TYPE_VIDEO_BACK_TRANSFER_CALL = 16,
	_PNAS_CALL_TYPE_ENV_WATCHING_CALL = 17,
	_PNAS_CALL_TYPE_HALF_DUPLEX_ACCEPT_CALL = 18,
	_PNAS_CALL_TYPE_VIDEO_GROUP_PUSH_CALL = 25,
	_PNAS_CALL_TYPE_VOICE_BROADCAST = 160,
	_PNAS_CALL_TYPE_VOICE_VIDEO_BROADCAST = 161,
	_PNAS_CALL_TYPE_VIDEO_BROADCAST = 162
}pnas_call_type_e;


pjmedia_vid_stream g_vid_stream;

static void on_rx_rtp(void *useData, void *pkt, pj_ssize_t bytes_read)
{
	//log_error("on_rx_rtp bytes_read:%d.", bytes_read);
	pjmedia_vid_stream *stream = (pjmedia_vid_stream *)useData;
	//rtp_packet_recv(stream, pkt, bytes_read);
	//calculation_video_rtp_loss

	int result = ringbuffer_write(stream->ringbuf, (pj_int8_t*)pkt, bytes_read);
	if(SEQ_DISCONTINUOUS == result) {
		//send nack
		LostPackets *lp = &stream->ringbuf->lostPack;
		log_error("on_rx_rtp discontinuous pack number:%d begin:%d end:%d\n", lp->packCount, lp->bgnSeq, lp->endSeq);

		if(lp->packCount>0) {
			char buffer[PJMEDIA_MAX_MTU] = {0};
			rtcp_nack_packet *nack = (rtcp_nack_packet*)buffer;
			nack->number 	= lp->packCount;
			nack->start_seq = lp->bgnSeq;
			nack->end_seq	= lp->endSeq;
			transport_send_rtcp(stream->trans, buffer, sizeof(rtcp_nack_packet));
			log_error("on_rx_rtp resend rtcp length:%d\n", sizeof(rtcp_nack_packet));
		}

	}
}

static void on_rx_rtcp(void *useData, void *pkt, pj_ssize_t bytes_read)
{
	log_error("on_rx_rtcp bytes_read:%d.", (int)bytes_read);
	rtcp_nack_packet *nack = (rtcp_nack_packet*)pkt;
	log_error("on_rx_rtcp recv number:%d startseq:%d endseq:%d\n", nack->number, nack->start_seq, nack->end_seq);
}

FILE* fp = NULL;
const char*filename = "result.h264";

RTC_API
char* getVersion(void) {
    return VERSION;
}

RTC_API
int vid_stream_create(const char*localAddr, unsigned short localRtpPort, void*surface, int codecType) {
	int status = 0;
	memset(&g_vid_stream, 0, sizeof(pjmedia_vid_stream));
	g_vid_stream.fmt_id = (codecType==H264_HARD_CODEC)?PJMEDIA_FORMAT_H264:PJMEDIA_FORMAT_H265;
	g_vid_stream.codecType = codecType;
	
	g_vid_stream.rtp_session.out_pt = RTP_PT_H264;
	g_vid_stream.rtp_session.out_extseq = 0;
	g_vid_stream.rto_to_h264_obj = Launch_CPlus(g_vid_stream.fmt_id);
	status = ringbuffer_create(0, 1, &g_vid_stream.ringbuf);
	
	status = transport_udp_create(&g_vid_stream.trans, localAddr, localRtpPort, &on_rx_rtp, &on_rx_rtcp);
	if(status<0) {
		log_error("transport_udp_create failed.");
		return status;
	}

	//set send list rtp packet type 
	struct  rtp_sendto_thread_list_header *list= &g_vid_stream.trans->rtp_thread_list_header;
	list->pack_type = (codecType==H264_HARD_CODEC)?H264_PACKET:H265_PACKET;

	#ifdef __ANDROID__
	g_vid_stream.vid_port.decoder = NULL;
	g_vid_stream.vid_port.surface = surface;
	#endif

	g_vid_stream.vid_port.useStream = &g_vid_stream;
	g_vid_stream.trans->user_data 	= &g_vid_stream;

	//fp = fopen(filename, "wb");
	return status;
}

RTC_API
int vid_stream_destroy() {
	int result = -1;
	transport_udp_destroy(g_vid_stream.trans);  //release transport_udp* trans
	if (g_vid_stream.ringbuf) {
		ringbuffer_destory(g_vid_stream.ringbuf); //release IBaseRunLib* jb_opt
		g_vid_stream.ringbuf = NULL;
	}
	// if(fp) {
	// 	fclose(fp);
	// 	fp = NULL;
	// }

	return result;
}

RTC_API
int vid_stream_start(const char*remoteAddr, unsigned short remoteRtpPort) {
	int status = -1;
	status = vid_port_start(&g_vid_stream.vid_port);
	status = transport_udp_start( g_vid_stream.trans, remoteAddr, remoteRtpPort);
	return status;
}

RTC_API
int vid_stream_stop() {
	int status = -1;
	status = vid_port_stop(&g_vid_stream.vid_port);
	transport_udp_stop( g_vid_stream.trans);
	return status;
}

RTC_API
int packet_and_send(char* frameBuffer, int frameLen) {
    return packet_and_send_(&g_vid_stream, frameBuffer, frameLen);
}

int packet_and_send_(pjmedia_vid_stream*stream, char* frameBuffer, int frameLen) {
	int result = -1, ext_len = 0, mark = 0;
	char rtpBuff[2000] = {0};
	pjmedia_frame package_out = {0};

	long timestamp = get_currenttime_us();
	
	char ori_ext = pjmedia_rtp_rotation_ext(90);
	ext_len = pjmedia_video_add_rtp_exten(rtpBuff + sizeof(pjmedia_rtp_hdr), 0xEA, (pj_uint8_t*)&ori_ext, sizeof(pj_uint16_t));
	package_out.buf = rtpBuff + sizeof(pjmedia_rtp_hdr) + ext_len;

	/* Loop while we have frame to send */
	do
	{
		pj_status_t status = 0;
		if(stream->codecType == H264_HARD_CODEC)
			status = get_h264_package(frameBuffer, frameLen, &package_out);
		else
		{
			status = get_h265_package(frameBuffer, frameLen, &package_out);
		}
		
		mark = !package_out.has_more;
		rtp_update_hdr(&stream->rtp_session, rtpBuff, mark, package_out.size, timestamp );//set rtp head parameter and out_extseq++
		pjmedia_rtp_hdr *hdr_data = (pjmedia_rtp_hdr*)rtpBuff;
		hdr_data->x = (ext_len>0)?1:0;
		//if(stream->rtp_session.out_extseq!=5779)
		result = transport_send_rtp_seq(stream->trans, rtpBuff, package_out.size + sizeof(pjmedia_rtp_hdr) + ext_len, stream->rtp_session.out_extseq);
		if(package_out.enc_packed_pos>=frameLen)
			break;
		package_out.buf = rtpBuff + sizeof(pjmedia_rtp_hdr);
		ext_len = 0;

	}while(1);

	return result;
}

static pj_status_t send_rtcp_resend(pjmedia_vid_stream *stream, pj_uint16_t flag)
{
	void *sr_rr_pkt;
	pj_uint8_t *pkt;
	int len, max_len;
	pj_status_t status;

	/* Build RTCP RR/SR packet */
//	  pjmedia_rtcp_build_rtcp(&stream->rtcp, &sr_rr_pkt, &len);
	//pjmedia_rtcp_build_rtcp_rr(&stream->rtcp, &sr_rr_pkt, &len);

	if((flag & RTCP_SDES_FALG) != 0 || (flag & RTCP_BYTE_FALG) != 0)
	{ 
		pkt = (pj_uint8_t*) stream->out_rtcp_pkt;
		pj_memcpy(pkt, sr_rr_pkt, len);
		//max_len = stream->out_rtcp_pkt_size;
	} 
	else 
	{
		pkt = (pj_uint8_t*)sr_rr_pkt;
		//max_len = len;
	}
	
	log_debug("max_len=%d, out_rtcp_pkt_size=%d,len=%d", max_len,stream->out_rtcp_pkt_size,len);

	/* Build RTCP SDES packet */
	if((flag & RTCP_SDES_FALG) != 0)
	{	

	}

/* Build RTCP NACK packet */
	if((flag & RTCP_NACK_FALG) != 0)
	{
		pjmedia_rtcp_nack nack;
		pj_size_t nack_len;
		pj_bzero(&nack_len, sizeof(nack_len));
		// nack.ssrc = pj_htonl(stream->rtcp.peer_ssrc);
		// nack.base_seq = pj_htons(stream->dec->base_seq);
		// nack.flag = pj_htons(stream->dec->flag);
		nack_len = max_len - len;

		if (status != PJ_SUCCESS) 
		{
			log_error("Error generating RTCP NACK, status:%d", status);
		} 
		else 
		{
			len += (int)nack_len;
		}
		log_debug("rtcp nack packet, len=%d, nack_len=%d, status=%d", len,nack_len,status);
	}
	
	/* Build RTCP BYE packet */
	if((flag & RTCP_BYTE_FALG) != 0)
	{

	}
    
	/* add by j33783 20190805 Build RTCP FIR packet */
	if((flag & RTCP_FIR_FALG) != 0)
	{
		// pj_size_t bye_len;

		// bye_len = max_len - len;
		// status = pjmedia_rtcp_build_fir(&stream->rtcp, pkt+len, &bye_len);
		// if (status != PJ_SUCCESS) 
		// {
		// 	log_error("Error generating RTCP FIR, status:%d", status);
		// }
		// else
		// {
		// 	len += (int)bye_len;
		// }	
		// log_debug("rtcp fir packet, len=%d, max_len=%d,bye_len=%d", len,max_len,bye_len);
	}

	/* Send! */
	status = transport_send_rtcp(stream->trans, pkt, len);

	return status;
}

//-----------------------------------------------------------------------testing-----------------------------------------------------------------------------
#ifndef __ANDROID__
#ifndef TARGET_OS_IPHONE

int gCount = 0;
int rtp_packet_recv_264(pjmedia_vid_stream*stream, char* packetBuffer, int packetLen) {
	int status = 0, pk_len, frame_len;
	pj_int8_t rtpPack[PJMEDIA_MAX_MTU] = {0};
	
	//calculation_video_rtp_loss
	ringbuffer_write(stream->ringbuf, (pj_int8_t*)packetBuffer, packetLen);

	//pk_len = readPacketfromSub(stream->jb_opt, rtpPack, stream->fmt_id);
	ringbuffer_read(stream->ringbuf, rtpPack, &pk_len);
	if(pk_len>0) {
		frame_len = RTPUnpackH264(stream->rto_to_h264_obj, (char*)rtpPack, pk_len, &stream->rtp_unpack_buf);
		if(frame_len > 0)
		{
			gCount++;
			log_error("RTPUnpackH264 count:%d frame_len:%d\n", gCount, frame_len);
			// if(fp) {
			// 	fwrite(stream->rtp_unpack_buf, 1, frame_len, fp);
			// 	fflush(fp);
			// }
		}
	}

	return status;
}

int rtp_packet_and_unpack_notnet_test(pjmedia_vid_stream*stream, char* frameBuffer, int frameLen) {
	int result = -1, ext_len = 0, mark = 0;
	char rtpBuff[2000] = {0};
	pjmedia_frame package_out = {0};

	long timestamp = get_currenttime_us();
	char ori_ext = pjmedia_rtp_rotation_ext(90);
	ext_len = pjmedia_video_add_rtp_exten(rtpBuff + sizeof(pjmedia_rtp_hdr), 0xEA, (pj_uint8_t*)&ori_ext, sizeof(pj_uint16_t));
	package_out.buf = rtpBuff + sizeof(pjmedia_rtp_hdr) + ext_len;

	/* Loop while we have frame to send */
	do
	{
		pj_status_t res = get_h264_package(frameBuffer, frameLen, &package_out);
		mark = !package_out.has_more;
		rtp_update_hdr(&stream->rtp_session, rtpBuff, mark, package_out.size, timestamp );
		pjmedia_rtp_hdr *hdr_data = (pjmedia_rtp_hdr*)rtpBuff;
		hdr_data->x = (ext_len>0)?1:0;

		result = rtp_packet_recv_264(stream, rtpBuff, package_out.size + sizeof(pjmedia_rtp_hdr) + ext_len);
		if(package_out.enc_packed_pos>=frameLen)
			break;
		package_out.buf = rtpBuff + sizeof(pjmedia_rtp_hdr);
		ext_len = 0;

	}while(1);

	return result;
}

int stream_send_test(const char *localAddr, unsigned short localPort, const char*remoteAddr, unsigned short remotePort, const char*sendFile, int codecType) {
	int status = vid_stream_create(localAddr, localPort, NULL, codecType);//local
	if(status>=0)
		vid_stream_start( remoteAddr, remotePort);//remote
	getchar();
#ifdef SENDTEST
	vid_stream_test_file(sendFile);
#endif
	getchar();
	vid_stream_stop();
	log_error("vid_stream_stop done.");
	getchar();
	vid_stream_destroy();
	log_error("vid_stream_destroy done.");
	return status;
}

int stream_recv_test(const char *localAddr, short localPort, int codecType) {
	int status = 0;
	status = vid_stream_create(localAddr, localPort, NULL, codecType);//local
	if(status>=0)
		vid_stream_start( localAddr, localPort);//remote


	getchar();
	vid_stream_stop();
	log_error("vid_stream_stop done.");
	getchar();
	vid_stream_destroy();
	log_error("vid_stream_destroy done.");
	return status;
}

int stream_send_rtcp_test(const char *localAddr, short localPort, const char*remoteAddr,  short remotePort, int codecType) 
{
	int status = vid_stream_create(localAddr, localPort, NULL, codecType);//local
	if(status>=0)
		vid_stream_start( remoteAddr, remotePort);//remote
	getchar();
#ifdef SENDTEST
		char buffer[PJMEDIA_MAX_MTU] = {0};
		rtcp_nack_packet *nack = (rtcp_nack_packet*)buffer;
		nack->number 	= 2;
		nack->start_seq = 10000;
		nack->end_seq	= 10001;
		transport_send_rtcp(g_vid_stream.trans, buffer, sizeof(rtcp_nack_packet));
		log_error("on_rx_rtp resend rtcp length:%d\n", sizeof(rtcp_nack_packet));
#endif
	getchar();
	vid_stream_stop();
	log_error("vid_stream_stop done.");
	getchar();
	vid_stream_destroy();
	log_error("vid_stream_destroy done.");
	return status;
}

void rtp_packet_and_unpack_notnet_test_264(pj_uint8_t *frame, pj_size_t frame_len) {
	rtp_packet_and_unpack_notnet_test(&g_vid_stream, frame, frame_len);
}

void packet_and_send_test(pj_uint8_t *bits, pj_size_t bits_len) {
	//vid_stream_create();
	packet_and_send_(&g_vid_stream, bits, bits_len);
}

void h264_package_test(pj_uint8_t *bits, pj_size_t bits_len) {

	pj_uint8_t payload[PJMEDIA_MAX_VID_PAYLOAD_SIZE] = {0};
	pjmedia_frame package_out = {0};
	package_out.buf = payload;
	while(package_out.enc_packed_pos<bits_len) {
		pj_status_t res = get_h264_package(bits, bits_len, &package_out);
		log_debug("------get size:%d payload_len:%d enc_packed_pos:%d indicator:%02X res:%d\n", 
			(int)bits_len, (int)package_out.size, package_out.enc_packed_pos, payload[1], PJMEDIA_MAX_VID_PAYLOAD_SIZE);
	}
}

#endif //TARGET_OS_IPHONE
#endif //__ANDROID__















// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// static void resend_rtp(pjmedia_vid_stream *stream, pjmedia_vid_buf  *resend, pj_uint16_t pid_val,pj_uint16_t blp_val, pj_bool_t standard);

// static void create_and_send_heartbreak(pjmedia_vid_stream *stream,void *rtphdr);

// static pj_status_t send_rtcp_resend(pjmedia_vid_stream *stream, pj_uint16_t flag);
// #endif

// /* add by j33783 20190805 */
// static pj_status_t send_rtcp_fir(pjmedia_vid_stream *stream)
// {
// 	pj_status_t status = PJ_FALSE;
// 	pj_time_val tv;
// 	pj_uint64_t ts = 0;

// 	pj_gettimeofday(&tv);

// 	ts = tv.sec*1000 + tv.msec;
// 	if((ts - stream->send_fir_ts ) >= 500)
// 	{
// 		status = send_rtcp_resend(stream,RTCP_SDES_FALG | RTCP_FIR_FALG);
// 		log_debug("send RTCP FIR packet status:%d", status);
// 		stream->send_fir_ts = ts;
// 	}

// 	return status;
// }


// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// // \B7\A2\CB\CDrtcp\BF\D8\D6\C6֪ͨ\BDӿ\DA
// static void SendRtcpNackPacket(pjmedia_vid_stream *stream, pjmedia_resend_basereq_info*base_req_arr,pj_uint16_t usLastSeq, pj_uint16_t usCurSeq)
// {
// 	// \C5жϽӿڵ\C4\D0\F2\C1к\C5\D3\D0Ч\D0\D4
// 	int nSeqNum = usCurSeq - (usLastSeq + 1);
// 	if (nSeqNum <= 0)
// 	{
// 		return;
// 	}
// 	pjmedia_vid_channel *dec = stream->dec;

// 	//\BC\C7¼\C6\F0ʼ\B6\AAʧ\B5\C4\D0\F2\C1к\C5
// 	unsigned short usSeq = usLastSeq + 1;

// 	// \D7\EE\B6\E0ֻ\C4ܷ\A2\CB\CD16\B8\F6\D0\F2\C1к\C5ͼΪ
// 	if (nSeqNum > RTP_PACKET_SEQ_MAP_MAX)
// 	{
// 		nSeqNum = RTP_PACKET_SEQ_MAP_MAX;
// 	}

// 	// \C9趨\D0\F2\C1к\C5ͼλ
// 	unsigned short usSeqMap = 0;
// 	int i;
// 	for (i = 0; i < nSeqNum - 1; i ++)
// 	{
// 		usSeqMap += (1 << i);
// 	}


// 	int idx = usSeq % RESEND_REQ_BASESEQ_BUFF_LEN;
// 	if(base_req_arr[idx].base_seq != usSeq || base_req_arr[idx].seq_map != usSeqMap){
// 	//refresh the base_seq and seq_map
// 		base_req_arr[idx].base_seq = usSeq;
// 		base_req_arr[idx].req_times = 0;
// 		base_req_arr[idx].seq_map = usSeqMap;
// 	}else if(base_req_arr[idx].req_times > RESEND_TIMES_MAX){
// 	   //if the times of resend req is more than RESEND_TIMES_MAX, then return
// 		return;
// 	}
// 	dec->base_seq = usSeq;
// 	dec->flag = usSeqMap;
// 	base_req_arr[idx].req_times++;
// 	send_rtcp_resend(stream,RTCP_SDES_FALG | RTCP_NACK_FALG);

// 	return;
// }


// //\BC\EC\B2ⶪ\B0\FC\A3\AC\B2\A2\B7\A2\CB\CDRTCP\B0\FC\B8\F8\B7\A2\CBͶ\CB
// //void OnRtpLostHandleCallBack(void* rri, void * handle)
// static void resend_req_proc(pjmedia_vid_stream *stream)
// {
// 	BASE_MEDIA_CACHE_CH *pmci;
// 	if(stream == NULL || stream->jb_opt == NULL) return;
// 	pjmedia_resend_basereq_info base_req_arr[RESEND_REQ_BASESEQ_BUFF_LEN];
// 	memset(base_req_arr, 0x00, sizeof(base_req_arr));
// 	pmci = &(stream->jb_opt->catch_info); 
// 	pmci->lastRtcpTime = GetCurrentMsec();
// 	pjmedia_vid_channel *dec = stream->dec;

// 	/* begin, delete by l27913, 2019.12.24 */
// 	//pj_thread_sleep(100);
// 	/* end */

// 	while(stream->jbuf_stop !=  PJ_TRUE){
// 		// \B6\A8ʱ\B7\A2\CB\CDrtcp\BF\D8\D6ư\FC, 50msû\D3\D0\CAյ\BD\A3\AC\D6ط\A2
// 		if (GetCurrentMsec() - pmci->lastRtcpTime > _max_resend_wait_time)
// 		{
// 			// \B6\A8\D2\E5\BB\F1ȡ\B6\AAʧ\B5İ\FC\CA\FD\D7\E9
// 			int arrSeq[RTCP_RESEND_ARR_MAX_NUM];
// 			memset(arrSeq, 0x00, sizeof(arrSeq));
// 			// \BB\F1ȡ\D0\E8Ҫ\D6ش\AB\B5İ\FC
// 			int nCount = getLostPacket(stream->jb_opt, pmci, arrSeq, RTCP_RESEND_ARR_MAX_NUM,  _max_resend_wait_time,  _max_lost_wait_time);
// 			if (nCount > 0)
// 			{
// 				int i;			
// 				for (i = 0; i < nCount; i++)
// 				{
// 					// \BB\F1ȡ\BF\AAʼ\D0\F2\C1к\C5
// 					unsigned short  nSeqBgn = (unsigned short)(arrSeq[i] >> 16);
// 					// \BB\F1ȡ\BD\E1\CA\F8
// 					unsigned short  nSeqEnd = (unsigned short)(arrSeq[i] & 0xffff);//the end seq mask

// 					//\BC\C7¼\C6\F0ʼ\B6\AAʧ\B5\C4\D0\F2\C1к\C5	
// 					unsigned short usSeq = nSeqBgn;
// 					// \C9趨\D0\F2\C1к\C5ͼλ
// 					unsigned short usSeqMap = 0;

// 					//С\D3ڵ\C8\D3\DA17\A3\AC\D4ٵ\B1ǰ\B9\B9\D4\ECseqӳ\C9\E4
// 					if ((nSeqEnd - nSeqBgn < RTP_PACKET_SEQ_MAP_MAX) && (nSeqBgn <= nSeqEnd))
// 					{
// 						// \B9\B9\D4\ECӳ\C9\E4
// 						if(nSeqBgn < nSeqEnd){
// 							int nMap;
// 							for (nMap = nSeqBgn+1; nMap <= nSeqEnd; nMap++)
// 							{
// 								usSeqMap += (1 << (nMap - nSeqBgn - 1));
// 							}
// 						}

// 						// \BAϲ\A2\BF\C9\D2\D4\C8ںϵ\C4seqmap
// 						if(i < nCount - 1){
// 							int j;
// 							for (j = i + 1; j < nCount; j++)
// 							{
// 								// \BB\F1ȡ\BF\AAʼ\D0\F2\C1к\C5
// 								unsigned short  nSeqBgn2 = (unsigned short)(arrSeq[j] >> 16);
// 								// \BB\F1ȡ\BD\E1\CA\F8
// 								unsigned short  nSeqEnd2 = (unsigned short)(arrSeq[j] & 0xffff);

// 								// \C5ж\CF\D3\D0Ч\D0\D4
// 								if ((nSeqEnd2 - nSeqBgn < RTP_PACKET_SEQ_MAP_MAX) && (nSeqEnd < nSeqBgn2))
// 								{
// 									// \B9\B9\D4\ECӳ\C9\E4
// 									int nMap;
// 									for (nMap = nSeqBgn2; nMap <= nSeqEnd2; nMap++)
// 									{
// 										usSeqMap += (1 << (nMap - nSeqBgn - 1));
// 									}

// 									// \B8\B3ֵi =j
// 									i = j;

// 								}
// 								// \C8\E7\B9\FB\B4\F3\D3\DA17\CD˳\F6\B5\B1ǰѭ\BB\B7
// 								else
// 								{
// 									break;
// 								}

// 							}
// 						}

// 						// \B7\A2\CB\CDrtcp\B7\B4\C0\A1\B0\FC
// 						int idx = usSeq % RESEND_REQ_BASESEQ_BUFF_LEN;
// 						if(base_req_arr[idx].base_seq != usSeq || base_req_arr[idx].seq_map != usSeqMap){
// 							//refresh the base_seq and seq_map
// 							base_req_arr[idx].base_seq = usSeq;
// 							base_req_arr[idx].req_times = 0;
// 							base_req_arr[idx].seq_map = usSeqMap;
// 						}else if(base_req_arr[idx].req_times > 2){
// 						 //if the times of resend req is more than RESEND_TIMES_MAX, then return
// 							continue;
// 						}
// 						dec->base_seq = usSeq;
// 						dec->flag = usSeqMap;
// 						base_req_arr[idx].req_times++;
// 						log_debug("check_lost_rtp_proc_cycle  dec->base_seq=%d,dec->flag=%d",dec->base_seq,dec->flag);
// 						send_rtcp_resend(stream,RTCP_SDES_FALG | RTCP_NACK_FALG);
// 					}
// 					else
// 					{
						
// 						//\B7\D6\C5\FA\B7\A2\CB\CD
// 						if(nSeqEnd - nSeqBgn > DISCARD_RESEND_NUM_MAX)break;
// 						int nMap = 0;
// 						for (nMap = nSeqBgn; nMap <= (nSeqEnd - RTP_PACKET_SEQ_MAP_MAX); )
// 						{
// 							// \BB\F1ȡ\BD\E1\CA\F8\D0\F2\C1к\C5
// 							SendRtcpNackPacket(stream,base_req_arr,nMap - 1, nMap + RTP_PACKET_SEQ_MAP_MAX);
// 							// map \BC\D317
// 							nMap += RTP_PACKET_SEQ_MAP_MAX;
// 						}

// 						// \BB\F1ȡ\BD\E1\CA\F8\D0\F2\C1к\C5
// 						if (nMap < nSeqEnd)
// 						{
// 							SendRtcpNackPacket(stream,base_req_arr,nMap - 1, nSeqEnd + 1);
// 						}
// 					}

// 				}
// 			}

// 		}
// 		pj_thread_sleep(_max_resend_wait_time);

// 		}
// }


// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// static pj_status_t send_rtcp_resend(pjmedia_vid_stream *stream,
// 			     pj_uint16_t flag)
// {
// 	void *sr_rr_pkt;
// 	pj_uint8_t *pkt;
// 	int len, max_len;
// 	pj_status_t status;

// 	/* Build RTCP RR/SR packet */
// //	  pjmedia_rtcp_build_rtcp(&stream->rtcp, &sr_rr_pkt, &len);
// 	pjmedia_rtcp_build_rtcp_rr(&stream->rtcp, &sr_rr_pkt, &len);

// 	if((flag & RTCP_SDES_FALG) != 0 || (flag & RTCP_BYTE_FALG) != 0)
// 	{ 
// 		pkt = (pj_uint8_t*) stream->out_rtcp_pkt;
// 		pj_memcpy(pkt, sr_rr_pkt, len);
// 		max_len = stream->out_rtcp_pkt_size;
// 	} 
// 	else 
// 	{
// 		pkt = (pj_uint8_t*)sr_rr_pkt;
// 		max_len = len;
// 	}
	
// 	log_debug("max_len=%d, out_rtcp_pkt_size=%d,len=%d", max_len,stream->out_rtcp_pkt_size,len);

// 	/* Build RTCP SDES packet */
// 	if((flag & RTCP_SDES_FALG) != 0)
// 	{	
// 		pjmedia_rtcp_sdes sdes;
// 		pj_size_t sdes_len;

// 		pj_bzero(&sdes, sizeof(sdes));
// 		sdes.cname = stream->cname;
// 		sdes_len = max_len - len;
// 		status = pjmedia_rtcp_build_rtcp_sdes(&stream->rtcp, pkt+len,	  &sdes_len, &sdes);
// 		if (status != PJ_SUCCESS)
// 		{
// 			log_error("Error generating RTCP SDES, status:%d", status);
// 		} 
// 		else 
// 		{
// 			len += (int)sdes_len;
// 		}
		
// 		 log_debug("rtcp sdes packet, max_len=%d, sdes_len=%d,len=%d", max_len,sdes_len,len);
// 	}

// /* Build RTCP NACK packet */
// 	if((flag & RTCP_NACK_FALG) != 0)
// 	{
// 		pjmedia_rtcp_nack nack;
// 		pj_size_t nack_len;
// 		pj_bzero(&nack_len, sizeof(nack_len));
// 		nack.ssrc = pj_htonl(stream->rtcp.peer_ssrc);
// 		nack.base_seq = pj_htons(stream->dec->base_seq);
// 		nack.flag = pj_htons(stream->dec->flag);
// 		nack_len = max_len - len;
// 		log_debug("rtcp nack pakcet, ssrc=%x, nack_len=%d, base_seq=%d, flag=%d", 
// 					stream->rtcp.peer_ssrc, nack_len,nack.base_seq,nack.flag);
// 		status = pjmedia_rtcp_build_rtcp_nack(&stream->rtcp, pkt+len, &nack_len, &nack);
// 		if (status != PJ_SUCCESS) 
// 		{
// 			log_error("Error generating RTCP NACK, status:%d", status);
// 		} 
// 		else 
// 		{
// 			len += (int)nack_len;
// 		}
// 		log_debug("rtcp nack packet, len=%d, nack_len=%d, status=%d", len,nack_len,status);
// 	}
	
// 	/* Build RTCP BYE packet */
// 	if((flag & RTCP_BYTE_FALG) != 0)
// 	{
// 		pj_size_t bye_len;

// 		bye_len = max_len - len;
// 		status = pjmedia_rtcp_build_rtcp_bye(&stream->rtcp, pkt+len,	 &bye_len, NULL);
// 		if (status != PJ_SUCCESS) 
// 		{
// 			log_error("Error generating RTCP BYE, status:%d", status);
// 		}
// 		else
// 		{
// 			len += (int)bye_len;
// 		}	
// 		log_debug("rtcp bye packet, len=%d, max_len=%d,bye_len=%d", len,max_len,bye_len);
// 	}

// 	/* add by j33783 20190805 Build RTCP FIR packet */
// 	if((flag & RTCP_FIR_FALG) != 0)
// 	{
// 		pj_size_t bye_len;

// 		bye_len = max_len - len;
// 		status = pjmedia_rtcp_build_fir(&stream->rtcp, pkt+len, &bye_len);
// 		if (status != PJ_SUCCESS) 
// 		{
// 			log_error("Error generating RTCP FIR, status:%d", status);
// 		}
// 		else
// 		{
// 			len += (int)bye_len;
// 		}	
// 		log_debug("rtcp fir packet, len=%d, max_len=%d,bye_len=%d", len,max_len,bye_len);
// 	}

// 	/* Send! */
// 	status = pjmedia_transport_send_rtcp(stream->transport, pkt, len);

// 	return status;
// }
// #endif

// static pj_status_t send_rtcp(pjmedia_vid_stream *stream,
// 			     pj_bool_t with_sdes,
// 			     pj_bool_t with_bye)
// {
//     void *sr_rr_pkt;
//     pj_uint8_t *pkt;
//     int len, max_len;
//     pj_status_t status;

//     /* Build RTCP RR/SR packet */
//     pjmedia_rtcp_build_rtcp(&stream->rtcp, &sr_rr_pkt, &len);

//     if (with_sdes || with_bye) 
//     {
// 	 pkt = (pj_uint8_t*) stream->out_rtcp_pkt;
// 	 pj_memcpy(pkt, sr_rr_pkt, len);
// 	 max_len = stream->out_rtcp_pkt_size;
//     } 
//     else 
//     {
// 	 pkt = (pj_uint8_t*)sr_rr_pkt;
// 	 max_len = len;
//     }

//     /* Build RTCP SDES packet */
//     if (with_sdes) 
//    {
// 	pjmedia_rtcp_sdes sdes;
// 	pj_size_t sdes_len;

// 	pj_bzero(&sdes, sizeof(sdes));
// 	sdes.cname = stream->cname;
// 	sdes_len = max_len - len;
// 	status = pjmedia_rtcp_build_rtcp_sdes(&stream->rtcp, pkt+len, &sdes_len, &sdes);
// 	if (status != PJ_SUCCESS) 
// 	{
// 		log_error("Error generating RTCP SDES, status:%d",status);
// 	} 
// 	else 
// 	{
// 	len += (int)sdes_len;
// 	}
//     }

//     /* Build RTCP BYE packet */
//     if (with_bye) 
//    {
// 	pj_size_t bye_len;
// 	bye_len = max_len - len;
// 	status = pjmedia_rtcp_build_rtcp_bye(&stream->rtcp, pkt+len,  &bye_len, NULL);
// 	if (status != PJ_SUCCESS) 
// 	{
// 		log_error("Error generating RTCP BYE, status:%d",status);
// 	} 
// 	else 
// 	{
// 	    len += (int)bye_len;
// 	}
//      }

//     /* Send! */
//     status = pjmedia_transport_send_rtcp(stream->transport, pkt, len);

//     return status;
// }


// /**
//  * check_tx_rtcp()
//  *
//  * This function is can be called by either put_frame() or get_frame(),
//  * to transmit periodic RTCP SR/RR report.
//  */
// static void check_tx_rtcp(pjmedia_vid_stream *stream, pj_uint32_t timestamp)
// {
//     /* Note that timestamp may represent local or remote timestamp, 
//      * depending on whether this function is called from put_frame()
//      * or get_frame().
//      */


//     if (stream->rtcp_last_tx == 0)
//     {	
// 	stream->rtcp_last_tx = timestamp;
//     } 
//    else if (timestamp - stream->rtcp_last_tx >= stream->rtcp_interval) 
//    {
// 	pj_status_t status;
	
// 	status = send_rtcp(stream, !stream->rtcp_sdes_bye_disabled, PJ_FALSE);
// 	if (status != PJ_SUCCESS) 
// 	{
// 	    log_error("Error sending RTCP, status:%d", status);
// 	}

// 	stream->rtcp_last_tx = timestamp;
//     }
// }

// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE 
// static void resend_rtp(pjmedia_vid_stream *stream, pjmedia_vid_buf  *resend, pj_uint16_t pid_val,pj_uint16_t blp_val,pj_bool_t standard){
//        unsigned blp_bit_idx, num_max;
// 	pj_uint16_t    rtp_seq, rtp_pkt_len, index;
// 	pj_status_t status = 0;
// 	void *rtp_pkt;
// 	if(standard){
// 		num_max = 17;
// 	}else{
// 		num_max = 16;
// 	}
// 	for(blp_bit_idx = 0; blp_bit_idx < num_max; blp_bit_idx++){
// 		if((blp_bit_idx == 0 && standard) || (blp_val & 0x01) != 0){
// 			 rtp_seq = pid_val + blp_bit_idx;
// 			 index = rtp_seq % resend->buf_size;
// 			 if(resend->resend_times[index] < RESEND_TIMES_MAX){
// 			 	 resend->resend_times[index]++;
// 				 rtp_pkt = resend->buf + index * PJMEDIA_MAX_MTU;
// 				 rtp_pkt_len = resend->pkt_len[index];
// 				log_debug("rtp_pkt_len=%d,rtp_seq=%d",rtp_pkt_len,rtp_seq);
// 				 /* Resend the RTP packet to the transport. */
// 				 if(rtp_pkt && rtp_pkt_len > 0){
// 					 status = pjmedia_transport_priority_send_rtp(stream->transport,
// 							 (char*)rtp_pkt,rtp_pkt_len);
// 					 if(status != PJ_SUCCESS){
// 						 pjmedia_transport_priority_send_rtp(stream->transport,
// 								 (char*)rtp_pkt,rtp_pkt_len);
// 					 }
// 				}
// 			 }
// 		}
// 		if(standard){
// 			if(blp_bit_idx >0) blp_val >>= 1;
// 		}else{
// 			blp_val >>= 1;
// 		}
// 	}
// }

// static void create_and_send_heartbreak(pjmedia_vid_stream *stream,void *rtphdr){
// 	int mlen = 0;
// 	unsigned idx;
// 	pj_status_t status = 0;
// 	char mbuff[RESEND_HEARTBREAK_MAX_LEN];
// 	memset(mbuff,0x00,sizeof(mbuff));
// 	pj_memcpy(mbuff,rtphdr,sizeof(pjmedia_rtp_hdr));
// 	mlen = sizeof(pjmedia_rtp_hdr);
// 	*(int *)(mbuff + mlen) = htonl(RESEND_HEARTBREAK_TYPE);
// 	mlen += sizeof(pj_uint32_t);
// 	*(int *)(mbuff + mlen )= htonl(0);
// 	mlen += sizeof(pj_uint32_t);
// 	*(pj_uint16_t *)(mbuff + mlen) =  htons(0);
// 	mlen += sizeof(pj_uint16_t);
// 	*(pj_uint16_t *)(mbuff + mlen) =  htons(5);
// 	mlen += sizeof(pj_uint16_t);
// 	((pjmedia_rtp_hdr *) mbuff)->pt = HEARTBREAK_RTP_PT; 
	
// 	for(idx = 0; idx < RESEND_BREAKBEART_TIMES;idx++){
// 		status = pjmedia_transport_send_rtp(stream->transport,
// 				(char*)mbuff,mlen);
// 		if(status != PJ_SUCCESS){
// 			pjmedia_transport_send_rtp(stream->transport,
// 					(char*)mbuff,mlen);
// 		}
// 	}
	
// } 

// static pj_status_t check_rtp_nack_resend(pjmedia_vid_stream *stream,
//                         void *pkt, pj_ssize_t pkt_size){
// 	pj_status_t status = 0;
//     pj_uint8_t *p,blp_bit_idx,*pt, type;
//     pj_uint16_t rtp_seq,index,*p16,blp_val,pid_val,rtcp_len;
// 	unsigned rtp_pkt_len;
// 	unsigned rtcp_count = 0;
// 	void *rtp_pkt;
//      pj_bool_t nack_found = PJ_FALSE;
//     pj_uint32_t *nack_ptr; 
// 	pjmedia_vid_buf  *resend;

//    if(((pjmedia_rtp_hdr*)pkt)->pt == 127 && pkt_size > 12 && pkt_size <= 24){
// 	    p = (pj_uint8_t*)pkt;
// 	    p += 12;
// 	    nack_ptr = (pj_uint32_t *)p;
// 	    log_debug(" ntohs(*nack_ptr) =%d ",ntohl(*nack_ptr));
// 	    if(ntohl(*nack_ptr) == RTCP_NACK){

// 		//if(nack_found){
// 			p += 8 ;
// 			p16 = (pj_uint16_t*)p;
// 			pid_val = ntohs(*p16);
// 			p16++;
// 			blp_val = ntohs(*p16);

// 			//if(pid_val || blp_val){ 
// 			resend = stream->enc->resend_vbuf;
// 			resend_rtp(stream,resend,pid_val,blp_val,1); 
// 			return PJ_SUCCESS; // NACK
// 	    		//}
// 	    	}else{
// 	    	       return 5;//NOT NACK
// 		}
		
//    	}else {
// 		return 5;//NOT NACK
// 	}
   	
// }
// #endif


// static void calculation_video_rtp_loss(pjmedia_vid_stream *stream, const pjmedia_rtp_hdr *hdr)
// {
//     pj_uint16_t now_seq = pj_ntohs(hdr->seq);
   
//     if(!stream || !hdr)
//     {
//         log_error("the stream or hdr is NULL");
//         return;
//     }

// 	//log_debug("rtp seq:%d, mark:%d, loss_rtp:%d,all_rtp:%d",
// 	//	now_seq,hdr->m, stream->loss_rtp,stream->all_rtp);

// 	if((hdr->m && 0 == stream->last_seq) || (now_seq<stream->now_seq))
// 	{
// 	    return;
// 	}

//     if(0 == stream->last_seq)
//     {
//         stream->last_seq = now_seq;
// 		stream->start_seq = now_seq;
// 		stream->now_seq = now_seq;
// 		stream->all_rtp = 0;
// 		stream->loss_rtp = 0;
// 		return;
//     }

// 	if(now_seq - stream->last_seq>1)
// 	{
// 	    stream->loss_rtp += (now_seq - stream->last_seq - 1);
// 	    log_debug("loss video packet,last seq:%d, new seq:%d, loss rtp count:%d",
// 			stream->last_seq,now_seq,stream->loss_rtp);
// 	}

// 	stream->last_seq = now_seq;
	
// 	if(hdr->m)
// 	{
// 	     stream->last_seq = 0;
// 		 stream->all_rtp = now_seq - stream->start_seq + 1;
// 		 log_debug("after rtp loss_rtp:%d,all_rtp:%d",stream->loss_rtp,stream->all_rtp);
// 	}
// 	stream->now_seq = now_seq;
// }


// /*
//  * This callback is called by stream transport on receipt of packets
//  * in the RTP socket. 
//  */
// static void on_rx_rtp_process( void *data, void *pkt, pj_ssize_t bytes_read)

// {
//     pjmedia_vid_stream *stream = (pjmedia_vid_stream*) data;
//     pjmedia_vid_channel *channel = stream->dec;
//     const pjmedia_rtp_hdr *hdr = (pjmedia_rtp_hdr*)pkt;

// 	/* Check for errors and Ignore keep-alive packets */
// 	if ((bytes_read < 0) || (bytes_read < (pj_ssize_t) sizeof(pjmedia_rtp_hdr))) 
// 	{
// 		log_error("name:%.*s, RTP recv() error, bytes_read:%d", 
// 			channel->port.info.name.slen,
// 			channel->port.info.name.ptr, bytes_read);
// 		return;
// 	}

//     if (hdr->pt == HEARTBREAK_RTP_PT)	
// 	{		
// 		int nRHeartId = pj_ntohl(*((int*)pkt+3));
// 		if (307 == nRHeartId) {
// 			unsigned int unSeq = pj_ntohl(*((int*)pkt+4));
// 			unsigned int unJamAskTime = pj_ntohl(*((int*)pkt+5));
			
//             pj_int32_t downlink_bitrate=0; // kbps
//             float pLostRate=0.0000;
//             float pFrameRate=0.00;

//             if (stream->jb_opt) {
//             	pjmedia_jbuf_packet_recv_report(stream->jb_opt, &downlink_bitrate, &pLostRate, &pFrameRate); // from jbuf
//             }
// 			create_and_send_317(stream,hdr,unSeq,pLostRate,unJamAskTime);
// 		}
// 	}

//     /* add by t35599*/
//     if (stream->cctrl && stream->cc_irtp) 
// 	{
//        stream->cc_irtp(stream->cctrl, pkt, sizeof(pjmedia_rtp_hdr), bytes_read - sizeof(pjmedia_rtp_hdr));
//     }
//     /* end */

// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// 	if(check_rtp_nack_resend(stream,pkt,bytes_read) == PJ_SUCCESS)
// 	{
// 		return;
// 	}
// #endif

// 	/*ignore heartbreak rtp or nat rtp*/ 
// 	if (hdr->pt == HEARTBREAK_RTP_PT)	
// 	{		
// 		log_debug("ignore video rtp, pt:%d, seq:%d",hdr->pt,pj_ntohs(hdr->seq));		
// 		return; 
// 	}

//     calculation_video_rtp_loss(stream, hdr);

// 	writePackettoSub( stream->jb_opt, /*pj_int32_t id*/0, (pj_int8_t*)pkt, bytes_read,	pj_ntohs(hdr->seq), _max_lost_wait_time);
// }

// /*
//  * This callback is called by stream transport on receipt of packets
//  * in the RTP socket. 
//  */
// static void on_rx_rtp( void *data, void *pkt, pj_ssize_t bytes_read)

// {
//     pjmedia_vid_stream *stream = (pjmedia_vid_stream*) data;
//     pjmedia_vid_channel *channel = stream->dec;
//     pj_status_t status;

// 	/* Check for errors */
// 	if (bytes_read < 0) {
// 		log_error("name:%.*s, RTP recv() error, status:%d", 
// 			channel->port.info.name.slen,
// 			channel->port.info.name.ptr, status);
// 		return;
// 	}

// 	/* Ignore keep-alive packets */
// 	if (bytes_read < (pj_ssize_t) sizeof(pjmedia_rtp_hdr) && !stream->rstp_ip)
// 	return;

// #ifdef HYT_PTT
// 	if (((pjmedia_rtp_hdr*)pkt)->pt == 127 && 
// 		 pj_ntohs(((pjmedia_rtp_hdr*)pkt)->seq) < 30)	{		
// 		log_debug("receive vid rtp correction packet");		
// 		return; 
// 	}
// #endif

// 	if(PJMEDIA_RTP_PT_HYT_FEC == ((pjmedia_rtp_hdr*)pkt)->pt) return;
// 	on_rx_rtp_process(data, pkt, bytes_read);
// }

// /*
//  * This callback is called by stream transport on receipt of packets
//  * in the RTCP socket. 
//  */
// static void on_rx_rtcp( void *data,
//                         void *pkt, 
//                         pj_ssize_t bytes_read)
// {
//     pjmedia_vid_stream *stream = (pjmedia_vid_stream*) data;
// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// 	pj_status_t status = 0;
//     pj_uint8_t *p,blp_bit_idx,*pt, type;
//     pj_uint16_t rtp_seq,index,*p16,blp_val,pid_val,rtcp_len;
// 	unsigned rtp_pkt_len;
// 	unsigned rtcp_count = 0;
// 	void *rtp_pkt;
//      pj_bool_t nack_found = PJ_FALSE; 
//      pj_bool_t standard = PJ_FALSE; 	
//      pjmedia_vid_buf *resend = stream->enc->resend_vbuf;

// #endif

//     /* Check for errors */
//     if (bytes_read < 0) {
// 	log_error("RTCP recv() error, return:%d", (pj_status_t)-bytes_read);	
// 	return;
//     }

//     /* add by t35599*/
//     if (stream->cctrl && stream->cc_irtcp) {
//       stream->cc_irtcp(stream->cctrl, pkt, bytes_read);
//     }
//     /* end */

// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
//     p = (pj_uint8_t*)pkt;
//     pjmedia_rtcp_common *common = (pjmedia_rtcp_common*)p;
//     if(common->pt == RTCP_RR || common->pt == RTCP_SR){
// 	   rtcp_count = 1;
// 	   rtcp_len = ntohs(common->length);
// 	   while(rtcp_count < 16){
// 		    p += (rtcp_len  + 1)* 4 ;

// 		 /* add by j33783 20190508 */
// 		  if(p > (pj_uint8_t*)pkt + bytes_read - 4)
// 		  {
// 		  	log_debug("read buffer out of packet range, rtcp count:%u,  packet:%p, bytes:%u, p:%p", rtcp_count, pkt, bytes_read, p);
// 			break;
// 		  }

// 		  pj_uint16_t rtcp_len_next = ntohs(*((pj_uint16_t*)(p+2)));
// 		   if((p + (rtcp_len_next +1)*4) > (pj_uint8_t*)pkt + bytes_read)
// 		   {
// 		   	 log_debug("read buffer out of packet range, rtcp count:%u, packet:%p, bytes:%u, p:%p, next_rtcp_len:%u", rtcp_count,  pkt, bytes_read, p, (rtcp_len_next+1)*4);
// 		   	 break;
// 		   }
		   
// 		    pt = p + 1;
// 		    log_debug("parse rtcp packet pt=%d,rtcp_count=%d, rtcp_len=%d",*pt,rtcp_count,rtcp_len);
// 		    if(*pt == RTCP_NACK){
// 			   p16 = (pj_uint16_t *)(pt + 1);
// 		          rtcp_len = ntohs(*p16);
// 			   nack_found = PJ_TRUE;
// 			   standard = (*p & 0x1f) == 1 ? PJ_TRUE : PJ_FALSE;
// 			   break;
// 		    }
// 		    else if(*pt == 192 || *pt == 206) /* add by j33783 20190805 support FIR parsing */
// 		    {
// 		    	  log_debug("found rtcp fir, need to send force keyframe.");
// 			  pjmedia_vid_stream_send_keyframe(stream);
// 		    	  break;
// 		    }
// 		    else if(*pt >= 203  || *pt < 200){
// 		          break;
// 		    }
// 		    p16 = (pj_uint16_t*)(pt + 1);
// 		    rtcp_len = ntohs(*p16);
// 		    rtcp_count++;
// 	   }

//     }

// 	if(nack_found){
// 		p += rtcp_len * 4 ;
// 		p16 = (pj_uint16_t*)p;
// 		pid_val = ntohs(*p16);
// 		p16++;
// 		blp_val = ntohs(*p16);
// 		if(standard){
// 			resend_rtp(stream,resend, pid_val,blp_val,1); 

// 		}else{
// 			if(pid_val || blp_val){
// 				resend_rtp(stream,resend, pid_val,blp_val,0); 
// 			}else{
// 				pjmedia_rtcp_rx_rtcp(&stream->rtcp, pkt, bytes_read); 
// 			}
// 		}
	
// 	}else{
// 		pjmedia_rtcp_rx_rtcp(&stream->rtcp, pkt, bytes_read);
// 	} 
// #else
//     pjmedia_rtcp_rx_rtcp(&stream->rtcp, pkt, bytes_read);
// #endif

// /** Begin add by h15130 for when video call network is wrong we need close video only keep audio */
// #ifdef HYT_PTT
// 	log_debug("transmit_need_end:%d", stream->rtcp.transmit_need_end);
// 	if(stream->rtcp.transmit_need_end)
// 	{
// 		log_debug("enc paused:%d", stream->enc->paused);
// 		if(!stream->enc->paused && stream->ext_user_data.user_cb.on_rx_video_transmit_end_cb != NULL)
// 		{
// 			(*stream->ext_user_data.user_cb.on_rx_video_transmit_end_cb)(stream, stream->ext_user_data.user_data, 0, 0, 0);
// 		}
// 		stream->rtcp.transmit_need_end = PJ_FALSE;
// 	}
// #endif
// /** End add by h15130 for when video call network is wrong we need close video only keep audio */
// }

// #ifdef PJMEDIA_VIDEO_RESEND_OPTIMIZE
// static void resend_save_rtp( pjmedia_vid_channel *channel, pj_uint16_t pkt_len){
// 	pj_uint16_t index;  
// 	pjmedia_vid_buf *resend = channel->resend_vbuf; 
// 	index = channel->rtp.out_extseq % resend->buf_size; 
// 	pj_memcpy(resend->buf + index * PJMEDIA_MAX_MTU, channel->buf, pkt_len);
// 	resend->pkt_len[index] = pkt_len; 
// 	resend->resend_times[index] = 0;
// 	log_debug("save rtp, seq:%d, len:%d", channel->rtp.out_extseq,pkt_len);
// }
// #endif

// static pj_bool_t determine_rtp_orientation_extension(pj_uint8_t fu0, pj_uint8_t fu1)
// {
// 	pj_uint8_t FU_Type;
// 	log_trace("%02x %02x", fu0, fu1);
// 	FU_Type = fu0 & 0x1F;
	
// 	/*modify by 180732700*/
//     switch(FU_Type)
//     {
// 	 case AVC_NAL_FU_TYPE_A: 	
// 		  return (fu1 & 0x80)?(1):(0); break;
// 	 case AVC_NAL_UNIT_SPS: 
// 		  return 1; break;
// 	 case AVC_NAL_UNIT_IDR: 
// 		  return (fu1 & 0x80)?(1):(0); break;
// 	 case AVC_NAL_SINGLE_NAL_MIN: 
// 		  return (fu1 & 0x80)?(1):(0); break;
// 	 default: 
// 		  return 0; break;	
//     }

//     log_trace("No case matched,return 0;");
// 	return 0;
// }

// static pj_bool_t determine_rtp_orientation_extension_H265(pj_uint8_t fu0, pj_uint8_t fu1, pj_uint8_t fu2)
// {
// 	pj_uint8_t FU_Type;
// 	FU_Type = (fu0 & 0x7E)>>1;
	
// 	/*modify by 180732700*/
// 	switch(FU_Type)
// 	{
// 	  case HEVC_NAL_FU_TYPE:
// 		return (fu2 & 0x80)?(1):(0); break;
// 	  /*case HEVC_NAL_UNIT_VPS: return 1; break;*/
// 	  case HEVC_NAL_UNIT_SPS:
// 		return 1; break;
// 	  default:
// 		return 0; break;
// 	}

// 	log_debug("%02x %02x %02x   No case matched,return 0;", fu0, fu1, fu2);

// 	return 0;

// }




