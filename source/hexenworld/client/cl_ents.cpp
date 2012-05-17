// cl_ents.c -- entity parsing and management

#include "quakedef.h"

/*
===================
CLHW_ParsePlayerinfo
===================
*/
void CL_SavePlayer(void)
{
	int num;
	h2player_info_t* info;
	hwplayer_state_t* state;

	num = net_message.ReadByte();

	if (num > HWMAX_CLIENTS)
	{
		Sys_Error("CLHW_ParsePlayerinfo: bad num");
	}

	info = &cl.h2_players[num];
	state = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].playerstate[num];

	state->messagenum = cl.qh_parsecount;
	state->state_time = cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].senttime;
}
