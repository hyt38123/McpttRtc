/* $Id: h264_packetizer.c 4537 2013-06-19 06:47:43Z riza $ */
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

#include <assert.h>
#include <stdint.h>
#include "h264_packetizer.h"

//#include "pj_string.h"
//#include <pjmedia/types.h>
//#include <pj/assert.h>
//#include <pj/errno.h>
//#include <pj/log.h>
//#include <pj/pool.h>
//#include <pj/string.h>

#define log_debug(fmt, ...)
#define log_error(fmt, ...)


#define HYT_PTT 1
#define PJMEDIA_HAS_VIDEO 1

#if defined(PJMEDIA_HAS_VIDEO) && (PJMEDIA_HAS_VIDEO != 0)


#define THIS_FILE		"h264_packetizer.c"

#ifdef VID_HARDWARE_DEBUG
#define DBG_PACKETIZE		1
#define DBG_UNPACKETIZE		1
#else 
#define DBG_PACKETIZE		0
#define DBG_UNPACKETIZE		0
#endif


/**
 * PJ_ERRNO_START is where PJLIB specific error values start.
 */
#define PJ_ERRNO_START		20000

/**
 * PJ_ERRNO_SPACE_SIZE is the maximum number of errors in one of 
 * the error/status range below.
 */
#define PJ_ERRNO_SPACE_SIZE	50000

/**
 * PJ_ERRNO_START_STATUS is where PJLIB specific status codes start.
 * Effectively the error in this class would be 70000 - 119000.
 */
#define PJ_ERRNO_START_STATUS	(PJ_ERRNO_START + PJ_ERRNO_SPACE_SIZE)

/**
 * @hideinitializer
 * Invalid argument.
 */
#define PJ_EINVAL	    (PJ_ERRNO_START_STATUS + 4)	/* 70004 */

/**
 * @hideinitializer
 * The specified option is not supported.
 */
#define PJ_ENOTSUP	    (PJ_ERRNO_START_STATUS + 12)/* 70012 */

/**
 * @hideinitializer
 * Size is too small.
 */
#define PJ_ETOOSMALL	    (PJ_ERRNO_START_STATUS + 19)/* 70019 */

/**
 * @hideinitializer
 * Ignored
 */
#define PJ_EIGNORED	    (PJ_ERRNO_START_STATUS + 20)/* 70020 */

/**
 * @hideinitializer
 * Operation is cancelled.
 */
#define PJ_ECANCELLED	    (PJ_ERRNO_START_STATUS + 14)/* 70014 */


#ifndef pj_assert
#   define pj_assert(expr)   assert(expr)
#endif

#define PJ_ASSERT_RETURN(expr,retval)    pj_assert(expr)

#define PJ_UNUSED_ARG(arg)  (void)arg



/* Enumeration of H.264 NAL unit types */
enum
{
    NAL_TYPE_SINGLE_NAL_MIN	= 1,
    NAL_TYPE_SINGLE_NAL_MAX	= 23,
    NAL_TYPE_STAP_A		= 24,
    NAL_TYPE_FU_A		= 28,
};

 
/*
 * Find next NAL unit from the specified H.264 bitstream data.
 */
pj_uint8_t* find_next_nal_unit(pj_uint8_t *start, pj_uint8_t *end)
{
    pj_uint8_t *p = start;

    /* Simply lookup "0x000001" pattern */
    while (p <= end-3 && (p[0] || p[1] || p[2]!=1))
        ++p;

    if (p > end-3)
	/* No more NAL unit in this bitstream */
        return NULL;

    /* Include 8 bits leading zero */
    if (p>start && *(p-1)==0)
		return (p-1);

    return p;
}


/*
 * Create H264 packetizer.
 */
// PJ_DEF(pj_status_t) pjmedia_h264_packetizer_create(
// 				pj_pool_t *pool,
// 				const pjmedia_h264_packetizer_cfg *cfg,
// 				pjmedia_h264_packetizer **p)
// {
//     pjmedia_h264_packetizer *p_;

//     PJ_ASSERT_RETURN(pool && p, PJ_EINVAL);

//     if (cfg &&
// 		cfg->mode != PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED &&
// 		cfg->mode != PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL)
//     {
// 		return PJ_ENOTSUP;
//     }

//     p_ = PJ_POOL_ZALLOC_T(pool, pjmedia_h264_packetizer);
//     if (cfg) {
// 		pj_memcpy(&p_->cfg, cfg, sizeof(*cfg));
//     } 
// 	else {
// 		p_->cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
// 		p_->cfg.mtu = PJMEDIA_MAX_VID_PAYLOAD_SIZE;
//     }

//     *p = p_;

//     return PJ_SUCCESS;
// }



/*
 * Generate an RTP payload from H.264 frame bitstream, in-place processing.
 */
