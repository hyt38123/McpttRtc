
#ifndef NAL_DECODER_H_
#define NAL_DECODER_H_

#include <stdio.h>

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

NALU_t*	AllocNALU(int buffersize);
void 	FreeNALU(NALU_t *n);
FILE* 	OpenBitstreamFile (const char *filename);
void  	CloseBitstreamFile(FILE*file);
int 	GetAnnexbNALU (FILE *file, NALU_t *nalu);
void 	dump(NALU_t *n);

#endif
