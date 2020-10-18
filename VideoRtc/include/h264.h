// MPEG2RTP.h
#ifndef H264_H_
#define H264_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//#include <mutex>

//#include "protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif
#ifdef HAV_FFMPEG
	#include "libavformat/avformat.h"
#endif
#ifdef __cplusplus
};
#endif

// #define PACKET_BUFFER_END            (unsigned int)0x00000000
// #define MAX_RTP_PKT_LENGTH     1360

// #define H264                    96


typedef struct 
{
    /**//* byte 0 */
    unsigned char csrc_len:4;        /**//* expect 0 */
    unsigned char extension:1;        /**//* expect 1, see RTP_OP below */
    unsigned char padding:1;        /**//* expect 0 */
    unsigned char version:2;        /**//* expect 2 */
    /**//* byte 1 */
    unsigned char payload:7;        /**//* RTP_PAYLOAD_RTSP */
    unsigned char marker:1;        /**//* expect 1 */
    /**//* bytes 2, 3 */
    unsigned short seq_no;            
    /**//* bytes 4-7 */
    unsigned  long timestamp;        
    /**//* bytes 8-11 */
    unsigned long ssrc;            /**//* stream number is used here. */
} RTP_FIXED_HEADER;

typedef enum MSG_TYPE {
	FILE_RECV_MSG = 1,
	FILE_RECV_STREAM,
	FILE_SEND_MSG,
	FILE_SEND_STREAM,

	VIDEO_RECV_MSG = 101,
	VIDEO_RECV_STREAM,
	VIDEO_SEND_MSG,
	VIDEO_SEND_STREAM,
	VIDEO_REAL_MSG,
	VIDEO_REAL_STREAM,
}MSG_TYPE_t;

typedef struct {
    //byte 0
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;    
} MSG_HEADER; /**//* 1 BYTES */

typedef struct {
    //byte 0
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;
} NALU_HEADER;

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char NRI:2; 
	unsigned char F:1;    
} FU_INDICATOR; /**//* 1 BYTES */

typedef struct {
	char CC:4;
	char X:1;
	char M:1;
	char V:2;
}PACK_TYPE;

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;    
} FU_HEADER; /**//* 1 BYTES */

typedef struct {
	MSG_HEADER  	head;	//1
	unsigned char	type;	//1
	unsigned short 	pid;	//2  every transmit package id
	unsigned int 	fid;	//4	 every media frame id
	unsigned int    len;	//4  len for every frame
}__attribute__((packed))PACK_HEAD,*LPPACK_HEAD;//按照实际占用字节数进行对齐， 8 byte

