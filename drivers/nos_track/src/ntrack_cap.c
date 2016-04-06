#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <asm/smp.h>

#include <linux/nos_track.h>
#include <ntrack_rbf.h>

#include "ntrack_cap.h"

#define nt_assert(x) 	BUG_ON(!(x))
#define KEY_TO_CORE(k) 	((k) % nr_cpu_ids)
#define NCAP_MAX_COUNT 	(2048)

typedef struct {
	struct list_head list;
	spinlock_t lock;
	uint64_t count;
	struct work_struct wq_cap;
	rbf_t* rbfp;
} ncap_header_t;

static struct workqueue_struct *ncap_wq = NULL;
static ncap_header_t ncap_cpus[NR_CPUS];

static inline ncap_header_t * ncap_target_cpu(uint32_t cpu)
{
	/* FIXME: hash user node. */
	return &ncap_cpus[cpu];
}

static void ncap_wq_dequeue_func(struct work_struct *wq);

int ncap_init(void)
{
	uint32_t size, i;

	nt_assert(nos_track_cap_base != NULL);
	nt_assert((nos_track_cap_size / nr_cpu_ids) >= RBF_NODE_SIZE * 32);

	ncap_wq = alloc_workqueue("ncap", 
		WQ_FREEZABLE | WQ_HIGHPRI | WQ_CPU_INTENSIVE, nr_cpu_ids);
	if(!ncap_wq) {
		nt_error("alloc workqueue ncap.\n");
		return -ENOMEM;
	}

	/* sizeof percpu's message buffer */
	size = nos_track_cap_size / nr_cpu_ids;
	for(i=0; i<nr_cpu_ids; i++) {
		ncap_header_t *ncap = ncap_target_cpu(i);
		ncap->count = 0;
		spin_lock_init(&ncap->lock);
		INIT_LIST_HEAD(&ncap->list);
		ncap->rbfp = rbf_init((void*)(nos_track_cap_base + size * i), size);
		INIT_WORK(&ncap->wq_cap, ncap_wq_dequeue_func);
		/* test call */
		queue_work_on(i, ncap_wq, &ncap->wq_cap);
	}

	return 0;
}

void ncap_cleanup(void)
{
	if(ncap_wq) {
		destroy_workqueue(ncap_wq);
	}
}

int ncap_enqueue(void *buf_in, uint32_t size, uint32_t key)
{
	ncap_header_t *ncap_hdr = ncap_target_cpu(KEY_TO_CORE(key));
	ncap_node_t *node;

	nt_assert(ncap_hdr);
	if(ncap_hdr->count > NCAP_MAX_COUNT) {
		nt_debug("queue list full.\n");
		return -ENOMEM;
	}

	node = kmalloc(sizeof(ncap_node_t), GFP_ATOMIC);
	if(!node) {
		nt_error("not enough mem.\n");
		return -ENOMEM;
	}
	spin_lock_bh(&ncap_hdr->lock);
	ncap_hdr->count ++;
	list_add(&node->head, &ncap_hdr->list);
	spin_unlock_bh(&ncap_hdr->lock);

	nt_assert(size <= sizeof(node->buff));

	node->buff_size = size;
	memcpy(node->buff, buf_in, size);

	/* raise the wq */
	queue_work_on(KEY_TO_CORE(key), ncap_wq, &ncap_hdr->wq_cap);
	return 0;
}

static int ncap_fill_buffer(rbf_t *rbfp, ncap_node_t *node)
{
	void *p = rbf_get_buff(rbfp);
	if (!p) {
		nt_debug("message ring full...\n");
		return -ENOMEM;
	}
	memcpy(p, node->buff, node->buff_size);
	rbf_release_buff(rbfp);
	return 0;
}

static int ncap_dequeue(void)
{
	ncap_header_t *ncap_hdr = ncap_target_cpu(smp_processor_id());
	ncap_node_t *node;

	if (list_empty(&ncap_hdr->list)) {
		return -1;
	}

	spin_lock_bh(&ncap_hdr->lock);
	if ((node = list_first_entry_or_null(&ncap_hdr->list, ncap_node_t, head)) != NULL) {
		ncap_fill_buffer(ncap_hdr->rbfp, node);
		ncap_hdr->count --;
		list_del(&node->head);
		kfree(node);
	}
	spin_unlock_bh(&ncap_hdr->lock);
	return 0;
}

static void ncap_wq_dequeue_func(struct work_struct *wq)
{
	int count = 0;

	while(ncap_dequeue() == 0) {
		count ++;
	}

	if(count > 1) {
		nt_debug("[%d] - dequeue %d messages.\n", smp_processor_id(), count);
	}
}
