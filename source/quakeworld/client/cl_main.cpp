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

#include "../../common/qcommon.h"
#include "../../server/public.h"
#include "../../client/public.h"
#include "../../apps/main.h"

#define VERSION     2.40

static double oldtime;

void aaa()
{
	SV_IsServerActive();
}

/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal(void)
{
	CL_Init();
	CL_StartHunkUsers();

	//
	// register our commands
	//
	com_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times

	com_maxfps = Cvar_Get("com_maxfps", "72", CVAR_ARCHIVE);

	COM_InitCommonCommands();
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
int nopacketcount;
void Com_SharedFrame()
{
	// find time spent rendering last frame
	double newtime = Sys_DoubleTime();
	double time = newtime - oldtime;

	static double time3 = 0;
	int pass1, pass2, pass3;
	float fps;
	if (setjmp(abortframe))
	{
		return;		// something bad happened, or the server disconnected

	}
	// decide the simulation time
	if (com_maxfps->value)
	{
		fps = com_maxfps->value;
	}
	else
	{
		fps = 72.0;
	}

	if (!CLQH_IsTimeDemo() && time < 1.0 / fps)
	{
		return;		// framerate is too high
	}
	oldtime = newtime;

	double host_frametime = time;
	if (host_frametime > 0.2)
	{
		host_frametime = 0.2;
	}

	// get new key events
	Com_EventLoop();

	// allow mice or other external controllers to add commands
	IN_Frame();

	// process console commands
	Cbuf_Execute();

	CL_Frame(host_frametime * 1000);

	if (com_speeds->value)
	{
		pass1 = time_before_ref - time3 * 1000;
		time3 = Sys_DoubleTime();
		pass2 = time_after_ref - time_before_ref;
		pass3 = time3 * 1000 - time_after_ref;
		common->Printf("%3i tot %3i server %3i gfx %3i snd\n",
			pass1 + pass2 + pass3, pass1, pass2, pass3);
	}
	oldtime = newtime;
}

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	Sys_Init();

	GGameType = GAME_Quake | GAME_QuakeWorld;
	Sys_SetHomePathSuffix("jlquake");
	COM_InitArgv2(argc, argv);
	COM_AddParm("-game");
	COM_AddParm("qw");

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

	Com_InitByteOrder();

	COM_InitCommonCvars();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();

	NETQHW_Init(QWPORT_CLIENT);
	// pick a port value that should be nice and random
	Netchan_Init(Com_Milliseconds() & 0xffff);

	CL_InitKeyCommands();

	Cbuf_InsertText("exec quake.rc\n");
	Cbuf_AddText("cl_warncmd 1\n");
	Cbuf_Execute();

	CL_InitLocal();

	Sys_ShowConsole(0, false);

	Cbuf_AddText("echo Type connect <internet address> or use GameSpy to connect to a game.\n");

	com_fullyInitialized = true;

	oldtime = Sys_DoubleTime();

	common->Printf("\nClient Version %4.2f\n\n", VERSION);

	common->Printf("������� QuakeWorld Initialized �������\n");
}
