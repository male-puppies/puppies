#pragma once

#include <linux/nos_track.h>

#define USER_PRIV_SZ sizeof((user_info_t*)(void*(0))->private)
#define FLOW_PRIV_SZ sizeof((flow_info_t*)(void*(0))->private)

#ifdef __KERNEL__

static inline flow_info_t * nt_flow(struct nos_track *nt)
{
	return nt->flow;
}

static inline user_info_t * nt_user(struct nos_track *nt)
{
	return nt->user;
}

static inline user_info_t * nt_peer(struct nos_track *nt)
{
	return nt->peer;
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
