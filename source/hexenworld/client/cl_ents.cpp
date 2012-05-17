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

//======================================================================

/*
===============
CL_SetSolid

Builds all the qh_pmove physents for the current frame
===============
*/
void CL_SetSolidEntities(void)
{
	int i;
	hwframe_t* frame;
	hwpacket_entities_t* pak;
	h2entity_state_t* state;

	qh_pmove.physents[0].model = 0;
	VectorCopy(vec3_origin, qh_pmove.physents[0].origin);
	qh_pmove.physents[0].info = 0;
	qh_pmove.numphysent = 1;

	frame = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW];
	pak = &frame->packet_entities;

	for (i = 0; i < pak->num_entities; i++)
	{
		state = &pak->entities[i];

		if (state->modelindex < 2)
		{
			continue;
		}
		if (!cl.model_clip[state->modelindex])
		{
			continue;
		}
		qh_pmove.physents[qh_pmove.numphysent].model = cl.model_clip[state->modelindex];
		VectorCopy(state->origin, qh_pmove.physents[qh_pmove.numphysent].origin);
		VectorCopy(state->angles, qh_pmove.physents[qh_pmove.numphysent].angles);
		qh_pmove.numphysent++;
	}

}
