#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/ip.h>

#include <net/netfilter/nf_conntrack.h>

#include <linux/nos_track.h>
#include <ntrack_log.h>
#include <ntrack_msg.h>

#define DRV_VERSION	"0.1.1"
#define DRV_DESC	"ntrack system driver"

static unsigned int ntrack_hook_fw(const struct nf_hook_ops *ops, 
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out, 
		int (*okfn)(struct sk_buff *))
{
	struct nf_conn *ct;
	struct iphdr *iph;
	// struct sk_buff *linear_skb = NULL, *use_skb = NULL;
	enum ip_conntrack_info ctinfo;

	// unsigned int res = NF_ACCEPT;
	// struct nos_track* nos;

	ct = nf_ct_get(skb, &ctinfo);
	if (!ct) {
		// nt_debug("null ct.\n");
		return NF_ACCEPT;
	}

	if(nf_ct_is_untracked(ct)) {
		// nt_debug("--------------- untracked ct.\n");
		return NF_ACCEPT;
	}

	//TCP, UDP, ICMP, supported.
	iph = ip_hdr(skb);
	if ((iph->protocol != IPPROTO_TCP) 
		&& (iph->protocol != IPPROTO_UDP) 
		&& (iph->protocol != IPPROTO_ICMP)) {
		return NF_ACCEPT;
	}

	//loopback, lbcast filter.
	if (ipv4_is_lbcast(iph->saddr) || 
		ipv4_is_lbcast(iph->daddr) ||
			ipv4_is_loopback(iph->saddr) || 
			ipv4_is_loopback(iph->daddr) ||
			ipv4_is_multicast(iph->saddr) ||
			ipv4_is_multicast(iph->daddr) || 
			ipv4_is_zeronet(iph->saddr) ||
			ipv4_is_zeronet(iph->daddr))
	{
		return NF_ACCEPT;
	}

	/* test */
	nmsg_hdr_t hdr;
	hdr.type = EN_MSG_T_PCAP;
	hdr.len = skb->len;
	if(nmsg_enqueue(&hdr, skb->data, skb->len, iph->saddr)) {
		nt_debug("skb cap failed.\n");
	}

	return NF_ACCEPT;
}

static struct nf_hook_ops ntrack_nf_hook_ops[] = {
	{
		.hook = ntrack_hook_fw,
		.owner = THIS_MODULE,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_FORWARD,
		.priority = NF_IP_PRI_LAST,
	},
};

void *ntrack_klog_fd = NULL;
static int __init ntrack_modules_init(void)
{
	int ret = 0;

	ntrack_klog_fd = klog_init("ntrack", 0x0e, 0);
	if(!ntrack_klog_fd) {
		return -ENOMEM;
	}

	nt_info("ntrack cap init.\n");
	ret = nmsg_init();
	if(ret) {
		goto __err;
	}

	nt_info("init nf hooks.\n");
	ret = nf_register_hooks(ntrack_nf_hook_ops, ARRAY_SIZE(ntrack_nf_hook_ops));
	if (ret) {
		goto __err;
	}

	return 0;

__err:
	nmsg_cleanup();
	if(ntrack_klog_fd) {
		klog_fini(ntrack_klog_fd);
	}
	return ret;
}

static void __exit ntrack_modules_exit(void)
{
	nt_info("module cleanup.\n");

	nf_unregister_hooks(ntrack_nf_hook_ops, ARRAY_SIZE(ntrack_nf_hook_ops));
	nmsg_cleanup();
	klog_fini(ntrack_klog_fd);
	return;
}

module_init(ntrack_modules_init);
module_exit(ntrack_modules_exit);

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("roy ppp <ROY@gmail.com>");
MODULE_LICENSE("GPL");