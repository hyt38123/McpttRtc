
#ifndef __PJMEDIA_RTP_H__
#define __PJMEDIA_RTP_H__


/**
 * @file rtp.h
 * @brief RTP packet and RTP session declarations.
 */
#include "types.h"


//PJ_BEGIN_DECL

#ifndef PJMEDIA_NEW_TIMESTAMP
#define PJMEDIA_NEW_TIMESTAMP 1
#endif


#ifdef _MSC_VER
#   pragma warning(disable:4214)    // bit field types other than int
#endif


#define RTP_VERSION	2
#define RTP_PT_H264 98
#define RTP_SSRC    0x517907a6

/**
 * RTP packet header. Note that all RTP functions here will work with this
 * header in network byte order.
 */
#pragma pack(1)
struct pjmedia_rtp_hdr
{
#if defined(PJ_IS_BIG_ENDIAN) && (PJ_IS_BIG_ENDIAN!=0)
    pj_uint16_t v:2;		/**< packet type/version	    */
    pj_uint16_t p:1;		/**< padding flag		    */
    pj_uint16_t x:1;		/**< extension flag		    */
    pj_uint16_t cc:4;		/**< CSRC count			    */
    pj_uint16_t m:1;		/**< marker bit			    */
    pj_uint16_t pt:7;		/**< payload type		    */
#else
    pj_uint16_t cc:4;		/**< CSRC count			    */
    pj_uint16_t x:1;		/**< header extension flag	    */ 
    pj_uint16_t p:1;		/**< padding flag		    */
    pj_uint16_t v:2;		/**< packet type/version	    */
    pj_uint16_t pt:7;		/**< payload type		    */
    pj_uint16_t m:1;		/**< marker bit			    */
#endif
    pj_uint16_t seq;		/**< sequence number		    */
    pj_uint32_t ts;		/**< timestamp			    */
    pj_uint32_t ssrc;		/**< synchronization source	    */
};
#pragma pack()

#define RTP_HEAD_LENGTH 12

/*
 *
 * RTP

	0                   1                   2                   3
	0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2
	+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	|  V=2  | P | X |       CC      | M |            PT             |                               sequence number                 |
	+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	|															 timestamp												            |
	+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=++=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
	|			      							                 SSRC                                                               |
	+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	|                                                            CSRC                                                               |
	+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

 *
*/

/**
 * @see pjmedia_rtp_hdr
 */
typedef struct pjmedia_rtp_hdr pjmedia_rtp_hdr;

/**
 * RTP extendsion header.
 */
struct pjmedia_rtp_ext_hdr
{
    pj_uint16_t	profile_data;	/**< Profile data.	    */
    pj_uint16_t	length;		/**< Length.		    */
};

/**
 * @see pjmedia_rtp_ext_hdr
 */
typedef struct pjmedia_rtp_ext_hdr pjmedia_rtp_ext_hdr;

typedef struct ext_element_hdr
{
	pj_uint8_t local_id:8;
	pj_uint8_t local_len:8;
}ext_element_hdr;

/**
 * RTP session descriptor.
 */
struct pjmedia_rtp_session
{
    pjmedia_rtp_hdr	    out_hdr;    /**< Saved hdr for outgoing pkts.   */
    pj_uint16_t		    out_pt;	/**< Default outgoing payload type. */
    pj_uint16_t		    out_extseq; /**< Outgoing extended seq #.	    */
    pj_uint32_t		    peer_ssrc;  /**< Peer SSRC.			    */
    pj_uint32_t		    received;   /**< Number of received packets.    */
};

/**
 * @see pjmedia_rtp_session
 */
typedef struct pjmedia_rtp_session pjmedia_rtp_session;

char pjmedia_rtp_rotation_ext(int oritentation);
unsigned int pjmedia_video_add_rtp_exten(void *rtp_buf, pj_uint8_t ext_id, pj_uint8_t *ext_val, int ext_len);
int rtp_update_hdr(pjmedia_rtp_session *session, void *rtphdr, int mark, int payload_len, int timestamp);

#endif	/* __PJMEDIA_RTP_H__ */
