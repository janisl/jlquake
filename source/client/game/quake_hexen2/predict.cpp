//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client_main.h"
#include "predict.h"

struct predicted_player
{
	int flags;
	bool active;
	vec3_t origin;	// predicted origin
};

static bool player_crouching;

static Cvar* cl_nopred;
static Cvar* cl_pushlatency;
static Cvar* cl_solid_players;
Cvar* cl_predict_players;
Cvar* cl_predict_players2;

static predicted_player predicted_players[MAX_CLIENTS_QHW];

//	Builds all the qh_pmove physents for the current frame
void CLQW_SetSolidEntities()
{
	qh_pmove.physents[0].model = 0;
	VectorCopy(vec3_origin, qh_pmove.physents[0].origin);
	qh_pmove.physents[0].info = 0;
	qh_pmove.numphysent = 1;

	qwframe_t* frame = &cl.qw_frames[cl.qh_parsecount &  UPDATE_MASK_QW];
	qwpacket_entities_t* pak = &frame->packet_entities;

	for (int i = 0; i < pak->num_entities; i++)
	{
		q1entity_state_t* state = &pak->entities[i];

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
		qh_pmove.numphysent++;
	}
}

//	Builds all the qh_pmove physents for the current frame
void CLHW_SetSolidEntities()
{
	qh_pmove.physents[0].model = 0;
	VectorCopy(vec3_origin, qh_pmove.physents[0].origin);
	qh_pmove.physents[0].info = 0;
	qh_pmove.numphysent = 1;

	hwframe_t* frame = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW];
	hwpacket_entities_t* pak = &frame->packet_entities;

	for (int i = 0; i < pak->num_entities; i++)
	{
		h2entity_state_t* state = &pak->entities[i];

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

void CLQW_PredictUsercmd(qwplayer_state_t* from, qwplayer_state_t* to, qwusercmd_t* u, bool spectator)
{
	// split up very long moves
	if (u->msec > 50)
	{
		qwplayer_state_t temp;
		qwusercmd_t split;

		split = *u;
		split.msec /= 2;

		CLQW_PredictUsercmd(from, &temp, &split, spectator);
		CLQW_PredictUsercmd(&temp, to, &split, spectator);
		return;
	}

	VectorCopy(from->origin, qh_pmove.origin);
	VectorCopy(u->angles, qh_pmove.angles);
	VectorCopy(from->velocity, qh_pmove.velocity);

	qh_pmove.oldbuttons = from->oldbuttons;
	qh_pmove.waterjumptime = from->waterjumptime;
	qh_pmove.dead = cl.qh_stats[Q1STAT_HEALTH] <= 0;
	qh_pmove.spectator = spectator;

	qh_pmove.cmd.Set(*u);

	PMQH_PlayerMove();
	to->waterjumptime = qh_pmove.waterjumptime;
	to->oldbuttons = qh_pmove.cmd.buttons;
	VectorCopy(qh_pmove.origin, to->origin);
	VectorCopy(qh_pmove.angles, to->viewangles);
	VectorCopy(qh_pmove.velocity, to->velocity);
	to->onground = qh_pmove.onground;

	to->weaponframe = from->weaponframe;
}

void CLHW_PredictUsercmd(hwplayer_state_t* from, hwplayer_state_t* to, hwusercmd_t* u, bool spectator)
{
	// split up very long moves
	if (u->msec > 50)
	{
		hwplayer_state_t temp;
		hwusercmd_t split;

		split = *u;
		split.msec /= 2;

		CLHW_PredictUsercmd(from, &temp, &split, spectator);
		CLHW_PredictUsercmd(&temp, to, &split, spectator);
		return;
	}

	VectorCopy(from->origin, qh_pmove.origin);
	VectorCopy(u->angles, qh_pmove.angles);
	VectorCopy(from->velocity, qh_pmove.velocity);

	qh_pmove.oldbuttons = from->oldbuttons;
	qh_pmove.waterjumptime = from->waterjumptime;
	qh_pmove.dead = cl.h2_v.health <= 0;
	qh_pmove.spectator = spectator;
	qh_pmove.hasted = cl.h2_v.hasted;
	qh_pmove.movetype = cl.h2_v.movetype;
	qh_pmove.teleport_time = cl.h2_v.teleport_time - cls.realtime / 1000.0;

	qh_pmove.cmd.Set(*u);
	qh_pmove.crouched = player_crouching;

	PMQH_PlayerMove();
	to->waterjumptime = qh_pmove.waterjumptime;
	to->oldbuttons = qh_pmove.cmd.buttons;
	VectorCopy(qh_pmove.origin, to->origin);
	VectorCopy(qh_pmove.angles, to->viewangles);
	VectorCopy(qh_pmove.velocity, to->velocity);
	to->onground = qh_pmove.onground;

	to->weaponframe = from->weaponframe;
}

/*
Calculate the new position of players, without other player clipping

We do this to set up real player prediction.
Players are predicted twice, first without clipping other players,
then with clipping against them.
This sets up the first phase.
*/
void CLQW_SetUpPlayerPrediction(bool dopred)
{
	int j;
	qwplayer_state_t* state;
	qwplayer_state_t exact;
	double playertime;
	int msec;
	qwframe_t* frame;
	struct predicted_player* pplayer;

	playertime = cls.realtime * 0.001 - cls.qh_latency + 0.02;
	if (playertime > cls.realtime * 0.001)
	{
		playertime = cls.realtime * 0.001;
	}

	frame = &cl.qw_frames[cl.qh_parsecount & UPDATE_MASK_QW];

	for (j = 0, pplayer = predicted_players, state = frame->playerstate;
		 j < MAX_CLIENTS_QHW;
		 j++, pplayer++, state++)
	{

		pplayer->active = false;

		if (state->messagenum != cl.qh_parsecount)
		{
			continue;	// not present this frame

		}
		if (!state->modelindex)
		{
			continue;
		}

		pplayer->active = true;
		pplayer->flags = state->flags;

		// note that the local player is special, since he moves locally
		// we use his last predicted postition
		if (j == cl.playernum)
		{
			VectorCopy(cl.qw_frames[clc.netchan.outgoingSequence & UPDATE_MASK_QW].playerstate[cl.playernum].origin,
				pplayer->origin);
		}
		else
		{
			// only predict half the move to minimize overruns
			msec = 500 * (playertime - state->state_time);
			if (msec <= 0 ||
				(!cl_predict_players->value && !cl_predict_players2->value) ||
				!dopred)
			{
				VectorCopy(state->origin, pplayer->origin);
				//common->DPrintf ("nopredict\n");
			}
			else
			{
				// predict players movement
				if (msec > 255)
				{
					msec = 255;
				}
				state->command.msec = msec;
				//common->DPrintf ("predict: %i\n", msec);

				CLQW_PredictUsercmd(state, &exact, &state->command, false);
				VectorCopy(exact.origin, pplayer->origin);
			}
		}
	}
}

/*
Calculate the new position of players, without other player clipping

We do this to set up real player prediction.
Players are predicted twice, first without clipping other players,
then with clipping against them.
This sets up the first phase.
*/
void CLHW_SetUpPlayerPrediction(bool dopred)
{
	int j;

	hwplayer_state_t* state;
	hwplayer_state_t exact;
	double playertime;
	int msec;
	hwframe_t* frame;
	struct predicted_player* pplayer;

	playertime = cls.realtime * 0.001 - cls.qh_latency + 0.02;
	if (playertime > cls.realtime * 0.001)
	{
		playertime = cls.realtime * 0.001;
	}

	frame = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW];

	for (j = 0, pplayer = predicted_players, state = frame->playerstate;
		 j < MAX_CLIENTS_QHW;
		 j++, pplayer++, state++)
	{

		pplayer->active = false;

		if (state->messagenum != cl.qh_parsecount)
		{
			continue;	// not present this frame

		}
		if (!state->modelindex)
		{
			continue;
		}

		pplayer->active = true;
		pplayer->flags = state->flags;

		// note that the local player is special, since he moves locally
		// we use his last predicted postition
		if (j == cl.playernum)
		{
			VectorCopy(cl.hw_frames[clc.netchan.outgoingSequence & UPDATE_MASK_HW].playerstate[cl.playernum].origin,
				pplayer->origin);
		}
		else
		{
			// only predict half the move to minimize overruns
			msec = 500 * (playertime - state->state_time);
			if (msec <= 0 ||
				(!cl_predict_players->value && !cl_predict_players2->value) ||
				!dopred)
			{
				VectorCopy(state->origin, pplayer->origin);
				//common->DPrintf ("nopredict\n");
			}
			else
			{
				// predict players movement
				if (msec > 255)
				{
					msec = 255;
				}
				state->command.msec = msec;
				//common->DPrintf ("predict: %i\n", msec);

				CLHW_PredictUsercmd(state, &exact, &state->command, false);
				VectorCopy(exact.origin, pplayer->origin);
			}
		}
	}
}

