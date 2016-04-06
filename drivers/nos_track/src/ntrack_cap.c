#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <asm/smp.h>

#include <linux/nos_track.h>
#include <ntrack_rbf.h>

#include "ntrack_cap.h"

#define nt_assert(x) BUG_ON(!(x))
#define KEY_TO_CORE(k) ((k) % NR_CPUS)

typedef struct {
	struct list_head list;
	spinlock_t lock;
	struct work_struct wq_cap;
	rbf_t* rbfp;
} ncap_header_t;

static struct workqueue_struct *ncap_wq = NULL;
static ncap_header_t ncap_cpus[NR_CPUS];

// extern void* 	nos_track_cap_base;
// extern uint32_t 	nos_track_cap_size;

static inline ncap_header_t * ncap_target_cpu(uint32_t key)
{
	/* FIXME: hash user node. */
	return &ncap_cpus[KEY_TO_CORE(key)];
}

static void ncap_wq_dequeue_func(struct work_struct *wq);

int ncap_init(void)
{
	uint32_t size, i;

	nt_assert(nos_track_cap_base != NULL);
	nt_assert((nos_track_cap_size / nr_cpu_ids) >= RBF_NODE_SIZE * 32);

	ncap_wq = alloc_workqueue("ncap", WQ_HIGHPRI | WQ_CPU_INTENSIVE, NR_CPUS);
	if(!ncap_wq) {
		nt_error("alloc workqueue ncap.\n");
		return -ENOMEM;
	}

	/* sizeof percpu's message buffer */
	size = nos_track_cap_size / nr_cpu_ids;
	for(i=0; i<nr_cpu_ids; i++) {
		ncap_header_t *ncap = ncap_target_cpu(i);
		spin_lock_init(&ncap->lock);
		INIT_LIST_HEAD(&ncap->list);
		ncap->rbfp = rbf_init((void*)(nos_track_cap_base + size * i), size);
		INIT_WORK(&ncap->wq_cap, ncap_wq_dequeue_func);
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
	ncap_header_t *ncap_hdr = ncap_target_cpu(key);
	ncap_node_t *node;

	node = kmalloc(sizeof(ncap_node_t), GFP_ATOMIC);
	if(!node) {
		return -ENOMEM;
	}
	spin_lock_bh(&ncap_hdr->lock);
	list_add(&node->head, &ncap_hdr->list);
	spin_unlock_bh(&ncap_hdr->lock);

	nt_assert(size <= RBF_NODE_SIZE);

	node->buff_size = size;
	memcpy(node->buff, buf_in, size);

	/* raise the wq */
	queue_work_on(KEY_TO_CORE(key), ncap_wq, &ncap_hdr->wq_cap);
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
		void *p = rbf_get_buff(ncap_hdr->rbfp);
		if (!p) {
			nt_debug("no mem.\n");
			spin_unlock_bh(&ncap_hdr->lock);
			return -ENOMEM;
		}
		memcpy(p, node->buff, node->buff_size);
		rbf_release_buff(ncap_hdr->rbfp);
		list_del(&node->head);
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

	nt_debug("dequeue %d messages.\n", count);
}
