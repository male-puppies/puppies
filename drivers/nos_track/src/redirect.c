#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/ip.h>

#include <net/ip.h>
#include <net/tcp.h>

#include <linux/nos_track.h>
#include <ntrack_log.h>
#include <ntrack_msg.h>
#include <ntrack_auth.h>


static int auth_reset(struct sk_buff *skb, const struct net_device *dev)
{
	int len;
	struct sk_buff *nskb;
	struct tcphdr *otcph, *ntcph;
	struct ethhdr *neth, *oeth;
	struct iphdr *niph, *oiph;
	unsigned int csum, header_len;

	oeth = (struct ethhdr *)skb_mac_header(skb);
	oiph = ip_hdr(skb);
	otcph = (struct tcphdr *)(skb_network_header(skb) + (oiph->ihl << 2));

	header_len = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr);
	nskb = alloc_skb(header_len, GFP_NOWAIT);
	if (!nskb) {
		nt_error("alloc_skb fail\n");
		return -1;
	}

	skb_reserve(nskb, header_len);
	ntcph = (struct tcphdr *)skb_push(nskb, sizeof(struct tcphdr));
	memset(ntcph, 0, sizeof(struct tcphdr));
	ntcph->source = otcph->source;
	ntcph->dest = otcph->dest;
	ntcph->seq = otcph->seq;
	ntcph->ack_seq = otcph->ack_seq;
	ntcph->doff = sizeof(struct tcphdr) / 4;
	((u_int8_t *)ntcph)[13] = 0;
	ntcph->rst = 1;
	ntcph->ack = otcph->ack;
	ntcph->window = htons(0);

	niph = (struct iphdr *)skb_push(nskb, sizeof(struct iphdr));
	memset(niph, 0, sizeof(struct iphdr));
	niph->saddr = oiph->saddr;
	niph->daddr = oiph->daddr;
	niph->version = oiph->version;
	niph->ihl = 5;
	niph->tos = 0;
	niph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
	niph->ttl = 0x80;
	niph->protocol = oiph->protocol;
	niph->id = 0;
	niph->frag_off = 0x0040;
	ip_send_check(niph);

	len = ntohs(niph->tot_len) - (niph->ihl<<2);
	csum = csum_partial((char*)ntcph, len, 0);
	ntcph->check = tcp_v4_check(len, niph->saddr, niph->daddr, csum);
	skb_reset_network_header(nskb);
	neth = (struct ethhdr *)skb_push(nskb, sizeof(struct ethhdr));
	memcpy(neth, oeth, sizeof(struct ethhdr));

	nskb->dev = (struct net_device *)dev;
	dev_queue_xmit(nskb);
	return 0;
}

static int auth_URL(const char *url, int urllen, struct sk_buff *skb, const struct net_device *dev) {
	struct sk_buff *nskb;
	struct ethhdr *neth, *oeth;
	struct iphdr *niph, *oiph;
	struct tcphdr *otcph, *ntcph;
	int len;
	unsigned int csum, header_len;
	char *data;

	oeth = (struct ethhdr *)skb_mac_header(skb);
	oiph = ip_hdr(skb);
	otcph = (struct tcphdr *)(skb_network_header(skb) + (oiph->ihl<<2));

	header_len = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr);
	nskb = alloc_skb(header_len + urllen, GFP_NOWAIT);
	if (!nskb) {
		nt_error("alloc_skb fail\n");
		return -1;
	}

	skb_reserve(nskb, header_len);

	data = (char *)skb_put(nskb, urllen);
	memcpy(data, url, urllen);

	ntcph = (struct tcphdr *)skb_push(nskb, sizeof(struct tcphdr));
	memset(ntcph, 0, sizeof(struct tcphdr));
	ntcph->source = otcph->dest;
	ntcph->dest = otcph->source;
	ntcph->seq = otcph->ack_seq;
	ntcph->ack_seq = htonl(ntohl(otcph->seq) + ntohs(oiph->tot_len) - (oiph->ihl<<2) - (otcph->doff<<2));
	ntcph->doff = 5;
	ntcph->ack = 1;
	ntcph->psh = 1;
	ntcph->fin = 1;
	ntcph->window = 65535;

	niph = (struct iphdr *)skb_push(nskb, sizeof(struct iphdr));
	memset(niph, 0, sizeof(struct iphdr));
	niph->saddr = oiph->daddr;
	niph->daddr = oiph->saddr;
	niph->version = oiph->version;
	niph->ihl = 5;
	niph->tos = 0;
	niph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr) + urllen);
	niph->ttl = 0x80;
	niph->protocol = oiph->protocol;
	niph->id = 0x2658;
	niph->frag_off = 0x0040;
	ip_send_check(niph);

	len = ntohs(niph->tot_len) - (niph->ihl<<2);
	csum = csum_partial((char*)ntcph, len, 0);
	ntcph->check = tcp_v4_check(len, niph->saddr, niph->daddr, csum);
	skb_reset_network_header(nskb);

	neth = (struct ethhdr *)skb_push(nskb, sizeof(struct ethhdr));
	memcpy(neth->h_dest, oeth->h_source, 6);
	memcpy(neth->h_source, oeth->h_dest, 6);
	neth->h_proto = htons(ETH_P_IP);
	nskb->dev = (struct net_device *)dev;
	dev_queue_xmit(nskb);
	return 0;
}

int ntrack_redirect(const char *url, int urllen, 
			struct sk_buff *skb,
			const struct net_device *in,
			const struct net_device *out)
{
	/* 构造一个URL重定向包, 从in接口发出去 */
	if(auth_URL(url, urllen, skb, in)){
		nt_error("error send redirect url.\n");
		return -1;
	}

	/* 构造一个reset包, 从out接口发出去 */
	if (out)
		auth_reset(skb, out);
	return 0;
}