// host.c -- coordinates spawning and killing of local servers

/*
 * $Header: /H2 Mission Pack/HOST.C 6     3/12/98 6:31p Mgummelt $
 */

#include "quakedef.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

void Host_WriteConfiguration(const char* fname);

quakeparms_t host_parms;

qboolean host_initialized;			// true if into command execution

double host_frametime;
double host_time;
double realtime;					// without any filtering or bounding
double oldrealtime;					// last frame run
int host_framecount;
int fps_count;

int host_hunklevel;

int minimum_memory;

client_t* host_client;				// current client

jmp_buf host_abortserver;

Cvar* host_framerate;	// set for slow motion
Cvar* host_speeds;				// set for running times

Cvar* sys_ticrate;
Cvar* serverprofile;

Cvar* samelevel;
Cvar* noexit;

Cvar* skill;						// 0 - 3
Cvar* randomclass;				// 0, 1, or 2

Cvar* pausable;
Cvar* sys_adaptive;

Cvar* temp1;

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

	if (sv.state != SS_DEAD)
	{
		Host_ShutdownServer(false);
	}

#ifdef DEDICATED
		Sys_Error("Host_EndGame: %s\n",string);		// dedicated servers exit
#else
	if (cls.state == CA_DEDICATED)
	{
		Sys_Error("Host_EndGame: %s\n",string);		// dedicated servers exit

	}
	if (cls.qh_demonum != -1)
	{
		CL_NextDemo();
	}
	else
	{
		CL_Disconnect();
	}

	longjmp(host_abortserver, 1);
#endif
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
	static qboolean inerror = false;

	if (inerror)
	{
		Sys_Error("Host_Error: recursively entered");
	}
	inerror = true;

#ifndef DEDICATED
	SCR_EndLoadingPlaque();			// reenable screen updates
#endif

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	common->Printf("Host_Error: %s\n",string);

	if (sv.state != SS_DEAD)
	{
		Host_ShutdownServer(false);
	}

#ifdef DEDICATED
	Sys_Error("Host_Error: %s\n",string);	// dedicated servers exit
#else
	if (cls.state == CA_DEDICATED)
	{
		Sys_Error("Host_Error: %s\n",string);	// dedicated servers exit

	}
	CL_Disconnect();
	cls.qh_demonum = -1;

	inerror = false;

	longjmp(host_abortserver, 1);
#endif
}

/*
================
Host_FindMaxClients
================
*/
void    Host_FindMaxClients(void)
{
	int i;

	svs.qh_maxclients = 1;

	i = COM_CheckParm("-dedicated");
	if (i)
	{
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

#ifndef DEDICATED
		cls.state = CA_DEDICATED;
#endif
		if (i != (COM_Argc() - 1))
		{
			svs.qh_maxclients = String::Atoi(COM_Argv(i + 1));
		}
		else
		{
			svs.qh_maxclients = 8;
		}
	}
	else
	{
#ifdef DEDICATED
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);
		svs.qh_maxclients = 8;
#else
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

		cls.state = CA_DISCONNECTED;
#endif
	}

#ifndef DEDICATED
	i = COM_CheckParm("-listen");
	if (i)
	{
		if (cls.state == CA_DEDICATED)
		{
			common->FatalError("Only one of -dedicated or -listen can be specified");
		}
		if (i != (COM_Argc() - 1))
		{
			svs.qh_maxclients = String::Atoi(COM_Argv(i + 1));
		}
		else
		{
			svs.qh_maxclients = 8;
		}
	}
#endif
	if (svs.qh_maxclients < 1)
	{
		svs.qh_maxclients = 8;
	}
	else if (svs.qh_maxclients > MAX_CLIENTS_QH)
	{
		svs.qh_maxclients = MAX_CLIENTS_QH;
	}

	svs.qh_maxclientslimit = svs.qh_maxclients;
	if (svs.qh_maxclientslimit < 4)
	{
		svs.qh_maxclientslimit = 4;
	}
	svs.clients = (client_t*)Hunk_AllocName(svs.qh_maxclientslimit * sizeof(client_t), "clients");

	if (svs.qh_maxclients > 1)
	{
		Cvar_SetValue("deathmatch", 1.0);
	}
	else
	{
		Cvar_SetValue("deathmatch", 0.0);
	}
}

