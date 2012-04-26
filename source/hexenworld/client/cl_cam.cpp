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

void Cam_Unlock(void);
void Cam_Lock(int playernum);
void Cam_CheckHighTarget(void);

void Cam_FinishMove(hwusercmd_t* cmd)
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
