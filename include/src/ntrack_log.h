#ifndef __NTRACK_LOGS_H__
#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/klog.h>

extern void *ntrack_klog_fd;
#define nt_debug(fmt...) 	klog_debug(ntrack_klog_fd, ##fmt)
#define nt_info(fmt...) 	klog_info(ntrack_klog_fd, ##fmt)
#define nt_warn(fmt...) 	klog_warn(ntrack_klog_fd, ##fmt)
#define nt_error(fmt...) 	klog_error(ntrack_klog_fd, ##fmt)
#define nt_dump(buf, size, fmt, args...) 	klog_dumpbuf(ntrack_klog_fd, buf, size, fmt, ##args)
#define nt_trace(level, fmt...) 	klog_trace(ntrack_klog_fd, level, ##fmt)

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