/*
Builds all the qh_pmove physents for the current frame
Note that CL_SetUpPlayerPrediction() must be called first!
qh_pmove must be setup with world and solid entity hulls before calling
(via CLQW_PredictMove)
*/
void CLQHW_SetSolidPlayers(int playernum)
{
	int j;
	struct predicted_player* pplayer;
	qhphysent_t* pent;

	if (!cl_solid_players->value)
	{
		return;
	}

	pent = qh_pmove.physents + qh_pmove.numphysent;

	for (j = 0, pplayer = predicted_players; j < MAX_CLIENTS_QHW; j++, pplayer++)
	{

		if (!pplayer->active)
		{
			continue;	// not present this frame

		}
		// the player object never gets added
		if (j == playernum)
		{
			continue;
		}

		if (pplayer->flags & (GGameType & GAME_Hexen2 ? HWPF_DEAD : QWPF_DEAD))
		{
			continue;	// dead players aren't solid

		}
		pent->model = -1;
		VectorCopy(pplayer->origin, pent->origin);
		VectorCopy(pmqh_player_mins, pent->mins);
		if (GGameType & GAME_Hexen2 && pplayer->flags & HWPF_CROUCH)
		{
			VectorCopy(pmqh_player_maxs_crouch, pent->maxs);
		}
		else
		{
			VectorCopy(pmqh_player_maxs, pent->maxs);
		}
		qh_pmove.numphysent++;
		pent++;
	}
}

