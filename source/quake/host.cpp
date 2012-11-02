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

#include "../common/qcommon.h"
#include "../server/public.h"
#include "../client/public.h"
#include "../apps/main.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

Cvar* host_framerate;	// set for slow motion

Cvar* sys_ticrate;

static double oldtime;

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
}

void Com_SharedFrame()
{
	if (setjmp(abortframe))
	{
		return;		// something bad happened, or the server disconnected
	}

	// find time spent rendering last frame
	double newtime = Sys_DoubleTime();
	double time = newtime - oldtime;

	if (com_dedicated->integer)
	{
		while (time < sys_ticrate->value)
		{
			Sys_Sleep(1);
			newtime = Sys_DoubleTime();
			time = newtime - oldtime;
		}
	}
	while (!CLQH_IsTimeDemo() && time < 1.0 / 72.0)
	{
		// framerate is too high
		// don't run too fast, or packets will flood out
		Sys_Sleep(1);
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;
	}

	oldtime = newtime;

	static double time3 = 0;
	int pass1, pass2, pass3;

	// keep the random time dependent
	rand();

	// decide the simulation time
	SVQH_SetRealTime(newtime * 1000);

	double host_frametime = time;

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

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	COM_InitArgv2(argc, argv);

	Sys_Init();

	GGameType = GAME_Quake;
	Sys_SetHomePathSuffix("jlquake");

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();
	Com_InitByteOrder();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();
	Host_InitLocal();
	CL_InitKeyCommands();
	SV_Init();

	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

	if (!com_dedicated->integer)
	{
		CL_Init();
	}

	Cbuf_InsertText("exec quake.rc\n");
	Cbuf_Execute();

	NETQH_Init();

	if (!com_dedicated->integer)
	{
		CL_StartHunkUsers();
		Sys_ShowConsole(0, false);
	}

	com_fullyInitialized = true;

	oldtime = Sys_DoubleTime();

	common->Printf("========Quake Initialized=========\n");
}