#ifndef DEDICATED
/*
===============
Host_SaveConfig_f
===============
*/
void Host_SaveConfig_f(void)
{

	if (cmd_source != src_command)
	{
		return;
	}

/*	if (!sv.active)
    {
        common->Printf ("Not playing a local game.\n");
        return;
    }

    if (cl.intermission)
    {
        common->Printf ("Can't save in intermission.\n");
        return;
    }
*/

	if (Cmd_Argc() != 2)
	{
		common->Printf("saveConfig <savename> : save a config file\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		common->Printf("Relative pathnames are not allowed.\n");
		return;
	}

	Host_WriteConfiguration(Cmd_Argv(1));
}
#endif


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal(void)
{
#ifndef DEDICATED
	Cmd_AddCommand("saveconfig", Host_SaveConfig_f);
#endif

	Host_InitCommands();

	COM_InitCommonCvars();

	sys_quake2 = Cvar_Get("sys_quake2", "1", CVAR_ARCHIVE);

	host_framerate = Cvar_Get("host_framerate", "0", 0);	// set for slow motion
	host_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times

	sys_ticrate = Cvar_Get("sys_ticrate", "0.05", 0);
	serverprofile = Cvar_Get("serverprofile", "0", 0);

	qh_fraglimit = Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	qh_timelimit = Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	svqh_teamplay = Cvar_Get("teamplay", "0", CVAR_SERVERINFO);
	samelevel = Cvar_Get("samelevel", "0", 0);
	noexit = Cvar_Get("noexit", "0", CVAR_SERVERINFO);
	skill = Cvar_Get("skill", "1", 0);						// 0 - 3
	svqh_deathmatch = Cvar_Get("deathmatch", "0", 0);			// 0, 1, or 2
	randomclass = Cvar_Get("randomclass", "0", 0);			// 0, 1, or 2
	svqh_coop = Cvar_Get("coop", "0", 0);			// 0 or 1

	pausable = Cvar_Get("pausable", "1", 0);

	sys_adaptive = Cvar_Get("sys_adaptive","1", CVAR_ARCHIVE);

	temp1 = Cvar_Get("temp1", "0", 0);

	Host_FindMaxClients();

	host_time = 1.0;		// so a think at time 0 won't get called
}

#ifndef DEDICATED
/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration(const char* fname)
{
	// dedicated servers initialize the host but don't parse and set the
	// config.cfg cvars
	if (host_initialized & !isDedicated)
	{
		fileHandle_t f = FS_FOpenFileWrite(fname);
		if (!f)
		{
			common->Printf("Couldn't write %s.\n",fname);
			return;
		}

		Key_WriteBindings(f);
		Cvar_WriteVariables(f);

		FS_FCloseFile(f);
	}
}
#endif

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(qboolean crash)
{
	int i;
	int count;
	QMsg buf;
	byte message[4];
	double start;

	if (sv.state == SS_DEAD)
	{
		return;
	}

	sv.state = SS_DEAD;

#ifndef DEDICATED
// stop all client sounds immediately
	if (cls.state == CA_ACTIVE)
	{
		CL_Disconnect();
	}
#endif

// flush any pending messages - like the score!!!
	start = Sys_DoubleTime();
	do
	{
		count = 0;
		for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		{
			if (host_client->state >= CS_CONNECTED && host_client->qh_message.cursize)
			{
				if (NET_CanSendMessage(host_client->qh_netconnection, &host_client->netchan))
				{
					NET_SendMessage(host_client->qh_netconnection, &host_client->netchan, &host_client->qh_message);
					host_client->qh_message.Clear();
				}
				else
				{
					NET_GetMessage(host_client->qh_netconnection, &host_client->netchan, &net_message);
					count++;
				}
			}
		}
		if ((Sys_DoubleTime() - start) > 3.0)
		{
			break;
		}
	}
	while (count);

// make sure all the clients know we're disconnecting
	buf.InitOOB(message, 4);
	buf.WriteByte(h2svc_disconnect);
	count = NET_SendToAll(&buf, 5);
	if (count)
	{
		common->Printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);
	}

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		if (host_client->state >= CS_CONNECTED)
		{
			SVQH_DropClient(host_client, crash);
		}

//
// clear structures
//
	Com_Memset(&sv, 0, sizeof(sv));
	Com_Memset(svs.clients, 0, svs.qh_maxclientslimit * sizeof(client_t));
}

