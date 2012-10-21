/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "client.h"
#include "../../common/file_formats/md2.h"
#include "../../server/public.h"

Cvar* cl_autoskins;
Cvar* cl_maxfps;

static bool vid_restart_requested;

/*
==================
CL_Pause_f
==================
*/
void CL_Pause_f(void)
{
	// never pause in multiplayer
	if (Cvar_VariableValue("maxclients") > 1 || !ComQ2_ServerState())
	{
		Cvar_SetValueLatched("paused", 0);
		return;
	}

	Cvar_SetValueLatched("paused", !cl_paused->value);
}

/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f(void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (clc.download)
	{
		return;
	}

	SCRQ2_BeginLoadingPlaque(false);
	cls.state = CA_CONNECTED;	// not active anymore, but not disconnected
	common->Printf("\nChanging map...\n");
}

/*
=================
CL_Skins_f

Load or download any custom player skins and models
=================
*/
void CL_Skins_f(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS_Q2; i++)
	{
		if (!cl.q2_configstrings[Q2CS_PLAYERSKINS + i][0])
		{
			continue;
		}
		common->Printf("client %i: %s\n", i, cl.q2_configstrings[Q2CS_PLAYERSKINS + i]);
		SCR_UpdateScreen();
		Com_EventLoop();	// pump message loop
		CLQ2_ParseClientinfo(i);
	}
}

//=============================================================================

/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f(void)
{
	common->Printf("User info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
			MAX_INFO_VALUE_Q2, true, false));
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem so it can pick up
new parameters and flush all sounds
=================
*/
void CL_Snd_Restart_f(void)
{
	S_Shutdown();
	S_Init();
	CLQ2_RegisterSounds();
}

//	The server will send this command right
// before allowing the client into the server
void CLQ2_Precache_f()
{
	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2)
	{
		int map_checksum;		// for detecting cheater maps
		CM_LoadMap(cl.q2_configstrings[Q2CS_MODELS + 1], true, &map_checksum);
		CLQ2_RegisterSounds();
		CLQ2_PrepRefresh();
		return;
	}

	clq2_precache_check = Q2CS_MODELS;
	clq2_precache_spawncount = String::Atoi(Cmd_Argv(1));
	clq2_precache_model = 0;
	clq2_precache_model_skin = 0;

	CLQ2_RequestNextDownload();
}

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh.
============
*/
static void VID_Restart_f(void)
{
	vid_restart_requested = true;
}

/*
===============
CL_Download_f

Request a download from the server
===============
*/
void    CL_Download_f(void)
{
	char filename[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: download <filename>\n");
		return;
	}

	String::Sprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));

	if (strstr(filename, ".."))
	{
		common->Printf("Refusing to download a path with ..\n");
		return;
	}

	if (FS_ReadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		common->Printf("File already exists.\n");
		return;
	}

	String::Cpy(clc.downloadName, filename);
	common->Printf("Downloading %s\n", clc.downloadName);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension(clc.downloadName, clc.downloadTempName);
	String::Cat(clc.downloadTempName, sizeof(clc.downloadTempName), ".tmp");

	CL_AddReliableCommand(va("download %s", clc.downloadName));

	clc.downloadNumber++;
}

/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal(void)
{
	cls.realtime = Sys_Milliseconds_();

//
// register our variables
//
	cl_autoskins = Cvar_Get("cl_autoskins", "0", 0);
	cl_maxfps = Cvar_Get("cl_maxfps", "90", 0);

	//
	// register our commands
	//
	Cmd_AddCommand("pause", CL_Pause_f);
	Cmd_AddCommand("skins", CL_Skins_f);

	Cmd_AddCommand("userinfo", CL_Userinfo_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);

	Cmd_AddCommand("changing", CL_Changing_f);

	Cmd_AddCommand("download", CL_Download_f);

	Cmd_AddCommand("vid_restart", VID_Restart_f);

	Cmd_AddCommand("precache", CLQ2_Precache_f);
}

/*
==================
CL_FixCvarCheats

==================
*/

typedef struct
{
	const char* name;
	const char* value;
	Cvar* var;
} cheatvar_t;

cheatvar_t cheatvars[] = {
	{"timescale", "1"},
	{"timedemo", "0"},
	{"r_drawworld", "1"},
	{"cl_testlights", "0"},
	{"r_fullbright", "0"},
	{"r_drawflat", "0"},
	{"paused", "0"},
	{"fixedtime", "0"},
	{"r_lightmap", "0"},
	{"r_saturatelighting", "0"},
	{NULL, NULL}
};

int numcheatvars;

