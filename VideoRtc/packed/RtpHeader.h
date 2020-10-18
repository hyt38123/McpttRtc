#ifndef RTPHEADER_H
#define RTPHEADER_H
 
/*
 little endian
*/

//read unsigned short from unsigned char*
inline unsigned short le_read_uint16(const unsigned char* ptr)
{
	return (((unsigned short)ptr[1]) << 8) | ptr[0];
}

//read unsigned int from unsigned char*
inline unsigned int le_read_uint32(const unsigned char* ptr)
{
	return (((unsigned int)ptr[3]) << 24) | (((unsigned int)ptr[2]) << 16) | (((unsigned int)ptr[1]) << 8) | ptr[0];
}

//write unsigned short to unsigned char*
inline void le_write_uint16(unsigned char* ptr, unsigned short val)
{
	ptr[1] = (unsigned char)((val >> 8) & 0xFF);
	ptr[0] = (unsigned char)(val & 0xFF);
}

//write unsigned int to unsigned char*
inline void le_write_uint32(unsigned char* ptr, unsigned int val)
{
	ptr[3] = (unsigned char)((val >> 24) & 0xFF);
	ptr[2] = (unsigned char)((val >> 16) & 0xFF);
	ptr[1] = (unsigned char)((val >> 8) & 0xFF);
	ptr[0] = (unsigned char)(val & 0xFF);
}



/*
 big endian(network byte order)
*/

//read unsigned short from unsigned char*
inline unsigned short be_read_uint16(const unsigned char* ptr)
{
	return (((unsigned short)ptr[0]) << 8) | ptr[1];
}

//read unsigned int from unsigned char*
inline unsigned int be_read_uint32(const unsigned char* ptr)
{
	return (((unsigned int)ptr[0]) << 24) | (((unsigned int)ptr[1]) << 16) | (((unsigned int)ptr[2]) << 8) | ptr[3];
}

//write unsigned short to unsigned char*
inline void be_write_uint16(unsigned char* ptr, unsigned short val)
{
	ptr[0] = (unsigned char)((val >> 8) & 0xFF);
	ptr[1] = (unsigned char)(val & 0xFF);
}

//write unsigned int to unsigned char*
inline void be_write_uint32(unsigned char* ptr, unsigned int val)
{
	ptr[0] = (unsigned char)((val >> 24) & 0xFF);
	ptr[1] = (unsigned char)((val >> 16) & 0xFF);
	ptr[2] = (unsigned char)((val >> 8) & 0xFF);
	ptr[3] = (unsigned char)(val & 0xFF);
}


//network byte order read and write
#define NBO_R16 be_read_uint16
#define NBO_R32 be_read_uint32
#define NBO_W16 be_write_uint16
#define NBO_W32 be_write_uint32


/*
 *************************************  RTP filed helper  ****************************
*/

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

/*网络字节序读写дunsigned int*/
#define RTP_R32 NBO_R32
#define RTP_W32 NBO_W32


inline unsigned int rtp_get_v(const unsigned char* ptr)
{
	unsigned int v = RTP_R32(ptr);
	return ((v >> 30) & 0x03);
}

inline unsigned int rtp_get_p(const unsigned char* ptr)
{
	unsigned int v = RTP_R32(ptr);
	return ((v >> 29) & 0x01);
}

inline unsigned int rtp_get_x(const unsigned char* ptr)
{
	unsigned int v = RTP_R32(ptr);
	return ((v >> 28) & 0x01);
}

inline unsigned int rtp_get_cc(const unsigned char* ptr)
{
	unsigned int v = RTP_R32(ptr);
	return ((v >> 24) & 0x0F);
}

inline unsigned int rtp_get_m(const unsigned char* ptr)
{
	unsigned int v = RTP_R32(ptr);
	return ((v >> 23) & 0x01);
}

