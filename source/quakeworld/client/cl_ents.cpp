/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_ents.c -- entity parsing and management

#include "quakedef.h"

extern	Cvar*	cl_predict_players;
extern	Cvar*	cl_predict_players2;
extern	Cvar*	cl_solid_players;

static struct predicted_player {
	int flags;
	qboolean active;
	vec3_t origin;	// predicted origin
} predicted_players[MAX_CLIENTS_QW];

/*
=========================================================================

PACKET ENTITY PARSING / LINKING

=========================================================================
*/

//========================================


/*
================
CL_AddFlagModels

Called when the CTF flags are set
================
*/
void CL_AddFlagModels (refEntity_t *ent, int team, vec3_t angles)
{
	int		i;
	float	f;
	vec3_t	v_forward, v_right, v_up;

	if (cl_flagindex == -1)
		return;

	f = 14;
	if (ent->frame >= 29 && ent->frame <= 40) {
		if (ent->frame >= 29 && ent->frame <= 34) { //axpain
			if      (ent->frame == 29) f = f + 2; 
			else if (ent->frame == 30) f = f + 8;
			else if (ent->frame == 31) f = f + 12;
			else if (ent->frame == 32) f = f + 11;
			else if (ent->frame == 33) f = f + 10;
			else if (ent->frame == 34) f = f + 4;
		} else if (ent->frame >= 35 && ent->frame <= 40) { // pain
			if      (ent->frame == 35) f = f + 2; 
			else if (ent->frame == 36) f = f + 10;
			else if (ent->frame == 37) f = f + 10;
			else if (ent->frame == 38) f = f + 8;
			else if (ent->frame == 39) f = f + 4;
			else if (ent->frame == 40) f = f + 2;
		}
	} else if (ent->frame >= 103 && ent->frame <= 118) {
		if      (ent->frame >= 103 && ent->frame <= 104) f = f + 6;  //nailattack
		else if (ent->frame >= 105 && ent->frame <= 106) f = f + 6;  //light 
		else if (ent->frame >= 107 && ent->frame <= 112) f = f + 7;  //rocketattack
		else if (ent->frame >= 112 && ent->frame <= 118) f = f + 7;  //shotattack
	}

	refEntity_t newent;
	Com_Memset(&newent, 0, sizeof(newent));

	newent.reType = RT_MODEL;
	newent.hModel = cl.model_draw[cl_flagindex];
	newent.skinNum = team;

	AngleVectors(angles, v_forward, v_right, v_up);
	v_forward[2] = -v_forward[2]; // reverse z component
	for (i=0 ; i<3 ; i++)
		newent.origin[i] = ent->origin[i] - f*v_forward[i] + 22*v_right[i];
	newent.origin[2] -= 16;

	vec3_t flag_angles;
	VectorCopy(angles, flag_angles);
	flag_angles[2] -= 45;
	CLQ1_SetRefEntAxis(&newent, flag_angles);
	R_AddRefEntityToScene(&newent);
}

/*
=============
CL_LinkPlayers

Create visible entities in the correct position
for all current players
=============
*/
void CL_LinkPlayers (void)
{
	int				j;
	q1player_info_t	*info;
	qwplayer_state_t	*state;
	qwplayer_state_t	exact;
	double			playertime;
	int				msec;
	qwframe_t			*frame;
	int				oldphysent;

	playertime = realtime - cls.qh_latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.qw_frames[cl.qh_parsecount&UPDATE_MASK_QW];

	for (j=0, info=cl.q1_players, state=frame->playerstate ; j < MAX_CLIENTS_QW 
		; j++, info++, state++)
	{
		if (state->messagenum != cl.qh_parsecount)
			continue;	// not present this frame

		// spawn light flashes, even ones coming from invisible objects
		if ((state->effects & (QWEF_BLUE | QWEF_RED)) == (QWEF_BLUE | QWEF_RED))
			CLQ1_DimLight (j, state->origin, 3);
		else if (state->effects & QWEF_BLUE)
			CLQ1_DimLight (j, state->origin, 1);
		else if (state->effects & QWEF_RED)
			CLQ1_DimLight (j, state->origin, 2);
		else if (state->effects & Q1EF_BRIGHTLIGHT)
			CLQ1_BrightLight(j, state->origin);
		else if (state->effects & Q1EF_DIMLIGHT)
			CLQ1_DimLight (j, state->origin, 0);

		// the player object never gets added
		if (j == cl.playernum)
			continue;

		if (!state->modelindex)
			continue;

		if (!Cam_DrawPlayer(j))
			continue;

		// grab an entity to fill in
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		ent.hModel = cl.model_draw[state->modelindex];
		ent.skinNum = state->skinnum;
		ent.frame = state->frame;
		if (state->modelindex == clq1_playerindex)
		{
			// use custom skin
			CLQW_HandlePlayerSkin(&ent, j);
		}

		//
		// angles
		//
		vec3_t angles;
		angles[PITCH] = -state->viewangles[PITCH]/3;
		angles[YAW] = state->viewangles[YAW];
		angles[ROLL] = 0;
		angles[ROLL] = V_CalcRoll(angles, state->velocity)*4;
		CLQ1_SetRefEntAxis(&ent, angles);

		// only predict half the move to minimize overruns
		msec = 500*(playertime - state->state_time);
		if (msec <= 0 || (!cl_predict_players->value && !cl_predict_players2->value))
		{
			VectorCopy (state->origin, ent.origin);
//Con_DPrintf ("nopredict\n");
		}
		else
		{
			// predict players movement
			if (msec > 255)
				msec = 255;
			state->command.msec = msec;
//Con_DPrintf ("predict: %i\n", msec);

			oldphysent = qh_pmove.numphysent;
			CL_SetSolidPlayers (j);
			CL_PredictUsercmd (state, &exact, &state->command, false);
			qh_pmove.numphysent = oldphysent;
			VectorCopy (exact.origin, ent.origin);
		}
		R_AddRefEntityToScene(&ent);

		if (state->effects & QWEF_FLAG1)
			CL_AddFlagModels(&ent, 0, angles);
		else if (state->effects & QWEF_FLAG2)
			CL_AddFlagModels(&ent, 1, angles);
	}
}

