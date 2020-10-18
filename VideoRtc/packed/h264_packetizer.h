/* $Id: h264_packetizer.h 3664 2011-07-19 03:42:28Z nanang $ */
/* 
 * Copyright (C) 2011 Teluu Inc. (http://www.teluu.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJMEDIA_H264_PACKETIZER_H__
#define __PJMEDIA_H264_PACKETIZER_H__

/**
 * @file h264_packetizer.h
 * @brief Packetizes H.264 bitstream into RTP payload and vice versa.
 */


#include "utils.h"

//PJ_BEGIN_DECL

#ifdef __cplusplus
extern "C" {
#endif


#define HYT_HEVC 1

#define PJ_DEF(type)		    type

/**
 * Enumeration of H.264 packetization modes.
 */
typedef enum
{
    /**
     * Single NAL unit packetization mode will only generate payloads
     * containing a complete single NAL unit packet. As H.264 NAL unit
     * size can be very large, this mode is usually not applicable for
     * network environments with MTU size limitation.
     */
    PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL,
    
    /**
     * Non-interleaved packetization mode will generate payloads with the
     * following possible formats:
     * - single NAL unit packets,
     * - NAL units aggregation STAP-A packets,
     * - fragmented NAL unit FU-A packets.
     */
    PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED,

    /**
     * Interleaved packetization mode will generate payloads with the
     * following possible formats:
     * - single NAL unit packets,
     * - NAL units aggregation STAP-A & STAP-B packets,
     * - fragmented NAL unit FU-A & FU-B packets.
     * This packetization mode is currently unsupported.
     */
    PJMEDIA_H264_PACKETIZER_MODE_INTERLEAVED,
} pjmedia_h264_packetizer_mode;

typedef enum H264_Nal_Unit_Type
{
    H264_NAL_UNIT_UNSPECIFED = 0,
	H264_NAL_UNIT_CODED_SLICE_NON_IDR,
	H264_NAL_UNIT_CODED_SLICE_A,
	H264_NAL_UNIT_CODED_SLICE_B,
	H264_NAL_UNIT_CODED_SLICE_C,
	H264_NAL_UNIT_IDR,
	H264_NAL_UNIT_SEI,
	H264_NAL_UNIT_SPS,
	H264_NAL_UNIT_PPS,
	H264_NAL_UNIT_DELIMITER_9,
	H264_NAL_UNIT_END_SEQUENCE_10,
	H264_NAL_UNIT_END_STREAM_11,
	H264_NAL_UNIT_FILLER_12,
	/*
	   13..23 Reserved
	   24..31 Unspecified
	*/
}H264_Nal_Unit_Type;

/**
 * H.264 packetizer setting.
 */
typedef struct pjmedia_h264_packetizer_cfg
{
    /**
     * Maximum payload length.
     * Default: PJMEDIA_MAX_MTU
     */
    int	mtu;

    /**
     * Packetization mode.
     * Default: PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED
     */
    pjmedia_h264_packetizer_mode mode;
}
pjmedia_h264_packetizer_cfg;


/* H.264 packetizer definition */
struct pjmedia_h264_packetizer
{
    /* Current settings */
    pjmedia_h264_packetizer_cfg cfg;
    
    /* Unpacketizer state */
    unsigned	    unpack_last_sync_pos;
    pj_bool_t	    unpack_prev_lost;
};

/**
 * Opaque declaration for H.264 packetizer.
 */
typedef struct pjmedia_h264_packetizer pjmedia_h264_packetizer;


pj_uint8_t* find_next_nal_unit(pj_uint8_t *start,pj_uint8_t *end);


/**
 * Create H.264 packetizer.
 *
 * @param pool		The memory pool.
 * @param cfg		Packetizer settings, if NULL, default setting
 *			will be used.
 * @param p_pktz	Pointer to receive the packetizer.
 *
 * @return		PJ_SUCCESS on success.
 */
// PJ_DECL(pj_status_t) pjmedia_h264_packetizer_create(
// 				    pj_pool_t *pool,
// 				    const pjmedia_h264_packetizer_cfg *cfg,
// 				    pjmedia_h264_packetizer **p_pktz);


/**
 * Generate an RTP payload from a H.264 picture bitstream. Note that this
 * function will apply in-place processing, so the bitstream may be modified
 * during the packetization.
 *
 * @param pktz		The packetizer.
 * @param bits		The picture bitstream to be packetized.
 * @param bits_len	The length of the bitstream.
 * @param bits_pos	The bitstream offset to be packetized.
 * @param payload	The output payload.
 * @param payload_len	The output payload length.
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_h264_packetize(pjmedia_h264_packetizer *pktz,
					    pj_uint8_t *bits,
                                            pj_size_t bits_len,
                                            unsigned *bits_pos,
                                            const pj_uint8_t *payload,
                                            pj_size_t *payload_len);

/**
 * Append an RTP payload to an H.264 picture bitstream. Note that in case of
 * noticing packet lost, application should keep calling this function with
 * payload pointer set to NULL, as the packetizer need to update its internal
 * state.
 *
 * @param pktz		The packetizer.
 * @param payload	The payload to be unpacketized.
 * @param payload_len	The payload length.
 * @param bits		The bitstream buffer.
 * @param bits_size	The bitstream buffer size.
 * @param bits_pos	The bitstream offset to put the unpacketized payload
 *			in the bitstream, upon return, this will be updated
 *			to the latest offset as a result of the unpacketized
 *			payload.
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_h264_unpacketize(pjmedia_h264_packetizer *pktz,
					      const pj_uint8_t *payload,
                                              pj_size_t   payload_len,
                                              pj_uint8_t *bits,
                                              pj_size_t   bits_len,
					      unsigned   *bits_pos);

/*add for h265 NALU TYPE by 180732700,20190701*/
typedef enum H265_Nal_Unit_Type
{
	HEVC_NAL_UNIT_CODED_SLICE_TRAIL_N = 0,
	HEVC_NAL_UNIT_CODED_SLICE_TRAIL_R,

	HEVC_NAL_UNIT_CODED_SLICE_TSA_N,
	HEVC_NAL_UNIT_CODED_SLICE_TLA,       // 3  // Current name in the spec: TSA_R

	HEVC_NAL_UNIT_CODED_SLICE_STSA_N,    // 4 
	HEVC_NAL_UNIT_CODED_SLICE_STSA_R,    // 5 

	HEVC_NAL_UNIT_CODED_SLICE_RADL_N,	// 6 
	HEVC_NAL_UNIT_CODED_SLICE_DLP,		// 7 // Current name in the spec: RADL_R 

	HEVC_NAL_UNIT_CODED_SLICE_RASL_N,	// 8 
	HEVC_NAL_UNIT_CODED_SLICE_TFD,		// 9 // Current name in the spec: RASL_R 

	HEVC_NAL_UNIT_RESERVED_10,
	HEVC_NAL_UNIT_RESERVED_11,
	HEVC_NAL_UNIT_RESERVED_12,
	HEVC_NAL_UNIT_RESERVED_13,
	HEVC_NAL_UNIT_RESERVED_14,
	HEVC_NAL_UNIT_RESERVED_15,
	HEVC_NAL_UNIT_CODED_SLICE_BLA,		 // 16 // Current name in the spec: BLA_W_LP 
	HEVC_NAL_UNIT_CODED_SLICE_BLANT,		 // 17 // Current name in the spec: BLA_W_DLP 
	HEVC_NAL_UNIT_CODED_SLICE_BLA_N_LP,	 // 18 
	HEVC_NAL_UNIT_IDR = 19,	 // 19 //Current name in the spec: IDR_W_DLP 
	HEVC_NAL_UNIT_CODED_SLICE_IDR_N_LP,	 // 20
	HEVC_NAL_UNIT_CODED_SLICE_CRA,	     // 21
	HEVC_NAL_UNIT_RESERVED_22,
	HEVC_NAL_UNIT_RESERVED_23,

	HEVC_NAL_UNIT_RESERVED_24,
	HEVC_NAL_UNIT_RESERVED_25,
	HEVC_NAL_UNIT_RESERVED_26,
	HEVC_NAL_UNIT_RESERVED_27,
	HEVC_NAL_UNIT_RESERVED_28,
	HEVC_NAL_UNIT_RESERVED_29,
	HEVC_NAL_UNIT_RESERVED_30,
	HEVC_NAL_UNIT_RESERVED_31,

	HEVC_NAL_UNIT_VPS = 32,	//32
	HEVC_NAL_UNIT_SPS = 33,	//33
	HEVC_NAL_UNIT_PPS = 34,	//34
	HEVC_NAL_UNIT_ACCESS_UNIT_DELIMITER,//35
	HEVC_NAL_UNIT_EOS,	   //36
	HEVC_NAL_UNIT_EOB,	   //37
	HEVC_NAL_UNIT_FILLER_DATA,	//38
	HEVC_NAL_UNIT_SEI,		    //39 Prefix SEI 
	HEVC_NAL_UNIT_SEI_SUFFIX,    //40 Suffix SEI 
	HEVC_NAL_UNIT_RESERVED_41,
	HEVC_NAL_UNIT_RESERVED_42,
	HEVC_NAL_UNIT_RESERVED_43,
	HEVC_NAL_UNIT_RESERVED_44,
	HEVC_NAL_UNIT_RESERVED_45,
	HEVC_NAL_UNIT_RESERVED_46,
	HEVC_NAL_UNIT_RESERVED_47,
	HEVC_NAL_UNIT_UNSPECIFIED_48,
	HEVC_NAL_UNIT_UNSPECIFIED_49,
	HEVC_NAL_UNIT_UNSPECIFIED_50,
	HEVC_NAL_UNIT_UNSPECIFIED_51,
	HEVC_NAL_UNIT_UNSPECIFIED_52,
	HEVC_NAL_UNIT_UNSPECIFIED_53,
	HEVC_NAL_UNIT_UNSPECIFIED_54,
	HEVC_NAL_UNIT_UNSPECIFIED_55,
	HEVC_NAL_UNIT_UNSPECIFIED_56,
	HEVC_NAL_UNIT_UNSPECIFIED_57,
	HEVC_NAL_UNIT_UNSPECIFIED_58,
	HEVC_NAL_UNIT_UNSPECIFIED_59,
	HEVC_NAL_UNIT_UNSPECIFIED_60,
	HEVC_NAL_UNIT_UNSPECIFIED_61,
	HEVC_NAL_UNIT_UNSPECIFIED_62,
	HEVC_NAL_UNIT_UNSPECIFIED_63,
	HEVC_NAL_UNIT_INVALID,
}H265_Nal_Unit_Type;



#ifdef HYT_HEVC
#define RTP_H265_HEADERS_SIZE   3
#define RTP_H265_PAYLOAD_SIZE   970  /*should be less than PJMEDIA_MAX_VID_PAYLOAD_SIZE*/
#define NALU_H265_HEADER_SIZE   4

/*find the start code within the FIND_NALU_IN_LEN length*/
#define WITHIN_LENGTH (RTP_H265_PAYLOAD_SIZE + NALU_H265_HEADER_SIZE)

/**
 * H.265 packetizer setting.
 */
typedef struct pjmedia_h265_packetizer_cfg
{
    /**
     * Maximum payload length.
     * Default: PJMEDIA_MAX_MTU
     */
    int	mtu;

    /**
     * Packetization mode.
     * Default: PJMEDIA_H265_PACKETIZER_MODE_NON_INTERLEAVED
     */
    //pjmedia_h264_packetizer_mode mode;
}pjmedia_h265_packetizer_cfg;

/* H.265 packetizer definition */
typedef struct pjmedia_h265_packetizer
{
    /* Current settings */
    pjmedia_h265_packetizer_cfg cfg;
    
    /* Unpacketizer state */
    unsigned	    unpack_last_sync_pos;
    pj_bool_t	    unpack_prev_lost;
}pjmedia_h265_packetizer;

/*packetize for a rtp payload*/
typedef struct pket_rtp_payload_msg
{
    pj_int32_t nal_type; /*the stream nalu type*/
	pj_int32_t start;    /*set FU header S bit*/
	pj_int32_t end;      /*set FU header E bit*/
	pj_int32_t len;      /*find length data should to packet*/
	pj_bool_t start_code; /*the length data have start code*/
	pj_size_t pos; /*had packet to position in stream*/
	pj_int32_t nalu_header_len;
	pj_bool_t fu_packet; /*use the FU slice packet*/
}pkt_rtp_payload_msg;

PJ_DEF(pj_status_t) pjmedia_h265_packetize(pjmedia_h265_packetizer *pktz,
					                       pj_uint8_t *buf,
                                           pj_size_t buf_len,
                                           unsigned *pos,
                                           pj_uint8_t *payload,
                                           pj_size_t *payload_len);

PJ_DEF(pj_status_t)  pjmeida_h265_unpacketize(pj_uint8_t *rtp_buf, int len,  pj_uint8_t* h265_buf, int* pos);


#endif /*HYT_HEVC*/

#ifdef __cplusplus
}
#endif


#endif	/* __PJMEDIA_H264_PACKETIZER_H__ */
