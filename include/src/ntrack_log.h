#ifndef __NTRACK_LOGS_H__

//common defs
#define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]

#define HIPQUAD(addr) \
	((unsigned char *)&addr)[3], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[0]

#define FMT_FLOW_STR "fid: %u-%u [%u.%u.%u.%u:%u -> %u.%u.%u.%u:%u-%u]"
#define FMT_FLOW(fi) \
			(fi)->id, (fi)->magic, \
			HIPQUAD((fi)->tuple.ip_src), (fi)->tuple.port_src, \
			HIPQUAD((fi)->tuple.ip_dst), (fi)->tuple.port_dst, \
			(fi)->tuple.proto

#define FMT_USER_STR "uid:%8u magic:%8u gid:%4d ref:%4u - %u.%u.%u.%u"
#define FMT_USER(ui) \
			(ui)->id, (ui)->magic, (ui)->hdr.group_id, (ui)->refcnt, \
			HIPQUAD((ui)->ip)

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/klog.h>

#define nt_print printk
extern void *ntrack_klog_fd;
#define nt_debug(fmt...) 	klog_debug(ntrack_klog_fd, ##fmt)
#define nt_info(fmt...) 	klog_info(ntrack_klog_fd, ##fmt)
#define nt_warn(fmt...) 	klog_warn(ntrack_klog_fd, ##fmt)
#define nt_error(fmt...) 	klog_error(ntrack_klog_fd, ##fmt)
#define nt_dump(buf, size, fmt, args...) 	klog_dumpbuf(ntrack_klog_fd, buf, size, fmt, ##args)
#define nt_trace(level, fmt...) 	klog_trace(ntrack_klog_fd, level, ##fmt)
#define nt_assert(x, fmt...) 	BUG_ON(!(x))

#else /* end kernel */

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <assert.h>

#include <sys/types.h>

#define nt_print printf
void hexdump(FILE *fp, const void *data, int size);

#define nt_debug(fmt...) 	do{ \
		fprintf(stderr, "[dbg] %s: ", __FUNCTION__); \
		fprintf(stderr, ##fmt); \
	}while(0)

#define nt_info(fmt...) 	do{ \
		fprintf(stderr, "[inf] %s: ", __FUNCTION__); \
		fprintf(stderr, ##fmt); \
	}while(0)

#define nt_warn(fmt...) 	do{ \
		fprintf(stderr, "[war] %s: ", __FUNCTION__); \
		fprintf(stderr, ##fmt); \
	}while(0)

#define nt_error(fmt...) 	do{ \
		fprintf(stderr, "[err] %s: ", __FUNCTION__); \
		fprintf(stderr, ##fmt); \
	}while(0)

#define nt_dump(buf, size, fmt...) 	do { \
		fprintf(stderr, "[dump] %s: ", __FUNCTION__); \
		fprintf(stderr, ##fmt); \
		hexdump(stderr, buf, size); \
	} while(0)

#define nt_assert(exp, fmt...) assert(exp)

#endif /* __KERNEL__ */

#endif //__NTRACK_LOGS_H__
