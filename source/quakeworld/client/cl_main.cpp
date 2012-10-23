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
#ifdef _WIN32
#include "../../client/windows_shared.h"
#else
#include <unistd.h>
#endif
#include "../../server/public.h"
#include "../../client/public.h"
#include "../../client/client.h"
#include "../../client/game/quake/local.h"

Cvar* cl_maxfps;

quakeparms_t host_parms;

double host_frametime;
double realtime;					// without any filtering or bounding
double oldrealtime;					// last frame run

jmp_buf host_abort;

void Master_Connect_f(void);

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Disconnect(const char* message);
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

	Host_EndGame(string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Sys_Error("%s", string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
}

void idCommonLocal::ServerDisconnected(const char* format, ...)
{
}

void idCommonLocal::Disconnect(const char* message)
{
	CL_Disconnect(true);
	SV_Shutdown("");
}

/*
=======================
CL_Version_f
======================
*/
void CL_Version_f(void)
{
	common->Printf("Version %4.2f\n", VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
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

	char st[80];
	sprintf(st, "%4.2f-%04d", VERSION, build_number());
	Info_SetValueForKey(cls.qh_userinfo, "*ver", st, MAX_INFO_STRING_QW, 64, 64, true, false);

	//
	// register our commands
	//
	com_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times

	cl_maxfps   = Cvar_Get("cl_maxfps", "0", CVAR_ARCHIVE);

	Cmd_AddCommand("version", CL_Version_f);

	Cmd_AddCommand("quit", Com_Quit_f);
}


/*
================
Host_EndGame

Call this to drop to a console without exiting the qwcl
================
*/
void Host_EndGame(const char* message, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,message);
	Q_vsnprintf(string, 1024, message, argptr);
	va_end(argptr);
	common->Printf("\n===========================\n");
	common->Printf("Host_EndGame: %s\n",string);
	common->Printf("===========================\n\n");

	CL_Disconnect(true);

	longjmp(host_abort, 1);
}

/*
================
Host_FatalError

This shuts down the client and exits qwcl
================
*/
void Host_FatalError(const char* error, ...)
{
	va_list argptr;
	char string[1024];

	if (com_errorEntered)
	{
		Sys_Error("Host_FatalError: recursively entered");
	}
	com_errorEntered = true;

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	common->Printf("Host_FatalError: %s\n",string);

	CL_Disconnect(true);
	cls.qh_demonum = -1;

	com_errorEntered = false;

// FIXME
	Sys_Error("Host_FatalError: %s\n",string);
}

//============================================================================

/*
==================
Host_Frame

Runs all active servers
==================
*/
int nopacketcount;
void Host_Frame(float time)
{
		static double time3 = 0;
		int pass1, pass2, pass3;
		float fps;
		if (setjmp(host_abort))
		{
			return;		// something bad happened, or the server disconnected

		}
		// decide the simulation time
		realtime += time;
		if (oldrealtime > realtime)
		{
			oldrealtime = 0;
		}

		if (cl_maxfps->value)
		{
			fps = max(30.0, min(cl_maxfps->value, 72.0));
		}
		else
		{
			fps = max(30.0, min(clqhw_rate->value / 80.0, 72.0));
		}

		if (!cls.qh_timedemo && realtime - oldrealtime < 1.0 / fps)
		{
			return;		// framerate is too high

		}
		host_frametime = realtime - oldrealtime;
		oldrealtime = realtime;
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
}

//============================================================================

/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t* parms)
{
		GGameType = GAME_Quake | GAME_QuakeWorld;
		Sys_SetHomePathSuffix("jlquake");
		COM_InitArgv2(parms->argc, parms->argv);
		COM_AddParm("-game");
		COM_AddParm("qw");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();

		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

		COM_Init();

		NETQHW_Init(QWPORT_CLIENT);
		// pick a port value that should be nice and random
		Netchan_Init(Com_Milliseconds() & 0xffff);

		CL_InitKeyCommands();
		Com_InitDebugLog();

		Cbuf_InsertText("exec quake.rc\n");
		Cbuf_AddText("cl_warncmd 1\n");
		Cbuf_Execute();

		CL_InitLocal();

		Sys_ShowConsole(0, false);

		Cbuf_AddText("echo Type connect <internet address> or use GameSpy to connect to a game.\n");

		com_fullyInitialized = true;

		common->Printf("\nClient Version %4.2f (Build %04d)\n\n", VERSION, build_number());

		common->Printf("������� QuakeWorld Initialized �������\n");
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
	CL_Shutdown();
	NET_Shutdown();
}

void server_referencer_dummy()
{
	svh2_kingofhill = 0;
}
