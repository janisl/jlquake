// host.c -- coordinates spawning and killing of local servers

/*
 * $Header: /H2 Mission Pack/HOST.C 6     3/12/98 6:31p Mgummelt $
 */

#include "quakedef.h"
#include "../common/hexen2strings.h"
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

jmp_buf host_abortserver;

Cvar* host_framerate;	// set for slow motion

Cvar* sys_ticrate;
Cvar* serverprofile;

Cvar* sys_adaptive;

void Com_Error(int code, const char* fmt, ...)
{
	if (code == ERR_DROP)
	{
		Host_Error(fmt);
	}
	else if (code == ERR_FATAL)
	{
		Sys_Error("%s", fmt);
	}
	else if (code == ERR_DISCONNECT)
	{
		CL_Disconnect(true);
		SV_Shutdown("");
	}
	else if (code == ERR_SERVERDISCONNECT)
	{
	}
	else if (code == ERR_ENDGAME)
	{
		Host_EndGame(fmt);
	}
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

	SV_Shutdown("");

	if (com_dedicated->integer)
	{
		Sys_Error("Host_EndGame: %s\n",string);		// dedicated servers exit

	}
	CLQH_OnEndGame();

	longjmp(host_abortserver, 1);
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

	SV_Shutdown("");

	if (com_dedicated->integer)
	{
		Sys_Error("Host_Error: %s\n",string);	// dedicated servers exit
	}

	CL_Disconnect(true);
	CLQH_StopDemoLoop();

	com_errorEntered = false;

	longjmp(host_abortserver, 1);
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

	if (setjmp(host_abortserver))
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

/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t* parms)
{
		GGameType = GAME_Hexen2;
#ifdef MISSIONPACK
		GGameType |= GAME_H2Portals;
#endif
		Sys_SetHomePathSuffix("jlhexen2");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();
		COM_Init(parms->basedir);
		Host_InitLocal();
		SVH2_RemoveGIPFiles(NULL);
#ifndef DEDICATED
		CL_InitKeyCommands();
#endif
		ComH2_LoadStrings();
		Com_InitDebugLog();
		SV_Init();

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

#ifndef DEDICATED
		if (!com_dedicated->integer)
		{
			CL_Init();
		}
#endif

		Cbuf_InsertText("exec hexen.rc\n");
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

		common->Printf("======== Hexen II Initialized =========\n");
}
