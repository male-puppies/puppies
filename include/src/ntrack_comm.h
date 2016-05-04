#pragma once

#include <linux/nos_track.h>

/* node track */
extern struct nos_flow_info *nos_flow_info_base;
extern struct nos_user_info *nos_user_info_base;

static inline struct nos_flow_info * 
nt_get_flow_by_id(uint32_t id, uint32_t magic)
{
	nt_aseert(id >= 0 && id < NOS_FLOW_TRACK_MAX);
	return &nos_flow_info_base[id];
}

static inline struct nos_user_info * 
nt_get_user_by_id(uint32_t id, uint32_t magic)
{
	nt_aseert(id >= 0 && id < NOS_USER_TRACK_MAX);
	return &nos_user_info_base[id];
}

static inline struct nos_user_info * 
nt_get_user_by_flow(uint32_t f_id, uint32_t f_magic)
{
	return &nos_user_info_base[nt_get_flow_by_id(f_id, f_magic)->user_id];
}
/* end node track */

/* auth conf & status update */
int nt_auth_conf(auth_conf_t o)
{
	return 0;
}

