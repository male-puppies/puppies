#include <linux/module.h>

#include <linux/nos_track.h>
#include <ntrack_log.h>
#include "ntrack_cap.h"

#define DRV_VERSION	"0.1.1"
#define DRV_DESC	"ntrack system driver"

void *ntrack_klog_fd = NULL;

static int __init ntrack_modules_init(void)
{
	int ret = 0;

	ntrack_klog_fd = klog_init("ntrack", 0x0e, 0);
	if(!ntrack_klog_fd) {
		return -ENOMEM;
	}
	nt_info("module init.\n");

	ret = ncap_init();
	if(ret) {
		goto __err;
	}

	return 0;

__err:
	ncap_cleanup();
	if(ntrack_klog_fd) {
		klog_fini(ntrack_klog_fd);
	}
	return ret;
}

static void __exit ntrack_modules_exit(void)
{
	nt_info("module cleanup.\n");

	klog_fini(ntrack_klog_fd);
	return;
}

module_init(ntrack_modules_init);
module_exit(ntrack_modules_exit);

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("roy ppp <ROY@gmail.com>");
MODULE_LICENSE("GPL");