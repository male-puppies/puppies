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

static int set_user_status(uint32_t uid, uint32_t magic, uint32_t status)
{
	uint32_t s_prev;
	user_info_t *ui = nt_get_user_by_id(&ntrack, uid, magic);
	if(!ui) {
		nt_error("user: %u-%u not found.\n", uid, magic);
		return -EINVAL;
	}
	nt_print(FMT_USER_STR"\n", FMT_USER(ui));

	s_prev = nt_auth_set_status(ui, status);
	nt_info("status: %u->%u\n", s_prev, status);
	return 0;
}

static void dump_flowinfo(void)
{
	int i;

	for(i=0; i<ntrack.fi_count; i++) {
		flow_info_t *fi = &ntrack.fi_base[i];
		user_info_t *ui, *pi;

		if(!magic_valid(fi->magic)) {
			continue;
		}

		/* check fi use api */
		fi = nt_get_flow_by_id(&ntrack, fi->id, fi->magic);
		if(!fi) {
			continue;
		}
		ui = nt_get_user_by_flow(&ntrack, fi);

		nt_print(FMT_FLOW_STR"\n\t"FMT_USER_STR"\n", FMT_FLOW(fi), FMT_USER(ui));
	}
}

static void dump_userinfo(void)
{
	int i;

	for(i=0; i<ntrack.ui_count; i++) {
		user_info_t *ui = &ntrack.ui_base[i];
		if (!magic_valid(ui->magic)) {
			continue;
		}
		nt_print(FMT_USER_STR"\n", FMT_USER(ui));
	}
}

int main(int argc, char *argv[])
{
	if(argc < 2) {
		nt_error("nquery <flow|user>\n");
		exit(0);
	}

	if (nt_message_init(&ntrack)) {
		nt_error("nquery message init failed.\n");
		return 0;
	}

	/* debug */
	// nt_dump(ntrack.ui_base, 64, "user base: %p\n", ntrack.ui_base);
	// nt_dump(ntrack.fi_base, 64, "flow base: %p\n", ntrack.fi_base);

	if(strcmp(argv[1], "flow") == 0) {
		dump_flowinfo();
	}

	if(strcmp(argv[1], "user") == 0) {
		dump_userinfo();
	}

	if(strcmp(argv[1], "set") == 0) {
		if (argc < 5) {
			nt_error("nquery set uid magic status\n");
			exit(0);
		}
		set_user_status(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	}

	return 0;
}
