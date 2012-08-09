
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

int host_hunklevel;

client_t* host_client;				// current client

Cvar* timeout;
Cvar* zombietime;

//
// game rules mirrored in svs.qh_info
//
Cvar* samelevel;
Cvar* spawn;

Cvar* noexit;

fileHandle_t sv_logfile;

int sv_net_port;

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
	static qboolean inerror = false;

	if (inerror)
	{
		Sys_Error("SV_Error: recursively entered (%s)", string);
	}

	inerror = true;

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);

	common->Printf("SV_Error: %s\n",string);

	SV_FinalMessage(va("server crashed: %s\n", string));

	SVQHW_Shutdown();

	Sys_Error("SV_Error: %s\n",string);
}

/*
==================
SV_FinalMessage

Used by common->Error and SV_Quit_f to send a final message to all connected
clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage(const char* message)
{
	int i;
	client_t* cl;

	net_message.Clear();
	net_message.WriteByte(h2svc_print);
	net_message.WriteByte(PRINT_HIGH);
	net_message.WriteString2(message);
	net_message.WriteByte(h2svc_disconnect);

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
		if (cl->state >= CS_ACTIVE)
		{
			Netchan_Transmit(&cl->netchan, net_message.cursize,
				net_message._data);
		}
}

//============================================================================

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client in timeout.value
seconds, drop the conneciton.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts(void)
{
	int i;
	client_t* cl;

	int droptime = realtime * 1000 - timeout->value * 1000;

	for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if ((cl->state == CS_CONNECTED || cl->state == CS_ACTIVE) &&
			cl->netchan.lastReceived < droptime)
		{
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s timed out\n", cl->name);
			SVQHW_DropClient(cl);
			cl->state = CS_FREE;	// don't bother with zombie state
		}
		if (cl->state == CS_ZOMBIE &&
			realtime - cl->qh_connection_started > zombietime->value)
		{
			cl->state = CS_FREE;	// can now be reused
		}
	}
}

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
===================
SV_CheckVars

===================
*/
void SV_CheckVars(void)
{
	static char* pw, * spw;
	int v;

	if (svqhw_password->string == pw && svqhw_spectator_password->string == spw)
	{
		return;
	}
	pw = svqhw_password->string;
	spw = svqhw_spectator_password->string;

	v = 0;
	if (pw && pw[0] && String::Cmp(pw, "none"))
	{
		v |= 1;
	}
	if (spw && spw[0] && String::Cmp(spw, "none"))
	{
		v |= 2;
	}

	common->Printf("Updated needpass.\n");
	if (!v)
	{
		Info_SetValueForKey(svs.qh_info, "needpass", "", MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	}
	else
	{
		Info_SetValueForKey(svs.qh_info, "needpass", va("%i",v), MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
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

// check timeouts
		SV_CheckTimeouts();

// toggle the log buffer if full
		SVQHW_CheckLog();

// move autonomous things around if enough time has passed
		SVQH_RunPhysicsForTime(realtime);

// get packets
		SVQHW_ReadPackets();

// check for commands typed to the host
		SV_GetConsoleCommands();

// process console commands
		Cbuf_Execute();

		SV_CheckVars();

// send messages back to the clients that had packets read this frame
		SVQHW_SendClientMessages();

// send a heartbeat to the master if needed
		SVQHW_Master_Heartbeat();

// collect timing statistics
		end = Sys_DoubleTime();
		svs.qh_stats.active += end - start;
		if (++svs.qh_stats.count == STATFRAMES)
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

/*
===============
SV_InitLocal
===============
*/
void SV_InitLocal(void)
{
	int i;
	
	SV_InitOperatorCommands();

	VQH_InitRollCvars();

	svqhw_spectalk = Cvar_Get("sv_spectalk", "1", 0);
	svhw_allowtaunts = Cvar_Get("sv_allowtaunts", "1", 0);

	svqhw_rcon_password = Cvar_Get("rcon_password", "", 0);	// svqhw_password for remote server commands
	svqhw_password = Cvar_Get("password", "", 0);	// password for entering the game
	svqhw_spectator_password = Cvar_Get("spectator_password", "", 0);	// password for entering as a sepctator

	qh_fraglimit = Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	qh_timelimit = Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	svqh_teamplay = Cvar_Get("teamplay", "0", CVAR_SERVERINFO);
	samelevel = Cvar_Get("samelevel", "0", CVAR_SERVERINFO);
	sv_maxclients = Cvar_Get("maxclients", "8", CVAR_SERVERINFO);
	svqhw_maxspectators = Cvar_Get("maxspectators", "8", CVAR_SERVERINFO);
	qh_skill = Cvar_Get("skill", "1", 0);						// 0 - 3
	svqh_deathmatch = Cvar_Get("deathmatch", "1", CVAR_SERVERINFO);			// 0, 1, or 2
	svqh_coop = Cvar_Get("coop", "0", CVAR_SERVERINFO);			// 0, 1, or 2
	h2_randomclass = Cvar_Get("randomclass", "0", CVAR_SERVERINFO);
	hw_damageScale = Cvar_Get("damagescale", "1.0", CVAR_SERVERINFO);
	hw_shyRespawn = Cvar_Get("shyRespawn", "0", CVAR_SERVERINFO);
	hw_spartanPrint = Cvar_Get("spartanPrint", "1.0", CVAR_SERVERINFO);
	hw_meleeDamScale = Cvar_Get("meleeDamScale", "0.66666", CVAR_SERVERINFO);
	hw_manaScale = Cvar_Get("manascale", "1.0", CVAR_SERVERINFO);
	hw_tomeMode = Cvar_Get("tomemode", "0", CVAR_SERVERINFO);
	hw_tomeRespawn = Cvar_Get("tomerespawn", "0", CVAR_SERVERINFO);
	hw_w2Respawn = Cvar_Get("w2respawn", "0", CVAR_SERVERINFO);
	hw_altRespawn = Cvar_Get("altrespawn", "0", CVAR_SERVERINFO);
	hw_fixedLevel = Cvar_Get("fixedlevel", "0", CVAR_SERVERINFO);
	hw_autoItems = Cvar_Get("autoitems", "0", CVAR_SERVERINFO);
	hw_dmMode = Cvar_Get("dmmode", "0", CVAR_SERVERINFO);
	hw_easyFourth = Cvar_Get("easyfourth", "0", CVAR_SERVERINFO);
	hw_patternRunner = Cvar_Get("patternrunner", "0", CVAR_SERVERINFO);
	spawn = Cvar_Get("spawn", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("hostname", "unnamed", CVAR_SERVERINFO);
	noexit = Cvar_Get("noexit", "0", CVAR_SERVERINFO);

	timeout = Cvar_Get("timeout", "65", 0);		// seconds without any message
	zombietime = Cvar_Get("zombietime", "2", 0);	// seconds to sink messages
	// after disconnect

	SVQH_RegisterPhysicsCvars();

	svqh_aim = Cvar_Get("sv_aim", "0.93", 0);

	qhw_filterban = Cvar_Get("filterban", "1", 0);

	qhw_allow_download = Cvar_Get("allow_download", "1", 0);
	qhw_allow_download_skins = Cvar_Get("allow_download_skins", "1", 0);
	qhw_allow_download_models = Cvar_Get("allow_download_models", "1", 0);
	qhw_allow_download_sounds = Cvar_Get("allow_download_sounds", "1", 0);
	qhw_allow_download_maps = Cvar_Get("allow_download_maps", "1", 0);

	svqh_highchars = Cvar_Get("sv_highchars", "1", 0);

	sv_phs = Cvar_Get("sv_phs", "1", 0);
	svhw_namedistance = Cvar_Get("sv_namedistance", "600", 0);

	sv_ce_scale = Cvar_Get("sv_ce_scale", "1", CVAR_ARCHIVE);
	sv_ce_max_size = Cvar_Get("sv_ce_max_size", "0", CVAR_ARCHIVE);

	Cmd_AddCommand("addip", SVQHW_AddIP_f);
	Cmd_AddCommand("removeip", SVQHW_RemoveIP_f);
	Cmd_AddCommand("listip", SVQHW_ListIP_f);
	Cmd_AddCommand("writeip", SVQHW_WriteIP_f);

	for (i = 0; i < MAX_MODELS_H2; i++)
		sprintf(svqh_localmodels[i], "*%i", i);

	Info_SetValueForKey(svs.qh_info, "*version", va("%4.2f", VERSION), MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);

	svs.clients = new client_t[MAX_CLIENTS_QHW];
	Com_Memset(svs.clients, 0, sizeof(client_t) * MAX_CLIENTS_QHW);

	// init fraglog stuff
	svs.qh_logsequence = 1;
	svs.qh_logtime = realtime;
	svs.qh_log[0].InitOOB(svs.qh_log_buf[0], sizeof(svs.qh_log_buf[0]));
	svs.qh_log[0].allowoverflow = true;
	svs.qh_log[1].InitOOB(svs.qh_log_buf[1], sizeof(svs.qh_log_buf[1]));
	svs.qh_log[1].allowoverflow = true;
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

	sv_net_port = PORT_SERVER;
	p = COM_CheckParm("-port");
	if (p && p < COM_Argc())
	{
		sv_net_port = String::Atoi(COM_Argv(p + 1));
		common->Printf("Port: %i\n", sv_net_port);
	}
	NET_Init(sv_net_port);

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

		if (COM_CheckParm("-minmemory"))
		{
			parms->memsize = MINIMUM_MEMORY;
		}

		host_parms = *parms;

		if (parms->memsize < MINIMUM_MEMORY)
		{
			common->Error("Only %4.1f megs of memory reported, can't execute game", parms->memsize / (float)0x100000);
		}

		Memory_Init(parms->membase, parms->memsize);
		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();

		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

		COM_Init(parms->basedir);

		ComH2_LoadStrings();
		PR_Init();

		SV_InitNet();

		SV_InitLocal();
		Sys_Init();
		PMQH_Init();

		Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
		host_hunklevel = Hunk_LowMark();

		Cbuf_InsertText("exec server.cfg\n");

		host_initialized = true;

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
		common->Printf("%4.1f megabyte heap\n",parms->memsize / (1024 * 1024.0));
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
