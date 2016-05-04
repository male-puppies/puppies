#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/ip.h>

#include <linux/netfilter/xt_set.h>

#include <net/ip.h>
#include <net/netfilter/nf_conntrack.h>

#include <linux/nos_track.h>
#include <ntrack_log.h>
#include <ntrack_msg.h>
#include <ntrack_auth.h>

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
	struct nos_track* nos;

	ct = nf_ct_get(skb, &ctinfo);
	if (!ct) {
		// nt_debug("null ct.\n");
		return NF_ACCEPT;
	}

	if(nf_ct_is_untracked(ct)) {
		// nt_debug("--------------- untracked ct.\n");
		return NF_ACCEPT;
	}

	if((nos = nf_ct_get_nos(ct)) == NULL) {
		nt_debug("nos untracked.\n");
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
	memset(&hdr, 0, sizeof(hdr));
	hdr.type = EN_MSG_T_NODE;
	hdr.data_len = sizeof(struct nos_track);
	if(nmsg_enqueue(&hdr, nos, hdr.data_len, 0)) {
		nt_debug("message en_q failed.\n");
	}

	return NF_ACCEPT;
}

static unsigned int ntrack_hook_test(const struct nf_hook_ops *ops, 
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out, 
		int (*okfn)(struct sk_buff *))
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;

	ct = nf_ct_get(skb, &ctinfo);
	if (!ct) {
		// nt_debug("null ct.\n");
		return NF_ACCEPT;
	}

	if(nf_ct_is_untracked(ct)) {
		return NF_ACCEPT;
	}

	if(!ct->mark) {
		return NF_ACCEPT;
	}
	
	/* user online message */
	// nmsg_hdr_t hdr;
	// memset(&hdr, 0, sizeof(hdr));
	// hdr.type = EN_MSG_T_AUTH;
	// hdr.len = sizeof(uint32_t);
	// if(nmsg_enqueue(&hdr, &ct->mark, hdr.len, 0)) {
	// 	nt_debug("skb cap failed.\n");
	// }

	return NF_ACCEPT;
}

/* ipset hash:ip hash:mac check src address from skb. */
static int ntrack_user_match(uint32_t addr, struct sk_buff *skb)
{
	int ret = 0, i, j;
	struct ip_set_adt_opt opt;
	struct xt_action_param act;
	struct net_device *indev, *dev;
	// const struct xt_set_info *set = (const void *) em->data;

	G_AUTHCONF_t *conf = rcu_dereference(G_AuthConf);
	if(!conf) {
		return 0;
	}

	if(!conf->num_rules) {
		return 0;
	}

	act.family = NFPROTO_IPV4;
	act.thoff = ip_hdrlen(skb);
	act.hooknum = 0;

	opt.family = act.family;
	opt.dim = IPSET_DIM_THREE;
	opt.flags = IPSET_DIM_ONE_SRC;
	opt.cmdflags = 0;
	opt.ext.timeout = ~0u;

	rcu_read_lock();
	dev = skb->dev;
	if (dev && skb->skb_iif)
		indev = dev_get_by_index_rcu(dev_net(dev), skb->skb_iif);

	/* conntrack init (pre routing) */
	act.in      = indev ? indev : dev;
	act.out     = dev;

	/* find src addr */
	for (i = 0; i < conf->num_rules; ++i) {
		auth_rule_t *rule = &conf->rules[i];
		for (j = 0; j < rule->num_idx; ++j) {
			ret = ip_set_test(rule->uset_idx[j], skb, &act, &opt);
			if (ret) {
				nt_debug("ipset test match: %d\n", ret);
				goto __matched;
			}
		}
	}

__matched:
	rcu_read_unlock();
	return ret; //not user
}

static struct nf_hook_ops ntrack_nf_hook_ops[] = {
	{
		.hook = ntrack_hook_fw,
		.owner = THIS_MODULE,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_FORWARD,
		.priority = NF_IP_PRI_LAST,
	},
	{
		.hook = ntrack_hook_test,
		.owner = THIS_MODULE,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_PRE_ROUTING,
		.priority = NF_IP_PRI_FILTER + 1,
	}
};

extern int ntrack_conf_init(void);
extern void ntrack_conf_exit(void);
void *ntrack_klog_fd = NULL;
static int __init ntrack_modules_init(void)
{
	int ret = 0;

	ntrack_klog_fd = klog_init("ntrack", 0x0e, 0);
	if(!ntrack_klog_fd) {
		return -ENOMEM;
	}

	nt_info("init netlink conf io.\n");
	ret = ntrack_conf_init();
	if (ret) {
		goto __err;
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

	/* setup user identify hook */
	rcu_assign_pointer(nos_user_match_fn, ntrack_user_match);
	return 0;

__err:
	nmsg_cleanup();
	if(ntrack_klog_fd) {
		klog_fini(ntrack_klog_fd);
	}
	ntrack_conf_exit();
	return ret;
}

static void __exit ntrack_modules_exit(void)
{
	nt_info("module cleanup.\n");

	/* cleanup user identify hook */
	rcu_assign_pointer(nos_user_match_fn, NULL);

	nf_unregister_hooks(ntrack_nf_hook_ops, ARRAY_SIZE(ntrack_nf_hook_ops));
	nmsg_cleanup();
	ntrack_conf_exit();
	klog_fini(ntrack_klog_fd);

	synchronize_rcu();
	return;
}

module_init(ntrack_modules_init);
module_exit(ntrack_modules_exit);

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("ppp <RRR@gmail.com>");
MODULE_LICENSE("GPL");