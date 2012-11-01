// host.c -- coordinates spawning and killing of local servers

/*
 * $Header: /H2 Mission Pack/HOST.C 6     3/12/98 6:31p Mgummelt $
 */

#include "../common/qcommon.h"
#include "../common/hexen2strings.h"
#include "../server/public.h"
#include "../client/public.h"
#include "../apps/main.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

double host_frametime;
double host_time;
double realtime;					// without any filtering or bounding
double oldrealtime;					// last frame run

Cvar* host_framerate;	// set for slow motion

Cvar* sys_ticrate;
Cvar* serverprofile;

Cvar* sys_adaptive;

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
	serverprofile = Cvar_Get("serverprofile", "0", 0);

	sys_adaptive = Cvar_Get("sys_adaptive","1", CVAR_ARCHIVE);

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
		if (host_frametime > 0.05 && !sys_adaptive->value)
		{
			host_frametime = 0.05;
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
	double save_host_frametime,total_host_frametime;

	if (setjmp(abortframe))
	{
		return;			// something bad happened, or the server disconnected
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

	save_host_frametime = total_host_frametime = host_frametime;
	if (sys_adaptive->value)
	{
		if (host_frametime > 0.05)
		{
			host_frametime = 0.05;
		}
	}

	if (total_host_frametime > 1.0)
	{
		total_host_frametime = 0.05;
	}

	do
	{
		SV_Frame(host_frametime * 1000);

		//-------------------
		//
		// client operations
		//
		//-------------------

#ifndef DEDICATED
		host_time += host_frametime;
#endif

		if (!sys_adaptive->value)
		{
			break;
		}

		total_host_frametime -= 0.05;
		if (total_host_frametime > 0 && total_host_frametime < 0.05)
		{
			save_host_frametime -= total_host_frametime;
			oldrealtime -= total_host_frametime;
			break;
		}

	}
	while (total_host_frametime > 0);


	host_frametime = save_host_frametime;

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
}

void Com_SharedFrame()
{
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

	double time1, time2;
	static double timetotal;
	static int timecount;
	int m;

	if (!serverprofile->value)
	{
		_Host_Frame(time);
		oldtime = newtime;
		return;
	}

	time1 = Sys_DoubleTime();
	_Host_Frame(time);
	time2 = Sys_DoubleTime();

	timetotal += time2 - time1;
	timecount++;

	if (timecount < 1000)
	{
		oldtime = newtime;
		return;
	}

	m = timetotal * 1000 / timecount;
	timecount = 0;
	timetotal = 0;

	common->Printf("serverprofile: %2i clients %2i msec\n",  SVQH_GetNumConnectedClients(),  m);
	oldtime = newtime;
}

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	COM_InitArgv2(argc, argv);

	Sys_Init();

	GGameType = GAME_Hexen2;
	if (COM_CheckParm("-portals"))
	{
		GGameType |= GAME_H2Portals;
	}
	Sys_SetHomePathSuffix("jlhexen2");

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();
	Com_InitByteOrder();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();
	Host_InitLocal();
	SVH2_RemoveGIPFiles(NULL);
	CL_InitKeyCommands();
	ComH2_LoadStrings();
	SV_Init();

	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

	if (!com_dedicated->integer)
	{
		CL_Init();
	}

	Cbuf_InsertText("exec hexen.rc\n");
	Cbuf_Execute();

	NETQH_Init();

	if (!com_dedicated->integer)
	{
		CL_StartHunkUsers();
		Sys_ShowConsole(0, false);
	}

	com_fullyInitialized = true;

	oldtime = Sys_DoubleTime();

	common->Printf("======== Hexen II Initialized =========\n");
}
