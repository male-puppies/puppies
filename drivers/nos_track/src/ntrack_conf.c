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

/* test */
/*
const char *conf_str = " \
	[{\
		\"Name\": \"\", \
		\"IPSets\": [\"baidu\", \"weixin\", \"3p\"], \
		\"Url\": { \
			\"url\": \"http://www.xxx.com\", \
			\"pars\": [\"ip\", \"mac\", \"uid\", \"umagic\"] \
		} \
	}, \
	{ \
		\"Name\": \"\", \
		\"IPSets\": [], \
		\"Url\": { \
			\"url\": \"\", \
			\"pars\": null \
		} \
	}]";
*/

void ntrack_conf_free(G_AUTHCONF_t *conf)
{
	/* FIXME: rcu need */
	synchronize_rcu();
	vfree(conf);
}

static G_AUTHCONF_t *G_AuthConf = NULL;
int ntrack_conf_sync(char *conf_str)
{
	int i, j;
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
	if (!json || (json && json->type != NX_JSON_ARRAY)) {
		nt_error("parse json failed or not array.\n");
		return -EINVAL;
	}

	i = 0;
	while((node = nx_json_item(json, i++)) != NULL) {
		const nx_json *str, *sets, *url;

		if (node->type == NX_JSON_NULL) {
			break;
		}
		memset(&rule, 0, sizeof(rule));
		if (node->type != NX_JSON_OBJECT) {
			nt_error("conf item not obj[%d].\n", node->type);
			continue;
		}
		
		/* ipset sets */
		sets = nx_json_get(node, "IPSets");
		if (sets->type != NX_JSON_ARRAY) {
			nt_error("ipsets nil or not string[].\n");
			continue;
		}

		/* apply sets. */
		j = 0;
		while((str = nx_json_item(sets, j++)) != NULL) {
			if(str->type == NX_JSON_NULL) {
				break;
			}
			if(str->type != NX_JSON_STRING) {
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
			strncpy(rule.name, str->text_value, RULE_NAME_SIZE);
			if(rule.num_idx > MAX_USR_SET) {
				nt_error("ipset rule num overflow.\n");
				break;
			}
		}

		/* get url */
		url = nx_json_get(node, "URL");
		if(url->type == NX_JSON_OBJECT) {
			int k;
			const nx_json *url, *pars;
			url = nx_json_get(url, "url");
			if(url->type == NX_JSON_STRING) {
				strncpy(rule.URL.url, url->text_value, sizeof(rule.URL.url));
				rule.URL.url_len = strlen(rule.URL.url);
				nt_info("rule: %d, url: [%s]\n", rule.URL.url_len, rule.URL.url);
			}
			pars = nx_json_get(url, "pars");
			if (pars->type == NX_JSON_ARRAY) {
				k = 0;
				while((str = nx_json_item(pars, k++)) != NULL) {
					if(str->type != NX_JSON_STRING) {
						continue;
					}
					strncpy(rule.URL.pars[k], str->text_value, sizeof(rule.URL.pars[0]));
					nt_info("\tpar: %d, %s\n", k, rule.URL.pars[k]);
					if(k >= MAX_URL_PAR_NUM) {
						nt_error("url par num overflow.\n");
						break;
					}
				}
			}
		} else {
			nt_error("rule: %i, URL error fmt.\n", i);
		}

		/* save rule to Global configure. */
		if (rule.num_idx) {
			nt_info("rule[%s] valid added.\n", rule.name);
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

/* ipset hash:ip hash:mac check src address from skb. */
int ntrack_user_match(struct nos_user_info *ui, struct sk_buff *skb)
{
	int ret = 0, i, j;
	struct ip_set_adt_opt opt;
	struct xt_action_param act;
	struct net_device *indev, *dev;
	struct iphdr *iph;
	// const struct xt_set_info *set = (const void *) em->data;

	G_AUTHCONF_t *conf = rcu_dereference(G_AuthConf);
	if(!conf) {
		return 0;
	}

	if(!conf->num_rules) {
		return 0;
	}

	iph = ip_hdr(skb);
	if(l3filter(iph)) {
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

	/* find addr & do match */
	for (i = 0; i < conf->num_rules; ++i) {
		auth_rule_t *rule = &conf->rules[i];
		for (j = 0; j < rule->num_idx; ++j) {
			ret = ip_set_test(rule->uset_idx[j], skb, &act, &opt);
			if (ret) {
				ui->hdr.rule_idx = i;
				nt_debug("ipset test match: %d\n", ret);
				goto __matched;
			}
			// nt_debug("no match: [%s] %pI4 -> %pI4\n", rule->name, &iph->saddr, &iph->daddr);
		}
	}

__matched:
	rcu_read_unlock();
	return ret; //not user
}

int ntrack_get_url(int idx, char **url, int *len)
{
	redir_url_t *URL;
	G_AUTHCONF_t *conf = rcu_dereference(G_AuthConf);

	if(!conf) {
		nt_error("url conf not found.\n");
		return -EINVAL;
	}
	if(idx >= conf->num_rules) {
		nt_error("idx: %d overflow vs rule num: %d\n", idx, conf->num_rules);
		return -EINVAL;
	}

	URL = &conf->rules[idx].URL;
	if(URL->url_len > 0) {
		*url = URL->url;
		*len = URL->url_len;
	}

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
        if(NLMSG_OK(nlh, __skb->len)) {
        	int len = nlh->nlmsg_len - NLMSG_HDRLEN;
            user_process.pid = nlh->nlmsg_pid;
            json_str = ((char *)kzalloc((len + 1) * sizeof(char), GFP_NOWAIT));
            if (!json_str) {
                goto finish;
            }
            nt_info("pid: %d, dlen=%d\n", user_process.pid, len);
            memcpy(json_str, (char *)NLMSG_DATA(nlh), len);
			ntrack_conf_sync(json_str);
        }
    }

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
