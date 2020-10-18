#include "vid_codec.h"
#include "h264_packetizer.h"

int get_h264_package(char* frameBuffer, int frameLen, pjmedia_frame *out_package) {
    int result = -1;
    pjmedia_h264_packetizer pktz;
	pktz.cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
	pktz.cfg.mtu  = PJMEDIA_MAX_VID_PAYLOAD_SIZE;

	if(out_package->enc_packed_pos<frameLen) {
		pj_status_t res =  pjmedia_h264_packetize(&pktz,
										frameBuffer,
										frameLen,
										&out_package->enc_packed_pos,
										out_package->buf, 
                                        &out_package->size);
        out_package->has_more = (out_package->enc_packed_pos < frameLen);
        return out_package->size;
	}
    return result;
}

int get_h265_package(char* frameBuffer, int frameLen, pjmedia_frame *out_package) {
	int result = -1;
    pjmedia_h265_packetizer pktz;
	//pktz.cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
	pktz.cfg.mtu  = PJMEDIA_MAX_VID_PAYLOAD_SIZE;

	if(out_package->enc_packed_pos<frameLen) {
		pj_status_t res =  pjmedia_h265_packetize(&pktz,
										frameBuffer,
										frameLen,
										&out_package->enc_packed_pos,
										out_package->buf, 
                                        &out_package->size);
        out_package->has_more = (out_package->enc_packed_pos < frameLen);
        return out_package->size;
	}
    return result;
}

