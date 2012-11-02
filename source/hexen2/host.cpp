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

Cvar* host_framerate;	// set for slow motion

static int oldtime;

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
	com_maxfps = Cvar_Get("com_maxfps", "72", CVAR_ARCHIVE);

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
	// find time spent rendering last frame
	int newtime = Sys_Milliseconds();
	int time = newtime - oldtime;

	if (!CLQH_IsTimeDemo() && time < 1000 / com_maxfps->value)
	{
		// framerate is too high
		// don't run too fast, or packets will flood out
		Sys_Sleep(1);
		newtime = Sys_Milliseconds();
		time = newtime - oldtime;
	}

	oldtime = newtime;

	static double time3 = 0;
	int pass1, pass2, pass3;

	if (setjmp(abortframe))
	{
		return;			// something bad happened, or the server disconnected
	}

	// keep the random time dependent
	rand();

	// decide the simulation time
	SVQH_SetRealTime(newtime);

	int frametime = time;

	if (host_framerate->value > 0)
	{
		frametime = host_framerate->value;
	}
	else
	{	// don't allow really long or short frames
		if (frametime > 50)
		{
			frametime = 50;
		}
		if (frametime < 1)
		{
			frametime = 1;
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

	SV_Frame(frametime);

	CL_Frame(frametime);

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

	oldtime = Sys_Milliseconds();

	common->Printf("======== Hexen II Initialized =========\n");
}
