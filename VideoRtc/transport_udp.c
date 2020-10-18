

#include "transport_udp.h"

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "rtp.h"
#include "glog.h"
#include "utils.h"

#ifdef __ANDROID__
#include "mediacodec.h"
#endif

void vid_sendto_exit_deal(int arg)
{	
	 //log_debug("sendto thread quit:%p", pthread_self());
	pthread_exit(0);
}

void vid_recv_from_exit_deal(int arg)
{	
	//log_debug("recvfrom thread quit:%p", pthread_self());
	pthread_exit(0);
}

static void worker_rtp_sendto(void *arg)
{	
	struct transport_udp *udp;
	ssize_t status;
	rtp_sendto_thread_list_node  *rtp_node = NULL;
	//pj_fd_set_t  send_set;
	size_t	send_len = 0;
	sigset_t all_mask, new_mask;
	struct sigaction new_act;
	struct timeval tv_s; //tv_e, tv_f;
	unsigned short *seq;
	unsigned int loop_cnt = 0;
	
	
	// int max = pj_thread_get_prio_max(pj_thread_this());
	// if (max > 0) 
	//     pj_thread_set_prio(pj_thread_this(), max);
	// log_debug("enter to worker_rtp_sendto thread, prior:%d", max);


	 sigprocmask(SIG_BLOCK, NULL , &all_mask);
	 new_mask = all_mask;
	 if(1==sigismember(&all_mask, SIGUSR1))
	 {
		sigdelset(&new_mask , SIGUSR1);
		//log_debug("worker_rtp_sendto sigismember SIGUSR1");
	 }
	 else
	 {
		sigaddset(&all_mask , SIGUSR1);
		 //log_debug("worker_rtp_sendto all_mask add  SIGUSR1");
	 }

	 if(0==sigismember(&new_mask, SIGUSR1))
	 {
	 	 //log_debug("worker_rtp_sendto sigismember new_mask no SIGUSR1");
	 }
	 
	memset(&new_act, 0, sizeof(new_act));	
	new_act.sa_handler = vid_sendto_exit_deal;	
	sigemptyset( &new_act.sa_mask);	
	sigaction(SIGUSR1, &new_act, NULL);


	udp = (struct transport_udp*)arg;


	unsigned optVal = 0;
	int optLen = sizeof(int);    
    unsigned nSendBuf = 2048*1024;
    unsigned nRecvBuf = 2048*1024;
	
	setsockopt(udp->rtp_sock,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
	setsockopt(udp->rtp_sock,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
	
	getsockopt(udp->rtp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, &optLen);
	//log_debug("socket send buffer size:%d", optVal);
	//getsockopt(udp->rtp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optVal, &optLen);
	log_debug("socket recv buffer size:%d", optVal);


	log_error("begin to start worker_rtp_sendto thread rtp_sock:%d", udp->rtp_sock);
	while(udp->rtp_sock != PJ_INVALID_SOCKET && !udp->vid_rtp_sendto_thread.thread_quit)
	{
		rtp_node = udp->rtp_thread_list_header.list_current_send;
		if(!rtp_node)
		{
			usleep(1000);
			loop_cnt++;
			if(!(loop_cnt%1000))
			{
				log_error("empty loop over 1000 times flag.");
			}
			continue;
		}
		else
		{
			loop_cnt = 0;
		}
		
	    gettimeofday(&tv_s, NULL);
		
		send_len = rtp_node->rtp_buf_size;
		if(send_len != 0)
		{
			//mutex_lock(udp->udp_socket_mutex);
            //sendto(int, const void *, size_t,int, const struct sockaddr *, socklen_t) __DARWIN_ALIAS_C(sendto);
			status = sendto(udp->rtp_sock, rtp_node->rtp_buf, send_len, 0, (const struct sockaddr *)&udp->rem_rtp_addr ,udp->addr_len);
			//mutex_unlock(udp->udp_socket_mutex);
			if(status > 0)
			{
				//gettimeofday(&tv_e, NULL);
				/*if(((tv_e.sec - tv_s.sec)*1000 + (tv_e.msec - tv_s.msec)) >= 10)*/
				//{
					//seq =pj_ntohs(*(unsigned short*)(&rtp_node->rtp_buf[2]));
					// log_debug("send rtp packet:[%u] sucesss, rtp seq:[%u], marker:[%d], size:[%d], expired:[%d]ms", 
					// 		udp->rtp_thread_list_header.list_send_size, pj_ntohs(*seq),  (rtp_node->rtp_buf[1]&0x80)?1:0,
					// 		status, (tv_e.sec - tv_s.sec)*1000 + (tv_e.msec - tv_s.msec));
				//}

				usleep(50);

				seq = (unsigned short *)(&rtp_node->rtp_buf[2]);
				#ifdef __ANDROID__
                    MediaCodec_LOGE("sendto status:%d seq:%d send length:%d\n", status, pj_ntohs(*seq), send_len);
                #else
                    log_error("sendto status:%d seq:%d send length:%d\n", status, pj_ntohs(*seq), send_len);
                #endif

//				pthread_mutex_lock(&udp->rtp_cache_mutex);
//				packet_list_node_offset(&udp->rtp_thread_list_header);
//				pthread_mutex_unlock(&udp->rtp_cache_mutex);
			}
			else
			{
				seq =  (unsigned short *)(&rtp_node->rtp_buf[2]);			
				log_error("send rtp packet:[%u] failure, send_len:%d rtp seq:[%u], errno:[%d] errstr:%s", udp->rtp_thread_list_header.list_send_size,  send_len, pj_ntohs(*seq), errno, strerror(errno));
				usleep(5000);	
			}
			 pthread_mutex_lock(&udp->rtp_cache_mutex);
			 packet_list_node_offset(&udp->rtp_thread_list_header);
			 pthread_mutex_unlock(&udp->rtp_cache_mutex);
		}

		// while(udp->rtp_sock != PJ_INVALID_SOCKET && !udp->vid_rtp_sendto_thread.thread_quit 
		// 	&& !udp->rtp_thread_list_header.list_current_send && !udp->rtp_thread_list_header.list_current_send->next)
		// {
		// 	usleep(5000);
		// }
		
		//sigprocmask(SIG_SETMASK, &new_mask , NULL);			
		// pthread_mutex_lock(&udp->rtp_cache_mutex);
		// packet_list_node_offset(&udp->rtp_thread_list_header);
		// pthread_mutex_unlock(&udp->rtp_cache_mutex);
		//sigprocmask(SIG_SETMASK, &all_mask , NULL);	
	}
	log_error("worker_rtp_sendto thread exit threadid:%ld", udp->vid_rtp_sendto_thread.threadId);

	// log_debug("thread end");
	return;
}

static void worker_rtp_recvfrom(void *arg)
{
	    struct transport_udp *udp;
	    pj_status_t status;
	    pj_ssize_t  bytes_read = 0;
	    pj_uint32_t  value = 0;

	    fd_set  recv_set; /* add by j33783 20190621 */
	    struct timeval timeout;
 
	   struct sigaction new_act;

	   //log_debug("thread starting.....");
	     
	   memset(&new_act, 0, sizeof(new_act));
	   
	   new_act.sa_handler = vid_recv_from_exit_deal;

	   sigemptyset( &new_act.sa_mask);
	   
	   sigaction(SIGUSR2, &new_act, NULL);
   
	   
	    udp = (struct transport_udp *) arg;

	   //log_debug("udp:%0x, rtp_cb:%0x", udp, udp->rtp_cb);
		
	   log_error("running recv_from thread %ld begin.",pthread_self());  /* modify by j33783 20190509 */


	   if (ioctl(udp->rtp_sock, FIONBIO, &value)) 
	   {
	    	//status  = pj_get_netos_error();
		  	return ;
	   }
	   
		void (*cb)(void*,void*,pj_ssize_t);
		void *user_data;
		pj_bool_t discard = PJ_FALSE;

		cb = udp->rtp_cb;
		user_data = udp->user_data;

	   while(udp->rtp_sock != PJ_INVALID_SOCKET && !udp->vid_rtp_recv_thread.thread_quit)  /* modify by j33783 20190509 */
	   {
            /* add by j33783 20190621 begin */
            timeout.tv_sec  = 0;
            timeout.tv_usec = 100000;//100ms
            FD_ZERO(&recv_set);
            FD_SET(udp->rtp_sock, &recv_set);
			FD_SET(udp->rtcp_sock, &recv_set);

            status = select(udp->rtp_sock + 1, &recv_set, NULL, NULL, &timeout);

            if(0 == status)
            {
                usleep(10000);//10ms
                continue;
            }
            else if(0 > status)
            {
                //log_error("thread will exit, pj_sock_select return:%d", status);
                break;
            }
            else
            {
                bytes_read = RTP_LEN;
                status = recvfrom(udp->rtp_sock,  udp->rtp_pkt, bytes_read, 0,
                                (struct sockaddr *)&udp->rem_rtp_addr, &udp->addr_len);
				//log_error("recvfrom status:%d", status);
                // if (status != PJ_EPENDING && status != PJ_SUCCESS)
                // {
                //     log_error("pj_sock_recvfrom status code:%d", status);
                //         bytes_read = -status;
                // }

                // if(status  == 120009 || pj_get_native_netos_error() == 9)
                // {
                //     log_error("thread will exit, pj_sock_recvfrom return:%d", status);
                //     break;
                // }

                if(0 == status)
                {
                    log_error("thread will exit, pj_sock_recvfrom receive socket close");
                    break;
                }
            }

            if (!discard && udp->attached && cb)  /* 20190328 vid_stream.c on_rx_rtp */   /* add by j33783 20190509 */
            {
                (*cb)(user_data, udp->rtp_pkt, status);
                //log_warn("recvfrom H264__ size:%d ", status);
            }
            else
            {
                //log_debug("skip a loop, discard:%d, attached:%d, cb:%p, bytes:%lld", discard, udp->attached, cb, bytes_read);
            }
            /* add by j33783 20190621 end */
	    }
		log_error("running worker_rtp_recvfrom thread exit threadid:%ld", udp->vid_rtp_recv_thread.threadId);

	 return ;
}

static void worker_rtcp_recvfrom(void *arg)
{
	    pj_status_t status;
	    pj_ssize_t  bytes_read = 0;
	    pj_uint32_t  value = 0;

	    fd_set  recv_set; /* add by j33783 20190621 */
	    struct timeval timeout;

		struct transport_udp *udp = (struct transport_udp *) arg;
   

	   //log_debug("udp:%0x, rtp_cb:%0x", udp, udp->rtp_cb);
		
	   log_error("running worker_rtcp_recvfrom thread %ld begin.",pthread_self());


	   if (ioctl(udp->rtcp_sock, FIONBIO, &value)) 
	   {
	    	//status  = pj_get_netos_error();
		  	return ;
	   }
	   
		void (*cb)(void*,void*,pj_ssize_t);
		void *user_data;
		pj_bool_t discard = PJ_FALSE;

		cb = udp->rtcp_cb;
		user_data = udp->user_data;

	   while(udp->rtcp_sock != PJ_INVALID_SOCKET && !udp->vid_rtcp_recv_thread.thread_quit)  /* modify by j33783 20190509 */
	   {
            /* add by j33783 20190621 begin */
            timeout.tv_sec  = 0;
            timeout.tv_usec = 100000;//100ms
            FD_ZERO(&recv_set);
            FD_SET(udp->rtcp_sock, &recv_set);

            status = select(udp->rtcp_sock + 1, &recv_set, NULL, NULL, &timeout);

            if(0 == status)
            {
                usleep(10000);//10ms
                continue;
            }
            else if(0 > status)
            {
                //log_error("thread will exit, pj_sock_select return:%d", status);
                break;
            }
            else
            {
                bytes_read = RTCP_LEN;
                status = recvfrom(udp->rtcp_sock,  udp->rtcp_pkt, bytes_read, 0,
                                (struct sockaddr *)&udp->rem_rtcp_addr, &udp->addr_len);

                if(0 == status)
                {
                    log_error("thread will exit, pj_sock_recvfrom receive socket close");
                    break;
                }
				log_error("rtcp recvfrom length:%d\n", status);
            }

            if (!discard && udp->attached && cb)  /* 20190328 vid_stream.c on_rx_rtp */   /* add by j33783 20190509 */
            {
                (*cb)(user_data, udp->rtcp_pkt, status);
                //log_warn("recvfrom H264__ size:%d ", status);
            }
            else
            {
                //log_debug("skip a loop, discard:%d, attached:%d, cb:%p, bytes:%lld", discard, udp->attached, cb, bytes_read);
            }
	    }
		log_error("running worker_rtp_recvfrom thread exit threadid:%ld", udp->vid_rtcp_recv_thread.threadId);

	 return ;
}

pjmedia_vid_buf *create_resend_buf() {
	pjmedia_vid_buf *pVidbuf = (pjmedia_vid_buf*)malloc(sizeof(pjmedia_vid_buf));
	pVidbuf->buf_size = RESEND_BUFF_NUMBER;//the number of resend package number
	pVidbuf->buf = malloc(RESEND_BUFF_NUMBER*PJMEDIA_MAX_MTU);
	pVidbuf->pkt_len = malloc(RESEND_BUFF_NUMBER*sizeof(pj_uint16_t));

	return pVidbuf;
}

void release_resend_buf(pjmedia_vid_buf*resend_buf) {
	if(resend_buf) {
		free(resend_buf->pkt_len);
		resend_buf->pkt_len = NULL;

		free(resend_buf->buf);
		resend_buf->buf = NULL;

		free(resend_buf);
		resend_buf = NULL;
	}
}

void resend_save_rtp( pjmedia_vid_buf *vidBuf, pj_uint16_t extSeq, char*sendBuf, pj_uint16_t pkt_len){
	if(!vidBuf || !sendBuf)
		return ;
	pj_uint16_t index;  
	index = extSeq % vidBuf->buf_size;
	pj_memcpy(vidBuf->buf + index * PJMEDIA_MAX_MTU, sendBuf, pkt_len);//pkt_len is smaller than PJMEDIA_MAX_MTU
	vidBuf->pkt_len[index] = pkt_len;
}

pj_status_t transport_udp_create( struct transport_udp** tpout, const char *localAddr, unsigned short rtpPort,
									void (*rtp_cb)(void*, void*, pj_ssize_t),
				                    void (*rtcp_cb)(void*, void*, pj_ssize_t))
{
    pj_status_t status = 0;
	int rtp_sock,rtcp_sock; 
	int optVal = 1;

    struct transport_udp* tp = (struct transport_udp*)malloc(sizeof(struct transport_udp));

	//create rtp
    rtp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	log_error("create port:%d rtpsock:%d", rtpPort, rtp_sock);
    if(rtp_sock < 0)
        return -1;

	setsockopt(rtp_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optVal, sizeof(optVal));

    struct timeval tv = {3, 0};
	setsockopt(rtp_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
	setsockopt(rtp_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(struct timeval));
    
    tp->addr_len = sizeof(struct sockaddr_in);
    memset(&tp->local_rtp_addr, 0, tp->addr_len);
    tp->local_rtp_addr.sin_family = AF_INET;
    tp->local_rtp_addr.sin_port = htons(rtpPort);
    tp->local_rtp_addr.sin_addr.s_addr = inet_addr(localAddr);//htonl(INADDR_ANY);
	
    status = bind(rtp_sock, (struct sockaddr *)&tp->local_rtp_addr, sizeof(struct sockaddr_in));
	if(status < 0)
    	log_error("bind rtp failed.\n");
		
	//create rtcp 
    rtcp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	log_error("create rtcp_sock:%d", rtcp_sock);
    if(rtcp_sock<0)
        return -1;
	optVal = 1;
	setsockopt(rtcp_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optVal, sizeof(optVal));
    
    tp->addr_len = sizeof(struct sockaddr_in);
    memset(&tp->local_rtcp_addr, 0, tp->addr_len);
    tp->local_rtcp_addr.sin_family 	= AF_INET;
    tp->local_rtcp_addr.sin_port 	= htons(rtpPort+1);
    tp->local_rtcp_addr.sin_addr.s_addr = inet_addr(localAddr);//htonl(INADDR_ANY);
    
    status = bind(rtcp_sock, (struct sockaddr *)&tp->local_rtcp_addr, sizeof(struct sockaddr_in));
	if(status < 0)
        log_error("bind rtcp failed.\n");

	//parameter and data
	tp->rtp_sock  	= rtp_sock;
	tp->rtcp_sock 	= rtcp_sock;
	tp->rtp_cb  	= rtp_cb;
	tp->rtcp_cb 	= rtcp_cb;

	//packet list create
	packet_list_create(&tp->rtp_thread_list_header);
	tp->rtp_thread_list_header.pack_type = H264_PACKET;

	//create resend
	tp->resend = create_resend_buf();

	//mutex lock initiate
    pthread_mutex_init(&tp->rtp_cache_mutex, NULL);
    pthread_mutex_init(&tp->udp_socket_mutex, NULL);

	*tpout = tp;

	log_error("transport_udp_create done rtp_sock:%d.\n", tp->rtp_sock);

    return status;
}

pj_status_t transport_udp_destroy( struct transport_udp* tp) {
    pj_status_t status = 0;

	if(tp->rtp_sock != PJ_INVALID_SOCKET) {
		close(tp->rtp_sock);
		tp->rtp_sock = PJ_INVALID_SOCKET;
    }

	if(tp->rtcp_sock != PJ_INVALID_SOCKET) {
		close(tp->rtcp_sock);
		tp->rtcp_sock = PJ_INVALID_SOCKET;
    }

	packet_list_destroy(&tp->rtp_thread_list_header);

	//release resend
	release_resend_buf(tp->resend);

	pthread_mutex_destroy(&tp->rtp_cache_mutex);
    pthread_mutex_destroy(&tp->udp_socket_mutex);

	free(tp);
	tp = NULL;

    return status;
}

pj_status_t transport_udp_start( struct transport_udp* tp, const char*remoteAddr, unsigned short remoteRtpPort) {
    pj_status_t status = 0;
	memset(&tp->rem_rtp_addr, 0, tp->addr_len);
    tp->rem_rtp_addr.sin_family = AF_INET;
    tp->rem_rtp_addr.sin_port   = htons(remoteRtpPort);
    tp->rem_rtp_addr.sin_addr.s_addr = inet_addr(remoteAddr);
	
	memset(&tp->rem_rtcp_addr, 0, tp->addr_len);
    tp->rem_rtcp_addr.sin_family = AF_INET;
    tp->rem_rtcp_addr.sin_port   = htons(remoteRtpPort+1);
    tp->rem_rtcp_addr.sin_addr.s_addr = inet_addr(remoteAddr);

	memset(&tp->vid_rtp_recv_thread, 0, sizeof(pj_thread_t));
	memset(&tp->vid_rtp_sendto_thread, 0, sizeof(pj_thread_t));
	memset(&tp->vid_rtcp_recv_thread, 0, sizeof(pj_thread_t));

    //int pj_thread_create( const char *thread_name, thread_proc *proc, void *arg, pj_size_t stack_size, unsigned flags, pj_thread_t *pthread_out);
	pj_thread_create("rtp_recv", (thread_proc *)&worker_rtp_recvfrom, tp, 0, 0, &tp->vid_rtp_recv_thread);
	pj_thread_create("rtp_send", (thread_proc *)&worker_rtp_sendto, tp, 0, 0, &tp->vid_rtp_sendto_thread);
	pj_thread_create("rtcp_recv", (thread_proc *)&worker_rtcp_recvfrom, tp, 0, 0, &tp->vid_rtcp_recv_thread);
    if (status != 0) {
		return status;
    }

	tp->attached = PJ_TRUE;

    return status;
}

pj_status_t transport_udp_stop( struct transport_udp* tp) {
    pj_status_t status = 0;
	tp->vid_rtp_sendto_thread.thread_quit 	= 1;
	tp->vid_rtp_recv_thread.thread_quit 	= 1;
	tp->vid_rtcp_recv_thread.thread_quit 	= 1;
    return status;
}

pj_status_t transport_send_rtp( struct transport_udp*tp, const void *rtpPacket, pj_size_t size) {
    pj_status_t status = 0;
	//pjmedia_rtp_hdr *head = (pjmedia_rtp_hdr *)rtpPacket;
	if(packet_list_check_overflow(tp->rtp_thread_list_header.list_send_size, tp->rtp_thread_list_header.list_write_size, RTP_LIST_MAX_SIZE))
	{
		packet_list_reset(&tp->rtp_thread_list_header);
		log_debug("---reset---\n");
	}
	//printf("send_rtp ext:%d size:%d\n", head->x, tp->rtp_thread_list_header.list_write_size);

	// if(head->x) {
	// 	int curPos = sizeof(pjmedia_rtp_hdr);
	// 	pjmedia_rtp_ext_hdr *ext_hdr_data = (pjmedia_rtp_ext_hdr*)(pkt+curPos);
	// 	curPos += sizeof(pjmedia_rtp_ext_hdr);
	// 	ext_element_hdr *element_hdr = (ext_element_hdr*)(pkt+curPos);
	// 	curPos += sizeof(ext_element_hdr);
	// 	log_debug("profile:%04x len:%d local_id:%02x local_len:%d value:%02x\n", 
	// 				htons(ext_hdr_data->profile_data), htons(ext_hdr_data->length), element_hdr->local_id, element_hdr->local_len, *(char*)(pkt+curPos));
	// }
	pthread_mutex_lock(&tp->rtp_cache_mutex);
	packet_list_node_add(&tp->rtp_thread_list_header, rtpPacket, size);
	pthread_mutex_unlock(&tp->rtp_cache_mutex);

    return status;
}

static pj_status_t transport_priority_send_rtp( transport_udp *udp,
							  const void *pkt,
							  pj_size_t size)
{
    //struct transport_udp *udp = (struct transport_udp*)tp;
	ssize_t status = 0;

	pthread_mutex_lock(&udp->udp_socket_mutex);
	log_debug("send_priority add the packet");
	status = sendto(udp->rtp_sock, pkt, size, 0, (const struct sockaddr *)&udp->rem_rtp_addr ,udp->addr_len);
	log_debug("send_priority add the packet status:%d",status);
	pthread_mutex_unlock(&udp->udp_socket_mutex);
	
	return PJ_SUCCESS;	
}

pj_status_t transport_send_rtp_seq(struct transport_udp*tp, const void *rtpPacket, pj_size_t size, unsigned short extSeq) {
    pj_status_t status = 0;
	//pjmedia_rtp_hdr *head = (pjmedia_rtp_hdr *)rtpPacket;
	if(packet_list_check_overflow(tp->rtp_thread_list_header.list_send_size, tp->rtp_thread_list_header.list_write_size, RTP_LIST_MAX_SIZE))
	{
		packet_list_reset(&tp->rtp_thread_list_header);
		log_debug("---reset---\n");
	}
	pthread_mutex_lock(&tp->rtp_cache_mutex);
	packet_list_node_add(&tp->rtp_thread_list_header, rtpPacket, size);
	resend_save_rtp( tp->resend, extSeq, (char*)rtpPacket, size);
	pthread_mutex_unlock(&tp->rtp_cache_mutex);

    return status;
}

pj_status_t transport_send_rtcp(struct transport_udp*tp, const void *rtpPacket, pj_size_t size)
{
    pj_status_t status = 0;
	if(tp&&rtpPacket&&(size>0)) {
		transport_udp*udp = (struct transport_udp*)tp;
		status = sendto(udp->rtcp_sock, rtpPacket, size, 0, (const struct sockaddr *)&udp->rem_rtcp_addr, udp->addr_len);
		log_error("rtcp send length:%d\n", status);
	}
    return status;
}


pj_status_t  transport_reset_socket(struct transport_udp*  tp) {
    pj_status_t status = 0;
    return status;
}

pj_status_t transport_reset_rtp_socket(struct transport_udp*  tp) {
    pj_status_t status = 0;
    return status;
}