typedef struct
{
  int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  unsigned max_size;            //! Nal Unit Buffer size
  int forbidden_bit;            //! should be always FALSE
  int nal_reference_idc;        //! NALU_PRIORITY_xxxx
  int nal_unit_type;            //! NALU_TYPE_xxxx
  unsigned char *buf;                    //! contains the first byte followed by the EBSP
  unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;


FILE* 	OpenBitstreamFile (const char *filename);
void  	CloseBitstreamFile(FILE *file);

NALU_t 	*AllocNALU(int buffersize);
void 	FreeNALU(NALU_t *n);
int 	GetAnnexbNALU (FILE *file, NALU_t *nalu);
void 	dump(NALU_t *n);


// #ifndef  MAX_LEN
// #define  MAX_LEN 1300
// #endif

// #ifndef  MAX_MTU
// const int  MAX_MTU  = MAX_LEN+sizeof(PACK_HEAD);
// #endif

// #ifndef  CMD_LEN
// #define  CMD_LEN 1500
// #endif


/*
struct tagFileProcBuffer {
	char cmmd[CMD_LEN];
	bool bProcCmmd;
	int  hasProcLen;
	int  totalLen;//1500 is cmd len
	int  dataLen;
	char *data;

	tagFileProcBuffer() {
		data 		= NULL;
		totalLen 	= 0;
		dataLen 	= 0;
		hasProcLen 	= 0;
		bProcCmmd 	= true;
	}

	void reset() {
		//std::lock_guard<std::mutex> lk(mut);
		memset(cmmd, 0, CMD_LEN);
		if(data) {
		}
		hasProcLen 	= 0;
		totalLen 	= 0;
		dataLen		= 0;
		bProcCmmd 	= true;
	}

	void createMem(int len) {
		if(data==NULL)
			data = (char*)malloc(len);
	}

	void releaseMem() {
		if(data) {
			free(data);
			data=NULL;
		}
	}

	bool isSendVideo() {
		return bProcCmmd==false;
	}
	void setToVideo() {
		bProcCmmd	= false;
		hasProcLen 	= 0;
		totalLen 	= dataLen;
	}
};

struct tagRealSendBuffer {
	char cmmd[CMD_LEN];
	bool bProcCmmd;
	int  hasProcLen;
	int  totalLen;	//1500 is cmd len
	int  dataLen;
	char*data;

	tagRealSendBuffer() {
		data 		= NULL;
		totalLen 	= 0;
		dataLen 	= 0;
		hasProcLen 	= 0;
		bProcCmmd 	= true;
	}

	void reset() {
		//std::lock_guard<std::mutex> lk(mut);
		memset(cmmd, 0, CMD_LEN);
		if(data) {
		}
		hasProcLen 	= 0;
		totalLen 	= 0;
		dataLen		= 0;
		bProcCmmd 	= true;
	}

	bool isSendVideo() {
		return bProcCmmd==false;
	}

	void setToVideo() {
		bProcCmmd	= false;
		hasProcLen 	= 0;
		totalLen 	= dataLen;
	}
};

struct tagNALSendBuffer {
	char cmmd[CMD_LEN];
	bool bProcCmmd;
	int  hasProcLen;
	int  totalLen;//1500 is cmd len
	int  dataLen;
	NALU_t 	*data;

	tagNALSendBuffer() {
		data 		= NULL;
		totalLen 	= 0;
		dataLen 	= 0;
		hasProcLen 	= 0;
		bProcCmmd 	= true;
	}

	void reset() {
		//std::lock_guard<std::mutex> lk(mut);
		memset(cmmd, 0, CMD_LEN);
		if(data) {
		}
		hasProcLen 	= 0;
		totalLen 	= 0;
		dataLen		= 0;
		bProcCmmd 	= true;
	}

	void createMem(int len) {
		if(data==NULL)
			data  = AllocNALU(len);
	}

	void releaseMem() {
		if(data) {
			FreeNALU(data);
			data=NULL;
		}
	}

	bool isSendVideo() {
		return bProcCmmd==false;
	}
	void setToVideo() {
		bProcCmmd	= false;
		hasProcLen 	= 0;
		totalLen 	= dataLen;
	}
};

struct tagFileSendBuffer {

	bool bProcCmmd;
	int  hasProcLen;
	int  totalLen;//1500 is cmd len
	char cmmd[CMD_LEN];
	int  dataLen;
	char *data;

	tagFileSendBuffer() {
		data 		= NULL;
		totalLen 	= 0;
		dataLen 	= 0;
		hasProcLen 	= 0;
		bProcCmmd 	= true;
	}

	void reset() {
		//std::lock_guard<std::mutex> lk(mut);
		memset(cmmd, 0, CMD_LEN);
		if(data) {

		}
		hasProcLen 	= 0;
		totalLen 	= 0;
		dataLen		= 0;
		bProcCmmd 	= true;
	}

	void createMem(int len) {
		if(data==NULL)
			data = (char*)malloc(len);
	}

	void releaseMem() {
		if(data) {
			free(data);
			data=NULL;
		}
	}

	bool isSendVideo() {
		return bProcCmmd==false;
	}
	void setToVideo() {
		bProcCmmd	= false;
		hasProcLen 	= 0;
		totalLen 	= dataLen;
	}
};


#ifdef HAV_FFMPEG
//send data struct
struct tagSendBuffer {
	bool bSendCmd;
	int  hasSendLen;
	int  totalLen;
	AVPacket avpack;
	char cmd[1500];
	mutable std::mutex mut;

	tagSendBuffer() {
		memset(&avpack, 0, sizeof(AVPacket));
	}

	void reset() {
		std::lock_guard<std::mutex> lk(mut);
		if(bSendCmd) memset(cmd, 0, 1500);
		hasSendLen 	= 0;
		totalLen 	= 0;
		//av_free_packet(&avpack);
		if(avpack.size>0)
			av_packet_unref(&avpack);
		memset(&avpack, 0, sizeof(AVPacket));
		avpack.size = 0;
		bSendCmd 	= true;
	}

	bool isSendVideo() {
		return avpack.size>0;
	}

	void setToVideo() {
		bSendCmd	= false;
		hasSendLen 	= 0;
		totalLen 	= avpack.size;
		memset(cmd,0, 1500);
	}
};
#endif
*/
//extern FILE *bits;


#endif
