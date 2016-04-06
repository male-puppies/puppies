#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/smp.h>

#include <linux/nos_track.h>

#include <ntrack_rbf.h>

#define nt_assert(x) BUG_ON(!(x))

rbf_t* nt_rbf[NR_CPUS];

// extern void* 	nos_track_cap_base;
// extern uint32_t 	nos_track_cap_size;

int ntrack_init(void)
{
	rbf_t* rbfp;
	uint32_t size, i;

	nt_assert(nos_track_cap_base != NULL);
	nt_assert((nos_track_cap_size / nr_cpu_ids) >= RBF_NODE_SIZE * 32);

	/* sizeof percpu's message buffer */
	size = nos_track_cap_size / nr_cpu_ids;
	for(i=0; i<nr_cpu_ids; i++) {
		rbfp = rbf_init((void*)(nos_track_cap_base + size * i), size);
		nt_rbf[i] = rbfp;
	}

	return 0;
}