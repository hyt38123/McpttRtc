
#include "rtp.h"
#include "utils.h"
#include <netinet/in.h>

#define PJ_DEF(type) type

#define VIDEO_RTP_EXT_PROFILE  0xC4D7
#define VIDEO_RTP_EXT_PROFILE_LEN  0x01
#define VIDEO_RTP_EXT_ID_CVO_DATA_ROTATION_0  0x00
#define VIDEO_RTP_EXT_ID_CVO_DATA_ROTATION_90  0x01
#define VIDEO_RTP_EXT_ID_CVO_DATA_ROTATION_180  0x02
#define VIDEO_RTP_EXT_ID_CVO_DATA_ROTATION_270  0x03
#define VIDEO_RTP_EXT_ID_CVO_DATA_FLIP_H_0_V_0  0x00
#define VIDEO_RTP_EXT_ID_CVO_DATA_FLIP_H_0_V_1  0x01
#define VIDEO_RTP_EXT_ID_CVO_DATA_FLIP_H_1_V_0  0x02
#define VIDEO_RTP_EXT_ID_CVO_DATA_FLIP_H_1_V_1  0x03

unsigned int pjmedia_video_add_rtp_exten(void *rtp_buf, pj_uint8_t ext_id, pj_uint8_t *ext_val, int ext_len) {
		int cur_ext_len = 0;
		if(!rtp_buf || !ext_val)
			return cur_ext_len;

		pjmedia_rtp_ext_hdr *ext_hdr_data = (pjmedia_rtp_ext_hdr*)(rtp_buf);
		ext_hdr_data->profile_data = htons(VIDEO_RTP_EXT_PROFILE);
		ext_hdr_data->length = htons(VIDEO_RTP_EXT_PROFILE_LEN);
		cur_ext_len += sizeof(pjmedia_rtp_ext_hdr);

		ext_element_hdr *element_hdr = (ext_element_hdr*)(rtp_buf + cur_ext_len);
		element_hdr->local_id  = ext_id;
		element_hdr->local_len = 1;
		cur_ext_len += sizeof(ext_element_hdr);

		pj_memcpy(rtp_buf + cur_ext_len, ext_val, ext_len);
		cur_ext_len += ext_len;

		return cur_ext_len;
}

 char pjmedia_rtp_rotation_ext(int oritentation)
 {
  	 char data;
	 switch(oritentation)
	 {
		 case 0:  data = VIDEO_RTP_EXT_ID_CVO_DATA_ROTATION_0; break;
		 case 90: data = VIDEO_RTP_EXT_ID_CVO_DATA_ROTATION_90; break;
		 case 180:data = VIDEO_RTP_EXT_ID_CVO_DATA_ROTATION_180; break;
		 case 270:data = VIDEO_RTP_EXT_ID_CVO_DATA_ROTATION_270; break;
	 }
	 if(oritentation == 270)
	 {
		 data |= VIDEO_RTP_EXT_ID_CVO_DATA_FLIP_H_0_V_1 << 2;
	 }
	 else
	 {
		 data |= VIDEO_RTP_EXT_ID_CVO_DATA_FLIP_H_0_V_0 << 2;
	 }
	 return data;
 }
 
int rtp_update_hdr(pjmedia_rtp_session *session, void *rtphdr, int mark, int payload_len, int timestamp)
{
	int result = 0;
	// if(!rtphdr)
	// 	return -1;
	pjmedia_rtp_hdr *hdr = (pjmedia_rtp_hdr*)rtphdr;
	hdr->v  	= RTP_VERSION;
	hdr->m  	= (pj_uint16_t) mark;
	hdr->pt 	= (pj_uint8_t) session->out_pt;
	hdr->ts 	= htonl(timestamp);
	hdr->seq 	= htons(session->out_extseq);
	hdr->ssrc 	= htonl(RTP_SSRC);

	session->out_extseq++;

	return result;
}

// #ifdef PJMEDIA_NEW_TIMESTAMP
// PJ_DEF(pj_status_t) pjmedia_rtp_encode_rtp_update_ts( pjmedia_rtp_session *ses, 
// 					    int pt, int m,
// 					    int payload_len, int ts_len,
// 					    const void **rtphdr, int *hdrlen )
// {
//     /* Update timestamp */
// 	ses->out_hdr.ts = pj_htonl(ts_len);

//     /* If payload_len is zero, bail out.
//      * This is a clock frame; we're not really transmitting anything.
//      */
//     if (payload_len == 0)
// 	return PJ_SUCCESS;

//     /* Update session. */
//     ses->out_extseq++;

//     /* Create outgoing header. */
//     ses->out_hdr.pt = (pj_uint8_t) ((pt == -1) ? ses->out_pt : pt);
//     ses->out_hdr.m = (pj_uint16_t) m;
//     ses->out_hdr.seq = pj_htons( (pj_uint16_t) ses->out_extseq);

