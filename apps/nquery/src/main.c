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
#include <sys/socket.h>

#include <linux/netlink.h>

#include <ntrack_rbf.h>
#include <ntrack_log.h>
#include <ntrack_msg.h>
#include <ntrack_auth.h>

flow_info_t *fi_base = NULL;
user_info_t *ui_base = NULL;

static uint32_t fi_count, ui_count;

int main(int argc, char *argv[])
{
	int i;

	if(argc < 2) {
		nt_error("nquery <flow|user>\n");
		exit(0);
	}

	if (nt_message_init(&ui_base, &fi_base)) {
		nt_error("nquery message init failed.\n");
		return 0;
	}

	/* debug */
	nt_dump(ui_base, 128, "user base: %p\n", ui_base);
	nt_dump(fi_base, 128, "flow base: %p\n", fi_base);

	if(strcmp(argv[1], "flow") == 0) {
		for(i=0; i<flow_info_count; i++) {
			flow_info_t *fi = &fi_base[i];
			if(fi->magic == 3217014719) {
				continue;
			}
			nt_dump(fi, 32, "%u-%u\n", i, fi->magic);
		}
	}

	if(strcmp(argv[1], "user") == 0) {
		for(i=0; i<user_info_count; i++) {
			user_info_t *ui = &ui_base[i];
			if (ui->magic == 2947526575) {
				continue;
			}
			nt_dump(ui, 32, "%u-%u %u.%u.%u.%u, ref: %u\n", i, 
				ui->magic, HIPQUAD(ui->ip), ui->refcnt);
		}
	}

	return 0;
}
