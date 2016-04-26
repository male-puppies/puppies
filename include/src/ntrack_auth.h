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


/* KERNEL use for parse json */
typedef struct {
	ip_list_t ip_list;
	ip_range_t ip_range;

} match_ip_t;

typedef struct {
	match_ip_t ips;
	match_mac_t macs;
	redir_url_t url;
} auth_conf_t; 