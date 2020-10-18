#ifndef SPS_DECODER_H_
#define SPS_DECODER_H_

#include <stdint.h>
#ifndef __linux__
#include <crtdefs.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#define SPS_MAX (32)
typedef struct decoder_sys_t
{
	// Useful values of the Sequence Parameter Set 
	int i_log2_max_frame_num;
	int b_frame_mbs_only;
	int i_pic_order_cnt_type;
	int i_delta_pic_order_always_zero_flag;
	int i_log2_max_pic_order_cnt_lsb;
} decoder_sys_t;

//video format description

typedef struct video_format_t
{
	unsigned int i_width;                                 // picture width 
	unsigned int i_height;                               // picture height 
	unsigned int i_x_offset;               // start offset of visible area 
	unsigned int i_y_offset;               // start offset of visible area 
	unsigned int i_visible_width;                 // width of visible area 
	unsigned int i_visible_height;               // height of visible area 
	unsigned int i_bits_per_pixel;             // number of bits per pixel 
	unsigned int i_sar_num;                   // sample/pixel aspect ratio 
	unsigned int i_sar_den;
	unsigned int i_frame_rate;                     // frame rate numerator 
	unsigned int i_frame_rate_base;              // frame rate denominator 
	unsigned int i_rmask, i_gmask, i_bmask;          //uint32_t// color masks for RGB chroma 
	int i_rrshift, i_lrshift;
	int i_rgshift, i_lgshift;
	int i_rbshift, i_lbshift;
} video_format_t;

typedef struct es_format_t
{
	video_format_t video;     // description of video format 
	int      i_profile;       // codec specific information (like real audio flavor, mpeg audio layer, h264 profile ...) 
	int      i_level;         // codec specific information: indicates maximum restrictions on the stream (resolution, bitrate, codec features ...) 
} es_format_t;

typedef struct decoder_s
{
	decoder_sys_t sys;
	es_format_t   fmt_out;
} decoder_t;
typedef struct bs_s
{
	uint8_t *p_start;
	uint8_t *p;
	uint8_t *p_end;

	int  i_left;    //ssize_t  // i_count number of available bits 
} bs_t;

static inline void bs_init(bs_t *s, const void *p_data, size_t i_data)
{
	s->p_start = (uint8_t *)p_data;
	s->p = s->p_start;
	s->p_end = s->p_start + i_data;
	s->i_left = 8;
}
static int CreateDecodedNAL(uint8_t *dst, int i_dst, const uint8_t *src, int i_src)//ssize_t
{
	if (!dst || !src || i_dst < i_src)
		return -1;

	const uint8_t *end = &src[i_src];
	uint8_t *p = dst;

	if (p)
	{
		while (src < end)
		{
			if (src < end - 3 && src[0] == 0x00 && src[1] == 0x00 &&
				src[2] == 0x03)
			{
				*p++ = 0x00;
				*p++ = 0x00;

				src += 3;
				continue;
			}
			*p++ = *src++;
		}
	}
	return (int)(p - dst);
}
static inline uint32_t bs_read(bs_t *s, int i_count)
{
	static const uint32_t i_mask[33] =
	{ 0x00,
	0x01, 0x03, 0x07, 0x0f,
	0x1f, 0x3f, 0x7f, 0xff,
	0x1ff, 0x3ff, 0x7ff, 0xfff,
	0x1fff, 0x3fff, 0x7fff, 0xffff,
	0x1ffff, 0x3ffff, 0x7ffff, 0xfffff,
	0x1fffff, 0x3fffff, 0x7fffff, 0xffffff,
	0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
	0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff };
	int      i_shr;
	uint32_t i_result = 0;

	while (i_count > 0)
	{
		if (s->p >= s->p_end)
		{
			break;
		}

		if ((i_shr = s->i_left - i_count) >= 0)
		{
			// more in the buffer than requested 
			i_result |= (*s->p >> i_shr)&i_mask[i_count];
			s->i_left -= i_count;
			if (s->i_left == 0)
			{
				s->p++;
				s->i_left = 8;
			}
			return(i_result);
		}
		else
		{
			// less in the buffer than requested 
			i_result |= (*s->p&i_mask[s->i_left]) << -i_shr;
			i_count -= s->i_left;
			s->p++;
			s->i_left = 8;
		}
	}

	return(i_result);
}
static inline uint32_t bs_read1(bs_t *s)
{
	if (s->p < s->p_end)
	{
		unsigned int i_result;

		s->i_left--;
		i_result = (*s->p >> s->i_left) & 0x01;
		if (s->i_left == 0)
		{
			s->p++;
			s->i_left = 8;
		}
		return i_result;
	}

	return 0;
}
static inline void bs_skip(bs_t *s, int i_count)//ssize_t
{
	s->i_left -= i_count;

	if (s->i_left <= 0)
	{
		const int i_bytes = (-s->i_left + 8) / 8;

		s->p += i_bytes;
		s->i_left += 8 * i_bytes;
	}
}

