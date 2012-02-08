#include "quakedef.h"
#ifdef _WIN32 
#include "../../client/windows_shared.h"
#endif

Cvar*	cl_nopred;
Cvar*	cl_pushlatency;

extern	hwframe_t		*view_frame;
qboolean player_crouching;

/*
=================
CL_NudgePosition

If pmove.origin is in a solid position,
try nudging slightly on all axis to
allow for the cut precision of the net coordinates
=================
*/
void CL_NudgePosition (void)
{
	vec3_t	base;
	int		x, y;

	if (CM_PointContentsQ1(qh_pmove.origin, CM_ModelHull(0, 1)) == BSP29CONTENTS_EMPTY)
		return;

	VectorCopy (qh_pmove.origin, base);
	for (x=-1 ; x<=1 ; x++)
	{
		for (y=-1 ; y<=1 ; y++)
		{
			qh_pmove.origin[0] = base[0] + x * 1.0/8;
			qh_pmove.origin[1] = base[1] + y * 1.0/8;
			if (CM_PointContentsQ1(qh_pmove.origin, CM_ModelHull(0, 1)) == BSP29CONTENTS_EMPTY)
				return;
		}
	}
	Con_DPrintf ("CL_NudgePosition: stuck\n");
}

/*
==============
CL_PredictUsercmd
==============
*/
void CL_PredictUsercmd (hwplayer_state_t *from, hwplayer_state_t *to, hwusercmd_t *u, qboolean spectator)
{
	// split up very long moves
	if (u->msec > 50)
	{
		hwplayer_state_t	temp;
		hwusercmd_t	split;

		split = *u;
		split.msec /= 2;

		CL_PredictUsercmd (from, &temp, &split, spectator);
		CL_PredictUsercmd (&temp, to, &split, spectator);
		return;
	}

	//Con_Printf("O  %hd %hd %hd\n",u->forwardmove, u->sidemove, u->upmove);

	VectorCopy (from->origin, qh_pmove.origin);
//	VectorCopy (from->viewangles, qh_pmove.angles);
	VectorCopy (u->angles, qh_pmove.angles);
	VectorCopy (from->velocity, qh_pmove.velocity);

	qh_pmove.oldbuttons = from->oldbuttons;
	qh_pmove.waterjumptime = from->waterjumptime;
	qh_pmove.dead = cl.h2_v.health <= 0;
	qh_pmove.spectator = spectator;
	qh_pmove.hasted = cl.h2_v.hasted;
	qh_pmove.movetype = cl.h2_v.movetype;
	qh_pmove.teleport_time = cl.h2_v.teleport_time;

	qh_pmove.cmd.Set(*u);
	qh_pmove.crouched = player_crouching;

	PlayerMove ();
//for (i=0 ; i<3 ; i++)
//qh_pmove.origin[i] = ((int)(qh_pmove.origin[i]*8))*0.125;
	to->waterjumptime = qh_pmove.waterjumptime;
	to->oldbuttons = qh_pmove.cmd.buttons;
	VectorCopy (qh_pmove.origin, to->origin);
	VectorCopy (qh_pmove.angles, to->viewangles);
	VectorCopy (qh_pmove.velocity, to->velocity);
	to->onground = qh_pmove.onground;

	to->weaponframe = from->weaponframe;
}



