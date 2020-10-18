
#include "packets_list.h"
#include "utils.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "rtp.h"
#include "glog.h"


#ifndef pj_assert
#   define pj_assert(expr)   assert(expr)
#endif


memory_block* memory_block_create(){
	memory_block *mem_block = (memory_block*)malloc(sizeof(memory_block));
	if(NULL == mem_block)
	{
		return NULL;
	}

	//init mem_block
	mem_block->next = NULL;
	mem_block->prev = NULL;
	mem_block->data_block = NULL;

	return mem_block;
}

void* memory_list_malloc(memory_list *mem_list){
	memory_block* free_block;
	pj_size_t buf_len = PJMEDIA_MAX_VID_PAYLOAD_SIZE + sizeof(rtp_sendto_list_node);

	pj_assert(mem_list != NULL);
	free_block= mem_list->free_head;

	if (NULL == free_block)
	{
		void* mem_buff = NULL;
		memory_block* used_block = NULL;

		mem_buff = malloc(buf_len);
		if (NULL == mem_buff)
		{
			return NULL;
		}

		used_block = memory_block_create();
		if (NULL == used_block)
		{
			free(mem_buff);
			return NULL;
		}
		used_block->data_block = mem_buff;

		if (NULL == mem_list->used_last)
		{
			mem_list->used_head = used_block;
			mem_list->used_last = used_block;
		}else{
			used_block->prev = mem_list->used_last;
			mem_list->used_last->next = used_block;
			mem_list->used_last = used_block;
		}

		pj_bzero(used_block->data_block, buf_len);
		return used_block->data_block;		
	}else{
		//use the first free block in list
		mem_list->free_head = free_block->next;
		if (mem_list->free_head != NULL)
		{
			mem_list->free_head->prev = NULL;
		}

		free_block->prev = mem_list->used_last;
		free_block->next = NULL;
		if (NULL == mem_list->used_last)
		{
			mem_list->used_head = free_block;
			mem_list->used_last = free_block;
		}else{
			pj_assert(mem_list->used_last->next == NULL);

			mem_list->used_last->next = free_block;
			mem_list->used_last = free_block;
		}
		
		pj_bzero(free_block->data_block, buf_len);
		return free_block->data_block;
	}	
}

pj_bool_t memory_list_free(memory_list* mem_list, void* mem_buff){
	memory_block* used_block = mem_list->used_head;
	while (used_block != NULL)
	{
		if (mem_buff == used_block->data_block)
		{
			//used part
			if (used_block == mem_list->used_head)
			{
				pj_assert(NULL == used_block->prev);

				mem_list->used_head = used_block->next;
				if (mem_list->used_head != NULL)
				{
					mem_list->used_head->prev = NULL;
				}else{
					pj_assert(used_block == mem_list->used_last);
					
					mem_list->used_last = NULL;
				}
			}else{
				used_block->prev->next = used_block->next;
				if (used_block->next != NULL)
				{
					used_block->next->prev = used_block->prev;
				}else{
					pj_assert(used_block == mem_list->used_last);
					mem_list->used_last = used_block->prev;
				}
			}
			
			//free part
			if (NULL == mem_list->free_head)
			{
				used_block->prev = NULL;
				used_block->next = NULL;

				mem_list->free_head = used_block;
				mem_list->free_last = used_block;
			}else{
				used_block->prev = mem_list->free_last;
				used_block->next = NULL;

				mem_list->free_last->next = used_block;
				mem_list->free_last = used_block;
			}
			
			return PJ_TRUE;
		}else{
			used_block = used_block->next;
		}		
	}
	return PJ_FALSE;
}

int memory_block_list_destroy(memory_block* block_head){
	int list_len = 0;
	memory_block* temp1;
	memory_block* temp2;

	temp1 = block_head;
	while (temp1 != NULL)
	{
		list_len++;
		if (temp1->data_block != NULL)
		{
			free(temp1->data_block);
			temp1->data_block = NULL;
		}
		temp2 = temp1->next;
		free(temp1);
		temp1 = temp2;
	}
	return list_len;
}




