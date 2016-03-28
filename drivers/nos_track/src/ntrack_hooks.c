#include <linux/module.h>
#include <linux/nos_track.h>

#define DRV_VERSION	"0.1.1"
#define DRV_DESC	"ntrack system driver"

static int __init ntrack_modules_init(void)
{
	return 0;
}

static void __exit ntrack_modules_exit(void)
{
	return;
}

module_init(ntrack_modules_init);
module_exit(ntrack_modules_exit);

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("roy ppp <ROY@gmail.com>");
MODULE_LICENSE("GPL V2");