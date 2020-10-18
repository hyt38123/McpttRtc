#ifndef HYT_INTERFACE_C_CPLUS_DEFINE_
#define HYT_INTERFACE_C_CPLUS_DEFINE_ 

void * Launch_CPlus(unsigned int nCodecID);
int RTPUnpackH264(void *obj, char *rtpsrc, int inputlen, void **h264dst );
void CRtpPackDecoderDestroy(void *obj);

#endif

