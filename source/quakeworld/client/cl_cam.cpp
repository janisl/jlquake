/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
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

extern bool locked;
extern int oldbuttons;

// track high fragger
extern Cvar* cl_hightrack;

extern Cvar* cl_chasecam;

// returns true if weapon model should be drawn in camera mode
qboolean Cam_DrawViewModel(void)
{
	if (!cl.qh_spectator)
	{
		return true;
	}

	if (autocam && locked && cl_chasecam->value)
	{
		return true;
	}
	return false;
}

// returns true if we should draw this player, we don't if we are chase camming
qboolean Cam_DrawPlayer(int playernum)
{
	if (cl.qh_spectator && autocam && locked && cl_chasecam->value &&
		spec_track == playernum)
	{
		return false;
	}
	return true;
}

void Cam_Unlock(void);
void Cam_Lock(int playernum);
void Cam_CheckHighTarget(void);

void Cam_FinishMove(qwusercmd_t* cmd)
{
	int i;
	int end;

	if (cls.state != CA_ACTIVE || (GGameType & GAME_HexenWorld && clqh_server_version < 1.57))
	{
		return;
	}

	if (!cl.qh_spectator)	// only in spectator mode
	{
		return;
	}

	if (cmd->buttons & QHBUTTON_ATTACK)
	{
		if (!(oldbuttons & QHBUTTON_ATTACK))
		{

			oldbuttons |= QHBUTTON_ATTACK;
			autocam++;

			if (autocam > CAM_TRACK)
			{
				Cam_Unlock();
				VectorCopy(cl.viewangles, cmd->angles);
				return;
			}
		}
		else
		{
			return;
		}
	}
	else
	{
		oldbuttons &= ~QHBUTTON_ATTACK;
		if (!autocam)
		{
			return;
		}
	}

	if (autocam && cl_hightrack->value)
	{
		Cam_CheckHighTarget();
		return;
	}

	if (locked)
	{
		if ((cmd->buttons & QHBUTTON_JUMP) && (oldbuttons & QHBUTTON_JUMP))
		{
			return;		// don't pogo stick

		}
		if (!(cmd->buttons & QHBUTTON_JUMP))
		{
			oldbuttons &= ~QHBUTTON_JUMP;
			return;
		}
		oldbuttons |= QHBUTTON_JUMP;	// don't jump again until released
	}

//	Con_Printf("Selecting track target...\n");

	if (locked && autocam)
	{
		end = (spec_track + 1) % (GGameType & GAME_HexenWorld ? HWMAX_CLIENTS : MAX_CLIENTS_QW);
	}
	else
	{
		end = spec_track;
	}
	i = end;
	do
	{
		if (GGameType & GAME_HexenWorld)
		{
			h2player_info_t* s = &cl.h2_players[i];
			if (s->name[0] && !s->spectator)
			{
				Cam_Lock(i);
				return;
			}
		}
		else
		{
			q1player_info_t* s = &cl.q1_players[i];
			if (s->name[0] && !s->spectator)
			{
				Cam_Lock(i);
				return;
			}
		}
		i = (i + 1) % (GGameType & GAME_HexenWorld ? HWMAX_CLIENTS : MAX_CLIENTS_QW);
	}
	while (i != end);
	// stay on same guy?
	i = spec_track;
	if (GGameType & GAME_HexenWorld)
	{
		h2player_info_t* s = &cl.h2_players[i];
		if (s->name[0] && !s->spectator)
		{
			Cam_Lock(i);
			return;
		}
	}
	else
	{
		q1player_info_t* s = &cl.q1_players[i];
		if (s->name[0] && !s->spectator)
		{
			Cam_Lock(i);
			return;
		}
	}
	Con_Printf("No target found ...\n");
	autocam = locked = false;
}