void CL_FixCvarCheats(void)
{
	int i;
	cheatvar_t* var;

	if (!String::Cmp(cl.q2_configstrings[Q2CS_MAXCLIENTS], "1") ||
		!cl.q2_configstrings[Q2CS_MAXCLIENTS][0])
	{
		return;		// single player can cheat

	}
	// find all the cvars if we haven't done it yet
	if (!numcheatvars)
	{
		while (cheatvars[numcheatvars].name)
		{
			cheatvars[numcheatvars].var = Cvar_Get(cheatvars[numcheatvars].name,
				cheatvars[numcheatvars].value, 0);
			numcheatvars++;
		}
	}

	// make sure they are all set to the proper values
	for (i = 0, var = cheatvars; i < numcheatvars; i++, var++)
	{
		if (String::Cmp(var->var->string, var->value))
		{
			Cvar_SetLatched(var->name, var->value);
		}
	}
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to
update the rendering DLL and/or video mode to match.
============
*/
static void VID_CheckChanges(void)
{
	if (vid_restart_requested)
	{
		S_StopAllSounds();
		/*
		** refresh has changed
		*/
		vid_restart_requested = false;
		cl.q2_refresh_prepped = false;
		cls.disable_screen = true;

		R_Shutdown(true);
		CL_InitRenderer();
		cls.disable_screen = false;
	}
}

//============================================================================

/*
==================
CL_SendCommand

==================
*/
void CL_SendCommand(void)
{
	// fix any cheating cvars
	CL_FixCvarCheats();

	// send intentions now
	CLQ2_SendCmd();

	// resend a connection request if necessary
	CLQ2_CheckForResend();
}


void CL_UpdateSounds()
{
	if (cl_paused->value)
	{
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		return;
	}

	if (!cl.q2_sound_prepped)
	{
		return;
	}

	for (int i = 0; i < MAX_EDICTS_Q2; i++)
	{
		vec3_t origin;
		CLQ2_GetEntitySoundOrigin(i, origin);
		S_UpdateEntityPosition(i, origin);
	}

	S_ClearLoopingSounds(false);
	for (int i = 0; i < cl.q2_frame.num_entities; i++)
	{
		int num = (cl.q2_frame.parse_entities + i) & (MAX_PARSE_ENTITIES_Q2 - 1);
		q2entity_state_t* ent = &clq2_parse_entities[num];
		if (!ent->sound)
		{
			continue;
		}
		S_AddLoopingSound(num, ent->origin, vec3_origin, 0, cl.sound_precache[ent->sound], 0, 0);
	}
}

/*
==================
CL_Frame

==================
*/
void CL_Frame(int msec)
{
	static int extratime;
	static int lasttimecalled;

	if (com_dedicated->value)
	{
		return;
	}

	extratime += msec;

	if (!cl_timedemo->value)
	{
		if (cls.state == CA_CONNECTED && extratime < 100)
		{
			return;			// don't flood packets out while connecting
		}
		if (extratime < 1000 / cl_maxfps->value)
		{
			return;			// framerate is too high
		}
	}

	// decide the simulation time
	cls.frametime = extratime;
	cls.q2_frametimeFloat = extratime / 1000.0;
	cl.serverTime += extratime;
	cls.realtime = curtime;

	extratime = 0;
	if (cls.q2_frametimeFloat > (1.0 / 5))
	{
		cls.q2_frametimeFloat = (1.0 / 5);
	}
	if (cls.frametime > 200)
	{
		cls.frametime = 200;
	}
	cls.realFrametime = cls.frametime;

	// if in the debugger last frame, don't timeout
	if (msec > 5000)
	{
		clc.netchan.lastReceived = Sys_Milliseconds_();
	}

	// fetch results from server
	CLQ2_ReadPackets();

	// send a new command message to the server
	CL_SendCommand();

	// predict all unacknowledged movements
	CLQ2_PredictMovement();

	// allow rendering DLL change
	VID_CheckChanges();
	if (!cl.q2_refresh_prepped && cls.state == CA_ACTIVE)
	{
		CLQ2_PrepRefresh();
	}

	// update the screen
	if (com_speeds->value)
	{
		time_before_ref = Sys_Milliseconds_();
	}
	SCR_UpdateScreen();
	if (com_speeds->value)
	{
		time_after_ref = Sys_Milliseconds_();
	}

	// update audio
	CL_UpdateSounds();

	S_Respatialize(cl.playernum + 1, cl.refdef.vieworg, cl.refdef.viewaxis, 0);

	S_Update();

	CDAudio_Update();

	// advance local effects for next frame
	CL_RunDLights();
	CL_RunLightStyles();
	SCR_RunCinematic();
	Con_RunConsole();

	cls.framecount++;

	if (log_stats->value)
	{
		if (cls.state == CA_ACTIVE)
		{
			if (!lasttimecalled)
			{
				lasttimecalled = Sys_Milliseconds_();
				if (log_stats_file)
				{
					FS_Printf(log_stats_file, "0\n");
				}
			}
			else
			{
				int now = Sys_Milliseconds_();

				if (log_stats_file)
				{
					FS_Printf(log_stats_file, "%d\n", now - lasttimecalled);
				}
				lasttimecalled = now;
			}
		}
	}
}


//============================================================================

/*
====================
CL_Init
====================
*/
void CL_Init(void)
{
	if (com_dedicated->value)
	{
		return;		// nothing running on the client

	}
	CL_SharedInit();

	// all archived variables will now be loaded

	IN_Init();
#if defined __linux__ || defined __sgi
	S_Init();
	CL_InitRenderer();
#else
	CL_InitRenderer();
	S_Init();	// sound must be initialized after window is created
#endif

	V_Init();

	cls.disable_screen = true;	// don't draw yet

	CDAudio_Init();
	CL_InitLocal();

//	Cbuf_AddText ("exec autoexec.cfg\n");
	FS_ExecAutoexec();
	Cbuf_Execute();

}


/*
===============
CL_Shutdown

FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CL_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf("recursive shutdown\n");
		return;
	}
	isdown = true;

	CL_Disconnect(true);
	Com_WriteConfiguration();

	CDAudio_Shutdown();
	S_Shutdown();
	IN_Shutdown();
	R_Shutdown(true);
}
