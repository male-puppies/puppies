#pragma once

/*
*  JSON format of CONF Object.
{
	Auto: {
		IPs: ["192.168.1.1-192.168.1.100", "192.168.2.0/24"],
		MACs: [aa:bb:cc:dd:ee:ff],
		URL: {
			url: "http://welcome.com/",
			pars: nil
		}
	},
	Web: {
		IPs: ["192.168.3.0/24"],
		URL: {
			url: "http://welcome.com/",
			pars: ["ip", "mac", "uid", "magic"]
		}
	}
}
*
*/

#ifdef __KERNEL__
#include <linux/list.h>
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
	char url[MAX_URL_LEN];
	char url_pars[MAX_URL_PAR_NUM][MAX_URL_PAR_LEN];
} redir_url_t;

/* ipset hash:ip hash:mac check src address from skb. */
#define MAX_USR_SET 4
typedef struct {
	uint32_t num_idx;
	ip_set_id_t uset_idx[MAX_USR_SET];
	redir_url_t url;
} auth_rule_t; 

#define MAX_URL_RULES 64
typedef struct {
	int num_rules;
	auth_rule_t rules[MAX_URL_RULES];
} G_AUTHCONF_t;
extern G_AUTHCONF_t *G_AuthConf;

#endif //__KERNEL__
