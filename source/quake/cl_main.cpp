/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_main.c  -- client main loop

#include "quakedef.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

Cvar*	cl_shownet;

Cvar*	sensitivity;

Cvar*	m_pitch;
Cvar*	m_yaw;
Cvar*	m_forward;
Cvar*	m_side;

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	if (!sv.active)
		Host_ClearMemory ();

// wipe the entire cl structure
	Com_Memset(&cl, 0, sizeof(cl));

	clc.netchan.message.Clear();

// clear other arrays	
	Com_Memset(clq1_entities, 0, sizeof(clq1_entities));
	Com_Memset(clq1_baselines, 0, sizeof(clq1_baselines));
	CL_ClearDlights();
	CL_ClearLightStyles();
	CLQ1_ClearTEnts();
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
// stop sounds (especially looping!)
	S_StopAllSounds();
	
// bring the console down and fade the colors back to normal
//	SCR_BringDownConsole ();

// if running a local server, shut it down
	if (clc.demoplaying)
		CL_StopPlayback ();
	else if (cls.state == CA_CONNECTED)
	{
		if (clc.demorecording)
			CL_Stop_f ();

		Con_DPrintf ("Sending q1clc_disconnect\n");
		clc.netchan.message.Clear();
		clc.netchan.message.WriteByte(q1clc_disconnect);
		NET_SendUnreliableMessage (cls.qh_netcon, &clc.netchan, &clc.netchan.message);
		clc.netchan.message.Clear();
		NET_Close (cls.qh_netcon, &clc.netchan);

		cls.state = CA_DISCONNECTED;
		if (sv.active)
			Host_ShutdownServer(false);
	}

	clc.demoplaying = cls.qh_timedemo = false;
	clc.qh_signon = 0;
}

void CL_Disconnect_f (void)
{
	CL_Disconnect ();
	if (sv.active)
		Host_ShutdownServer (false);
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (const char *host)
{
	if (cls.state == CA_DEDICATED)
		return;

	if (clc.demoplaying)
		return;

	CL_Disconnect ();

	Com_Memset(&clc.netchan, 0, sizeof(clc.netchan));
	clc.netchan.message.InitOOB(clc.netchan.messageBuffer, 1024);
	clc.netchan.sock = NS_CLIENT;
	cls.qh_netcon = NET_Connect (host, &clc.netchan);
	if (!cls.qh_netcon)
		Host_Error ("CL_Connect: connect failed\n");
	clc.netchan.lastReceived = net_time * 1000;
	Con_DPrintf ("CL_EstablishConnection: connected to %s\n", host);
	
	cls.qh_demonum = -1;			// not in the demo loop now
	cls.state = CA_CONNECTED;
	clc.qh_signon = 0;				// need all the signon messages before playing
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.qh_demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.qh_demos[cls.qh_demonum][0] || cls.qh_demonum == MAX_DEMOS)
	{
		cls.qh_demonum = 0;
		if (!cls.qh_demos[cls.qh_demonum][0])
		{
			Con_Printf ("No demos listed with startdemos\n");
			cls.qh_demonum = -1;
			return;
		}
	}

	sprintf (str,"playdemo %s\n", cls.qh_demos[cls.qh_demonum]);
	Cbuf_InsertText (str);
	cls.qh_demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f (void)
{
	q1entity_t	*ent;
	int			i;
	
	for (i=0,ent=clq1_entities ; i<cl.qh_num_entities ; i++,ent++)
	{
		Con_Printf ("%3i:",i);
		if (!ent->state.modelindex)
		{
			Con_Printf ("EMPTY\n");
			continue;
		}
		Con_Printf ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n"
		,R_ModelName(cl.model_draw[ent->state.modelindex]),ent->state.frame, ent->state.origin[0], ent->state.origin[1], ent->state.origin[2], ent->state.angles[0], ent->state.angles[1], ent->state.angles[2]);
	}
}

bool CL_IsServerActive()
{
	return !!sv.active;
}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer (void)
{
	int		ret;

	cl.qh_oldtime = cl.qh_serverTimeFloat;
	cl.qh_serverTimeFloat += host_frametime;
	cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);
	
	do
	{
		ret = CL_GetMessage ();
		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");
		if (!ret)
			break;
		
		cl.qh_last_received_message = realtime;
		CL_ParseServerMessage ();
	} while (ret && cls.state == CA_CONNECTED);
	
	if (cl_shownet->value)
		Con_Printf ("\n");

	CLQ1_RelinkEntities ();
	CLQ1_UpdateTEnts ();
	CLQ1_LinkStaticEntities();

//
// bring the links up to date
//
	return 0;
}

/*
=================
CL_Init
=================
*/
void CL_Init (void)
{	
	CL_SharedInit();

	CL_InitInput ();

	//
	// register our commands
	//
	clqh_name = Cvar_Get("_cl_name", "player", CVAR_ARCHIVE);
	clqh_color = Cvar_Get("_cl_color", "0", CVAR_ARCHIVE);
	cl_yawspeed = Cvar_Get("cl_yawspeed", "140", 0);
	cl_pitchspeed = Cvar_Get("cl_pitchspeed", "150", 0);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);
	cl_shownet = Cvar_Get("cl_shownet", "0", 0);	// can be 0, 1, or 2
	clqh_nolerp = Cvar_Get("cl_nolerp", "0", 0);
	sensitivity = Cvar_Get("sensitivity", "3", CVAR_ARCHIVE);

	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get("m_forward", "1", CVAR_ARCHIVE);
	m_side = Cvar_Get("m_side", "0.8", CVAR_ARCHIVE);

	cl_doubleeyes = Cvar_Get("cl_doubleeyes", "1", 0);

	Cmd_AddCommand ("entities", CL_PrintEntities_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
}

void CIN_StartedPlayback()
{
}

bool CIN_IsInCinematicState()
{
	return false;
}

void CIN_FinishCinematic()
{
}

float* CL_GetSimOrg()
{
	return clq1_entities[cl.viewentity].state.origin;
}
