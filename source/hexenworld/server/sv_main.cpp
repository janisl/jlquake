
#include "qwsvdef.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
#include "../../common/hexen2strings.h"
#include "../../server/public.h"
#include "../../client/public.h"

quakeparms_t host_parms;

void SV_AcceptClient(netadr_t adr, int userid, char* userinfo);
void SVQHW_Master_Shutdown(void);

void Com_Error(int code, const char* fmt, ...)
{
	if (code == ERR_DROP)
	{
		SV_Error("%s", fmt);
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
	}
}

/*
================
SV_Error

Sends a datagram to all the clients informing them of the server crash,
then exits
================
*/
void SV_Error(const char* error, ...)
{
	va_list argptr;
	static char string[1024];

	if (com_errorEntered)
	{
		Sys_Error("SV_Error: recursively entered (%s)", string);
	}

	com_errorEntered = true;

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);

	common->Printf("SV_Error: %s\n",string);

	SV_Shutdown(va("server crashed: %s\n", string));

	Sys_Error("SV_Error: %s\n",string);
}

//============================================================================

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

		COM_Init(parms->basedir);

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
