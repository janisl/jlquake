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
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

qboolean host_initialized;			// true if into command execution

double host_frametime;
double host_time;
double realtime;					// without any filtering or bounding
double oldrealtime;					// last frame run
int host_framecount;

jmp_buf host_abortserver;

Cvar* host_framerate;	// set for slow motion

Cvar* sys_ticrate;
Cvar* serverprofile;

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3)));
};

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

void idCommonLocal::Printf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Con_Printf("%s", string);
}

void idCommonLocal::DPrintf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Con_DPrintf("%s", string);
}

void idCommonLocal::Error(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw DropException(string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw Exception(string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw EndGameException(string);
}

void idCommonLocal::ServerDisconnected(const char* format, ...)
{
}

/*
================
Host_EndGame
================
*/
void Host_EndGame(const char* message, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,message);
	Q_vsnprintf(string, 1024, message, argptr);
	va_end(argptr);
	common->DPrintf("Host_EndGame: %s\n",string);

	if (sv.state != SS_DEAD)
	{
		SVQH_Shutdown(false);
	}

#ifdef DEDICATED
	Sys_Error("Host_EndGame: %s\n",string);		// dedicated servers exit
#else
	if (cls.state == CA_DEDICATED)
	{
		Sys_Error("Host_EndGame: %s\n",string);		// dedicated servers exit

	}
	if (cls.qh_demonum != -1)
	{
		CL_NextDemo();
	}
	else
	{
		CL_Disconnect();
	}

	longjmp(host_abortserver, 1);
#endif
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error(const char* error, ...)
{
	va_list argptr;
	char string[1024];

	if (com_errorEntered)
	{
		Sys_Error("Host_Error: recursively entered");
	}
	com_errorEntered = true;

#ifndef DEDICATED
	SCR_EndLoadingPlaque();			// reenable screen updates
#endif

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	common->Printf("Host_Error: %s\n",string);

	if (sv.state != SS_DEAD)
	{
		SVQH_Shutdown(false);
	}

#ifdef DEDICATED
	Sys_Error("Host_Error: %s\n",string);	// dedicated servers exit
#else
	if (cls.state == CA_DEDICATED)
	{
		Sys_Error("Host_Error: %s\n",string);	// dedicated servers exit

	}
	CL_Disconnect();
	cls.qh_demonum = -1;

	com_errorEntered = false;

	longjmp(host_abortserver, 1);
#endif
}

/*
================
Host_FindMaxClients
================
*/
void    Host_FindMaxClients(void)
{
	int i;

	svs.qh_maxclients = 1;

	i = COM_CheckParm("-dedicated");
	if (i)
	{
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

#ifndef DEDICATED
		cls.state = CA_DEDICATED;
#endif
		if (i != (COM_Argc() - 1))
		{
			svs.qh_maxclients = String::Atoi(COM_Argv(i + 1));
		}
		else
		{
			svs.qh_maxclients = 8;
		}
	}
	else
	{
#ifdef DEDICATED
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
		svs.qh_maxclients = 8;
#else
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

		cls.state = CA_DISCONNECTED;
#endif
	}

#ifndef DEDICATED
	i = COM_CheckParm("-listen");
	if (i)
	{
		if (cls.state == CA_DEDICATED)
		{
			common->FatalError("Only one of -dedicated or -listen can be specified");
		}
		if (i != (COM_Argc() - 1))
		{
			svs.qh_maxclients = String::Atoi(COM_Argv(i + 1));
		}
		else
		{
			svs.qh_maxclients = 8;
		}
	}
#endif
	if (svs.qh_maxclients < 1)
	{
		svs.qh_maxclients = 8;
	}
	else if (svs.qh_maxclients > MAX_CLIENTS_QH)
	{
		svs.qh_maxclients = MAX_CLIENTS_QH;
	}

	svs.qh_maxclientslimit = svs.qh_maxclients;
	if (svs.qh_maxclientslimit < 4)
	{
		svs.qh_maxclientslimit = 4;
	}
	svs.clients = (client_t*)Mem_ClearedAlloc(svs.qh_maxclientslimit * sizeof(client_t));

	if (svs.qh_maxclients > 1)
	{
		Cvar_SetValue("deathmatch", 1.0);
	}
	else
	{
		Cvar_SetValue("deathmatch", 0.0);
	}
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal(void)
{
	COM_InitCommonCvars();

	host_framerate = Cvar_Get("host_framerate", "0", 0);	// set for slow motion
	com_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times

	sys_ticrate = Cvar_Get("sys_ticrate", "0.05", 0);
	serverprofile = Cvar_Get("serverprofile", "0", 0);

	Host_FindMaxClients();

	Host_InitCommands();

	host_time = 1.0;		// so a think at time 0 won't get called
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration(void)
{
#ifndef DEDICATED
	// dedicated servers initialize the host but don't parse and set the
	// config.cfg cvars
	if (host_initialized & !isDedicated)
	{
		fileHandle_t f = FS_FOpenFileWrite("config.cfg");
		if (!f)
		{
			common->Printf("Couldn't write config.cfg.\n");
			return;
		}

		Key_WriteBindings(f);
		Cvar_WriteVariables(f);

		FS_FCloseFile(f);
	}
#endif
}

/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
qboolean Host_FilterTime(float time)
{
	realtime += time;
#ifdef DEDICATED
	if (realtime - oldrealtime < 1.0 / 72.0)
#else
	cls.realtime = realtime * 1000;
	svs.realtime = realtime * 1000;

	if (!cls.qh_timedemo && realtime - oldrealtime < 1.0 / 72.0)
#endif
	{
		return false;		// framerate is too high

	}
	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;

	if (host_framerate->value > 0)
	{
		host_frametime = host_framerate->value;
	}
	else
	{	// don't allow really long or short frames
		if (host_frametime > 0.1)
		{
			host_frametime = 0.1;
		}
		if (host_frametime < 0.001)
		{
			host_frametime = 0.001;
		}
	}

#ifndef DEDICATED
	cls.frametime = (int)(host_frametime * 1000);
	cls.realFrametime = cls.frametime;
#endif
	return true;
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands(void)
{
	char* cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput();
		if (!cmd)
		{
			break;
		}
		Cbuf_AddText(cmd);
	}
}


/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame(float time)
{
	static double time1 = 0;
	static double time2 = 0;
	static double time3 = 0;
	int pass1, pass2, pass3;

	try
	{
		if (setjmp(host_abortserver))
		{
			return;		// something bad happened, or the server disconnected

		}
// keep the random time dependent
		rand();

// decide the simulation time
		if (!Host_FilterTime(time))
		{
			return;		// don't run too fast, or packets will flood out

		}
#ifndef DEDICATED
// get new key events
		Sys_SendKeyEvents();

// allow mice or other external controllers to add commands
		IN_Frame();

		IN_ProcessEvents();
#endif

// process console commands
		Cbuf_Execute();

#ifndef DEDICATED
		NET_Poll();

// if running the server locally, make intentions now
		if (sv.state != SS_DEAD)
		{
			CL_SendCmd();
		}
#endif

//-------------------
//
// server operations
//
//-------------------

// check for commands typed to the host
		Host_GetConsoleCommands();

		if (sv.state != SS_DEAD)
		{
			SVQH_ServerFrame(host_frametime);
		}

//-------------------
//
// client operations
//
//-------------------

#ifndef DEDICATED
// if running the server remotely, send intentions now after
// the incoming messages have been read
		if (sv.state == SS_DEAD)
		{
			CL_SendCmd();
		}
#endif

		host_time += host_frametime;

#ifndef DEDICATED
// fetch results from server
		if (cls.state == CA_ACTIVE)
		{
			CL_ReadFromServer();
		}

// update video
		if (com_speeds->value)
		{
			time1 = Sys_DoubleTime();
		}

		Con_RunConsole();

		SCR_UpdateScreen();

		if (com_speeds->value)
		{
			time2 = Sys_DoubleTime();
		}

// update audio
		if (clc.qh_signon == SIGNONS)
		{
			S_Respatialize(cl.viewentity, cl.refdef.vieworg, cl.refdef.viewaxis, 0);
			CL_RunDLights();

			if (cl.qh_serverTimeFloat != cl.qh_oldtime)
			{
				CL_UpdateParticles(svqh_gravity->value);
			}
		}

		S_Update();

		CDAudio_Update();
#endif

		if (com_speeds->value)
		{
			pass1 = (time1 - time3) * 1000;
			time3 = Sys_DoubleTime();
			pass2 = (time2 - time1) * 1000;
			pass3 = (time3 - time2) * 1000;
			common->Printf("%3i tot %3i server %3i gfx %3i snd\n",
				pass1 + pass2 + pass3, pass1, pass2, pass3);
		}

		host_framecount++;
#ifndef DEDICATED
		cls.framecount++;
#endif
	}
	catch (DropException& e)
	{
		Host_Error(e.What());
	}
	catch (EndGameException& e)
	{
		Host_EndGame(e.What());
	}
}

void Host_Frame(float time)
{
	try
	{
		double time1, time2;
		static double timetotal;
		static int timecount;
		int i, c, m;

		if (!serverprofile->value)
		{
			_Host_Frame(time);
			return;
		}

		time1 = Sys_DoubleTime();
		_Host_Frame(time);
		time2 = Sys_DoubleTime();

		timetotal += time2 - time1;
		timecount++;

		if (timecount < 1000)
		{
			return;
		}

		m = timetotal * 1000 / timecount;
		timecount = 0;
		timetotal = 0;
		c = 0;
		for (i = 0; i < svs.qh_maxclients; i++)
		{
			if (svs.clients[i].state >= CS_CONNECTED)
			{
				c++;
			}
		}

		common->Printf("serverprofile: %2i clients %2i msec\n",  c,  m);
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

//============================================================================

/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t* parms)
{
	try
	{
		GGameType = GAME_Quake;
		Sys_SetHomePathSuffix("jlquake");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();
#ifndef DEDICATED
		V_Init();
		Chase_Init();
#endif
		COM_Init(parms->basedir);
		Host_InitLocal();
		Com_InitDebugLog();
#ifndef DEDICATED
		Key_Init();
		Con_Init();
		UI_Init();
#endif
		PR_Init();
		SVQH_Init();

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

#ifndef DEDICATED
		if (cls.state != CA_DEDICATED)
		{
			CL_Init();
		}
#endif

		Cbuf_InsertText("exec quake.rc\n");
		Cbuf_Execute();

		NET_Init();

#ifndef DEDICATED
		if (cls.state != CA_DEDICATED)
		{
			IN_Init();
			VID_Init();
			Draw_Init();
			SCR_Init();
			S_Init();
			CLQ1_InitTEnts();
			CDAudio_Init();
			SbarQ1_Init();
		}
#endif

		host_initialized = true;

		common->Printf("========Quake Initialized=========\n");
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf("recursive shutdown\n");
		return;
	}
	isdown = true;

#ifndef DEDICATED
// keep common->Printf from trying to update the screen
	cls.disable_screen = true;
#endif

	Host_WriteConfiguration();

#ifndef DEDICATED
	CDAudio_Shutdown();
#endif
	SVQH_Shutdown();
#ifndef DEDICATED
	CLQH_ShutdownNetwork();
#endif
	NETQH_Shutdown();
#ifndef DEDICATED
	S_Shutdown();
	IN_Shutdown();

	if (cls.state != CA_DEDICATED)
	{
		R_Shutdown(true);
	}
#endif
}

#ifdef DEDICATED
void CL_Disconnect()
{
}
void SCRQH_BeginLoadingPlaque()
{
}
void CL_EstablishConnection(const char* name)
{
}
void Host_Reconnect_f()
{
}
#endif

void CL_MapLoading(void)
{
}
void CL_ShutdownAll(void)
{
}
void CL_Disconnect(qboolean showMainMenu)
{
}
void CL_Drop()
{
}
void SCRQ2_BeginLoadingPlaque(bool Clear)
{
}