PJ_DEF(pj_status_t) pjmedia_h264_packetize(pjmedia_h264_packetizer *pktz,
					                       pj_uint8_t *buf,
                                           pj_size_t buf_len,
                                           unsigned *pos,
                                           const pj_uint8_t *payload,
                                           pj_size_t *payload_len)
{
    pj_uint8_t *nal_start = NULL, *nal_end = NULL, *nal_octet = NULL, *load_start = NULL;
    pj_uint8_t *p, *end;
    enum { 
		HEADER_SIZE_FU_A	     = 2,
		HEADER_SIZE_STAP_A	     = 3,
    };
    enum { MAX_NALS_IN_AGGR = 32 };

#if DBG_PACKETIZE
    if (*pos == 0 && buf_len) {
		log_debug("<< Start packing new frame >>");
    }
#endif

    p = buf + *pos;
    end = buf + buf_len;

    /* Find NAL unit startcode */
    if (end-p >= 4)
		nal_start = find_next_nal_unit(p, p+4);

    if (nal_start) {
		/* Get NAL unit octet pointer */
		while (*nal_start++ == 0);
		nal_octet = nal_start;
    } else {
		/* This NAL unit is being fragmented */
		nal_start = p;
    }

    /* Get end of NAL unit */
    p = nal_start+pktz->cfg.mtu+1;
    if (p > end || pktz->cfg.mode==PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL) 
		p = end;

    nal_end = find_next_nal_unit(nal_start, p); 
    if (!nal_end)
		nal_end = p;

    /* Validate MTU vs NAL length on single NAL unit packetization */
    if ((pktz->cfg.mode==PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL) &&
		nal_end - nal_start > pktz->cfg.mtu)
    {
		//pj_assert(!"MTU too small for H.264 single NAL packetization mode");
		log_error("MTU too small for H.264 (required=%u, MTU=%u)", nal_end - nal_start, pktz->cfg.mtu);
		return PJ_ETOOSMALL;
    }

    /* Evaluate the proper payload format structure */
    /* Fragmentation (FU-A) packet */
    if ((pktz->cfg.mode != PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL) &&
		(!nal_octet || nal_end-nal_start > pktz->cfg.mtu))
    {
		pj_uint8_t NRI, TYPE;

		if (nal_octet) {
			/* We have NAL unit octet, so this is the first fragment */
			NRI = (*nal_octet & 0x60) >> 5;
			TYPE = *nal_octet & 0x1F;

			/* Skip nal_octet in nal_start to be overriden by FU header */
			++nal_start;
		} else {
			/* Not the first fragment, get NRI and NAL unit type
			* from the previous fragment.
			*/
			p = nal_start - pktz->cfg.mtu;
			/* add by j33783 20190523 for crash insight begin */
			if(p < buf || p >= buf + buf_len)
			{
				log_error("out of buffer range, p=%p, buf_start=%p, buf_end=%p", p, buf, buf + buf_len);
				return PJ_ECANCELLED;
			}
			/* add by j33783 20190523 for crash insight end */
			NRI = (*p & 0x60) >> 5;
			TYPE = *(p+1) & 0x1F;
		}

		/* Init FU indicator (one octet: F+NRI+TYPE) */
		p = nal_start - HEADER_SIZE_FU_A;
		*p = (NRI << 5) | NAL_TYPE_FU_A;
		++p;

		/* Init FU header (one octed: S+E+R+TYPE) */
		*p = TYPE;
		if (nal_octet)
			*p |= (1 << 7); /* S bit flag = start of fragmentation */
		if (nal_end-nal_start+HEADER_SIZE_FU_A <= pktz->cfg.mtu)
			*p |= (1 << 6); /* E bit flag = end of fragmentation */

		/* Set payload, payload length */
		load_start = nal_start - HEADER_SIZE_FU_A;
		if (nal_end-nal_start+HEADER_SIZE_FU_A > pktz->cfg.mtu)
			*payload_len = pktz->cfg.mtu;
		else
			*payload_len = nal_end - nal_start + HEADER_SIZE_FU_A;
		memcpy((pj_uint8_t *)payload, load_start, *payload_len);
		*pos = (unsigned)(load_start + *payload_len - buf);

	//#if DBG_PACKETIZE
		log_debug("Packetized fragmented H264 NAL unit "
			"(pos=%d, type=%d, NRI=%d, S=%d, E=%d, len=%d/%d)",
			load_start-buf, TYPE, NRI, *p>>7, (*p>>6)&1, *payload_len,
			buf_len);
	//#endif

		return PJ_SUCCESS;
    }

    /* begin w13683 modify for h264 packetizer, forbid stap-a 20160108 */
#ifndef HYT_PTT
    /* Aggregation (STAP-A) packet */
    if ((pktz->cfg.mode != PJMEDIA_H264_PACKETIZER_MODE_SINGLE_NAL) &&
	(nal_end != end) && (nal_end - nal_start + HEADER_SIZE_STAP_A) < pktz->cfg.mtu) 
    {
		int total_size;
		unsigned nal_cnt = 1;
		pj_uint8_t *nal[MAX_NALS_IN_AGGR];
		pj_size_t nal_size[MAX_NALS_IN_AGGR];
		pj_uint8_t NRI;

		pj_assert(nal_octet);

		/* Init the first NAL unit in the packet */
		nal[0] = nal_start;
		nal_size[0] = nal_end - nal_start;
		total_size = (int)nal_size[0] + HEADER_SIZE_STAP_A;
		NRI = (*nal_octet & 0x60) >> 5;

		/* Populate next NAL units */
		while (nal_cnt < MAX_NALS_IN_AGGR) {
			pj_uint8_t *tmp_end;

			/* Find start address of the next NAL unit */
			p = nal[nal_cnt-1] + nal_size[nal_cnt-1];
			while (*p++ == 0);
			nal[nal_cnt] = p;

			/* Find end address of the next NAL unit */
			tmp_end = p + (pktz->cfg.mtu - total_size);
			if (tmp_end > end)
				tmp_end = end;
			p = find_next_nal_unit(p+1, tmp_end);
			if (p) {
				nal_size[nal_cnt] = p - nal[nal_cnt];
			} else {
				break;
			}

			/* Update total payload size (2 octet NAL size + NAL) */
			total_size += (2 + (int)nal_size[nal_cnt]);
			if (total_size <= pktz->cfg.mtu) {
				pj_uint8_t tmp_nri;

				/* Get maximum NRI of the aggregated NAL units */
				tmp_nri = (*(nal[nal_cnt]-1) & 0x60) >> 5;
				if (tmp_nri > NRI)
					NRI = tmp_nri;
			} 
			else {
				break;
			}

			++nal_cnt;
		}

		/* Only use STAP-A when we found more than one NAL units */
		if (nal_cnt > 1) {
			unsigned i;

			/* Init STAP-A NAL header (F+NRI+TYPE) */
			p = nal[0] - HEADER_SIZE_STAP_A;
			*p++ = (NRI << 5) | NAL_TYPE_STAP_A;

			/* Append all populated NAL units into payload (SIZE+NAL) */
			for (i = 0; i < nal_cnt; ++i) {
				/* Put size (2 octets in network order) */
				pj_assert(nal_size[i] <= 0xFFFF);
				*p++ = (pj_uint8_t)(nal_size[i] >> 8);
				*p++ = (pj_uint8_t)(nal_size[i] & 0xFF);
				
				/* Append NAL unit, watchout memmove()-ing bitstream! */
				if (p != nal[i])
					pj_memmove(p, nal[i], nal_size[i]);
				p += nal_size[i];
			}

			/* Set payload, payload length, and pos */
			load_start = nal[0] - HEADER_SIZE_STAP_A;
			pj_assert(load_start >= buf+*pos);
			*payload_len = p - load_start;
			memcpy(payload, load_start, *payload_len);
			*pos = (unsigned)(nal[nal_cnt-1] + nal_size[nal_cnt-1] - buf);

	//#if DBG_PACKETIZE
			log_debug("Packetized aggregation of "
				"%d H264 NAL units (pos=%d, NRI=%d len=%d/%d)",
				nal_cnt, load_start-buf, NRI, *payload_len, buf_len);
	//#endif

			return PJ_SUCCESS;
		}
    }
#endif
    /* end w13683 modify for h264 packetizer, forbid stap-a 20160108 */

    /* Single NAL unit packet */
    load_start = nal_start;
    *payload_len = nal_end - nal_start;
	memcpy((pj_uint8_t *)payload, load_start, *payload_len);
    *pos = (unsigned)(nal_end - buf);

//#if DBG_PACKETIZE
    log_debug("Packetized single H264 NAL unit "
	       "(pos=%d, type=%d, NRI=%d, len=%d/%d)",
	       nal_start-buf, *nal_octet&0x1F, (*nal_octet&0x60)>>5,
	       *payload_len, buf_len);
//#endif

    return PJ_SUCCESS;
}


