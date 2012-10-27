// cl_main.c  -- client main loop

#include "quakedef.h"
#ifdef _WIN32
#include "../../client/windows_shared.h"
#endif
#include "../../client/public.h"
#include "../../client/client.h"
#include "../../server/public.h"
#include "../../common/hexen2strings.h"
#include "../../client/game/hexen2/local.h"

quakeparms_t host_parms;

double host_frametime;
double realtime;					// without any filtering or bounding
double oldrealtime;					// last frame run

jmp_buf host_abort;

void Master_Connect_f(void);

void Com_Error(int code, const char* fmt, ...)
{
	if (code == ERR_DROP)
	{
		Host_EndGame(fmt);
	}
	else if (code == ERR_FATAL)
	{
		Host_FatalError("%s", fmt);
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
	}
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

	COM_InitCommonCommands();
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

#define max(a, b)   ((a) > (b) ? (a) : (b))
#define min(a, b)   ((a) < (b) ? (a) : (b))
		fps = max(30.0, min(clqhw_rate->value / 80.0, 72.0));

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

		// allow mice or other external controllers to add commands
		IN_Frame();

		Com_EventLoop();

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

/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t* parms)
{
		GGameType = GAME_Hexen2 | GAME_HexenWorld;
		Sys_SetHomePathSuffix("jlhexen2");

		COM_InitArgv2(parms->argc, parms->argv);
//	COM_AddParm ("-game");
//	COM_AddParm ("hw");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();

		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

		COM_Init(parms->basedir);

		NETQHW_Init(HWPORT_CLIENT);

		Netchan_Init(0);

		CL_InitKeyCommands();
		Com_InitDebugLog();
		ComH2_LoadStrings();

		Cbuf_InsertText("exec hexen.rc\n");
		Cbuf_AddText("cl_warncmd 1\n");
		Cbuf_Execute();

		CL_InitLocal();
		Sys_ShowConsole(0, false);

		com_fullyInitialized = true;

		common->Printf("������� HexenWorld Initialized �������\n");
}