void CLQW_PredictMove()
{
	if (cl_pushlatency->value > 0)
	{
		Cvar_Set("pushlatency", "0");
	}

	if (GGameType & GAME_QuakeWorld && cl.qh_paused)
	{
		return;
	}

	cl.serverTime = cls.realtime - cls.qh_latency * 1000 - cl_pushlatency->integer;
	if (cl.serverTime > cls.realtime)
	{
		cl.serverTime = cls.realtime;
	}
	cl.qh_serverTimeFloat = cl.serverTime * 0.001;

	if (cl.qh_intermission)
	{
		return;
	}

	if (!cl.qh_validsequence)
	{
		return;
	}

	if (clc.netchan.outgoingSequence - clc.netchan.incomingSequence >= UPDATE_BACKUP_QW - 1)
	{
		return;
	}

	VectorCopy(cl.viewangles, cl.qh_simangles);

	// this is the last frame received from the server
	qwframe_t* from = &cl.qw_frames[clc.netchan.incomingSequence & UPDATE_MASK_QW];

	// we can now render a frame
	if (cls.state == CA_LOADING)
	{
		// first update is the final signon stage
		cls.state = CA_ACTIVE;
	}

	if (cl_nopred->value)
	{
		VectorCopy(from->playerstate[cl.playernum].velocity, cl.qh_simvel);
		VectorCopy(from->playerstate[cl.playernum].origin, cl.qh_simorg);
		return;
	}

	// predict forward until cl.time <= to->senttime
	int oldphysent = qh_pmove.numphysent;
	CLQHW_SetSolidPlayers(cl.playernum);

	int i;
	qwframe_t* to = NULL;
	for (i = 1; i < UPDATE_BACKUP_QW - 1 && clc.netchan.incomingSequence + i <
		 clc.netchan.outgoingSequence; i++)
	{
		to = &cl.qw_frames[(clc.netchan.incomingSequence + i) & UPDATE_MASK_QW];
		CLQW_PredictUsercmd(&from->playerstate[cl.playernum],
			&to->playerstate[cl.playernum], &to->cmd, cl.qh_spectator);
		if (to->senttime >= cl.qh_serverTimeFloat)
		{
			break;
		}
		from = to;
	}

	qh_pmove.numphysent = oldphysent;

	if (i == UPDATE_BACKUP_QW - 1 || !to)
	{
		return;		// net hasn't deliver packets in a long time...

	}
	// now interpolate some fraction of the final frame
	float f;
	if (to->senttime == from->senttime)
	{
		f = 0;
	}
	else
	{
		f = (cl.qh_serverTimeFloat - from->senttime) / (to->senttime - from->senttime);

		if (f < 0)
		{
			f = 0;
		}
		if (f > 1)
		{
			f = 1;
		}
	}

	for (i = 0; i < 3; i++)
	{
		if (Q_fabs(from->playerstate[cl.playernum].origin[i] - to->playerstate[cl.playernum].origin[i]) > 128)
		{	// teleported, so don't lerp
			VectorCopy(to->playerstate[cl.playernum].velocity, cl.qh_simvel);
			VectorCopy(to->playerstate[cl.playernum].origin, cl.qh_simorg);
			return;
		}
	}

	for (i = 0; i < 3; i++)
	{
		cl.qh_simorg[i] = from->playerstate[cl.playernum].origin[i] +
			f * (to->playerstate[cl.playernum].origin[i] - from->playerstate[cl.playernum].origin[i]);
		cl.qh_simvel[i] = from->playerstate[cl.playernum].velocity[i] +
			f * (to->playerstate[cl.playernum].velocity[i] - from->playerstate[cl.playernum].velocity[i]);
	}
}

