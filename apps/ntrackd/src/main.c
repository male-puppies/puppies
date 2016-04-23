#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <ntrack_rbf.h>
#include <ntrack_log.h>

int main(int argc, char *argv[])
{
	cpu_set_t set;
	rbf_t *rbp;
	void *base_addr, *p;
	uint32_t size = (RBF_NODE_SIZE) * 1024 + sizeof(rbf_hdr_t);

	CPU_ZERO(&set);

	int fd = open("/dev/mem", O_RDWR);
	if(fd == -1) {
		nt_error("open shm.\n");
		exit(EXIT_FAILURE);
	}

	base_addr = mmap(0, 4<<20, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 16<<20);
	if (!base_addr) {
		nt_error("mem map.\n");
		exit(EXIT_FAILURE);
	}

	// memset(base_addr, 0xff, size);
	rbp = rbf_init(base_addr, size);
	if(!rbp) {
		nt_error("rbp init\n");
		exit(EXIT_FAILURE);
	}
	
	CPU_SET(0, &set);
	if(sched_setaffinity(getpid(), sizeof(set), &set) == -1) {
		nt_error("set p affinity.\n");
		exit(EXIT_FAILURE);
	}
	// fprintf(stderr, "%d->%d core: %d\n", getppid(), getpid(), sched_getcpu());

	while(1) {
		p = rbf_get_data(rbp);
		if (!p) {
			// nt_debug("read empty.\n");
			sleep(0.01);
			continue;
		}
		nt_dump(p, 128, "node\n");
		rbf_release_data(rbp);
	}

	return 0;
}