memory_list* memory_list_create(){
	memory_list *mem_list = (memory_list*)malloc(sizeof(memory_list));
	if (NULL == mem_list)
	{
		return NULL;
	}

	//init mem_list
	mem_list->free_head = NULL;
	mem_list->free_last = NULL;
	mem_list->used_head = NULL;
	mem_list->used_last = NULL;

	return mem_list;
}

void memory_list_destroy(memory_list* mem_list){
	int used_len, free_len;
	pj_assert(mem_list != NULL);
	used_len = memory_block_list_destroy(mem_list->used_head);
	free_len = memory_block_list_destroy(mem_list->free_head);
	//log_debug("memory_list_destroy, used_len is:%d, free_len is:%d", used_len, free_len);
	mem_list->used_head = NULL;  /* add by j33783 20190509 */
	mem_list->free_head = NULL;

	free(mem_list);
	return;	
}
/*-------------------------------------------------------------thread--------------------------------------------------------------*/

pj_bool_t packet_list_create(struct  rtp_sendto_thread_list_header *list_header) {
	if(!list_header)
		return 0;
	list_header->list_write_size 	= 0;
	list_header->list_send_size 	= 0;
	list_header->list_current_send 	= NULL;
	list_header->list_current_write = NULL;

	list_header->mem_list = memory_list_create();

	return PJ_SUCCESS;
}

pj_bool_t packet_list_destroy(struct  rtp_sendto_thread_list_header *list_header) {
	if(!list_header)
		return 0;
	if (list_header->mem_list != NULL)
	{
		memory_list_destroy(list_header->mem_list);
		list_header->mem_list = NULL;
	}
	list_header->list_current_send 	= 0;
	list_header->list_current_write = 0;
	list_header->list_write_size 	= 0;
	list_header->write_loop_flg		= PJ_FALSE;
	list_header->list_send_size 	= 0;
	list_header->send_loop_flg		= PJ_FALSE;
	
	return PJ_SUCCESS;
}

pj_bool_t packet_list_reset(struct  rtp_sendto_thread_list_header *list_header) 
{


	// rtp_sendto_thread_list_node* node =  list_header->list_current_send;
	// rtp_sendto_thread_list_node* trackNode = node;
	// int idrTrack = 0, idrIndex = 0, freeCount = 0, posType = sizeof(pjmedia_rtp_hdr);
	// if(head->x) {
	// 	int curPos = sizeof(pjmedia_rtp_hdr);
	// 	pjmedia_rtp_ext_hdr *ext_hdr_data = (pjmedia_rtp_ext_hdr*)(pkt+curPos);
	// 	curPos += sizeof(pjmedia_rtp_ext_hdr);
	// 	ext_element_hdr *element_hdr = (ext_element_hdr*)(pkt+curPos);
	// 	curPos += sizeof(ext_element_hdr);
	// 	log_debug("profile:%04x len:%d local_id:%02x local_len:%d value:%02x\n", 
	// 				htons(ext_hdr_data->profile_data), htons(ext_hdr_data->length), element_hdr->local_id, element_hdr->local_len, *(char*)(pkt+curPos));
	// }


	rtp_sendto_thread_list_node* node =  list_header->list_current_send;
	int idrTrack = 0, idrIndex = 0, freeCount = 0, i=0;
	rtp_sendto_thread_list_node* trackNode = node;
	switch(list_header->pack_type) {
		case H264_PACKET:
			while(trackNode) {
				idrTrack++;
				pjmedia_rtp_hdr *head = (pjmedia_rtp_hdr *)trackNode->rtp_buf;
				int posType = sizeof(pjmedia_rtp_hdr);//behead pjmedia_rtp_ext_hdr
				if(head->x)
				{
					pjmedia_rtp_ext_hdr *ext = (pjmedia_rtp_ext_hdr*)(trackNode->rtp_buf+posType);
					posType += sizeof(pjmedia_rtp_ext_hdr);//behead ext_element_hdr
					int extLen = htons(ext->length);
					for(; i < extLen; i++)
					{
						ext_element_hdr * element = (ext_element_hdr*)(trackNode->rtp_buf+posType);
						posType += 4*ceil((sizeof(ext_element_hdr) + (double)(element->local_len))/4);
					}
					i = 0;
				}
				
				int type = trackNode->rtp_buf[posType] & 0x1F;
				if(7 == type) {//sps package
					idrIndex = idrTrack;
				}
				trackNode = trackNode->next;
			}//find last GoP position

			if(idrIndex==0)
				idrIndex = idrTrack;//drop all package if not find idr
			else
				idrIndex--;//not delete the sps package

			//should not out of the list size
			idrIndex = (idrIndex>RTP_LIST_MAX_SIZE)?RTP_LIST_MAX_SIZE:idrIndex;

			//printf("@@now idrIndex:%d\n", idrIndex);

			//drop package to last GoP
			while(node&&(idrIndex>0)) {
				memory_list_free(list_header->mem_list, node);
				node = node->next;
				idrIndex--;
				freeCount++;
			}
			list_header->list_current_send 	= node;
			list_header->list_write_size 	= RTP_LIST_MAX_SIZE-freeCount;
			list_header->list_send_size 	= 0;
			//printf("@@left writesize:%d type:%d\n", list_header->list_write_size, node->rtp_buf[20] & 0x1F);
		break;
		case H265_PACKET:
		default:
			while(node) {
				memory_list_free(list_header->mem_list, node);
				node = node->next;
			}	
			node = (rtp_sendto_thread_list_node*)memory_list_malloc(list_header->mem_list);
			node->next = NULL;
			node->rtp_buf_size = 0;	
			list_header->list_current_send 	= node;
			list_header->list_current_write = node;
			list_header->list_write_size 	= 0;
			list_header->list_send_size 	= 0;
		break;
	}
	
	list_header->write_loop_flg		= PJ_FALSE;
	list_header->send_loop_flg		= PJ_FALSE;
	
	return PJ_SUCCESS;
}

