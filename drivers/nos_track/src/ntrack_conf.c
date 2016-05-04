#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/netfilter.h>
#include <linux/ip.h>

#include <linux/netfilter/xt_set.h>

#include <net/ip.h>
#include <net/netfilter/nf_conntrack.h>

#include <ntrack_auth.h>
#include <ntrack_log.h>

#include "nxjson.h"

// ip_set_get_byname

const char *conf_str = " \
	[{\
		Name: "", \
		IPSets: [\"office\", \"custom\", \"guest\"], \
		Url: { \
			url: \"http://www.xxx.com\", \
			pars: [\"ip\", \"mac\", \"uid\", \"umagic\"] \
		} \
	}, \
	{ \
		Name: "", \
		IPSets: [], \
		Url: { \
			url: \"\", \
			pars: nil, \
		} \
	}]";

void ntrack_conf_free(G_AUTHCONF_t *conf)
{
	/* FIXME: rcu need */
	synchronize_rcu();
	vfree(conf);
}

G_AUTHCONF_t *G_AuthConf = NULL;
int ntrack_conf_sync(void)
{
	int i = 0;
	auth_rule_t rule;
	struct ip_set *set;
	ip_set_id_t idx;
	const nx_json *json = NULL, *node;
	G_AUTHCONF_t *conf_tmp;

	conf_tmp = vmalloc(sizeof(G_AUTHCONF_t));
	if(!conf_tmp) {
		nt_error("vmalloc failed.\n");
	}
	memset(conf_tmp, 0, sizeof(G_AUTHCONF_t));

	json = nx_json_parse_utf8(conf_str);
	if (!json && json->type != NX_JSON_ARRAY) {
		nt_error("parse json failed or not array.\n");
		return -EINVAL;
	}

	while((node = nx_json_item(json, i++)) != NULL) {
		memset(&rule, 0, sizeof(rule));
		if (node->type != NX_JSON_OBJECT) {
			nt_error("conf item not obj.\n");
			break;
		}
		const nx_json *items = nx_json_get(node, "IPSets");
		if(!items || (items && items->type != NX_JSON_ARRAY)) {
			nt_error("ipsets nil or not string[].\n");
			continue;
		}
		int j = 0;
		const nx_json *str;
		while((str = nx_json_item(items, j++)) != NULL) {
			if(!str || (str && str->type != NX_JSON_STRING)) {
				nt_error("name nil or not string.\n");
				continue;
			}
			idx = ip_set_get_byname(&init_net, str->text_value, &set);
			if (idx == IPSET_INVALID_ID) {
				nt_error("ipset[%s] not found.\n", str->text_value);
				continue;
			}
			rule.uset_idx[rule.num_idx] = idx;
			rule.num_idx ++;
			if(rule.num_idx > MAX_USR_SET) {
				nt_error("ipset rule num overflow.\n");
				break;
			}
		}
		if (rule.num_idx) {
			nt_info("rule[%s] valid added.\n");
			conf_tmp->rules[conf_tmp->num_rules] = rule;
			conf_tmp->num_rules ++;
			if(conf_tmp->num_rules > MAX_URL_RULES) {
				nt_error("url rules overflow.\n");
				break;
			}
		}
	}

	if(G_AuthConf) {
		G_AUTHCONF_t *tmp = G_AuthConf;
		rcu_assign_pointer(G_AuthConf, NULL);
		ntrack_conf_free(tmp);
	}
	rcu_assign_pointer(G_AuthConf, conf_tmp);

	nx_json_free(json);
	return 0;
}

static struct sock *nl_sock = NULL;
struct {
    __u32 pid;
}user_process;

void nl_recv(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh = NULL;
	char *json_str = NULL;

	skb = skb_get(__skb);

	if(skb->len >= sizeof(struct nlmsghdr)){
        nlh = (struct nlmsghdr *)skb->data;
        if((nlh->nlmsg_len >= sizeof(struct nlmsghdr))
            && (__skb->len >= nlh->nlmsg_len)){
            user_process.pid = nlh->nlmsg_pid;
            json_str = ((char *)kzalloc((nlh->nlmsg_len + 1) * sizeof(char), GFP_NOWAIT));
            if (!json_str) {
                goto finish;
            }
            nt_info("pid: %d, dlen=%d\n", user_process.pid, nlh->nlmsg_len);
            memcpy(json_str, (char *)NLMSG_DATA(nlh), nlh->nlmsg_len);
        }
    }
    ntrack_conf_sync();

finish:
	if(json_str){
		kfree(json_str);
	}
	kfree_skb(skb);
}

int ntrack_conf_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.input	= nl_recv,
	};

	nl_sock = netlink_kernel_create(&init_net, NETLINK_NTRACK, &cfg);
	if(!nl_sock) {
		nt_error("netlink create failed.\n");
		return -EINVAL;
	}

	return 0;
}

void ntrack_conf_exit(void)
{
	if (nl_sock) {
		netlink_kernel_release(nl_sock);
	}
}
