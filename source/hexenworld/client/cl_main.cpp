// cl_main.c  -- client main loop

#include "../../common/qcommon.h"
#include "../../client/public.h"
#include "../../server/public.h"
#include "../../common/hexen2strings.h"
#include "../../apps/main.h"

static int oldtime;

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
void Com_SharedFrame()
{
	if (setjmp(abortframe))
	{
		return;		// something bad happened, or the server disconnected
	}

	// find time spent rendering last frame
	int newtime = Sys_Milliseconds();
	int time = newtime - oldtime;

	static double time3 = 0;
	int pass1, pass2, pass3;
	float fps;
	// decide the simulation time
	fps = com_maxfps->value;

	if (!CLQH_IsTimeDemo() && time < 1000 / fps)
	{
		return;		// framerate is too high
	}
	oldtime = newtime;
	int frametime = time;
	if (frametime > 200)
	{
		frametime = 200;
	}

	// allow mice or other external controllers to add commands
	IN_Frame();

	Com_EventLoop();

	// process console commands
	Cbuf_Execute();

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
	Sys_Init();

	GGameType = GAME_Hexen2 | GAME_HexenWorld;
	Sys_SetHomePathSuffix("jlhexen2");

	COM_InitArgv2(argc, argv);

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

	Com_InitByteOrder();

	COM_InitCommonCvars();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();

	NETQHW_Init(HWPORT_CLIENT);

	Netchan_Init(0);

	CL_InitKeyCommands();
	ComH2_LoadStrings();

	Cbuf_InsertText("exec hexen.rc\n");
	Cbuf_AddText("cl_warncmd 1\n");
	Cbuf_Execute();

	CL_InitLocal();
	Sys_ShowConsole(0, false);

	com_fullyInitialized = true;

	oldtime = Sys_Milliseconds();

	common->Printf("������� HexenWorld Initialized �������\n");
}
