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
// cl_main.c  -- client main loop

#include "quakedef.h"
#include "../server/public.h"
#include "../client/game/quake_hexen2/demo.h"
#include "../client/game/quake_hexen2/connection.h"

void CL_Disconnect_f(void)
{
	CL_Disconnect(true);
	SV_Shutdown("");
}

/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection(const char* host)
{
	if (com_dedicated->integer)
	{
		return;
	}

	if (clc.demoplaying)
	{
		return;
	}

	CL_Disconnect(true);

	netadr_t addr = {};
	Netchan_Setup(NS_CLIENT, &clc.netchan, addr, 0);
	cls.qh_netcon = NET_Connect(host, &clc.netchan);
	if (!cls.qh_netcon)
	{
		common->Error("CL_Connect: connect failed\n");
	}
	clc.netchan.lastReceived = net_time * 1000;
	common->DPrintf("CL_EstablishConnection: connected to %s\n", host);

	cls.qh_demonum = -1;			// not in the demo loop now
	cls.state = CA_ACTIVE;
	clc.qh_signon = 0;				// need all the signon messages before playing
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo(void)
{
	char str[1024];

	if (cls.qh_demonum == -1)
	{
		return;		// don't play demos

	}
	SCRQH_BeginLoadingPlaque();

	if (!cls.qh_demos[cls.qh_demonum][0] || cls.qh_demonum == MAX_DEMOS)
	{
		cls.qh_demonum = 0;
		if (!cls.qh_demos[cls.qh_demonum][0])
		{
			common->Printf("No demos listed with startdemos\n");
			cls.qh_demonum = -1;
			return;
		}
	}

	sprintf(str,"playdemo %s\n", cls.qh_demos[cls.qh_demonum]);
	Cbuf_InsertText(str);
	cls.qh_demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f(void)
{
	q1entity_t* ent;
	int i;

	for (i = 0,ent = clq1_entities; i < cl.qh_num_entities; i++,ent++)
	{
		common->Printf("%3i:",i);
		if (!ent->state.modelindex)
		{
			common->Printf("EMPTY\n");
			continue;
		}
		common->Printf("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",
			R_ModelName(cl.model_draw[ent->state.modelindex]),ent->state.frame, ent->state.origin[0], ent->state.origin[1], ent->state.origin[2], ent->state.angles[0], ent->state.angles[1], ent->state.angles[2]);
	}
}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer(void)
{
	int ret;

	cl.qh_oldtime = cl.qh_serverTimeFloat;
	cl.qh_serverTimeFloat += host_frametime;
	cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);

	// allocate space for network message buffer
	QMsg net_message;
	byte net_message_buf[MAX_MSGLEN_Q1];
	net_message.InitOOB(net_message_buf, MAX_MSGLEN_Q1);

	do
	{
		ret = CLQH_GetMessage(net_message);
		if (ret == -1)
		{
			common->Error("CL_ReadFromServer: lost server connection");
		}
		if (!ret)
		{
			break;
		}

		cl.qh_last_received_message = realtime;
		CLQ1_ParseServerMessage(net_message);
	}
	while (ret && cls.state == CA_ACTIVE);

	if (cl_shownet->value)
	{
		common->Printf("\n");
	}

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
void CL_Init(void)
{
	CL_SharedInit();

	CL_InitInput();

	//
	// register our commands
	//
	clqh_name = Cvar_Get("_cl_name", "player", CVAR_ARCHIVE);
	clqh_color = Cvar_Get("_cl_color", "0", CVAR_ARCHIVE);
	clqh_nolerp = Cvar_Get("cl_nolerp", "0", 0);

	cl_doubleeyes = Cvar_Get("cl_doubleeyes", "1", 0);

	Cmd_AddCommand("entities", CL_PrintEntities_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CLQH_Record_f);
	Cmd_AddCommand("stop", CLQH_Stop_f);
	Cmd_AddCommand("playdemo", CLQH_PlayDemo_f);
	Cmd_AddCommand("timedemo", CLQH_TimeDemo_f);
	Cmd_AddCommand("slist", NET_Slist_f);
}

float* CL_GetSimOrg()
{
	return clq1_entities[cl.viewentity].state.origin;
}
