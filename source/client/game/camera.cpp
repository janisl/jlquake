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

#include "../client.h"

int spec_track = 0;	// player# of who we are tracking
int autocam = CAM_NONE;

static vec3_t desired_position;	// where the camera wants to be
bool locked = false;
int oldbuttons;

// track high fragger
Cvar* cl_hightrack;

Cvar* cl_chasecam;

static int cam_lastviewtime;

void CL_InitCam()
{
	cl_hightrack = Cvar_Get("cl_hightrack", "0", 0);
	cl_chasecam = Cvar_Get("cl_chasecam", "0", 0);
}

void Cam_Reset()
{
	autocam = CAM_NONE;
	spec_track = 0;
}

void Cam_Lock(int playernum)
{
	char st[40];
	String::Sprintf(st, sizeof(st), "ptrack %i", playernum);
	clc.netchan.message.WriteByte(GGameType & GAME_Hexen2 ? h2clc_stringcmd : q1clc_stringcmd);
	clc.netchan.message.WriteString2(st);
	spec_track = playernum;
	locked = false;
}

void Cam_Unlock()
{
	if (!autocam)
	{
		return;
	}
	clc.netchan.message.WriteByte(GGameType & GAME_Hexen2 ? h2clc_stringcmd : q1clc_stringcmd);
	clc.netchan.message.WriteString2("ptrack");
	autocam = CAM_NONE;
	locked = false;
}

static void Cam_CheckHighTargetQW()
{
	int j = -1;
	int max = -9999;
	for (int i = 0; i < MAX_CLIENTS_QW; i++)
	{
		q1player_info_t* s = &cl.q1_players[i];
		if (s->name[0] && !s->spectator && s->frags > max)
		{
			max = s->frags;
			j = i;
		}
	}
	if (j >= 0)
	{
		if (!locked || cl.q1_players[j].frags > cl.q1_players[spec_track].frags)
		{
			Cam_Lock(j);
		}
	}
	else
	{
		Cam_Unlock();
	}
}

static void Cam_CheckHighTargetHW()
{
	int j = -1;
	int max = -9999;
	for (int i = 0; i < HWMAX_CLIENTS; i++)
	{
		h2player_info_t* s = &cl.h2_players[i];
		if (s->name[0] && !s->spectator && s->frags > max)
		{
			max = s->frags;
			j = i;
		}
	}
	if (j >= 0)
	{
		if (!locked || cl.h2_players[j].frags > cl.h2_players[spec_track].frags)
		{
			Cam_Lock(j);
		}
	}
	else
	{
		Cam_Unlock();
	}
}

void Cam_CheckHighTarget()
{
	if (GGameType & GAME_Hexen2)
	{
		Cam_CheckHighTargetHW();
	}
	else
	{
		Cam_CheckHighTargetQW();
	}
}

static q1trace_t Cam_DoTrace(const vec3_t vec1, vec3_t vec2)
{
	VectorCopy(vec1, qh_pmove.origin);
	return PMQH_TestPlayerMove(qh_pmove.origin, vec2);
}

// Is player visible?
static bool Cam_IsVisible(const vec3_t origin, vec3_t vec)
{
	q1trace_t trace = Cam_DoTrace(origin, vec);
	if (trace.fraction != 1 || trace.inwater)
	{
		return false;
	}
	// check distance, don't let the player get too far away or too close
	vec3_t v;
	VectorSubtract(origin, vec, v);
	float d = VectorLength(v);
	if (d < 16)
	{
		return false;
	}
	return true;
}

// Returns distance or 9999 if invalid for some reason
static float Cam_TryFlyby(const vec3_t self_origin, const vec3_t player_origin, vec3_t vec, bool checkvis)
{
	vec3_t v;
	VecToAnglesBuggy(vec, v);
	VectorCopy(v, qh_pmove.angles);
	VectorNormalize(vec);
	VectorMA(player_origin, 800, vec, v);
	// v is endpos
	// fake a player move
	q1trace_t trace = Cam_DoTrace(player_origin, v);
	if (trace.inwater)
	{
		return 9999;
	}
	VectorCopy(trace.endpos, vec);
	VectorSubtract(trace.endpos, player_origin, v);
	float len = VectorLength(v);
	if (len < 32 || len > 800)
	{
		return 9999;
	}
	if (checkvis)
	{
		VectorSubtract(trace.endpos, self_origin, v);
		len = VectorLength(v);

		trace = Cam_DoTrace(self_origin, vec);
		if (trace.fraction != 1 || trace.inwater)
		{
			return 9999;
		}
	}
	return len;
}