//     /* Return values */
//     *rtphdr = &ses->out_hdr;
//     *hdrlen = sizeof(pjmedia_rtp_hdr);

//     return PJ_SUCCESS;
// }
// #endif

// PJ_DEF(pj_status_t) pjmedia_rtp_decode_rtp( pjmedia_rtp_session *ses, 
// 					    const void *pkt, int pkt_len,
// 					    const pjmedia_rtp_hdr **hdr,
// 					    const void **payload,
// 					    unsigned *payloadlen)
// {
//     int offset = 0;
// 	int num = 0;
// 	int type = 0;

//     PJ_UNUSED_ARG(ses);

//     /* Assume RTP header at the start of packet. We'll verify this later. */
//     *hdr = (pjmedia_rtp_hdr*)pkt;

//     /* Check RTP header sanity. */
//     if ((*hdr)->v != RTP_VERSION) {
// 	return PJMEDIA_RTP_EINVER;
//     }

//     /* Payload is located right after header plus CSRC */
//     offset = sizeof(pjmedia_rtp_hdr) + ((*hdr)->cc * sizeof(pj_uint32_t));

//     /* Adjust offset if RTP extension is used. */
//     if ((*hdr)->x) {
// 	pjmedia_rtp_ext_hdr *ext = (pjmedia_rtp_ext_hdr*) 
// 				    (((pj_uint8_t*)pkt) + offset);
	
	
// 	//Parse the RTP extension add by ddj 2017/12/18
// 	if ((pj_ntohs(ext->profile_data) == NB_RTP_PROFILE) && (pj_ntohs(ext->length)> 0)){
// 		num += sizeof(pjmedia_rtp_ext_hdr);
// 		ext_element_hdr *element_hdr = (ext_element_hdr *)(((pj_uint8_t*)pkt) + offset + num);

// 		int len = pj_ntohs(ext->length)*sizeof(pjmedia_rtp_ext_hdr);
// 		while (num <= len) {
// 			if (element_hdr->local_id != NB_RTP_EXT_ID_TYPE){
// 				type = 1;
// 			}else{
// 				type = ( (*(((pj_uint8_t*)pkt) + offset + num + sizeof(ext_element_hdr))) == 1) ? 1 : 0;
// 			}
			
// 			num += sizeof(ext_element_hdr)+ (element_hdr->local_len)/8 + ((element_hdr->local_len)/8)%2;
// 			if (num <= len){
// 				element_hdr = (ext_element_hdr *)(((pj_uint8_t*)pkt) + offset + num);
// 			}
// 		}
// 	}	

//     rtp_stream_type = type;
// 	offset += ((pj_ntohs(ext->length)+1) * sizeof(pj_uint32_t));
//  	}
//     /* Check that offset is less than packet size */
//     if (offset > pkt_len)
// 	return PJMEDIA_RTP_EINLEN;

//     /* Find and set payload. */
//     *payload = ((pj_uint8_t*)pkt) + offset;
//     *payloadlen = pkt_len - offset;
 
//     /* Remove payload padding if any */
//     if ((*hdr)->p && *payloadlen > 0) {
// 		log_debug("Warning: padding isn't 0, p:%d, payloadlen:%d, pkt:", (*hdr)->p, *payloadlen);
// 		print_value_every_8bit((char *)pkt, pkt_len);
		
// 		pj_uint8_t pad_len;
// 		pad_len = ((pj_uint8_t*)(*payload))[*payloadlen - 1];
// 		log_debug("pad_len:%d", pad_len);
		
// 		if (pad_len <= *payloadlen)
// 		    *payloadlen -= pad_len;
//     }

//     return PJ_SUCCESS;
// }


// PJ_DEF(void) pjmedia_rtp_session_update( pjmedia_rtp_session *ses, 
// 					 const pjmedia_rtp_hdr *hdr,
// 					 pjmedia_rtp_status *p_seq_st)
// {
//     pjmedia_rtp_session_update2(ses, hdr, p_seq_st, PJ_TRUE);
// }

// PJ_DEF(void) pjmedia_rtp_session_update2( pjmedia_rtp_session *ses, 
// 					  const pjmedia_rtp_hdr *hdr,
// 					  pjmedia_rtp_status *p_seq_st,
// 					  pj_bool_t check_pt)
// {
//     pjmedia_rtp_status seq_st;

//     /* for now check_pt MUST be either PJ_TRUE or PJ_FALSE.
//      * In the future we might change check_pt from boolean to 
//      * unsigned integer to accommodate more flags.
//      */
//     pj_assert(check_pt==PJ_TRUE || check_pt==PJ_FALSE);

//     /* Init status */
//     seq_st.status.value = 0;
//     seq_st.diff = 0;

