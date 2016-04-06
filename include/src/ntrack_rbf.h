#ifndef __NTRACK_RBF_H__
#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/module.h>

#else /* end kernel */

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#endif /* __KERNEL__ */

#include <ntrack_log.h>

// #define RBF_IDX_SIZE 	(1024)
#define RBF_NODE_SIZE	(1024 * 2)

typedef struct ringbuffer_header {
	/* idx read, write record the idx[N] */
	uint16_t r,w;

	uint32_t size;	//buffer size;
	uint16_t count;	//node count;
	uint16_t pad;
} rbf_hdr_t;

typedef struct ringbuffer {
	rbf_hdr_t hdr;

	uint8_t buffer[0];
} rbf_t;

rbf_t* rbf_init(void *mem, uint32_t size)
{
	rbf_t *rbp = (rbf_t*)mem;

	memset(rbp, 0, sizeof(rbf_t));
	rbp->hdr.size = size - sizeof(rbf_hdr_t);
	rbp->hdr.count = rbp->hdr.size / RBF_NODE_SIZE;

	nt_info("mem: %d, node: %d-%d\n", mem, size, rbp->hdr.count);

	return rbp;
}

void *rbf_get_buff(rbf_t* rbp)
{
	uint16_t idx = (rbp->hdr.w + 1) % rbp->hdr.count;

	/* overflow ? */
	if (idx != rbp->hdr.r) {
		return (void *)&rbp->buffer[RBF_NODE_SIZE * rbp->hdr.w];
	}

	return NULL;
}

void rbf_release_buff(rbf_t* rbp)
{
	rbp->hdr.w = (rbp->hdr.w + 1) % rbp->hdr.count;
}

void *rbf_get_data(rbf_t *rbp)
{
	uint16_t idx = rbp->hdr.r;

	if(idx != rbp->hdr.w) {
		return (void*)&rbp->buffer[RBF_NODE_SIZE * idx];
	}

	return NULL;
}

void rbf_release_data(rbf_t *rbp)
{
	rbp->hdr.r = (rbp->hdr.r + 1) % rbp->hdr.count;
}

#endif //__NTRACK_RBF_H__