/*
==============
CL_PredictMove
==============
*/
void CL_PredictMove (void)
{
	int			i;
	float		f;
	hwframe_t		*from, *to = NULL;
	int			oldphysent;

	if (cl_pushlatency->value > 0)
		Cvar_Set ("pushlatency", "0");

	cl.qh_serverTimeFloat = realtime - cls.qh_latency - cl_pushlatency->value*0.001;
	if (cl.qh_serverTimeFloat > realtime)
		cl.qh_serverTimeFloat = realtime;
	cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);

	if (cl.qh_intermission)
		return;

	if (!cl.qh_validsequence)
		return;

	if (clc.netchan.outgoingSequence - clc.netchan.incomingSequence >= UPDATE_BACKUP_HW-1)
		return;

	VectorCopy (cl.viewangles, cl.qh_simangles);

	// this is the last frame received from the server
	from = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW];
	player_crouching = ((from->playerstate[cl.playernum].flags) & PF_CROUCH)>>10;

	// we can now render a frame
	if (cls.state == CA_LOADING)
	{	// first update is the final signon stage
		char		text[1024];

		cls.state = CA_ACTIVE;
		sprintf (text, "HexenWorld: %s", cls.servername);
#ifdef _WIN32
		SetWindowText (GMainWindow, text);
#endif
	}

	if (cl_nopred->value)
	{
		VectorCopy (from->playerstate[cl.playernum].velocity, cl.qh_simvel);
		VectorCopy (from->playerstate[cl.playernum].origin, cl.qh_simorg);
		return;
	}

	// predict forward until cl.time <= to->senttime
	oldphysent = qh_pmove.numphysent;
	CL_SetSolidPlayers (cl.playernum);

//	to = &cl.frames[clc.netchan.incoming_sequence & UPDATE_MASK_HW];

	for (i=1 ; i<UPDATE_BACKUP_HW-1 && clc.netchan.incomingSequence+i <
			clc.netchan.outgoingSequence; i++)
	{
		to = &cl.hw_frames[(clc.netchan.incomingSequence+i) & UPDATE_MASK_HW];
		CL_PredictUsercmd (&from->playerstate[cl.playernum]
			, &to->playerstate[cl.playernum], &to->cmd, cl.qh_spectator);
		if (to->senttime >= cl.qh_serverTimeFloat)
			break;
		from = to;
	}

	qh_pmove.numphysent = oldphysent;

	if (i == UPDATE_BACKUP_HW-1 || !to)
		return;		// net hasn't deliver packets in a long time...

	// now interpolate some fraction of the final frame
	if (to->senttime == from->senttime)
		f = 0;
	else
	{
		f = (cl.qh_serverTimeFloat - from->senttime) / (to->senttime - from->senttime);

		if (f < 0)
			f = 0;
		if (f > 1)
			f = 1;
	}

	for (i=0 ; i<3 ; i++)
		if ( Q_fabs(from->playerstate[cl.playernum].origin[i] - to->playerstate[cl.playernum].origin[i]) > 128)
		{	// teleported, so don't lerp
			VectorCopy (to->playerstate[cl.playernum].velocity, cl.qh_simvel);
			VectorCopy (to->playerstate[cl.playernum].origin, cl.qh_simorg);
			return;
		}
		
	for (i=0 ; i<3 ; i++)
	{
		cl.qh_simorg[i] = from->playerstate[cl.playernum].origin[i] 
			+ f*(to->playerstate[cl.playernum].origin[i] - from->playerstate[cl.playernum].origin[i]);
		cl.qh_simvel[i] = from->playerstate[cl.playernum].velocity[i] 
			+ f*(to->playerstate[cl.playernum].velocity[i] - from->playerstate[cl.playernum].velocity[i]);
	}		

	/*Con_Printf("(%5.2f %5.2f %5.2f)  (%5.2f %5.2f %5.2f)\n",
		cl.simorg[0] - to->playerstate[cl.playernum].origin[0],
		cl.simorg[1] - to->playerstate[cl.playernum].origin[1],
		cl.simorg[2] - to->playerstate[cl.playernum].origin[2],
		cl.simvel[0] - to->playerstate[cl.playernum].velocity[0],
		cl.simvel[1] - to->playerstate[cl.playernum].velocity[1],
		cl.simvel[2] - to->playerstate[cl.playernum].velocity[2]);*/
}


/*
==============
CL_InitPrediction
==============
*/
void CL_InitPrediction (void)
{
	cl_nopred = Cvar_Get("cl_nopred","0", 0);
	cl_pushlatency = Cvar_Get("pushlatency","-50", CVAR_ARCHIVE);
}

