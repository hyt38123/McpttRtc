#include "h264.h"
#include "basedef.h"
#include "rtp.h"

#include "vid_stream.h"
#include "h264_packetizer.h"

typedef enum frame_type { 
	H264_FRAME = 1,
	H265_FRAME = 2
}frame_type;

void h265_test(pj_uint8_t *bits, pj_size_t bits_len) {
	static pjmedia_h265_packetizer *pktz;
	if(!pktz) {
		pktz = (pjmedia_h265_packetizer*)malloc(sizeof(pjmedia_h265_packetizer));
		memset(pktz, 0, sizeof(pjmedia_h265_packetizer));
		//pktz->cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
		pktz->cfg.mtu  = PJMEDIA_MAX_VID_PAYLOAD_SIZE;
	}

	pj_size_t payload_len = 0;
	unsigned int	enc_packed_pos = 0;
	pj_uint8_t payload[PJMEDIA_MAX_VID_PAYLOAD_SIZE] = {0};
	while(enc_packed_pos<bits_len) {
		pj_status_t res =  pjmedia_h265_packetize(pktz,
										bits,
										bits_len,
										&enc_packed_pos,
										payload,
										&payload_len);
        UNUSED(res);
		//GLOGE("------get size:%d payload_len:%d enc_packed_pos:%d indicator:%02X\n", bits_len, payload_len, enc_packed_pos, payload[0]);
	}

}

void get_file_exten(const char*fileName, char*outExten) {
    int i=0,length= strlen(fileName);
    while(fileName[i]) {
        if(fileName[i]=='.')
            break;
            i++;
    }
    if(i<length)
        strcpy(outExten, fileName+i+1);
    else
        strcpy(outExten, "\0");
}

void vid_stream_test_file(char*filename) {
    FILE			*pFile;
	NALU_t 			*mNALU;
    char idrBuf[2*1024*1024] = {0};
	pFile = OpenBitstreamFile( filename ); //camera_640x480.h264 //camera_1280x720.h264
	if(pFile==NULL) {
		GLOGE("open file:%s failed.", filename);
		return ;
	}

    //vid_stream_create();

    char extenName[12] = {0};
    get_file_exten(filename, extenName);
    GLOGE("filename exten:%s\n", extenName);

    int frameType = 0;
    if(0==strcmp("h265", extenName));
        frameType = H265_FRAME;
    if(0==strcmp("h264", extenName))
        frameType = H264_FRAME;

	mNALU  = AllocNALU(8000000);

	int count = 0, cpyPos = 0;
	do {
		count++;
		int size=GetAnnexbNALU(pFile, mNALU);//每执行一次，文件的指针指向本次找到的NALU的末尾，下一个位置即为下个NALU的起始码0x000001
		GLOGE("GetAnnexbNALU type:0x%02X size:%d count:%d\n", mNALU->buf[4], size, count);
		if(size<4) {
			GLOGE("get nul error!\n");
			continue;
		}

        int type = mNALU->buf[4] & 0x1F;
        if(type==7 || type==8) {
            memcpy(idrBuf+cpyPos, mNALU->buf, size);
            cpyPos += size;
            continue;
        }

        memcpy(idrBuf+cpyPos, mNALU->buf, size);
        cpyPos += size;
		//GLOGE("GetAnnexbNALU frame size:%d\n", cpyPos);

        switch(frameType) {
            case H264_FRAME:
                //h264_test(mNALU->buf, size);
                //h264_package_test(idrBuf, cpyPos);
                packet_and_send_test(idrBuf, cpyPos);
				//rtp_packet_recv_test(idrBuf, cpyPos);
                break;
            case H265_FRAME:
                h265_test(mNALU->buf, size);
                break;
        }
        cpyPos = 0;
	}while(!feof(pFile));

	if(pFile != NULL)
		fclose(pFile);

	FreeNALU(mNALU);

    //vid_stream_destroy();
}

int main( int argc, char ** argv)
{
    if ((argc != 2)) {
		printf("Usage: exe codecType.\n");
		return 0;
	}

    int frameType = 0;
    if(0==strcmp("2", argv[1]));
        frameType = H265_HARD_CODEC;
    if(0==strcmp("1", argv[1]))
        frameType = H264_HARD_CODEC;

    stream_recv_test("127.0.0.1", 38123, frameType);
    return 0;
}