static inline int bs_read_ue(bs_t *s)
{
	int i = 0;

	while (bs_read1(s) == 0 && s->p < s->p_end && i < 32)
	{
		i++;
	}

	return((1 << i) - 1 + bs_read(s, i));
}
static inline int bs_read_se(bs_t *s)
{
	int val = bs_read_ue(s);

	return val & 0x01 ? (val + 1) / 2 : -(val / 2);
}

static inline int GetSpsLen(uint8_t *buffer, size_t bufsiz)
{
	unsigned char *p = buffer;

	if (p)
	{
		while (p<buffer + bufsiz - 3 && !(p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01))
		{
			p++;
		}

		if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01)
		{
			return (int)(p - buffer);
		}
	}

	return (int)bufsiz;
}

static void PutSPS(decoder_t *p_dec, uint8_t *buffer, size_t bufsiz)
{
	decoder_sys_t *p_sys = &p_dec->sys;

	uint8_t *pb_dec = NULL;
	int     i_dec = 0;
	bs_t s;
	int i_tmp;
	int i_sps_id;

	int spsLen = GetSpsLen(buffer + 1, bufsiz - 1);

	//pb_dec = (uint8_t *)alloca(spsLen);	 //栈上分配,不需要释放
	//i_dec = CreateDecodedNAL(pb_dec, (int)(bufsiz - 1), &buffer[1], (int)(bufsiz - 1));
	
	pb_dec = (uint8_t *) new (std::nothrow) char[spsLen + 1];
	if (pb_dec == NULL)
	{
		return;
	}

	i_dec = CreateDecodedNAL(pb_dec, spsLen, buffer + 1, spsLen);

	if (i_dec < 0)
	{
		if (pb_dec != NULL)
		{
			delete[] pb_dec;
			pb_dec = NULL;
		}
		
		return;
	}

	bs_init(&s, pb_dec, i_dec);
	int i_profile_idc = bs_read(&s, 8);
	p_dec->fmt_out.i_profile = i_profile_idc;

	// Skip constraint_set0123, reserved(4) 
	bs_skip(&s, 1 + 1 + 1 + 1 + 4);
	p_dec->fmt_out.i_level = bs_read(&s, 8);

	//sps id 
	i_sps_id = bs_read_ue(&s);
	if (i_sps_id >= SPS_MAX || i_sps_id < 0)
	{
		if (pb_dec != NULL)
		{
			delete[] pb_dec;
			pb_dec = NULL;
		}

		return;
	}

	if (i_profile_idc == 100 || i_profile_idc == 110 ||
		i_profile_idc == 122 || i_profile_idc == 244 ||
		i_profile_idc == 44 || i_profile_idc == 83 ||
		i_profile_idc == 86)
	{
		// chroma_format_idc 
		const int i_chroma_format_idc = bs_read_ue(&s);
		if (i_chroma_format_idc == 3)
			bs_skip(&s, 1); // separate_colour_plane_flag 
		// bit_depth_luma_minus8 
		bs_read_ue(&s);
		// bit_depth_chroma_minus8 
		bs_read_ue(&s);
		// qpprime_y_zero_transform_bypass_flag 
		bs_skip(&s, 1);
		// seq_scaling_matrix_present_flag 
		i_tmp = bs_read(&s, 1);
		if (i_tmp)
		{
			int i;
			for (i = 0; i < ((3 != i_chroma_format_idc) ? 8 : 12); i++)
			{
				// seq_scaling_list_present_flag[i] 
				i_tmp = bs_read(&s, 1);
				if (!i_tmp)
					continue;
				const int i_size_of_scaling_list = (i < 6) ? 16 : 64;
				// scaling_list (...) 
				int i_lastscale = 8;
				int i_nextscale = 8;
				int j;
				for (j = 0; j < i_size_of_scaling_list; j++)
				{
					if (i_nextscale != 0)
					{
						// delta_scale 
						i_tmp = bs_read_se(&s);
						i_nextscale = (i_lastscale + i_tmp + 256) % 256;
						// useDefaultScalingMatrixFlag = ... 
					}
					// scalinglist[j] 
					i_lastscale = (i_nextscale == 0) ? i_lastscale : i_nextscale;
				}
			}
		}
	}

	//Skip i_log2_max_frame_num 
	p_sys->i_log2_max_frame_num = bs_read_ue(&s);
	if (p_sys->i_log2_max_frame_num > 12)
		p_sys->i_log2_max_frame_num = 12;
	// Read poc_type 
	p_sys->i_pic_order_cnt_type = bs_read_ue(&s);
	if (p_sys->i_pic_order_cnt_type == 0)
	{
		// skip i_log2_max_poc_lsb 
		p_sys->i_log2_max_pic_order_cnt_lsb = bs_read_ue(&s);
		if (p_sys->i_log2_max_pic_order_cnt_lsb > 12)
			p_sys->i_log2_max_pic_order_cnt_lsb = 12;
	}
	else if (p_sys->i_pic_order_cnt_type == 1)
	{
		int i_cycle;
		// skip b_delta_pic_order_always_zero 
		p_sys->i_delta_pic_order_always_zero_flag = bs_read(&s, 1);
		// skip i_offset_for_non_ref_pic 
		bs_read_se(&s);
		// skip i_offset_for_top_to_bottom_field 
		bs_read_se(&s);
		// read i_num_ref_frames_in_poc_cycle 
		i_cycle = bs_read_ue(&s);
		if (i_cycle > 256) i_cycle = 256;
		while (i_cycle > 0)
		{
			//skip i_offset_for_ref_frame 
			bs_read_se(&s);
			i_cycle--;
		}
	}
	// i_num_ref_frames 
	bs_read_ue(&s);
	// b_gaps_in_frame_num_value_allowed 
	bs_skip(&s, 1);

	// Read size 
	p_dec->fmt_out.video.i_width = 16 * (bs_read_ue(&s) + 1);
	p_dec->fmt_out.video.i_height = 16 * (bs_read_ue(&s) + 1);

	// b_frame_mbs_only 
	p_sys->b_frame_mbs_only = bs_read(&s, 1);
	p_dec->fmt_out.video.i_height *= (2 - p_sys->b_frame_mbs_only);
	if (p_sys->b_frame_mbs_only == 0)
	{
		bs_skip(&s, 1);
	}
	// b_direct8x8_inference 
	bs_skip(&s, 1);

	// crop 
	i_tmp = bs_read(&s, 1);
	if (i_tmp)
	{
		// left 
		int crop_left = bs_read_ue(&s);
		// right 
		int crop_right = bs_read_ue(&s);
		// top 
		int crop_top = bs_read_ue(&s);
		// bottom 
		int crop_bottom = bs_read_ue(&s);

		p_dec->fmt_out.video.i_width -= crop_left * 2 + crop_right * 2;
		p_dec->fmt_out.video.i_height -= crop_top * 2 + crop_bottom * 2;
	}

	// vui 
	i_tmp = bs_read(&s, 1);
	if (i_tmp)
	{
		// read the aspect ratio part if any 
		i_tmp = bs_read(&s, 1);
		if (i_tmp)
		{
			static const struct { int w, h; } sar[17] =
			{
				{ 0, 0 }, { 1, 1 }, { 12, 11 }, { 10, 11 },
				{ 16, 11 }, { 40, 33 }, { 24, 11 }, { 20, 11 },
				{ 32, 11 }, { 80, 33 }, { 18, 11 }, { 15, 11 },
				{ 64, 33 }, { 160, 99 }, { 4, 3 }, { 3, 2 },
				{ 2, 1 },
			};
			int i_sar = bs_read(&s, 8);
			int w, h;

			if (i_sar < 17)
			{
				w = sar[i_sar].w;
				h = sar[i_sar].h;
			}
			else if (i_sar == 255)
			{
				w = bs_read(&s, 16);
				h = bs_read(&s, 16);
			}
			else
			{
				w = 0;
				h = 0;
			}

			if (w != 0 && h != 0)
			{
				p_dec->fmt_out.video.i_sar_num = w;
				p_dec->fmt_out.video.i_sar_den = h;
			}
			else
			{
				p_dec->fmt_out.video.i_sar_num = 1;
				p_dec->fmt_out.video.i_sar_den = 1;
			}
		}
	}

	if (pb_dec != NULL)
	{
		delete[] pb_dec;
		pb_dec = NULL;
	}
}

//sps 不带帧起始码
//size 不带起始码长度
int h264_sps_get_dimension(uint8_t *sps, size_t size, int *width, int *height, int *parW, int *parH)
{
	decoder_t dec;
	memset(&dec, 0, sizeof(decoder_t));
	uint8_t *frame = sps;

	if (frame [0] == 0x00 && frame [1] == 0x00 && frame [2] == 0x01 && (frame [3] & 0x1f) == 7) 
	{
		frame = frame + 3;
		size -= 3;
	}
	else if (frame [0] == 0x00 && frame [1] == 0x00 && frame [2] == 0x00 && frame [3] == 0x01 && (frame [4] & 0x1f) == 7) 
	{
		frame = frame + 4;
		size -= 4;
	}
	else 
	{
		return -1;
	}

	PutSPS(&dec, frame, size);

	*width = dec.fmt_out.video.i_width;
	*height = dec.fmt_out.video.i_height;

	*parW = dec.fmt_out.video.i_sar_num;
	*parH = dec.fmt_out.video.i_sar_den;

	return 0;
}
#endif




