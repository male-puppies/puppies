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
		URL: {
			url: "http://www.xxx.com",
			pars: ["ip", "mac", "uid", "umagic"]
		}
	},
	Web: {
		IPSets: [],
		URL: {
			url: "",
			pars: [],
		}
	}
}
*/
#define NETLINK_NTRACK 28

#ifdef __KERNEL__

/* KERNEL use for parse json */
#define MAX_URL_LEN 512
#define MAX_URL_PAR_NUM 32
#define MAX_URL_PAR_LEN 32
typedef struct {
	int url_len, num_pars;
	char url[MAX_URL_LEN];
	char pars[MAX_URL_PAR_NUM][MAX_URL_PAR_LEN];
} redir_url_t;

/* ipset hash:ip hash:mac check src address from skb. */
#define MAX_USR_SET 4
#define RULE_NAME_SIZE 64
typedef struct {
	uint32_t num_idx;
	ip_set_id_t uset_idx[MAX_USR_SET];
	redir_url_t URL;
	char name[RULE_NAME_SIZE];
} auth_rule_t; 

#define MAX_URL_RULES 64
typedef struct {
	int num_rules;
	auth_rule_t rules[MAX_URL_RULES];
} G_AUTHCONF_t;

static inline int user_need_redirect(struct nos_user_info *ui) 
{
	return ui->hdr.rule_idx >= 0;
}

int l3filter(struct iphdr* iph);
int ntrack_get_url(int rule_idx, char **url, int *len);
int ntrack_redirect(const char *url, int urllen, 
			struct sk_buff *skb,
			const struct net_device *in,
			const struct net_device *out);
#endif //__KERNEL__