#ifndef DEDICATED
/*
===================
Mod_ClearAll
===================
*/
static void Mod_ClearAll(void)
{
	R_Shutdown(false);
	R_BeginRegistration(&cls.glconfig);

	CLH2_ClearEntityTextureArrays();
	Com_Memset(translate_texture, 0, sizeof(translate_texture));

	Draw_Init();
	SB_Init();
}
#endif

/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory(void)
{
	common->DPrintf("Clearing memory\n");
#ifndef DEDICATED
	Mod_ClearAll();
#endif
	if (host_hunklevel)
	{
		Hunk_FreeToLowMark(host_hunklevel);
	}

#ifndef DEDICATED
	clc.qh_signon = 0;
#endif
	Com_Memset(&sv, 0, sizeof(sv));
#ifndef DEDICATED
	Com_Memset(&cl, 0, sizeof(cl));
#endif
}


//============================================================================


/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
qboolean Host_FilterTime(float time)
{
	realtime += time;
#ifdef DEDICATED
	if (realtime - oldrealtime < 1.0 / 72.0)
#else
	cls.realtime = realtime * 1000;

	if (!cls.qh_timedemo && realtime - oldrealtime < 1.0 / 72.0)
#endif
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

#ifndef DEDICATED
	cls.frametime = (int)(host_frametime * 1000);
	cls.realFrametime = cls.frametime;
#endif
	return true;
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands(void)
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
Host_ServerFrame

==================
*/
void Host_ServerFrame(void)
{
// run the world state
	*pr_globalVars.frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram();

// check for new clients
	SV_CheckForNewClients();

// read client messages
	SV_RunClients();

// move things around and think
// always pause in single player if in console or menus
#ifdef DEDICATED
	if (!sv.qh_paused)
#else
	if (!sv.qh_paused && (svs.qh_maxclients > 1 || in_keyCatchers == 0))
#endif
	{
		SVQH_RunPhysicsAndUpdateTime(host_frametime, realtime);
	}

// send all messages to the clients
	SV_SendClientMessages();
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame(float time)
{
	static double time1 = 0;
	static double time2 = 0;
	static double time3 = 0;
	int pass1, pass2, pass3;
	double save_host_frametime,total_host_frametime;

	if (setjmp(host_abortserver))
	{
		return;			// something bad happened, or the server disconnected

	}
	try
	{
// keep the random time dependent
		rand();

// decide the simulation time
		if (!Host_FilterTime(time))
		{
			return;		// don't run too fast, or packets will flood out

		}
#ifndef DEDICATED
// get new key events
		Sys_SendKeyEvents();

// allow mice or other external controllers to add commands
		IN_Frame();

		IN_ProcessEvents();
#endif

// process console commands
		Cbuf_Execute();

#ifndef DEDICATED
		NET_Poll();

// if running the server locally, make intentions now
		if (sv.state != SS_DEAD)
		{
			CL_SendCmd();
		}
#endif

//-------------------
//
// server operations
//
//-------------------

// check for commands typed to the host
		Host_GetConsoleCommands();

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
			if (sv.state != SS_DEAD)
			{
				Host_ServerFrame();
			}

			//-------------------
			//
			// client operations
			//
			//-------------------

#ifndef DEDICATED
			// if running the server remotely, send intentions now after
			// the incoming messages have been read
			if (sv.state == SS_DEAD)
			{
				CL_SendCmd();
			}

			host_time += host_frametime;

			// fetch results from server
			if (cls.state == CA_ACTIVE)
			{
				CL_ReadFromServer();

				if (cl.qh_serverTimeFloat != cl.qh_oldtime)
				{
					CL_UpdateParticles(svqh_gravity->value);
				}
				CLH2_UpdateEffects();
			}
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
// update video
		if (host_speeds->value)
		{
			time1 = Sys_DoubleTime();
		}

		Con_RunConsole();

		SCR_UpdateScreen();

		if (host_speeds->value)
		{
			time2 = Sys_DoubleTime();
		}

// update audio
		if (clc.qh_signon == SIGNONS)
		{
			S_Respatialize(cl.viewentity, cl.refdef.vieworg, cl.refdef.viewaxis, 0);
			CL_RunDLights();
		}

		S_Update();

		CDAudio_Update();
#endif

		if (host_speeds->value)
		{
			pass1 = (time1 - time3) * 1000;
			time3 = Sys_DoubleTime();
			pass2 = (time2 - time1) * 1000;
			pass3 = (time3 - time2) * 1000;
			common->Printf("%3i tot %3i server %3i gfx %3i snd\n",
				pass1 + pass2 + pass3, pass1, pass2, pass3);
		}

		host_framecount++;
#ifndef DEDICATED
		cls.framecount++;
#endif
		fps_count++;
	}
	catch (DropException& e)
	{
		Host_Error(e.What());
	}
	catch (EndGameException& e)
	{
		Host_EndGame(e.What());
	}
}

void Host_Frame(float time)
{
	try
	{
		double time1, time2;
		static double timetotal;
		static int timecount;
		int i, c, m;

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
		c = 0;
		for (i = 0; i < svs.qh_maxclients; i++)
		{
			if (svs.clients[i].state >= CS_CONNECTED)
			{
				c++;
			}
		}

		common->Printf("serverprofile: %2i clients %2i msec\n",  c,  m);
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

//============================================================================

#ifndef DEDICATED
/*
===============
CL_InitRenderStuff
===============
*/
static void CL_InitRenderStuff(void)
{
	playerTranslation = (byte*)COM_LoadHunkFile("gfx/player.lmp");
	if (!playerTranslation)
	{
		common->FatalError("Couldn't load gfx/player.lmp");
	}
}
#endif


/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t* parms)
{
	try
	{
		GGameType = GAME_Hexen2;
#ifdef MISSIONPACK
		GGameType |= GAME_H2Portals;
#endif
		Sys_SetHomePathSuffix("jlhexen2");

		minimum_memory = MINIMUM_MEMORY;

		if (COM_CheckParm("-minmemory"))
		{
			parms->memsize = minimum_memory;
		}

		host_parms = *parms;

		if (parms->memsize < minimum_memory)
		{
			common->FatalError("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);
		}

		Memory_Init(parms->membase, parms->memsize);
		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();
#ifndef DEDICATED
		V_Init();
		Chase_Init();
#endif
		COM_Init(parms->basedir);
		Host_InitLocal();
#ifndef DEDICATED
		Key_Init();
#endif
		ComH2_LoadStrings();
		Con_Init();
#ifndef DEDICATED
		M_Init();
#endif
		PR_Init();
		SV_Init();

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
		common->Printf("%4.1f megabyte heap\n",parms->memsize / (1024 * 1024.0));

#ifndef DEDICATED
		if (cls.state != CA_DEDICATED)
		{
			CL_Init();
		}
#endif

		Cbuf_InsertText("exec hexen.rc\n");
		Cbuf_Execute();

		NET_Init();

#ifndef DEDICATED
		if (cls.state != CA_DEDICATED)
		{
			IN_Init();
			VID_Init();
			Draw_Init();
			CL_InitRenderStuff();
			SCR_Init();
			S_Init();
			CLH2_InitTEnts();
			CDAudio_Init();
			MIDI_Init();
			SB_Init();
		}
#endif

		Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
		host_hunklevel = Hunk_LowMark();

		host_initialized = true;

		common->Printf("======== Hexen II Initialized =========\n");
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
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
	static qboolean isdown = false;

	if (isdown)
	{
		printf("recursive shutdown\n");
		return;
	}
	isdown = true;

#ifndef DEDICATED
// keep common->Printf from trying to update the screen
	cls.disable_screen = true;

	Host_WriteConfiguration("config.cfg");

	CDAudio_Shutdown();
	MIDI_Cleanup();
#endif
	SVQH_Shutdown();
#ifndef DEDICATED
	CLQH_ShutdownNetwork();
#endif
	NETQH_Shutdown();
#ifndef DEDICATED
	S_Shutdown();
	IN_Shutdown();

	if (cls.state != CA_DEDICATED)
	{
		R_Shutdown(true);
	}
#endif
}