//     /* Check SSRC. */
//     if (ses->peer_ssrc == 0) ses->peer_ssrc = pj_ntohl(hdr->ssrc);

//     if (pj_ntohl(hdr->ssrc) != ses->peer_ssrc) {
// 	seq_st.status.flag.badssrc = 1;
// 	ses->peer_ssrc = pj_ntohl(hdr->ssrc);
//     }

//     /* Check payload type. */
//     if (check_pt && hdr->pt != ses->out_pt) {
// 	if (p_seq_st) {
// 	    p_seq_st->status.value = seq_st.status.value;
// 	    p_seq_st->status.flag.bad = 1;
// 	    p_seq_st->status.flag.badpt = 1;
// 	}
// 	return;
//     }

//     /* Initialize sequence number on first packet received. */
//     if (ses->received == 0)
// 	pjmedia_rtp_seq_init( &ses->seq_ctrl, pj_ntohs(hdr->seq) );

//     /* Check sequence number to see if remote session has been restarted. */
//     pjmedia_rtp_seq_update( &ses->seq_ctrl, pj_ntohs(hdr->seq), &seq_st);
//     if (seq_st.status.flag.restart) {
// 	++ses->received;

//     } else if (!seq_st.status.flag.bad) {
// 	++ses->received;
//     }

//     if (p_seq_st) {
// 	p_seq_st->status.value = seq_st.status.value;
// 	p_seq_st->diff = seq_st.diff;
//     }
// }



// void pjmedia_rtp_seq_restart(pjmedia_rtp_seq_session *sess, pj_uint16_t seq)
// {
//     sess->base_seq = seq;
//     sess->max_seq = seq;
//     sess->bad_seq = RTP_SEQ_MOD + 1;
//     sess->cycles = 0;
// }


// void pjmedia_rtp_seq_init(pjmedia_rtp_seq_session *sess, pj_uint16_t seq)
// {
//     pjmedia_rtp_seq_restart(sess, seq);

//     sess->max_seq = (pj_uint16_t) (seq - 1);
//     sess->probation = MIN_SEQUENTIAL;
// }


// void pjmedia_rtp_seq_update( pjmedia_rtp_seq_session *sess, 
// 			     pj_uint16_t seq,
// 			     pjmedia_rtp_status *seq_status)
// {
//     pj_uint16_t udelta = (pj_uint16_t) (seq - sess->max_seq);
//     pjmedia_rtp_status st;
    
//     /* Init status */
//     st.status.value = 0;
//     st.diff = 0;

//     /*
//      * Source is not valid until MIN_SEQUENTIAL packets with
//      * sequential sequence numbers have been received.
//      */
//     if (sess->probation) {

// 	st.status.flag.probation = 1;
	
//         if (seq == sess->max_seq+ 1) {
// 	    /* packet is in sequence */
// 	    st.diff = 1;
// 	    sess->probation--;
//             sess->max_seq = seq;
//             if (sess->probation == 0) {
// 		st.status.flag.probation = 0;
//             }
// 	} else {

// 	    st.diff = 0;

// 	    st.status.flag.bad = 1;
// 	    if (seq == sess->max_seq)
// 		st.status.flag.dup = 1;
// 	    else
// 		st.status.flag.outorder = 1;

// 	    sess->probation = MIN_SEQUENTIAL - 1;
// 	    sess->max_seq = seq;
//         }


//     } else if (udelta == 0) {

// 	st.status.flag.dup = 1;

//     } else if (udelta < MAX_DROPOUT) {
// 	/* in order, with permissible gap */
// 	if (seq < sess->max_seq) {
// 	    /* Sequence number wrapped - count another 64K cycle. */
// 	    sess->cycles += RTP_SEQ_MOD;
//         }
//         sess->max_seq = seq;

// 	st.diff = udelta;

//     } else if (udelta <= (RTP_SEQ_MOD - MAX_MISORDER)) {
// 	/* the sequence number made a very large jump */
//         if (seq == sess->bad_seq) {
// 	    /*
// 	     * Two sequential packets -- assume that the other side
// 	     * restarted without telling us so just re-sync
// 	     * (i.e., pretend this was the first packet).
// 	     */
// 	    pjmedia_rtp_seq_restart(sess, seq);
// 	    st.status.flag.restart = 1;
// 	    st.status.flag.probation = 1;
// 	    st.diff = 1;
// 	}
//         else {
// 	    sess->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
//             st.status.flag.bad = 1;
// 	    st.status.flag.outorder = 1;
//         }
//     } else {
// 	/* old duplicate or reordered packet.
// 	 * Not necessarily bad packet (?)
// 	 */
// 	st.status.flag.outorder = 1;
//     }
    

//     if (seq_status) {
// 	seq_status->diff = st.diff;
// 	seq_status->status.value = st.status.value;
//     }
// }

