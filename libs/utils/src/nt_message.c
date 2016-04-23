#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <ntrack_rbf.h>
#include <ntrack_log.h>

const char *fn_sys_mem = "/dev/mem";
const char *fn_shm_uoff = "/proc/sys/kernel/nt_user_offset";
const char *fn_shm_foff = "/proc/sys/kernel/nt_flow_offset";
const char *fn_shm_cap_sz = "/proc/sys/kernel/nt_cap_block_sz";

static void* shm_base_addr = NULL;
static void* shm_base_user = NULL;
static void* shm_base_flow = NULL;
static uint32_t shm_user_offset;
static uint32_t shm_flow_offset;
static uint32_t nt_cap_block_sz;

static int proc_uint(uint32_t *out, const char *fname)
{
	int fd = open(fname, O_RDONLY);
	if(fd < 0) {
		nt_error("open %s\n", fname);
		return -EINVAL;
	}

	char buff[32];
	memset(buff, 0, sizeof(buff));
	ssize_t c = read(fd, buff, sizeof(buff));
	close(fd);

	if(c > 0) {
		*out = atoi(buff);
		return 0;
	} else {
		nt_error("%s\n", strerror(errno));
		return errno;
	}
}

static int proc_pars_init(void)
{
	int res = 0;

	if((res=proc_uint(&nt_cap_block_sz, fn_shm_cap_sz))) {
		nt_error("read nt cap block failed: %d\n", res);
		return res;
	}
	if((res=proc_uint(&shm_user_offset, fn_shm_uoff))) {
		nt_error("read nt user offset failed: %d\n", res);
		return res;
	}
	if((res=proc_uint(&shm_flow_offset, fn_shm_foff))) {
		nt_error("read nt flow offset failed: %d\n", res);
		return res;
	}

	nt_info("nt proc: \n"
		"\tuser offset: %d\n", shm_user_offset,
		"\tflow offset: %d\n", shm_flow_offset,
		"\tblock cap size: %d\n", nt_cap_block_sz);

	return 0;
}

static int shm_init(void)
{
	int fd = open(fn_sys_mem, O_RDWR);
	if(fd == -1) {
		nt_error("open shm.\n");
		return -EINVAL;
	}

	shm_base_addr = mmap(0, 4<<20, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 16<<20);
	if (!shm_base_addr) {
		nt_error("mem map.\n");
		return -ENOMEM;
	}
	shm_base_user = shm_base_addr + shm_user_offset;
	shm_base_flow = shm_base_addr + shm_flow_offset;

	nt_info("shm base: %p, user: %p, flow: %p\n", \
		shm_base_addr, shm_base_user, shm_base_flow);

	return 0;
}

static int nt_msg_init(uint32_t core_idx)
{
	rbf_t *rbp;

	rbp = rbf_init(shm_base_addr + nt_cap_block_sz * core_idx, nt_cap_block_sz);
	if(!rbp) {
		nt_error("rbp init\n");
		return -EINVAL;
	}

	return 0;
}

int nt_init(void)
{
	if (proc_pars_init()) {
		return -EINVAL;
	}

	if (shm_init()) {
		return -EINVAL;
	}
}
