
#include "qwsvdef.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
#include "../../common/hexen2strings.h"
#include "../../server/public.h"
#include "../../client/public.h"
#include "../../apps/main.h"

quakeparms_t host_parms;

void SV_AcceptClient(netadr_t adr, int userid, char* userinfo);
void SVQHW_Master_Shutdown(void);

/*
==================
COM_ServerFrame

==================
*/
void COM_ServerFrame(float time)
{
		// keep the random time dependent
		rand();

		Com_EventLoop();

		// process console commands
		Cbuf_Execute();

		SV_Frame(time * 1000);
}

//============================================================================

/*
====================
COM_InitServer
====================
*/
void COM_InitServer(quakeparms_t* parms)
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

		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

		COM_Init();

		ComH2_LoadStrings();

		COM_InitCommonCommands();

		Cvar_Set("cl_warncmd", "1");

		SV_Init();
		Sys_Init();
		PMQH_Init();

		Cbuf_InsertText("exec server.cfg\n");

		com_fullyInitialized = true;

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
		common->Printf("======== HexenWorld Initialized ========\n");

// process command line arguments
		Cbuf_AddLateCommands();
		Cbuf_Execute();

// if a map wasn't specified on the command line, spawn start.map

		if (!SV_IsServerActive())
		{
			Cmd_ExecuteString("map demo1");
		}
		if (!SV_IsServerActive())
		{
			common->Error("Couldn't spawn a server");
		}
}

#ifndef _WIN32
static double oldtime;

void Com_SharedInit(int argc, const char* argv[], char* cmdline)
{
	quakeparms_t parms;
	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(argc, argv);
	parms.argc = argc;
	parms.argv = argv;

	COM_InitServer(&parms);

	// run one frame immediately for first heartbeat
	COM_ServerFrame(0.1);

	oldtime = Sys_DoubleTime() - 0.1;
}

void Com_SharedFrame()
{
	// select on the net socket and stdin
	// the only reason we have a timeout at all is so that if the last
	// connected client times out, the message would not otherwise
	// be printed until the next event.
	if (!SOCK_Sleep(ip_sockets[0], 1000))
	{
		return;
	}

	// find time passed since last cycle
	double newtime = Sys_DoubleTime();
	double time = newtime - oldtime;
	oldtime = newtime;

	COM_ServerFrame(time);
}
#endif
