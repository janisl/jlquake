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
#include "../server/public.h"
#include "../client/public.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

double host_frametime;
double host_time;
double realtime;					// without any filtering or bounding
double oldrealtime;					// last frame run

Cvar* host_framerate;	// set for slow motion

Cvar* sys_ticrate;
Cvar* serverprofile;

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

	if (COM_CheckParm("-dedicated"))
	{
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
	}
	else
	{
#ifdef DEDICATED
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
#else
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);
#endif
	}

	COM_InitCommonCommands();

	host_time = 1.0;		// so a think at time 0 won't get called
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
	SVQH_SetRealTime(realtime * 1000);
	if (!CLQH_IsTimeDemo() && realtime - oldrealtime < 1.0 / 72.0)
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
	return true;
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame(float time)
{
	static double time3 = 0;
	int pass1, pass2, pass3;

		if (setjmp(abortframe))
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

// allow mice or other external controllers to add commands
		IN_Frame();

		Com_EventLoop();

// process console commands
		Cbuf_Execute();

//-------------------
//
// server operations
//
//-------------------

		SV_Frame(host_frametime * 1000);

//-------------------
//
// client operations
//
//-------------------

		host_time += host_frametime;

#ifndef DEDICATED
		CL_Frame(host_frametime * 1000);
#endif

		if (com_speeds->value)
		{
			pass1 = time_before_ref - time3 * 1000;
			time3 = Sys_DoubleTime();
			pass2 = time_after_ref - time_before_ref;
			pass3 = time3 * 1000 - time_after_ref;
			common->Printf("%3i tot %3i server %3i gfx %3i snd\n",
				pass1 + pass2 + pass3, pass1, pass2, pass3);
		}
}

void Host_Frame(float time)
{
		double time1, time2;
		static double timetotal;
		static int timecount;
		int m;

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

		common->Printf("serverprofile: %2i clients %2i msec\n",  SVQH_GetNumConnectedClients(),  m);
}

//============================================================================

/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t* parms)
{
		GGameType = GAME_Quake;
		Sys_SetHomePathSuffix("jlquake");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();
		COM_Init();
		Host_InitLocal();
#ifndef DEDICATED
		CL_InitKeyCommands();
#endif
		SV_Init();

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

#ifndef DEDICATED
		if (!com_dedicated->integer)
		{
			CL_Init();
		}
#endif

		Cbuf_InsertText("exec quake.rc\n");
		Cbuf_Execute();

		NETQH_Init();

#ifndef DEDICATED
		if (!com_dedicated->integer)
		{
			CL_StartHunkUsers();
			Sys_ShowConsole(0, false);
		}
#endif

		com_fullyInitialized = true;

		common->Printf("========Quake Initialized=========\n");
}