static bool InitFlyby(const vec3_t self_origin, const vec3_t player_origin,
	const vec3_t player_viewangles, bool checkvis)
{
	vec3_t vec;
	VectorCopy(player_viewangles, vec);
	vec[0] = 0;
	vec3_t forward, right, up;
	AngleVectors(vec, forward, right, up);

	float max = 1000;
	vec3_t vec2;
	VectorAdd(forward, up, vec2);
	VectorAdd(vec2, right, vec2);
	float f;
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorAdd(forward, up, vec2);
	VectorSubtract(vec2, right, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorAdd(forward, right, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorSubtract(forward, right, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorAdd(forward, up, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorSubtract(forward, up, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorAdd(up, right, vec2);
	VectorSubtract(vec2, forward, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorSubtract(up, right, vec2);
	VectorSubtract(vec2, forward, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	// invert
	VectorSubtract(vec3_origin, forward, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorCopy(forward, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	// invert
	VectorSubtract(vec3_origin, right, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}
	VectorCopy(right, vec2);
	if ((f = Cam_TryFlyby(self_origin, player_origin, vec2, checkvis)) < max)
	{
		max = f;
		VectorCopy(vec2, vec);
	}

	// ack, can't find him
	if (max >= 1000)
	{
		return false;
	}
	locked = true;
	VectorCopy(vec, desired_position);
	return true;
}

void ExecuteTrack(vec3_t self_origin, int& self_weaponframe, const vec3_t player_origin, const vec3_t player_viewangles, int player_weaponframe, in_usercmd_t* cmd)
{
	if (!locked || !Cam_IsVisible(player_origin, desired_position))
	{
		if (!locked || cls.realtime - cam_lastviewtime > 100)
		{
			if (!InitFlyby(self_origin, player_origin, player_viewangles, true))
			{
				InitFlyby(self_origin, player_origin, player_viewangles, false);
			}
			cam_lastviewtime = cls.realtime;
		}
	}
	else
	{
		cam_lastviewtime = cls.realtime;
	}

	// couldn't track for some reason
	if (!locked || !autocam)
	{
		return;
	}

	cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;

	if (cl_chasecam->value)
	{
		VectorCopy(player_viewangles, cl.viewangles);
		VectorCopy(player_origin, desired_position);
		if (memcmp(&desired_position, &self_origin, sizeof(desired_position)) != 0)
		{
			clc.netchan.message.WriteByte(GGameType & GAME_Hexen2 ? hwclc_tmove : qwclc_tmove);
			clc.netchan.message.WriteCoord(desired_position[0]);
			clc.netchan.message.WriteCoord(desired_position[1]);
			clc.netchan.message.WriteCoord(desired_position[2]);
			// move there locally immediately
			VectorCopy(desired_position, self_origin);
		}
		self_weaponframe = player_weaponframe;

	}
	else
	{
		// Ok, move to our desired position and set our angles to view
		// the player
		vec3_t vec;
		VectorSubtract(desired_position, self_origin, vec);
		float len = VectorLength(vec);
		if (len > 16)	// close enough?
		{
			clc.netchan.message.WriteByte(GGameType & GAME_Hexen2 ? hwclc_tmove : qwclc_tmove);
			clc.netchan.message.WriteCoord(desired_position[0]);
			clc.netchan.message.WriteCoord(desired_position[1]);
			clc.netchan.message.WriteCoord(desired_position[2]);
		}

		// move there locally immediately
		VectorCopy(desired_position, self_origin);

		VectorSubtract(player_origin, desired_position, vec);
		VecToAngles(vec, cl.viewangles);
	}
}

// ZOID
//
// Take over the user controls and track a player.
// We find a nice position to watch the player and move there
void Cam_Track(in_usercmd_t* cmd)
{
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		return;
	}
	if (!cl.qh_spectator)
	{
		return;
	}

	if (cl_hightrack->value && !locked)
	{
		Cam_CheckHighTarget();
	}

	if (!autocam || cls.state != CA_ACTIVE)
	{
		return;
	}

	if (locked &&
		((GGameType & GAME_Quake && (!cl.q1_players[spec_track].name[0] || cl.q1_players[spec_track].spectator)) ||
		(GGameType & GAME_Hexen2 && (!cl.h2_players[spec_track].name[0] || cl.h2_players[spec_track].spectator))))
	{
		locked = false;
		if (cl_hightrack->value)
		{
			Cam_CheckHighTarget();
		}
		else
		{
			Cam_Unlock();
		}
		return;
	}

	if (GGameType & GAME_Hexen2)
	{
		hwframe_t* frame = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW];
		hwplayer_state_t* player = frame->playerstate + spec_track;
		hwplayer_state_t* self = frame->playerstate + cl.playernum;

		ExecuteTrack(self->origin, self->weaponframe, player->origin, player->viewangles, player->weaponframe, cmd);
	}
	else
	{
		qwframe_t* frame = &cl.qw_frames[clc.netchan.incomingSequence & UPDATE_MASK_QW];
		qwplayer_state_t* player = frame->playerstate + spec_track;
		qwplayer_state_t* self = frame->playerstate + cl.playernum;

		ExecuteTrack(self->origin, self->weaponframe, player->origin, player->viewangles, player->weaponframe, cmd);
	}
}
