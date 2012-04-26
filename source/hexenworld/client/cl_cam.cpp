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
	h2player_info_t* s;
	int end;

	if (cls.state != CA_ACTIVE || server_version < 1.57)
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
		end = (spec_track + 1) % HWMAX_CLIENTS;
	}
	else
	{
		end = spec_track;
	}
	i = end;
	do
	{
		s = &cl.h2_players[i];
		if (s->name[0] && !s->spectator)
		{
			Cam_Lock(i);
			return;
		}
		i = (i + 1) % HWMAX_CLIENTS;
	}
	while (i != end);
	// stay on same guy?
	i = spec_track;
	s = &cl.h2_players[i];
	if (s->name[0] && !s->spectator)
	{
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
}