/*
 * Append RTP payload to a H.264 picture bitstream. Note that the only
 * payload format that cares about packet lost is the NAL unit
 * fragmentation format (FU-A/B), so we will only manage the "prev_lost"
 * state for the FU-A/B packets.
 */
PJ_DEF(pj_status_t) pjmedia_h264_unpacketize(pjmedia_h264_packetizer *pktz,
					     const pj_uint8_t *payload,
                                             pj_size_t   payload_len,
                                             pj_uint8_t *bits,
                                             pj_size_t   bits_len,
					     unsigned   *bits_pos)
{
    const pj_uint8_t nal_start_code[4] = {0, 0, 0, 1};
    enum { MIN_PAYLOAD_SIZE = 2 };
    pj_uint8_t nal_type;

    PJ_UNUSED_ARG(pktz);

#if DBG_UNPACKETIZE
    if (*bits_pos == 0 && payload_len) {
		log_debug(">> Start unpacking new frame <<");
    }
#endif

    /* Check if this is a missing/lost packet */
    if (payload == NULL) {
		pktz->unpack_prev_lost = PJ_TRUE;
		return PJ_SUCCESS;
    }

    /* H264 payload size */
    if (payload_len < MIN_PAYLOAD_SIZE) {
		/* Invalid bitstream, discard this payload */
		pktz->unpack_prev_lost = PJ_TRUE;
		return PJ_EINVAL;
    }

    /* Reset last sync point for every new picture bitstream */
    if (*bits_pos == 0)
		pktz->unpack_last_sync_pos = 0;

    nal_type = *payload & 0x1F;
    if (nal_type >= NAL_TYPE_SINGLE_NAL_MIN &&
		nal_type <= NAL_TYPE_SINGLE_NAL_MAX)
    {
		/* Single NAL unit packet */
		pj_uint8_t *p = bits + *bits_pos;

		/* Validate bitstream length */
		if (bits_len-*bits_pos < payload_len+PJ_ARRAY_SIZE(nal_start_code)) {
			/* Insufficient bistream buffer, discard this payload */
			pj_assert(!"Insufficient H.263 bitstream buffer");
			return PJ_ETOOSMALL;
		}

		/* Write NAL unit start code */
		pj_memcpy(p, &nal_start_code, PJ_ARRAY_SIZE(nal_start_code));
		p += PJ_ARRAY_SIZE(nal_start_code);

		/* Write NAL unit */
		pj_memcpy(p, payload, payload_len);
		p += payload_len;

		/* Update the bitstream writing offset */
		*bits_pos = (unsigned)(p - bits);
		pktz->unpack_last_sync_pos = *bits_pos;

	#if DBG_UNPACKETIZE
		log_debug("Unpacked single H264 NAL unit "
			"(type=%d, NRI=%d, len=%d)",
			nal_type, (*payload&0x60)>>5, payload_len);
	#endif

    }
    else if (nal_type == NAL_TYPE_STAP_A)
    {
		/* Aggregation packet */
		pj_uint8_t *p, *p_end;
		const pj_uint8_t *q, *q_end;
		unsigned cnt = 0;

		/* Validate bitstream length */
		if (bits_len - *bits_pos < payload_len + 32) {
			/* Insufficient bistream buffer, discard this payload */
			pj_assert(!"Insufficient H.263 bitstream buffer");
			return PJ_ETOOSMALL;
		}

		/* Fill bitstream */
		p = bits + *bits_pos;
		p_end = bits + bits_len;
		q = payload + 1;
		q_end = payload + payload_len;
		while (q < q_end && p < p_end) {
			pj_uint16_t tmp_nal_size;

			/* Write NAL unit start code */
			pj_memcpy(p, &nal_start_code, PJ_ARRAY_SIZE(nal_start_code));
			p += PJ_ARRAY_SIZE(nal_start_code);

			/* Get NAL unit size */
			tmp_nal_size = (*q << 8) | *(q+1);
			q += 2;
			if (q + tmp_nal_size > q_end) {
				/* Invalid bitstream, discard the rest of the payload */
				return PJ_EINVAL;
			}

			/* Write NAL unit */
			pj_memcpy(p, q, tmp_nal_size);
			p += tmp_nal_size;
			q += tmp_nal_size;
			++cnt;

			/* Update the bitstream writing offset */
			*bits_pos = (unsigned)(p - bits);
			pktz->unpack_last_sync_pos = *bits_pos;
		}

	#if DBG_UNPACKETIZE
		log_debug("Unpacked %d H264 NAL units (len=%d)",
			cnt, payload_len);
	#endif

    }
    else if (nal_type == NAL_TYPE_FU_A)
    {
		/* Fragmentation packet */
		pj_uint8_t *p;
		const pj_uint8_t *q = payload;
		pj_uint8_t NRI, TYPE, S, E;

		p = bits + *bits_pos;

		/* Validate bitstream length */
		if (bits_len-*bits_pos < payload_len+PJ_ARRAY_SIZE(nal_start_code)) {
			/* Insufficient bistream buffer, drop this packet */
			pj_assert(!"Insufficient H.263 bitstream buffer");
			pktz->unpack_prev_lost = PJ_TRUE;
			return PJ_ETOOSMALL;
		}

		/* Get info */
		S = *(q+1) & 0x80;    /* Start bit flag	*/
		E = *(q+1) & 0x40;    /* End bit flag	*/
		TYPE = *(q+1) & 0x1f;
		NRI = (*q & 0x60) >> 5;

		/* Fill bitstream */
		if (S) {
			/* This is the first part, write NAL unit start code */
			pj_memcpy(p, &nal_start_code, PJ_ARRAY_SIZE(nal_start_code));
			p += PJ_ARRAY_SIZE(nal_start_code);

			/* Write NAL unit octet */
			*p++ = (NRI << 5) | TYPE;
		} else if (pktz->unpack_prev_lost) {
			/* If prev packet was lost, revert the bitstream pointer to
			* the last sync point.
			*/
			pj_assert(pktz->unpack_last_sync_pos <= *bits_pos);
			*bits_pos = pktz->unpack_last_sync_pos;
			/* And discard this payload (and the following fragmentation
			* payloads carrying this same NAL unit.
			*/
			return PJ_EIGNORED;
		}
		q += 2;

		/* Write NAL unit */
		pj_memcpy(p, q, payload_len - 2);
		p += (payload_len - 2);

		/* Update the bitstream writing offset */
		*bits_pos = (unsigned)(p - bits);
		if (E) {
			/* Update the sync pos only if the end bit flag is set */
			pktz->unpack_last_sync_pos = *bits_pos;
		}

	#if DBG_UNPACKETIZE
		log_debug("Unpacked fragmented H264 NAL unit "
			"(type=%d, NRI=%d, len=%d)",
			TYPE, NRI, payload_len);
	#endif

    } 
	else 
	{
		*bits_pos = 0;
		return PJ_ENOTSUP;
    }

    pktz->unpack_prev_lost = PJ_FALSE;

    return PJ_SUCCESS;
}