pj_bool_t packet_list_check_overflow(pj_uint32_t send, pj_uint32_t write, pj_uint32_t bufsize)
{
	return (abs((int)(write - send)) > bufsize) ? (PJ_TRUE):(PJ_FALSE);	
}

pj_bool_t packet_list_node_add(struct  rtp_sendto_thread_list_header *list_header, const void *pkt, pj_size_t size) 
{
	if(!list_header || !pkt)
		return 0;

	/** copy the rtp pkg into the list of sendto thread  and return */
	rtp_sendto_thread_list_node *node;     /* add by j33783 20190509 */
	
	if(list_header->mem_list)    /* add by j33783 20190509 */
	{
		node = (rtp_sendto_thread_list_node*)memory_list_malloc(list_header->mem_list);
	}else
	{
		return 0;
	}
	
	
	node->rtp_buf_size = size;
	
	pj_memcpy(node->rtp_buf, pkt, size);
	node->next = NULL;
	if(list_header->list_write_size == MAX_LOOP_NUM)
	{	
		list_header->list_write_size = 0;
		list_header->write_loop_flg = PJ_TRUE;
	}else{
		list_header->list_write_size++;
		list_header->write_loop_flg = PJ_FALSE;
	} 

	/* modify by j33783 20190509 begin */
	if(list_header->list_current_send == NULL)
	{
		list_header->list_current_send = node;
	}

	if(list_header->list_current_write == NULL)
	{
		list_header->list_current_write = node;
	}
	else
	{
		list_header->list_current_write->next = node;
		list_header->list_current_write = node;
	}
	/* modify by j33783 20190509 end */
	//printf("@@packet_list_node_add size:%d\n", size);
	return PJ_SUCCESS;
}

rtp_sendto_thread_list_node  *packet_list_node_get(struct  rtp_sendto_thread_list_header *list_header) 
{
	if(list_header)
		return list_header->list_current_send;
	return NULL;
}

pj_bool_t packet_list_node_offset(struct  rtp_sendto_thread_list_header *list_header)
{
	if(!list_header)
		return 0;

	if(list_header->list_send_size == MAX_LOOP_NUM)
	{	
		list_header->list_send_size = 0;
		list_header->send_loop_flg = PJ_TRUE;
	}
	else
	{
		if(list_header->list_send_size < list_header->list_write_size)
			list_header->list_send_size++;
	}

	rtp_sendto_thread_list_node* node =  list_header->list_current_send;
	list_header->list_current_send = list_header->list_current_send->next;

	if(list_header->mem_list)
	{
		memory_list_free(list_header->mem_list, node);
		node = NULL;
	}

	return PJ_SUCCESS;
}

