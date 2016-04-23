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
	int running = 1;

	if (nt_message_init()) {
		nt_error("ntrack message init failed.\n");
		return 0;
	}

	nt_message_process(&running, fn_message_disp);

	return 0;
}