/*add for HEVC by 180732700,20190629*/
#ifdef HYT_HEVC

#define H265_UNPACK_INVALID_DATA      -1
#define H265_UNPACK_MULTI_LAYER       -2
#define H265_UNPACK_UNSUP_NALTYPE     -3
#define H265_UNPACK_EAGAIN            -4

#define RTP_H265_PAYLOAD_HEADER_SIZE       2
#define RTP_H265_FU_HEADER_SIZE            1
#define RTP_H265_DONL_FIELD_SIZE           2
#define RTP_H265_DOND_FIELD_SIZE           1
#define RTP_H265_AP_NALU_LENGTH_FIELD_SIZE 2
#define RTP_MAX_HEAD_EXTERN_SIZE           12
#define RTP_H265_BASE_HEADLEN              12

uint8_t  H265_FRAME_START_SEQ[] = {0x00, 0x00, 0x00, 0x01};
static pkt_rtp_payload_msg pkt_msg;

static int pjmedia_calc_rtp_hdr_len(uint8_t *rtp_buf, int len)
{
	int eLen = 0;
	int extension = rtp_buf[0]&0x10;
	int csrc = rtp_buf[0]&0x0F;

	if(csrc)
	{
		eLen += (csrc*4);
	}

	if (extension) 
	{
		if (len < eLen + RTP_H265_BASE_HEADLEN + 4) 
		{
			return -1;
		}

		int iExtenLen = (rtp_buf [eLen + RTP_H265_BASE_HEADLEN + 2] << 8) | rtp_buf [eLen + RTP_H265_BASE_HEADLEN + 3];
		iExtenLen = iExtenLen * 4;

		if (iExtenLen > RTP_MAX_HEAD_EXTERN_SIZE)
		{
			return -1;
		}

		eLen += 4; //ID + LEN
		eLen += iExtenLen; // DATA 
	}

	return RTP_H265_BASE_HEADLEN + eLen;
}

