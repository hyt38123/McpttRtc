

#ifndef __RTCP_H__
#define __RTCP_H__

#include "types.h"

#ifndef RTCP_APP

#define RTCP_FIR  192   /* add by j33783 20190805 */
#define RTCP_SR   200
#define RTCP_RR   201
#define RTCP_SDES 202
#define RTCP_BYE  203
#define RTCP_NACK 205
#define RTCP_XR   207

#define RTCP_SR_RR_FALG 1
#define RTCP_SDES_FALG (RTCP_SR_RR_FALG << 1)
#define RTCP_NACK_FALG (RTCP_SDES_FALG << 1)
#define RTCP_BYTE_FALG (RTCP_NACK_FALG << 1)
#define RTCP_FIR_FALG (RTCP_BYTE_FALG << 1)   

#define RTCP_APP 204
#endif

#pragma pack(1)

/**
 * RTCP sender report.
 */
typedef struct pjmedia_rtcp_sr
{
    pj_uint32_t	    ntp_sec;	    /**< NTP time, seconds part.	*/
    pj_uint32_t	    ntp_frac;	    /**< NTP time, fractions part.	*/
    pj_uint32_t	    rtp_ts;	    /**< RTP timestamp.			*/
    pj_uint32_t	    sender_pcount;  /**< Sender packet cound.		*/
    pj_uint32_t	    sender_bcount;  /**< Sender octet/bytes count.	*/
} pjmedia_rtcp_sr;


/**
 * RTCP receiver report.
 */
typedef struct pjmedia_rtcp_rr
{
    pj_uint32_t	    ssrc;	    /**< SSRC identification.		*/
#if defined(PJ_IS_BIG_ENDIAN) && PJ_IS_BIG_ENDIAN!=0
    pj_uint32_t	    fract_lost:8;   /**< Fraction lost.			*/
    pj_uint32_t	    total_lost_2:8; /**< Total lost, bit 16-23.		*/
    pj_uint32_t	    total_lost_1:8; /**< Total lost, bit 8-15.		*/
    pj_uint32_t	    total_lost_0:8; /**< Total lost, bit 0-7.		*/
#else
    pj_uint32_t	    fract_lost:8;   /**< Fraction lost.			*/
    pj_uint32_t	    total_lost_2:8; /**< Total lost, bit 0-7.		*/
    pj_uint32_t	    total_lost_1:8; /**< Total lost, bit 8-15.		*/
    pj_uint32_t	    total_lost_0:8; /**< Total lost, bit 16-23.		*/
#endif	
    pj_uint32_t	    last_seq;	    /**< Last sequence number.		*/
    pj_uint32_t	    jitter;	    /**< Jitter.			*/
    pj_uint32_t	    lsr;	    /**< Last SR.			*/
    pj_uint32_t	    dlsr;	    /**< Delay since last SR.		*/
} pjmedia_rtcp_rr;


/**
 * RTCP common header.
 */
typedef struct pjmedia_rtcp_common
{
#if defined(PJ_IS_BIG_ENDIAN) && PJ_IS_BIG_ENDIAN!=0
    unsigned	    version:2;	/**< packet type            */
    unsigned	    p:1;	/**< padding flag           */
    unsigned	    count:5;	/**< varies by payload type */
    unsigned	    pt:8;	/**< payload type           */
#else
    unsigned	    count:5;	/**< varies by payload type */
    unsigned	    p:1;	/**< padding flag           */
    unsigned	    version:2;	/**< packet type            */
    unsigned	    pt:8;	/**< payload type           */
#endif
    unsigned	    length:16;	/**< packet length          */
    pj_uint32_t	    ssrc;	/**< SSRC identification    */
} pjmedia_rtcp_common;


/**
 * This structure declares default RTCP packet (SR) that is sent by pjmedia.
 * Incoming RTCP packet may have different format, and must be parsed
 * manually by application.
 */
typedef struct pjmedia_rtcp_sr_pkt
{
    pjmedia_rtcp_common  common;	/**< Common header.	    */
    pjmedia_rtcp_sr	 sr;		/**< Sender report.	    */
    pjmedia_rtcp_rr	 rr;		/**< variable-length list   */
} pjmedia_rtcp_sr_pkt;

/**
 * This structure declares RTCP RR (Receiver Report) packet.
 */
typedef struct pjmedia_rtcp_rr_pkt
{
    pjmedia_rtcp_common  common;	/**< Common header.	    */
    pjmedia_rtcp_rr	 rr;		/**< variable-length list   */
} pjmedia_rtcp_rr_pkt;


#pragma pack()


/**
 * RTCP SDES structure.
 */
typedef struct pjmedia_rtcp_sdes
{
    pj_str_t	cname;		/**< RTCP SDES type CNAME.	*/
    pj_str_t	name;		/**< RTCP SDES type NAME.	*/
    pj_str_t	email;		/**< RTCP SDES type EMAIL.	*/
    pj_str_t	phone;		/**< RTCP SDES type PHONE.	*/
    pj_str_t	loc;		/**< RTCP SDES type LOC.	*/
    pj_str_t	tool;		/**< RTCP SDES type TOOL.	*/
    pj_str_t	note;		/**< RTCP SDES type NOTE.	*/
} pjmedia_rtcp_sdes;



/**
 * RTCP NACK structure.
 */
typedef struct pjmedia_rtcp_nack_common{
#if defined(PJ_IS_BIG_ENDIAN) && PJ_IS_BIG_ENDIAN!=0
		unsigned		version:2;	/**< packet type			*/
		unsigned		p:1;	/**< padding flag			*/
		unsigned		fmt:5;	/**< varies by payload type */
		unsigned		pt:8;	/**< payload type			*/
#else
		unsigned		fmt:5;	/**< varies by payload type */
		unsigned		p:1;	/**< padding flag			*/
		unsigned		version:2;	/**< packet type			*/
		unsigned		pt:8;	/**< payload type			*/
#endif
		unsigned		length:16;	/**< packet length			*/
		pj_uint32_t 	ssrc;	/**< SSRC identification	*/
}pjmedia_rtcp_nack_common;


typedef struct pjmedia_rtcp_nack
{
    pj_uint32_t	ssrc;		/**< RTCP media source.	*/
	unsigned	base_seq:16;	/**< Based seq number of resend RTP seq*/
	unsigned	flag:16;	/**< The flag of resend RTP seq number */
} pjmedia_rtcp_nack;


typedef struct rtcp_nack_packet
{
    pj_uint32_t	ssrc;		/**< RTCP media source.	*/
    unsigned short  number;
	unsigned short	start_seq;	/**< Based seq number of resend RTP seq*/
	unsigned short	end_seq;	/**< The flag of resend RTP seq number */
} rtcp_nack_packet;


pj_status_t rtcp_build_rtcp_nack( //pjmedia_rtcp_session *session, 
					    void *buf,
					    pj_size_t *length,
					    const pjmedia_rtcp_nack *nack);



#endif