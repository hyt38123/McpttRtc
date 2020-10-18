

#include "rtcp.h"
#include "utils.h"



pj_status_t rtcp_build_rtcp_nack( //pjmedia_rtcp_session *session, 
					    void *buf,
					    pj_size_t *length,
					    const pjmedia_rtcp_nack *nack){
    pjmedia_rtcp_nack_common *hdr;
    pj_uint8_t *p;
	pj_uint32_t *p32;
	pj_uint16_t *p16;
    pj_size_t len;

    //PJ_ASSERT_RETURN(session && buf && length && nack, PJ_EINVAL);

    /* Verify buffer length */
    len = sizeof(*hdr);
	len += 8;

    if (len > *length)
	    return -1;//PJ_ETOOSMALL;

    /* Build RTCP SDES header */
    hdr = (pjmedia_rtcp_nack_common*)buf;
    //pj_memcpy(hdr, (pjmedia_rtcp_nack_common *)&session->rtcp_sr_pkt.common,  sizeof(*hdr));
    hdr->pt = RTCP_NACK;
    hdr->length = pj_htons((pj_uint16_t)(len/4 - 1));
	hdr->fmt = 1;

    /* Build RTCP nack items */
    p = (pj_uint8_t*)hdr + sizeof(*hdr);
    p32 = (pj_uint32_t*)p;
	*p32++ = nack->ssrc;
	p16 = (pj_uint16_t*)p32;
	*p16++ = nack->base_seq;
	*p16 = nack->flag;

    /* Null termination */
	p = p + 8;

    /* Finally */
    //pj_assert((int)len == p-(pj_uint8_t*)buf);

    *length = len;

    return PJ_SUCCESS;
}