void CLHW_PredictMove()
{
	if (cl_pushlatency->value > 0)
	{
		Cvar_Set("pushlatency", "0");
	}

	if (GGameType & GAME_QuakeWorld && cl.qh_paused)
	{
		return;
	}

	cl.serverTime = cls.realtime - cls.qh_latency * 1000 - cl_pushlatency->integer;
	if (cl.serverTime > cls.realtime)
	{
		cl.serverTime = cls.realtime;
	}
	cl.qh_serverTimeFloat = cl.serverTime * 0.001;

	if (cl.qh_intermission)
	{
		return;
	}

	if (!cl.qh_validsequence)
	{
		return;
	}

	if (clc.netchan.outgoingSequence - clc.netchan.incomingSequence >= UPDATE_BACKUP_HW - 1)
	{
		return;
	}

	VectorCopy(cl.viewangles, cl.qh_simangles);

	// this is the last frame received from the server
	hwframe_t* from = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW];
	player_crouching = ((from->playerstate[cl.playernum].flags) & HWPF_CROUCH) >> 10;

	// we can now render a frame
	if (cls.state == CA_LOADING)
	{
		// first update is the final signon stage
		cls.state = CA_ACTIVE;
	}

	if (cl_nopred->value)
	{
		VectorCopy(from->playerstate[cl.playernum].velocity, cl.qh_simvel);
		VectorCopy(from->playerstate[cl.playernum].origin, cl.qh_simorg);
		return;
	}

	// predict forward until cl.time <= to->senttime
	int oldphysent = qh_pmove.numphysent;
	CLQHW_SetSolidPlayers(cl.playernum);

	int i;
	hwframe_t* to = NULL;
	for (i = 1; i < UPDATE_BACKUP_HW - 1 && clc.netchan.incomingSequence + i <
		 clc.netchan.outgoingSequence; i++)
	{
		to = &cl.hw_frames[(clc.netchan.incomingSequence + i) & UPDATE_MASK_HW];
		CLHW_PredictUsercmd(&from->playerstate[cl.playernum],
			&to->playerstate[cl.playernum], &to->cmd, cl.qh_spectator);
		if (to->senttime >= cl.qh_serverTimeFloat)
		{
			break;
		}
		from = to;
	}

	qh_pmove.numphysent = oldphysent;

	if (i == UPDATE_BACKUP_HW - 1 || !to)
	{
		return;		// net hasn't deliver packets in a long time...

	}
	// now interpolate some fraction of the final frame
	float f;
	if (to->senttime == from->senttime)
	{
		f = 0;
	}
	else
	{
		f = (cl.qh_serverTimeFloat - from->senttime) / (to->senttime - from->senttime);

		if (f < 0)
		{
			f = 0;
		}
		if (f > 1)
		{
			f = 1;
		}
	}

	for (i = 0; i < 3; i++)
		if (Q_fabs(from->playerstate[cl.playernum].origin[i] - to->playerstate[cl.playernum].origin[i]) > 128)
		{	// teleported, so don't lerp
			VectorCopy(to->playerstate[cl.playernum].velocity, cl.qh_simvel);
			VectorCopy(to->playerstate[cl.playernum].origin, cl.qh_simorg);
			return;
		}

	for (i = 0; i < 3; i++)
	{
		cl.qh_simorg[i] = from->playerstate[cl.playernum].origin[i] +
			f * (to->playerstate[cl.playernum].origin[i] - from->playerstate[cl.playernum].origin[i]);
		cl.qh_simvel[i] = from->playerstate[cl.playernum].velocity[i] +
			f * (to->playerstate[cl.playernum].velocity[i] - from->playerstate[cl.playernum].velocity[i]);
	}
}

void CLQHW_InitPrediction()
{
	cl_nopred = Cvar_Get("cl_nopred", "0", 0);
	if (GGameType & GAME_Hexen2)
	{
		cl_pushlatency = Cvar_Get("pushlatency", "-50", CVAR_ARCHIVE);
	}
	else
	{
		cl_pushlatency = Cvar_Get("pushlatency", "-999", 0);
	}
	cl_solid_players = Cvar_Get("cl_solid_players", "1", 0);
	cl_predict_players = Cvar_Get("cl_predict_players", "1", 0);
	cl_predict_players2 = Cvar_Get("cl_predict_players2", "1", 0);
}
