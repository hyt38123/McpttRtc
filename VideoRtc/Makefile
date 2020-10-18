CXXFLAGS += -std=c++11
 
CC 	= gcc
CPP = g++
AR  = ar

COMPLIE_INCL = -I./ -I./include -I./packed -I./packed/pj

COMPLIE_LIB  = -L./ -lpthread -lm
CFLAGS 		= -Wall -DHAVE_CONFIG_H -DSENDTEST -g -fPIC #-DHAVE_SYS_TIME_H
CXXFLAGS += $(COMPLIE_INCL) 
LDFLAGS += $(COMPLIE_LIB)

CFLAGS  += -I./ -I./packed
CFLAGS	+= $(COMPLIE_INCL) 

TARGET:SEND RECV UNITTest

OBJECTS = NALDecoder.o vid_codec.o vid_port.o vid_stream.o transport_udp.o  \
		  packed/packets_list.o packed/jitter_buffer.o packed/callCplus.o packed/dealrtpunpack.o \
		  packed/h264_packetizer.o packed/rtp.o packed/pj/os_core_unix.o packed/pj/utils.o packed/pj/glog.o

SEND_OBJ = $(OBJECTS) \
			rtc_send.o

RECV_OBJ = $(OBJECTS) \
			rtc_recv.o

UNIT_OBJ = $(OBJECTS) \
			UnitTest.o

SEND=SEND
RECV=RECV
UNITTest=UNITTest
RTCLIB=RTCLIB

$(SEND):$(SEND_OBJ) 
	$(CC) -o $(SEND) $(SEND_OBJ) $(LDFLAGS)

$(RECV):$(RECV_OBJ) 
	$(CC) -o $(RECV) $(RECV_OBJ) $(LDFLAGS)

$(UNITTest):$(UNIT_OBJ) 
	$(CC) -o $(UNITTest) $(UNIT_OBJ) $(LDFLAGS)

$(RTCLIB):$(OBJECTS)
	$(AR) rcs librtc.a $(OBJECTS)

.PHONY:clean
clean:
	rm $(SEND) $(RECV) $(OBJECTS) *.o
