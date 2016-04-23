#pragma once

#ifdef __KERNEL__
#include <linux/list.h>
#include <ntrack_rbf.h>
#else
#include <stdint.h>
#endif

/*
* ntrack message system defines.
*/
enum {
	EN_MSG_T_PCAP = 1,
	EN_MSG_T_AUTH,
};

typedef struct {
	uint16_t type;
	uint16_t prio;
	uint16_t len;
	uint16_t crc;
} nmsg_hdr_t;

#ifdef __KERNEL__
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
#else /* __KERNEL__ */

#endif /* KERNEL */

