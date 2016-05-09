#pragma once

#include <linux/nos_track.h>

#ifdef __KERNEL__
#include <linux/list.h>
#include <linux/netfilter/xt_set.h>
#else
#include <stdint.h>
#endif

/* kernel conf json */
/*
{
	Auto: {
		IPSets: ["office", "custom", "guest"],
		RedirectFlags: 0/1
	},
	Web: {
		IPSets: [],
		RedirectFlags: 0/1
	}
}
*/
#define NETLINK_NTRACK 28

#ifdef __KERNEL__

/* KERNEL use for parse json */

/* ipset hash:ip hash:mac check src address from skb. */
#define MAX_USR_SET 4
#define RULE_NAME_SIZE 64
typedef struct {
	uint32_t num_idx;
	uint32_t redirect_flags;
	ip_set_id_t uset_idx[MAX_USR_SET];
	char name[RULE_NAME_SIZE];
} auth_rule_t; 

#define MAX_URL_RULES 64
typedef struct {
	int num_rules;
	auth_rule_t rules[MAX_URL_RULES];
} G_AUTHCONF_t;

int l3filter(struct iphdr* iph);
int user_need_redirect(struct nos_user_info *ui);
int ntrack_redirect(struct nos_user_info *ui, 
			struct sk_buff *skb,
			const struct net_device *in,
			const struct net_device *out);
#endif //__KERNEL__
