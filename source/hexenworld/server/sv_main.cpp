
#include "qwsvdef.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
#include "../../common/hexen2strings.h"

quakeparms_t host_parms;

qboolean host_initialized;			// true if into command execution (compatability)

double realtime;					// without any filtering or bounding

void SV_AcceptClient(netadr_t adr, int userid, char* userinfo);
void SVQHW_Master_Shutdown(void);

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3)));
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

	throw DropException(string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw Exception(string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw EndGameException(string);
}

void idCommonLocal::ServerDisconnected(const char* format, ...)
{
}

//============================================================================

/*
================
SVQHW_Shutdown

Quake calls this before calling Sys_Quit or Sys_Error
================
*/
void SVQHW_Shutdown(void)
{
	SVQHW_Master_Shutdown();
	if (sv_logfile)
	{
		FS_FCloseFile(sv_logfile);
		sv_logfile = 0;
	}
	if (svqhw_fraglogfile)
	{
		FS_FCloseFile(svqhw_fraglogfile);
		svqhw_fraglogfile = 0;
	}
	NET_Shutdown();
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

	SVQHW_FinalMessage(va("server crashed: %s\n", string));

	SVQHW_Shutdown();

	Sys_Error("SV_Error: %s\n",string);
}

//============================================================================

/*
===================
SV_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void SV_GetConsoleCommands(void)
{
	char* cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput();
		if (!cmd)
		{
			break;
		}
		Cbuf_AddText(cmd);
	}
}

/*
==================
SV_Frame

==================
*/
void SV_Frame(float time)
{
	try
	{
		static double start, end;

		start = Sys_DoubleTime();
		svs.qh_stats.idle += start - end;

// keep the random time dependent
		rand();

// decide the simulation time
		realtime += time;
		sv.qh_time += time;
		svs.realtime = realtime * 1000;

		for (sysEvent_t ev = Sys_SharedGetEvent(); ev.evType; ev = Sys_SharedGetEvent())
		{
			switch (ev.evType)
			{
			case SE_CONSOLE:
				Cbuf_AddText((char*)ev.evPtr);
				Cbuf_AddText("\n");
				break;
			default:
				break;
			}

			// free any block data
			if (ev.evPtr)
			{
				Mem_Free(ev.evPtr);
			}
		}

// check for commands typed to the host
		SV_GetConsoleCommands();

// process console commands
		Cbuf_Execute();

		SVQHW_ServerFrame();

// collect timing statistics
		end = Sys_DoubleTime();
		svs.qh_stats.active += end - start;
		if (++svs.qh_stats.count == QHW_STATFRAMES)
		{
			svs.qh_stats.latched_active = svs.qh_stats.active;
			svs.qh_stats.latched_idle = svs.qh_stats.idle;
			svs.qh_stats.latched_packets = svs.qh_stats.packets;
			svs.qh_stats.active = 0;
			svs.qh_stats.idle = 0;
			svs.qh_stats.packets = 0;
			svs.qh_stats.count = 0;
		}
	}
	catch (DropException& e)
	{
		SV_Error("%s", e.What());
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

//============================================================================

void CL_Disconnect()
{
}

/*
====================
SV_InitNet
====================
*/
void SV_InitNet(void)
{
	int p;

	svqhw_net_port = HWPORT_SERVER;
	p = COM_CheckParm("-port");
	if (p && p < COM_Argc())
	{
		svqhw_net_port = String::Atoi(COM_Argv(p + 1));
		common->Printf("Port: %i\n", svqhw_net_port);
	}
	NET_Init(svqhw_net_port);

	Netchan_Init(0);

	// heartbeats will allways be sent to the id master
	svs.qh_last_heartbeat = -99999;		// send immediately

	SOCK_StringToAdr("208.135.137.23", &hw_idmaster_adr, 26900);
}


/*
====================
SV_Init
====================
*/
void SV_Init(quakeparms_t* parms)
{
	try
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
		PR_Init();

		SV_InitNet();

		SV_InitOperatorCommands();

		SVQH_Init();
		Sys_Init();
		PMQH_Init();

		Cbuf_InsertText("exec server.cfg\n");

		host_initialized = true;

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
		common->Printf("======== HexenWorld Initialized ========\n");

// process command line arguments
		Cbuf_AddLateCommands();
		Cbuf_Execute();

// if a map wasn't specified on the command line, spawn start.map

		if (sv.state == SS_DEAD)
		{
			Cmd_ExecuteString("map demo1");
		}
		if (sv.state == SS_DEAD)
		{
			common->Error("Couldn't spawn a server");
		}
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

void CL_EstablishConnection(const char* name)
{
}
void Host_Reconnect_f()
{
}

void CL_MapLoading(void)
{
}
void CL_ShutdownAll(void)
{
}
void CL_Disconnect(qboolean showMainMenu)
{
}
void CL_Drop()
{
}
