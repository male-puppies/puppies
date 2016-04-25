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
#include <ntrack_msg.h>

static int fn_message_disp(void *p)
{
	nt_dump(p, 128, "cap:\n");
	
	return 0;
}

int main(int argc, char *argv[])
{
	cpu_set_t set;
	int running = 1;

	if(argc < 2) {
		nt_error("ntrackd <core_num>\n");
		exit(0);
	}

	CPU_ZERO(&set);

	CPU_SET(atoi(argv[1]), &set);
	if(sched_setaffinity(getpid(), sizeof(set), &set) == -1) {
		nt_error("set c affinity.\n");
		exit(EXIT_FAILURE);
	}

	if (nt_message_init()) {
		nt_error("ntrack message init failed.\n");
		return 0;
	}

	nt_message_process(&running, fn_message_disp);

	return 0;
}
