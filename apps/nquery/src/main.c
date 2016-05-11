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

ntrack_t ntrack;
int main(int argc, char *argv[])
{
	int i;

	if(argc < 2) {
		nt_error("nquery <flow|user>\n");
		exit(0);
	}

	if (nt_message_init(&ntrack)) {
		nt_error("nquery message init failed.\n");
		return 0;
	}

	/* debug */
	nt_dump(ntrack.ui_base, 128, "user base: %p\n", ntrack.ui_base);
	nt_dump(ntrack.fi_base, 128, "flow base: %p\n", ntrack.fi_base);

	if(strcmp(argv[1], "flow") == 0) {
		for(i=0; i<ntrack.fi_count; i++) {
			flow_info_t *fi = &ntrack.fi_base[i];
			user_info_t *ui, *pi;

			if(fi->magic == 3217014719u) {
				continue;
			}

			/* check fi use api */
			fi = nt_get_flow_by_id(&ntrack, fi->id, fi->magic);
			if(!fi) {
				continue;
			}
			ui = nt_get_user_by_flow(&ntrack, fi);
			nt_dump(fi, 32, "%u-%u\n", i, fi->magic);
		}
	}

	if(strcmp(argv[1], "user") == 0) {
		for(i=0; i<ntrack.ui_count; i++) {
			user_info_t *ui = &ntrack.ui_base[i];
			if (ui->magic == 2947526575u) {
				continue;
			}
			nt_dump(ui, 32, "%u-%u %u.%u.%u.%u, ref: %u, group: %d.\n", 
				i, ui->magic, HIPQUAD(ui->ip), ui->refcnt, ui->hdr.group_id);
		}
	}

	return 0;
}
