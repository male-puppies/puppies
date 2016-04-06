#ifndef __NTRACK_CAP_H__
#define __NTRACK_CAP_H__

#include <linux/list.h>
#include <ntrack_rbf.h>

typedef struct {
	struct list_head head;

	uint32_t buff_size;
	uint8_t buff[RBF_NODE_SIZE];
} ncap_node_t;

int ncap_init(void);
void ncap_cleanup(void);
int ncap_enqueue(void *buff, uint32_t buff_size, uint32_t to_cpu);

#endif /* __NTRACK_CAP_H__ */