static pj_int32_t find_nalu_heard_len(const pj_uint8_t *nalu)
{
	if(nalu[0]==0 && nalu[1]==0 && nalu[2]==0 && nalu[3]==1) //start nalu = 0x00000001
	{
	    return 4;
	}
	else if(nalu[0]==0 && nalu[1]==0 && nalu[2]==1) //start nalu = 0x000001
	{
	    return 3;
	}
	else
	{
	    return 0;
	}
}

static pj_bool_t find_nalu_confirm_fu_packet(pj_uint8_t *buf_tart, 
	                         pj_uint8_t *buf_end, 
	                         pkt_rtp_payload_msg *msg)
{
    uint8_t *nal_end = NULL;
	int len = 0;

	nal_end =  find_next_nal_unit(buf_tart+msg->nalu_header_len, buf_end);
	if(nal_end)
	{
	    len = nal_end - buf_tart - msg->nalu_header_len;
	}
	else
	{
	   len = buf_end - buf_tart - msg->nalu_header_len;
	}

    log_debug("the last len:%d, is cut header len",len);
	return (len<RTP_H265_PAYLOAD_SIZE)?(0):(1); 
}

/*only to find the a message to packet*/
static pj_bool_t packetize_rtp_payload_msg(pj_uint8_t *buf, 
                           pj_size_t buf_len,
                           pkt_rtp_payload_msg *msg)
{
 	
    PJ_ASSERT_RETURN(buf && msg, PJ_EINVAL);

    uint8_t *nal_start = NULL;
	uint8_t *nal_end = NULL;
	uint8_t *buf_start = NULL;
	pj_int32_t pos = msg->pos;
	pj_int32_t nal_type = 0;
	pj_size_t  lave_nal_len = buf_len - pos;
	pj_size_t find_nal_within_len = WITHIN_LENGTH;

	if((buf_len - pos)<WITHIN_LENGTH)
	{
	    find_nal_within_len = buf_len - pos;
	}

    msg->start = PJ_FALSE; /*reset the FU start and end bit*/
	msg->end   = PJ_FALSE;
    buf_start = buf + msg->pos;
    //log_debug("dym_h265 buf:%p, buf_start:%p, pos:%d, lave_nal_len:%d",buf,buf_start,pos,lave_nal_len);
	
	nal_start = find_next_nal_unit(buf_start, buf_start + NALU_H265_HEADER_SIZE);
	if(NULL==nal_start)
	{
	    msg->start_code = 0;
		nal_start = find_next_nal_unit(buf_start, buf_start + find_nal_within_len);
		if(nal_start)
		{
		    /*find the start code,it is mean have many frame in buf*/
			msg->len = nal_start - buf_start;
			msg->end = 1;
			nal_type = (nal_start[msg->nalu_header_len] >> 1) & 0x3F;
			log_debug("dym_h265 have many frame, nal_type:%d",nal_type);
		}
		else
		{
		    msg->len = (lave_nal_len<RTP_H265_PAYLOAD_SIZE)?(lave_nal_len):(RTP_H265_PAYLOAD_SIZE);
			msg->end = (lave_nal_len<RTP_H265_PAYLOAD_SIZE)?(1):(0);
		}
	}
	else  
	{
	    msg->start_code = 1; /*find the start code*/ 
		msg->nal_type = (nal_start[msg->nalu_header_len] >> 1) & 0x3F;
		log_debug("dym_h265 the nalu start code,nal_start:%p, nalu_header_len:%d, type:%d",
			nal_start,msg->nalu_header_len,msg->nal_type);

		if(HEVC_NAL_UNIT_VPS != msg->nal_type && HEVC_NAL_UNIT_SPS != msg->nal_type
		   && HEVC_NAL_UNIT_PPS != msg->nal_type && HEVC_NAL_UNIT_SEI != msg->nal_type)
		{
		    msg->start = 1;
            msg->fu_packet = find_nalu_confirm_fu_packet(nal_start, buf+buf_len, msg);
		}
		
	    /*find the nalu len within the max length*/
	    nal_end = find_next_nal_unit(nal_start + msg->nalu_header_len, nal_start + find_nal_within_len);
		if(nal_end)
		{
		    msg->len = nal_end - nal_start;
			nal_type = (nal_end[msg->nalu_header_len] >> 1) & 0x3F;
			log_debug("dym_h265 the next nalu type:%d",nal_type);
			if(HEVC_NAL_UNIT_IDR == msg->nal_type 
			   || HEVC_NAL_UNIT_CODED_SLICE_TRAIL_R == msg->nal_type)
			{
			    msg->end = 1;
			}
		}
		else
		{
		    //log_debug("dym_h265 msg->len:%d, lave_nal_len:%d, RTP_H265_PAYLOAD_SIZE:%d ",msg->len,lave_nal_len,RTP_H265_PAYLOAD_SIZE);
		    msg->len = (lave_nal_len<RTP_H265_PAYLOAD_SIZE)?(lave_nal_len):(RTP_H265_PAYLOAD_SIZE);
			msg->end = (lave_nal_len<RTP_H265_PAYLOAD_SIZE)?(1):(0);
		}
	}

	log_debug("dym_h265 msg,type:%d, start:%d, end:%d, len:%d, start_code:%d, pos:%d, header:%d,fu_packet:%d",
		msg->nal_type,msg->start,msg->end,msg->len,msg->start_code,msg->pos,msg->nalu_header_len,msg->fu_packet);
	return PJ_SUCCESS;
}


