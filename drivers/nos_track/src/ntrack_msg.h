#ifndef __NTRACK_MSG_H__
#define __NTRACK_MSG_H__

#include <linux/list.h>
#include <ntrack_rbf.h>

/* 
* ntrack message queue init/fini.
*/
int nmsg_init(void);
void nmsg_cleanup(void);

/*
* @buff 	message content.
* @buff_size 	message size.
* @key 	hash key for SMP delivery.
* @return success 0, -num failed.
*/
int nmsg_enqueue(nmsg_hdr_t *hdr, void *buff, uint32_t buff_size, uint32_t key);


#endif /* __NTRACK_MSG_H__ */