inline unsigned int rtp_get_pt(const unsigned char* ptr)
{
	unsigned int v = RTP_R32(ptr);
	return ((v >> 16) & 0x7F);
}

inline unsigned int rtp_get_seq(const unsigned char* ptr)
{
	unsigned int v = RTP_R32(ptr);
	return ((v >> 00) & 0xFFFF);
}

inline unsigned int rtp_get_timestamp(const unsigned char* ptr)
{
	ptr += 4;
	return RTP_R32(ptr);
}

inline void rtp_set_v(unsigned char* ptr, unsigned int v)
{
	unsigned int value = RTP_R32(ptr);
	value |= ((v & 0x03) << 30);
	RTP_W32(ptr, value);
}

inline void rtp_set_p(unsigned char* ptr, unsigned int v)
{
	unsigned int value = RTP_R32(ptr);
	value |= ((v & 0x01) << 29);
	RTP_W32(ptr, value);
}

inline void rtp_set_x(unsigned char* ptr, unsigned int v)
{
	unsigned int value = RTP_R32(ptr);
	value |= ((v & 0x01) << 28);
	RTP_W32(ptr, value);
}

inline void rtp_set_cc(unsigned char* ptr, unsigned int v)
{
	unsigned int value = RTP_R32(ptr);
	value |= ((v & 0x0F) << 24);
	RTP_W32(ptr, value);
}

inline void rtp_set_m(unsigned char* ptr, unsigned int v)
{
	unsigned int value = RTP_R32(ptr);
	value |= ((v & 0x01) << 23);
	RTP_W32(ptr, value);
}

inline void rtp_set_pt(unsigned char* ptr, unsigned int v)
{
	unsigned int value = RTP_R32(ptr);
	value |= ((v & 0x7F) << 16);
	RTP_W32(ptr, value);
}

inline void rtp_set_seq(unsigned char* ptr, unsigned int v)
{
	unsigned int value = RTP_R32(ptr);
	value |= ((v & 0xFFFF) << 00);
	RTP_W32(ptr, value);
}

inline void rtp_set_timestamp(unsigned char* ptr, unsigned int v)
{
	ptr += 4;
	RTP_W32(ptr, v);
}



/*获取rtp字段值*/
#define RTP_GET_V(ptr)			rtp_get_v(ptr) /* protocol version */
#define RTP_GET_P(ptr)			rtp_get_p(ptr) /* padding flag */
#define RTP_GET_X(ptr)			rtp_get_x(ptr) /* header extension flag */
#define RTP_GET_CC(ptr)		    rtp_get_cc(ptr) /* CSRC count */
#define RTP_GET_M(ptr)			rtp_get_m(ptr) /* marker bit */
#define RTP_GET_PT(ptr)		    rtp_get_pt(ptr) /* payload type */
#define RTP_GET_SEQ(ptr)		rtp_get_seq(ptr) /* sequence number */
#define RTP_GET_TIMESTAMP(ptr)  rtp_get_timestamp(ptr) /*timestamp*/

/*����rtp�ֶ�ֵ*/
#define RTP_SET_V(ptr,v)		rtp_set_v(ptr,v) /* protocol version */
#define RTP_SET_P(ptr,v)		rtp_set_p(ptr,v) /* padding flag */
#define RTP_SET_X(ptr,v)		rtp_set_x(ptr,v) /* header extension flag */
#define RTP_SET_CC(ptr,v)		rtp_set_cc(ptr,v) /* CSRC count */
#define RTP_SET_M(ptr,v)		rtp_set_m(ptr,v) /* marker bit */
#define RTP_SET_PT(ptr,v)		rtp_set_pt(ptr,v) /* payload type */
#define RTP_SET_SEQ(ptr,v)		rtp_set_seq(ptr,v) /* sequence number */
#define RTP_SET_TIMESTAMP(ptr,v) rtp_set_timestamp(ptr,v) /*timestamp*/

#endif //RTPHEADER_H