//======================================================================

/*
===============
CL_SetSolid

Builds all the qh_pmove physents for the current frame
===============
*/
void CL_SetSolidEntities (void)
{
	int		i;
	qwframe_t	*frame;
	qwpacket_entities_t	*pak;
	q1entity_state_t		*state;

	qh_pmove.physents[0].model = 0;
	VectorCopy (vec3_origin, qh_pmove.physents[0].origin);
	qh_pmove.physents[0].info = 0;
	qh_pmove.numphysent = 1;

	frame = &cl.qw_frames[cl.qh_parsecount &  UPDATE_MASK_QW];
	pak = &frame->packet_entities;

	for (i=0 ; i<pak->num_entities ; i++)
	{
		state = &pak->entities[i];

		if (state->modelindex < 2)
			continue;
		if (!cl.model_clip[state->modelindex])
			continue;
		qh_pmove.physents[qh_pmove.numphysent].model = cl.model_clip[state->modelindex];
		VectorCopy(state->origin, qh_pmove.physents[qh_pmove.numphysent].origin);
		qh_pmove.numphysent++;
	}

}

/*
===
Calculate the new position of players, without other player clipping

We do this to set up real player prediction.
Players are predicted twice, first without clipping other players,
then with clipping against them.
This sets up the first phase.
===
*/
void CL_SetUpPlayerPrediction(qboolean dopred)
{
	int				j;
	qwplayer_state_t	*state;
	qwplayer_state_t	exact;
	double			playertime;
	int				msec;
	qwframe_t			*frame;
	struct predicted_player *pplayer;

	playertime = realtime - cls.qh_latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.qw_frames[cl.qh_parsecount&UPDATE_MASK_QW];

	for (j=0, pplayer = predicted_players, state=frame->playerstate; 
		j < MAX_CLIENTS_QW;
		j++, pplayer++, state++) {

		pplayer->active = false;

		if (state->messagenum != cl.qh_parsecount)
			continue;	// not present this frame

		if (!state->modelindex)
			continue;

		pplayer->active = true;
		pplayer->flags = state->flags;

		// note that the local player is special, since he moves locally
		// we use his last predicted postition
		if (j == cl.playernum) {
			VectorCopy(cl.qw_frames[clc.netchan.outgoingSequence&UPDATE_MASK_QW].playerstate[cl.playernum].origin,
				pplayer->origin);
		} else {
			// only predict half the move to minimize overruns
			msec = 500*(playertime - state->state_time);
			if (msec <= 0 ||
				(!cl_predict_players->value && !cl_predict_players2->value) ||
				!dopred)
			{
				VectorCopy (state->origin, pplayer->origin);
	//Con_DPrintf ("nopredict\n");
			}
			else
			{
				// predict players movement
				if (msec > 255)
					msec = 255;
				state->command.msec = msec;
	//Con_DPrintf ("predict: %i\n", msec);

				CL_PredictUsercmd (state, &exact, &state->command, false);
				VectorCopy (exact.origin, pplayer->origin);
			}
		}
	}
}

/*
===============
CL_SetSolid

Builds all the qh_pmove physents for the current frame
Note that CL_SetUpPlayerPrediction() must be called first!
qh_pmove must be setup with world and solid entity hulls before calling
(via CL_PredictMove)
===============
*/
void CL_SetSolidPlayers (int playernum)
{
	int		j;
	struct predicted_player *pplayer;
	qhphysent_t *pent;

	if (!cl_solid_players->value)
		return;

	pent = qh_pmove.physents + qh_pmove.numphysent;

	for (j=0, pplayer = predicted_players; j < MAX_CLIENTS_QW;	j++, pplayer++) {

		if (!pplayer->active)
			continue;	// not present this frame

		// the player object never gets added
		if (j == playernum)
			continue;

		if (pplayer->flags & QWPF_DEAD)
			continue; // dead players aren't solid

		pent->model = -1;
		VectorCopy(pplayer->origin, pent->origin);
		VectorCopy(pmqh_player_mins, pent->mins);
		VectorCopy(pmqh_player_maxs, pent->maxs);
		qh_pmove.numphysent++;
		pent++;
	}
}

/*
===============
CL_EmitEntities

Builds the visedicts array for cl.time

Made up of: clients, packet_entities, nails, and tents
===============
*/
void CL_EmitEntities (void)
{
	if (cls.state != CA_ACTIVE)
		return;
	if (!cl.qh_validsequence)
		return;

	R_ClearScene();

	CL_LinkPlayers ();
	CLQW_LinkPacketEntities ();
	CLQ1_LinkProjectiles ();
	CLQ1_UpdateTEnts ();
	CLQ1_LinkStaticEntities();
}

