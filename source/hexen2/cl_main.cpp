// cl_main.c  -- client main loop

/*
 * $Header: /H2 Mission Pack/CL_MAIN.C 12    3/16/98 5:32p Jweier $
 */

#include "quakedef.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

static float save_sensitivity;

/*
===================
Mod_ClearAll
===================
*/
static void Mod_ClearAll(void)
{
	R_Shutdown(false);
	R_BeginRegistration(&cls.glconfig);

	CLH2_ClearEntityTextureArrays();
	Com_Memset(mh2_translate_texture, 0, sizeof(mh2_translate_texture));

	Draw_Init();
	SbarH2_Init();
}

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState(void)
{
	Mod_ClearAll();
	clc.qh_signon = 0;

	// wipe the entire cl structure
	Com_Memset(&cl, 0, sizeof(cl));

	clc.netchan.message.Clear();

// clear other arrays
	Com_Memset(h2cl_entities, 0, sizeof(h2cl_entities));
	Com_Memset(clh2_baselines, 0, sizeof(clh2_baselines));
	CL_ClearDlights();
	CL_ClearLightStyles();
	CLH2_ClearTEnts();
	CLH2_ClearEffects();

	cl.h2_current_frame = cl.h2_current_sequence = 99;
	cl.h2_reference_frame = cl.h2_last_sequence = 199;
	cl.h2_need_build = 2;

	plaquemessage = "";

	SbarH2_InvReset();
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on common->Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(void)
{
	CL_ClearParticles();	//jfm: need to clear parts because some now check world
	S_StopAllSounds();	// stop sounds (especially looping!)
	loading_stage = 0;

// if running a local server, shut it down
	if (clc.demoplaying)
	{
		CL_StopPlayback();
	}
	else if (cls.state == CA_ACTIVE)
	{
		if (clc.demorecording)
		{
			CL_Stop_f();
		}

		common->DPrintf("Sending h2clc_disconnect\n");
		clc.netchan.message.Clear();
		clc.netchan.message.WriteByte(h2clc_disconnect);
		NET_SendUnreliableMessage(cls.qh_netcon, &clc.netchan, &clc.netchan.message);
		clc.netchan.message.Clear();
		NET_Close(cls.qh_netcon, &clc.netchan);

		cls.state = CA_DISCONNECTED;
		if (sv.state != SS_DEAD)
		{
			SVQH_Shutdown(false);
		}

		SVH2_RemoveGIPFiles(NULL);
	}

	clc.demoplaying = cls.qh_timedemo = false;
	clc.qh_signon = 0;
}

void CL_Disconnect_f(void)
{
	CL_Disconnect();
	if (sv.state != SS_DEAD)
	{
		SVQH_Shutdown(false);
	}
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection(const char* host)
{
	if (cls.state == CA_DEDICATED)
	{
		return;
	}

	if (clc.demoplaying)
	{
		return;
	}

	CL_Disconnect();

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
	h2entity_t* ent;
	int i;

	for (i = 0,ent = h2cl_entities; i < cl.qh_num_entities; i++,ent++)
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

	do
	{
		ret = CL_GetMessage();
		if (ret == -1)
		{
			common->Error("CL_ReadFromServer: lost server connection");
		}
		if (!ret)
		{
			break;
		}

		cl.qh_last_received_message = realtime;
		CL_ParseServerMessage();
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

void CL_Sensitivity_save_f(void)
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("sensitivity_save <save/restore>\n");
		return;
	}

	if (String::ICmp(Cmd_Argv(1),"save") == 0)
	{
		save_sensitivity = cl_sensitivity->value;
	}
	else if (String::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		Cvar_SetValue("sensitivity", save_sensitivity);
	}
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
	clh2_playerclass = Cvar_Get("_cl_playerclass", "5", CVAR_ARCHIVE);
	clqh_nolerp = Cvar_Get("cl_nolerp", "0", 0);

	Cmd_AddCommand("entities", CL_PrintEntities_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("stop", CL_Stop_f);
	Cmd_AddCommand("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand("timedemo", CL_TimeDemo_f);
	Cmd_AddCommand("sensitivity_save", CL_Sensitivity_save_f);
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
	return h2cl_entities[cl.viewentity].state.origin;
}
