/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/* ZOID
 *
 * Player camera tracking in Spectator mode
 *
 * This takes over player controls for spectator automatic camera.
 * Player moves as a spectator, but the camera tracks and enemy player
 */

#include "quakedef.h"

#define	PM_SPECTATORMAXSPEED	500
#define	PM_STOPSPEED	100
#define	PM_MAXSPEED			320
#define MAX_ANGLE_TURN 10

static vec3_t desired_position; // where the camera wants to be
static qboolean locked = false;
static int oldbuttons;

// track high fragger
Cvar* cl_hightrack;

Cvar* cl_chasecam;

//Cvar* cl_camera_maxpitch;
//Cvar* cl_camera_maxyaw;

qboolean cam_forceview;
vec3_t cam_viewangles;
double cam_lastviewtime;

int spec_track = 0; // player# of who we are tracking
int autocam = CAM_NONE;

static float vlen(vec3_t v)
{
	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

// returns true if weapon model should be drawn in camera mode
qboolean Cam_DrawViewModel(void)
{
	if (!cl.qh_spectator)
		return true;

	if (autocam && locked && cl_chasecam->value)
		return true;
	return false;
}

// returns true if we should draw this player, we don't if we are chase camming
qboolean Cam_DrawPlayer(int playernum)
{
	if (cl.qh_spectator && autocam && locked && cl_chasecam->value && 
		spec_track == playernum)
		return false;
	return true;
}

void Cam_Unlock(void)
{
	if (autocam) {
		clc.netchan.message.WriteByte(q1clc_stringcmd);
		clc.netchan.message.WriteString2("ptrack");
		autocam = CAM_NONE;
		locked = false;
	}
}

void Cam_Lock(int playernum)
{
	char st[40];

	sprintf(st, "ptrack %i", playernum);
	clc.netchan.message.WriteByte(q1clc_stringcmd);
	clc.netchan.message.WriteString2(st);
	spec_track = playernum;
	cam_forceview = true;
	locked = false;
}

q1trace_t Cam_DoTrace(vec3_t vec1, vec3_t vec2)
{
	VectorCopy (vec1, qh_pmove.origin);
	return PM_PlayerMove(qh_pmove.origin, vec2);
}
	
// Returns distance or 9999 if invalid for some reason
static float Cam_TryFlyby(qwplayer_state_t *self, qwplayer_state_t *player, vec3_t vec, qboolean checkvis)
{
	vec3_t v;
	q1trace_t trace;
	float len;

	VecToAnglesBuggy(vec, v);
//	v[0] = -v[0];
	VectorCopy (v, qh_pmove.angles);
	VectorNormalize(vec);
	VectorMA(player->origin, 800, vec, v);
	// v is endpos
	// fake a player move
	trace = Cam_DoTrace(player->origin, v);
	if (/*trace.inopen ||*/ trace.inwater)
		return 9999;
	VectorCopy(trace.endpos, vec);
	VectorSubtract(trace.endpos, player->origin, v);
	len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (len < 32 || len > 800)
		return 9999;
	if (checkvis) {
		VectorSubtract(trace.endpos, self->origin, v);
		len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

		trace = Cam_DoTrace(self->origin, vec);
		if (trace.fraction != 1 || trace.inwater)
			return 9999;
	}
	return len;
}

// Is player visible?
static qboolean Cam_IsVisible(qwplayer_state_t *player, vec3_t vec)
{
	q1trace_t trace;
	vec3_t v;
	float d;

	trace = Cam_DoTrace(player->origin, vec);
	if (trace.fraction != 1 || /*trace.inopen ||*/ trace.inwater)
		return false;
	// check distance, don't let the player get too far away or too close
	VectorSubtract(player->origin, vec, v);
	d = vlen(v);
	if (d < 16)
		return false;
	return true;
}

static qboolean InitFlyby(qwplayer_state_t *self, qwplayer_state_t *player, int checkvis) 
{
    float f, max;
    vec3_t vec, vec2;
	vec3_t forward, right, up;

	VectorCopy(player->viewangles, vec);
    vec[0] = 0;
	AngleVectors (vec, forward, right, up);
//	for (i = 0; i < 3; i++)
//		forward[i] *= 3;

    max = 1000;
	VectorAdd(forward, up, vec2);
	VectorAdd(vec2, right, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorAdd(forward, up, vec2);
	VectorSubtract(vec2, right, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorAdd(forward, right, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorSubtract(forward, right, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorAdd(forward, up, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorSubtract(forward, up, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorAdd(up, right, vec2);
	VectorSubtract(vec2, forward, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorSubtract(up, right, vec2);
	VectorSubtract(vec2, forward, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	// invert
	VectorSubtract(vec3_origin, forward, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorCopy(forward, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	// invert
	VectorSubtract(vec3_origin, right, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }
	VectorCopy(right, vec2);
    if ((f = Cam_TryFlyby(self, player, vec2, checkvis)) < max) {
        max = f;
		VectorCopy(vec2, vec);
    }

	// ack, can't find him
    if (max >= 1000) {
//		Cam_Unlock();
		return false;
	}
	locked = true;
	VectorCopy(vec, desired_position); 
	return true;
}

static void Cam_CheckHighTarget(void)
{
	int i, j, max;
	q1player_info_t	*s;

	j = -1;
	for (i = 0, max = -9999; i < MAX_CLIENTS_QW; i++) {
		s = &cl.q1_players[i];
		if (s->name[0] && !s->spectator && s->frags > max) {
			max = s->frags;
			j = i;
		}
	}
	if (j >= 0) {
		if (!locked || cl.q1_players[j].frags > cl.q1_players[spec_track].frags)
			Cam_Lock(j);
	} else
		Cam_Unlock();
}
	
// ZOID
//
// Take over the user controls and track a player.
// We find a nice position to watch the player and move there
void Cam_Track(qwusercmd_t *cmd)
{
	qwplayer_state_t *player, *self;
	qwframe_t *frame;
	vec3_t vec;
	float len;

	if (!cl.qh_spectator)
		return;
	
	if (cl_hightrack->value && !locked)
		Cam_CheckHighTarget();

	if (!autocam || cls.state != CA_ACTIVE)
		return;

	if (locked && (!cl.q1_players[spec_track].name[0] || cl.q1_players[spec_track].spectator)) {
		locked = false;
		if (cl_hightrack->value)
			Cam_CheckHighTarget();
		else
			Cam_Unlock();
		return;
	}

	frame = &cl.qw_frames[clc.netchan.incomingSequence & UPDATE_MASK_QW];
	player = frame->playerstate + spec_track;
	self = frame->playerstate + cl.playernum;

	if (!locked || !Cam_IsVisible(player, desired_position)) {
		if (!locked || realtime - cam_lastviewtime > 0.1) {
			if (!InitFlyby(self, player, true))
				InitFlyby(self, player, false);
			cam_lastviewtime = realtime;
		}
	} else
		cam_lastviewtime = realtime;
	
	// couldn't track for some reason
	if (!locked || !autocam)
		return;

	if (cl_chasecam->value) {
		cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;

		VectorCopy(player->viewangles, cl.viewangles);
		VectorCopy(player->origin, desired_position);
		if (memcmp(&desired_position, &self->origin, sizeof(desired_position)) != 0) {
			clc.netchan.message.WriteByte(qwclc_tmove);
			clc.netchan.message.WriteCoord(desired_position[0]);
			clc.netchan.message.WriteCoord(desired_position[1]);
			clc.netchan.message.WriteCoord(desired_position[2]);
			// move there locally immediately
			VectorCopy(desired_position, self->origin);
		}
		self->weaponframe = player->weaponframe;

	} else {
		// Ok, move to our desired position and set our angles to view
		// the player
		VectorSubtract(desired_position, self->origin, vec);
		len = vlen(vec);
		cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;
		if (len > 16) { // close enough?
			clc.netchan.message.WriteByte(qwclc_tmove);
			clc.netchan.message.WriteCoord(desired_position[0]);
			clc.netchan.message.WriteCoord(desired_position[1]);
			clc.netchan.message.WriteCoord(desired_position[2]);
		}

		// move there locally immediately
		VectorCopy(desired_position, self->origin);
										 
		VectorSubtract(player->origin, desired_position, vec);
		VecToAngles(vec, cl.viewangles);
	}
}

void Cam_FinishMove(qwusercmd_t *cmd)
{
	int i;
	q1player_info_t	*s;
	int end;

	if (cls.state != CA_ACTIVE)
		return;

	if (!cl.qh_spectator) // only in spectator mode
		return;

	if (cmd->buttons & QHBUTTON_ATTACK) {
		if (!(oldbuttons & QHBUTTON_ATTACK)) {

			oldbuttons |= QHBUTTON_ATTACK;
			autocam++;

			if (autocam > CAM_TRACK) {
				Cam_Unlock();
				VectorCopy(cl.viewangles, cmd->angles);
				return;
			}
		} else
			return;
	} else {
		oldbuttons &= ~QHBUTTON_ATTACK;
		if (!autocam)
			return;
	}

	if (autocam && cl_hightrack->value) {
		Cam_CheckHighTarget();
		return;
	}

	if (locked) {
		if ((cmd->buttons & QHBUTTON_JUMP) && (oldbuttons & QHBUTTON_JUMP))
			return;		// don't pogo stick

		if (!(cmd->buttons & QHBUTTON_JUMP)) {
			oldbuttons &= ~QHBUTTON_JUMP;
			return;
		}
		oldbuttons |= QHBUTTON_JUMP;	// don't jump again until released
	}

//	Con_Printf("Selecting track target...\n");

	if (locked && autocam)
		end = (spec_track + 1) % MAX_CLIENTS_QW;
	else
		end = spec_track;
	i = end;
	do {
		s = &cl.q1_players[i];
		if (s->name[0] && !s->spectator) {
			Cam_Lock(i);
			return;
		}
		i = (i + 1) % MAX_CLIENTS_QW;
	} while (i != end);
	// stay on same guy?
	i = spec_track;
	s = &cl.q1_players[i];
	if (s->name[0] && !s->spectator) {
		Cam_Lock(i);
		return;
	}
	Con_Printf("No target found ...\n");
	autocam = locked = false;
}

void Cam_Reset(void)
{
	autocam = CAM_NONE;
	spec_track = 0;
}

void CL_InitCam(void)
{
	cl_hightrack = Cvar_Get("cl_hightrack", "0", 0);
	cl_chasecam = Cvar_Get("cl_chasecam", "0", 0);
	//cl_camera_maxpitch = Cvar_Get("cl_camera_maxpitch", "10", 0);
	//cl_camera_maxyaw = Cvar_Get("cl_camera_maxyaw", "30", 0);
}


