#ifndef __PJMEDIA_PACKETIZER_LIST_H__
#define __PJMEDIA_PACKETIZER_LIST_H__

#include "types.h"
#include <stdlib.h>

#define RTP_LIST_MAX_SIZE   640 /* modify by j33783 20190618*/

#define MAX_LOOP_NUM 	4000000000u

typedef enum packets_type {
	H264_PACKET = 1,
	H265_PACKET = 2
}packets_type;

typedef struct memory_block
{
	struct memory_block *next;
	struct memory_block *prev;
	void*	data_block;
} memory_block;

typedef struct memory_list
{
	memory_block *free_head;
	memory_block *free_last;

	memory_block *used_head;
	memory_block *used_last;
} memory_list;

typedef struct rtp_sendto_thread_list_node
{	
	struct rtp_sendto_thread_list_node  *next;
	pj_size_t		rtp_buf_size;
	char			rtp_buf[1];
}rtp_sendto_thread_list_node;

struct rtp_sendto_thread_list_header
{	
	pj_uint32_t  	list_write_size;
	pj_uint32_t  	list_send_size;
	pj_bool_t		send_loop_flg;
	pj_bool_t		write_loop_flg;
	packets_type    pack_type;
	memory_list 	*mem_list;
	rtp_sendto_thread_list_node 	*list_current_send;
	rtp_sendto_thread_list_node 	*list_current_write;
};

typedef struct rtp_sendto_list_node
{
	struct rtp_sendto_list_node *next;
	pj_size_t rtp_buf_size;
	char	  rtp_buf[1];
} rtp_sendto_list_node;

struct rtp_sendto_list_header
{
	rtp_sendto_list_node *list_current_send;
	rtp_sendto_list_node *list_current_write;
};

memory_block* memory_block_create();
void* memory_list_malloc(memory_list *mem_list);
pj_bool_t memory_list_free(memory_list* mem_list, void* mem_buff);
int memory_block_list_destroy(memory_block* block_head);
memory_list* memory_list_create();
void memory_list_destroy(memory_list* mem_list);


pj_bool_t packet_list_create(struct  rtp_sendto_thread_list_header *list_header);
pj_bool_t packet_list_destroy(struct  rtp_sendto_thread_list_header *list_header);
pj_bool_t packet_list_reset(struct  rtp_sendto_thread_list_header *list_header);
pj_bool_t packet_list_check_overflow(pj_uint32_t send, pj_uint32_t write, pj_uint32_t bufsize);
pj_bool_t packet_list_node_add(struct  rtp_sendto_thread_list_header *list_header, const void *pkt, pj_size_t size);
rtp_sendto_thread_list_node  *packet_list_node_get(struct  rtp_sendto_thread_list_header *list_header);
pj_bool_t packet_list_node_offset(struct  rtp_sendto_thread_list_header *list_header);

#endif
