#pragma once

#include <linux/nos_track.h>
#include <ntrack_log.h>

#define USER_PRIV_SZ 		sizeof((user_info_t*)(void*(0))->private)
#define FLOW_PRIV_SZ 		sizeof((flow_info_t*)(void*(0))->private)
#define flow_info_count 	NOS_FLOW_TRACK_MAX
#define user_info_count 	NOS_USER_TRACK_MAX

#ifdef __KERNEL__

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

#else //user space.

/* node track */
extern flow_info_t *nos_flow_info_base;
extern user_info_t *nos_user_info_base;

static inline flow_info_t * nt_get_flow_by_id(uint32_t id, uint32_t magic)
{
	nt_assert(id >= 0 && id < NOS_FLOW_TRACK_MAX);
	return &nos_flow_info_base[id];
}

static inline user_info_t * nt_get_user_by_id(uint32_t id, uint32_t magic)
{
	nt_assert(id >= 0 && id < NOS_USER_TRACK_MAX);
	return &nos_user_info_base[id];
}

static inline user_info_t * nt_get_user_by_flow(uint32_t f_id, uint32_t f_magic)
{
	return &nos_user_info_base[nt_get_flow_by_id(f_id, f_magic)->user_id];
}
/* end node track */

#endif //__KERNEL__

/* KERNEL & USER comm use. */
static inline void *nt_user_priv(user_info_t *ui)
{
	return ui->private;
}

static inline void *nt_flow_priv(flow_info_t *fi)
{
	return fi->private;
}