/**********************************************************************************
encode and decode the HEVC payload header according to section 4 of draft version 6

create the HEVC payload header and transmit the buffer as fragmentation units (FU)
				0				1
				0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 
				+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
				|F|   Type	  |  LayerId  | TID | 
				+-------------+-----------------+ 
				F		= 0 
				Type	= 49 (fragmentation unit (FU)) 
				LayerId = 0 
				TID 	= 1 


			   create the FU header
				0 1 2 3 4 5 6 7
				+-+-+-+-+-+-+-+-+
				|S|E|  FuType	|
				+---------------+
				S		= variable	
				E		= variable
				FuType	= NAL unit type
**********************************************************************************/

/*
 * Generate an RTP payload from H.265 frame bitstream, in-place processing.
 * buf --- one frame video' H265 bitstreams
 * buf_len --- bitstreams length
 * pos --- sign packet rtp whether over
 * payload --- H265 bitstreams packe to RTP payload
 * payload_len --- RTP payload length
 */
PJ_DEF(pj_status_t) pjmedia_h265_packetize(pjmedia_h265_packetizer *pktz,
										   pj_uint8_t *buf,
										   pj_size_t buf_len,
										   unsigned *pos,
										   pj_uint8_t *payload,
										   pj_size_t *payload_len)
{
	pkt_rtp_payload_msg *msg = &pkt_msg;
	pj_int32_t nalu_len = 0;

	if(!buf || !buf_len || buf_len<=NALU_H265_HEADER_SIZE-1)
	{
		log_error("dym_h265 no h265 bitstreams");
		return -1;
	}

	if (0 == *pos) 
	{
	    memset(msg, 0, sizeof(pkt_msg));
		msg->nalu_header_len = find_nalu_heard_len(buf);
		msg->fu_packet = PJ_FALSE;
		log_debug("dym_h265 <<Start pack buf:%p, len:%d>>",buf,buf_len);
	}

	msg->pos = *pos;
    
	packetize_rtp_payload_msg(buf, buf_len, msg);

	//moved over the nalu header 00 00 00 01 or 00 00 01
	nalu_len = msg->len - msg->start_code * msg->nalu_header_len;
	*pos = *pos + msg->start_code * msg->nalu_header_len;

	if(HEVC_NAL_UNIT_VPS == msg->nal_type || HEVC_NAL_UNIT_SPS == msg->nal_type 
	   || HEVC_NAL_UNIT_PPS == msg->nal_type || HEVC_NAL_UNIT_SEI == msg->nal_type
	   || (!msg->fu_packet))
	{
		memcpy(payload, buf + *pos, nalu_len);
		*payload_len = nalu_len;
		*pos += nalu_len;
		//log_debug("dym_h265 all packet, *pos:%d, nal_len:%d, payload_len:%d",*pos,nalu_len,*payload_len);
		return PJ_SUCCESS;
	}
	else
	{
	    /*creat the fragmentation units*/
        payload[0] = 49 << 1;  /*sign the packet is FU subcontracting*/
        payload[1] = 1;        /*the value should calculation*/
		payload[2] = 0;        /*first reset the FU header*/
		payload[2] =  msg->nal_type;
		payload[2] |= (msg->start << 7);  /*set the S bit ,mark as first fragment */
		payload[2] |= (msg->end << 6);    /*set the E bit ,mark as last fragment */
		
		nalu_len = nalu_len - ((msg->start_code)?(2):(0));
		*pos = *pos + ((msg->start_code)?(2):(0)); /* pass the original NAL header */

		//log_debug("dym_h265 packet rtp,buf:%p, *pos:%d, nal_len:%d",buf,*pos,nalu_len);

		memcpy(payload+RTP_H265_HEADERS_SIZE, buf + *pos, nalu_len);
		*payload_len = nalu_len + RTP_H265_HEADERS_SIZE/* + ((msg->end)?(2):(0))*/;
		*pos += nalu_len;
		log_debug("dym_h265 all packet, *pos:%d, nal_len:%d,S:%d,E:%d, payload_len:%d \n",
			*pos,nalu_len,payload[2]>>7,(payload[2]>>6)&0x01,*payload_len);
		
		return PJ_SUCCESS;
	}
	
	return -1;
}

