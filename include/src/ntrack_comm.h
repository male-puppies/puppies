#pragma once

#include <linux/nos_track.h>
#include <ntrack_log.h>

#define flow_info_count 	NOS_FLOW_TRACK_MAX
#define user_info_count 	NOS_USER_TRACK_MAX

/* KERNEL & USER comm use. */
#define USER_PRIV_SZ 		sizeof((user_info_t*)(void*(0))->private)
#define FLOW_PRIV_SZ 		sizeof((flow_info_t*)(void*(0))->private)
static inline void *nt_user_priv(user_info_t *ui)
{
	return ui->private;
}

static inline void *nt_flow_priv(flow_info_t *fi)
{
	return fi->private;
}

static inline uint32_t magic_valid(uint32_t m)
{
	return m % 2 == 0;
}

#ifdef __KERNEL__
/* kernel node opt apis */
static inline flow_info_t * nt_flow(struct nos_track *nt)
{
	flow_info_t *fi = nt->flow;
	nt_assert(fi);
	nt_assert(fi->id >= 0 && fi->id < NOS_FLOW_TRACK_MAX);
	return fi;
}

static inline user_info_t * nt_user(struct nos_track *nt)
{
	user_info_t *ui = nt->user;
	nt_assert(ui);
	nt_assert(ui->id >= 0 && ui->id < NOS_USER_TRACK_MAX);
	return ui;
}

static inline user_info_t * nt_peer(struct nos_track *nt)
{
	user_info_t *ui = nt->peer;
	nt_assert(ui);
	nt_assert(ui->id >= 0 && ui->id < NOS_USER_TRACK_MAX);
	return ui;
}
#else /* __KERNEL__ */

/* node track base address mmmap used. */
typedef struct {
	uint32_t fi_count, ui_count;
	flow_info_t *fi_base;
	user_info_t *ui_base;
} ntrack_t;

/* userspace node opt apis */
static inline flow_info_t * nt_get_flow_by_id(ntrack_t *nt, uint32_t id, uint32_t magic)
{
	flow_info_t *fi = &nt->fi_base[id];

	nt_assert(id >= 0 && id < NOS_FLOW_TRACK_MAX);
	/* check magic */
	if (fi->magic != magic) {
		nt_warn("fid: %u, magic inv: %u-%u\n", id, magic, fi->magic);
		return NULL;
	}
	return fi;
}

static inline user_info_t * nt_get_user_by_id(ntrack_t *nt, uint32_t id, uint32_t magic)
{
	user_info_t *ui = &nt->ui_base[id];

	nt_assert(id >= 0 && id < NOS_USER_TRACK_MAX);
	/* check magic */
	if (ui->magic != magic) {
		nt_warn("uid: %u, magic inv: %u-%u\n", id, magic, ui->magic);
		return NULL;
	}
	return ui;
}

static inline user_info_t * nt_get_user_by_flow(ntrack_t *nt, flow_info_t *fi)
{
	uint32_t uid = fi->user_id;

	nt_assert(uid >=0 && uid < NOS_USER_TRACK_MAX);
	return &nt->ui_base[uid];
}
/* end node track */
#endif /* __KERNEL__ */
