#include "utils.h"

long get_currenttime_us() {
	struct timeval tv_cur;
	gettimeofday(&tv_cur, NULL);
	long time = tv_cur.tv_sec*1000000 + tv_cur.tv_usec;
	return time;
}

long get_timeofday_us(const struct timeval *tval) {
    if(tval==NULL)
        return 0;
    return  tval->tv_sec*1000000 + tval->tv_usec;
}


/**
 * Fill the memory location with zero.
 *
 * @param dst        The destination buffer.
 * @param size        The number of bytes.
 */
void pj_bzero(void *dst, pj_size_t size)
{
#if defined(PJ_HAS_BZERO) && PJ_HAS_BZERO!=0
    bzero(dst, size);
#else
    memset(dst, 0, size);
#endif
}

void* pj_memcpy(void *dst, const void *src, pj_size_t size)
{
    return memcpy(dst, src, size);
}

/*
 * Convert 16-bit value from network byte order to host byte order.
 */
pj_uint16_t pj_ntohs(pj_uint16_t netshort)
{
    return ntohs(netshort);
}

pj_uint16_t pj_htons(pj_uint16_t netshort)
{
    return htons(netshort);
}