PJ_DEF(pj_status_t)  pjmeida_h265_unpacketize(uint8_t *rtp_buf, int len,  uint8_t* h265_buf, int* pos)
{
    const uint8_t *rtp_pl;
    int tid, lid, nal_type;
    int first_fragment, last_fragment, fu_type;
    uint8_t new_nal_header[2];
    int res = 0;
    int rtp_hdr_len = 0;
    uint8_t* data;

    uint16_t* pSeq = (uint16_t*)(rtp_buf+2);
    int seq = pj_ntohs(*pSeq);

    /* skip rtp header */
    rtp_hdr_len = pjmedia_calc_rtp_hdr_len(rtp_buf, len);
    if(-1 ==  rtp_hdr_len)
    {
   	    log_error("parse rtp header failure");
        return H265_UNPACK_INVALID_DATA;
    }

    rtp_buf += rtp_hdr_len;
    len -= rtp_hdr_len;

    log_debug("dym_h265 hevc seq:%d, payload len:%d, rtp_hdr_len:%d, ahead:[%02x %02x %02x], FU nal_type:%d",  
		seq, len,  rtp_hdr_len, rtp_buf[0], rtp_buf[1], rtp_buf[2], (rtp_buf[0] >> 1) & 0x3f);

    rtp_pl = rtp_buf;
    data = h265_buf + *pos;

    /* sanity check for size of input packet: 1 byte payload at least */
    if (len < 3) {
	    log_error("dym_h265 Too short RTP/HEVC packet, got %d bytes", len);
        return H265_UNPACK_INVALID_DATA;
    }

    /*decode the HEVC payload header */
    nal_type =  (rtp_buf[0] >> 1) & 0x3f;
    lid  = ((rtp_buf[0] << 5) & 0x20) | ((rtp_buf[1] >> 3) & 0x1f);
    tid  =   rtp_buf[1] & 0x07;

    if (lid) 
	{
        /* future scalable or 3D video coding extensions */
	    log_error("dym_h265 Multi-layer HEVC coding, lid=%d",lid);
        return H265_UNPACK_MULTI_LAYER;
    }

    if (!tid)
	{
        log_error("dym_h265 Illegal temporal ID in RTP/HEVC packet");
        return H265_UNPACK_INVALID_DATA;
    }

    if (nal_type > 50) 
	{
	    log_error("dym_h265 Unsupported (HEVC) NAL type (%d)", nal_type);
        return H265_UNPACK_UNSUP_NALTYPE;
    }

    switch (nal_type) 
	{
		case HEVC_NAL_UNIT_VPS:
		case HEVC_NAL_UNIT_SPS:
		case HEVC_NAL_UNIT_PPS:
		case HEVC_NAL_UNIT_SEI:
		default: /* single NAL unit packet */
			if (len < 1) 
			{
				log_error("dym_h265 Too short RTP/HEVC packet, got %d bytes of NAL unit type %d", len, nal_type);
				return H265_UNPACK_INVALID_DATA;
			}

			/* A/V packet: copy start sequence */
			memcpy(data, H265_FRAME_START_SEQ, sizeof(H265_FRAME_START_SEQ));
			/* A/V packet: copy NAL unit data */
			memcpy(data + sizeof(H265_FRAME_START_SEQ), rtp_buf, len);
			(*pos) +=(sizeof(H265_FRAME_START_SEQ) + len);
			break;
		case HEVC_NAL_UNIT_UNSPECIFIED_49:
			/* pass the HEVC payload header */
			rtp_buf += RTP_H265_PAYLOAD_HEADER_SIZE;
			len -= RTP_H265_PAYLOAD_HEADER_SIZE;

			/*decode the FU header*/
			first_fragment = rtp_buf[0] & 0x80;
			last_fragment  = rtp_buf[0] & 0x40;
			fu_type        = rtp_buf[0] & 0x3f;

			/* pass the HEVC FU header */
			rtp_buf += RTP_H265_FU_HEADER_SIZE;
			len -= RTP_H265_FU_HEADER_SIZE;
			
			log_debug("dym_h265 hevc startbit:%d, endbit:%d, fu-type:%d, length:%d", first_fragment>>7, last_fragment>>6, fu_type, len);

			if (len > 0) 
			{
				new_nal_header[0] = (rtp_pl[0] & 0x81) | (fu_type << 1);
				new_nal_header[1] = rtp_pl[1];

				/* start fragment vs. subsequent fragments */
				if (first_fragment) 
				{
					if (!last_fragment) 
					{
						/* A/V packet: copy start sequence */
						memcpy(data, H265_FRAME_START_SEQ, sizeof(H265_FRAME_START_SEQ));
						/* A/V packet: copy new NAL header */
						memcpy(data + sizeof(H265_FRAME_START_SEQ), new_nal_header, sizeof(new_nal_header));
						/* A/V packet: copy NAL unit data */
						memcpy(data + sizeof(H265_FRAME_START_SEQ) + sizeof(new_nal_header), rtp_buf, len);

						(*pos) +=(sizeof(H265_FRAME_START_SEQ) + sizeof(new_nal_header) + len);
					} 
					else 
					{
						log_debug("dym_h265 Illegal combination of S and E bit in RTP/HEVC packet");
						res = H265_UNPACK_INVALID_DATA;
					}
				} 
				else 
				{
					/* A/V packet: copy NAL unit data */
					memcpy(data, rtp_buf, len);
					(*pos) += len;

				if(last_fragment)
				{
					res = *pos;
					*pos = 0;
				}
				}
			}
			else 
			{
			if (len < 0) {
				log_error("dym_h265 Too short RTP/HEVC packet, got %d bytes of NAL unit type %d", len, nal_type);
					res = H265_UNPACK_INVALID_DATA;
				} else {
					res = H265_UNPACK_EAGAIN;
				}
			}

			break;
		case HEVC_NAL_UNIT_UNSPECIFIED_48: /* aggregated packets (AP) */
			rtp_buf += RTP_H265_PAYLOAD_HEADER_SIZE;
			len -= RTP_H265_PAYLOAD_HEADER_SIZE;
		case HEVC_NAL_UNIT_UNSPECIFIED_50:
			/* Temporal scalability control information (TSCI) */
			log_debug("dym_h265  PACI packets for RTP/HEVC");
			res = H265_UNPACK_MULTI_LAYER;
			break;
    }

    return res;
}

#endif /*HYT_HEVC*/
/*end for HEVC by 180732700,20190629*/


#endif /* PJMEDIA_HAS_VIDEO */
