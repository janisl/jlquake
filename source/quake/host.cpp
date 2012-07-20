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
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

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

Cvar* fraglimit;
Cvar* timelimit;

Cvar* samelevel;
Cvar* noexit;

Cvar* skill;						// 0 - 3

Cvar* pausable;

Cvar* temp1;


class QMainLog : public LogListener
{
public:
	void serialise(const char* text)
	{
		Con_Printf("%s", text);
	}
	void develSerialise(const char* text)
	{
		Con_DPrintf("%s", text);
	}
} MainLog;
//	Log::addListener(&MainLog);

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
	Con_DPrintf("Host_EndGame: %s\n",string);

	if (sv.state != SS_DEAD)
	{
		Host_ShutdownServer(false);
	}

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

	SCR_EndLoadingPlaque();			// reenable screen updates

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	Con_Printf("Host_Error: %s\n",string);

	if (sv.state != SS_DEAD)
	{
		Host_ShutdownServer(false);
	}

	if (cls.state == CA_DEDICATED)
	{
		Sys_Error("Host_Error: %s\n",string);	// dedicated servers exit

	}
	CL_Disconnect();
	cls.qh_demonum = -1;

	inerror = false;

	longjmp(host_abortserver, 1);
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

		cls.state = CA_DEDICATED;
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
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_ROM);

		cls.state = CA_DISCONNECTED;
	}

	i = COM_CheckParm("-listen");
	if (i)
	{
		if (cls.state == CA_DEDICATED)
		{
			Sys_Error("Only one of -dedicated or -listen can be specified");
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


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal(void)
{
	Host_InitCommands();

	COM_InitCommonCvars();

	host_framerate = Cvar_Get("host_framerate", "0", 0);	// set for slow motion
	host_speeds = Cvar_Get("host_speeds", "0", 0);			// set for running times

	sys_ticrate = Cvar_Get("sys_ticrate", "0.05", 0);
	serverprofile = Cvar_Get("serverprofile", "0", 0);

	fraglimit = Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	svqh_teamplay = Cvar_Get("teamplay", "0", CVAR_SERVERINFO);
	samelevel = Cvar_Get("samelevel", "0", 0);
	noexit = Cvar_Get("noexit", "0", CVAR_SERVERINFO);
	skill = Cvar_Get("skill", "1", 0);			// 0 - 3
	svqh_deathmatch = Cvar_Get("deathmatch", "0", 0);	// 0, 1, or 2
	svqh_coop = Cvar_Get("coop", "0", 0);			// 0 or 1

	pausable = Cvar_Get("pausable", "1", 0);

	temp1 = Cvar_Get("temp1", "0", 0);

	Host_FindMaxClients();

	host_time = 1.0;		// so a think at time 0 won't get called
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration(void)
{
	// dedicated servers initialize the host but don't parse and set the
	// config.cfg cvars
	if (host_initialized & !isDedicated)
	{
		fileHandle_t f = FS_FOpenFileWrite("config.cfg");
		if (!f)
		{
			Con_Printf("Couldn't write config.cfg.\n");
			return;
		}

		Key_WriteBindings(f);
		Cvar_WriteVariables(f);

		FS_FCloseFile(f);
	}
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf(const char* fmt, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	host_client->qh_message.WriteByte(q1svc_print);
	host_client->qh_message.WriteString2(string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf(const char* fmt, ...)
{
	va_list argptr;
	char string[1024];
	int i;

	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	for (i = 0; i < svs.qh_maxclients; i++)
		if (svs.clients[i].state == CS_ACTIVE)
		{
			svs.clients[i].qh_message.WriteByte(q1svc_print);
			svs.clients[i].qh_message.WriteString2(string);
		}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands(const char* fmt, ...)
{
	va_list argptr;
	char string[1024];

	va_start(argptr,fmt);
	Q_vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);

	host_client->qh_message.WriteByte(q1svc_stufftext);
	host_client->qh_message.WriteString2(string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient(qboolean crash)
{
	int saveSelf;
	int i;
	client_t* client;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage(host_client->qh_netconnection, &host_client->netchan))
		{
			host_client->qh_message.WriteByte(q1svc_disconnect);
			NET_SendMessage(host_client->qh_netconnection, &host_client->netchan, &host_client->qh_message);
		}

		if (host_client->qh_edict && host_client->state == CS_ACTIVE)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			saveSelf = *pr_globalVars.self;
			*pr_globalVars.self = EDICT_TO_PROG(host_client->qh_edict);
			PR_ExecuteProgram(*pr_globalVars.ClientDisconnect);
			*pr_globalVars.self = saveSelf;
		}

		Con_Printf("Client %s removed\n",host_client->name);
	}

// break the net connection
	NET_Close(host_client->qh_netconnection, &host_client->netchan);
	host_client->qh_netconnection = NULL;

// free the client (the body stays around)
	host_client->state = CS_FREE;
	host_client->name[0] = 0;
	host_client->qh_old_frags = -999999;
	net_activeconnections--;

// send notification to all clients
	for (i = 0, client = svs.clients; i < svs.qh_maxclients; i++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		client->qh_message.WriteByte(q1svc_updatename);
		client->qh_message.WriteByte(host_client - svs.clients);
		client->qh_message.WriteString2("");
		client->qh_message.WriteByte(q1svc_updatefrags);
		client->qh_message.WriteByte(host_client - svs.clients);
		client->qh_message.WriteShort(0);
		client->qh_message.WriteByte(q1svc_updatecolors);
		client->qh_message.WriteByte(host_client - svs.clients);
		client->qh_message.WriteByte(0);
	}
}

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

// stop all client sounds immediately
	if (cls.state == CA_ACTIVE)
	{
		CL_Disconnect();
	}

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
	buf.WriteByte(q1svc_disconnect);
	count = NET_SendToAll(&buf, 5);
	if (count)
	{
		Con_Printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);
	}

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		if (host_client->state >= CS_CONNECTED)
		{
			SV_DropClient(crash);
		}

//
// clear structures
//
	Com_Memset(&sv, 0, sizeof(sv));
	Com_Memset(svs.clients, 0, svs.qh_maxclientslimit * sizeof(client_t));
}


/*
===================
Mod_ClearAll
===================
*/
static void Mod_ClearAll(void)
{
	R_Shutdown(false);
	R_BeginRegistration(&cls.glconfig);

	Com_Memset(clq1_playertextures, 0, sizeof(clq1_playertextures));
	translate_texture = NULL;

	Draw_Init();
	Sbar_Init();
}

/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory(void)
{
	Con_DPrintf("Clearing memory\n");
	Mod_ClearAll();
	if (host_hunklevel)
	{
		Hunk_FreeToLowMark(host_hunklevel);
	}

	clc.qh_signon = 0;
	Com_Memset(&sv, 0, sizeof(sv));
	Com_Memset(&cl, 0, sizeof(cl));
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
	cls.realtime = realtime * 1000;

	if (!cls.qh_timedemo && realtime - oldrealtime < 1.0 / 72.0)
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
		if (host_frametime > 0.1)
		{
			host_frametime = 0.1;
		}
		if (host_frametime < 0.001)
		{
			host_frametime = 0.001;
		}
	}

	cls.frametime = (int)(host_frametime * 1000);
	cls.realFrametime = cls.frametime;
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
	if (!sv.qh_paused && (svs.qh_maxclients > 1 || in_keyCatchers == 0))
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

	try
	{
		if (setjmp(host_abortserver))
		{
			return;		// something bad happened, or the server disconnected

		}
// keep the random time dependent
		rand();

// decide the simulation time
		if (!Host_FilterTime(time))
		{
			return;		// don't run too fast, or packets will flood out

		}
// get new key events
		Sys_SendKeyEvents();

// allow mice or other external controllers to add commands
		IN_Frame();

		IN_ProcessEvents();

// process console commands
		Cbuf_Execute();

		NET_Poll();

// if running the server locally, make intentions now
		if (sv.state != SS_DEAD)
		{
			CL_SendCmd();
		}

//-------------------
//
// server operations
//
//-------------------

// check for commands typed to the host
		Host_GetConsoleCommands();

		if (sv.state != SS_DEAD)
		{
			Host_ServerFrame();
		}

//-------------------
//
// client operations
//
//-------------------

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
		}

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

			if (cl.qh_serverTimeFloat != cl.qh_oldtime)
			{
				CL_UpdateParticles(svqh_gravity->value);
			}
		}

		S_Update();

		CDAudio_Update();

		if (host_speeds->value)
		{
			pass1 = (time1 - time3) * 1000;
			time3 = Sys_DoubleTime();
			pass2 = (time2 - time1) * 1000;
			pass3 = (time3 - time2) * 1000;
			Con_Printf("%3i tot %3i server %3i gfx %3i snd\n",
				pass1 + pass2 + pass3, pass1, pass2, pass3);
		}

		host_framecount++;
		cls.framecount++;
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

		Con_Printf("serverprofile: %2i clients %2i msec\n",  c,  m);
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
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
	try
	{
		GGameType = GAME_Quake;
		Sys_SetHomePathSuffix("jlquake");
		Log::addListener(&MainLog);

		if (standard_quake)
		{
			minimum_memory = MINIMUM_MEMORY;
		}
		else
		{
			minimum_memory = MINIMUM_MEMORY_LEVELPAK;
		}

		if (COM_CheckParm("-minmemory"))
		{
			parms->memsize = minimum_memory;
		}

		host_parms = *parms;

		if (parms->memsize < minimum_memory)
		{
			Sys_Error("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);
		}

		Memory_Init(parms->membase, parms->memsize);
		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();
		V_Init();
		Chase_Init();
		COM_Init(parms->basedir);
		Host_InitLocal();
		Key_Init();
		Con_Init();
		M_Init();
		PR_Init();
		SV_Init();

		Con_Printf("Exe: "__TIME__ " "__DATE__ "\n");
		Con_Printf("%4.1f megabyte heap\n",parms->memsize / (1024 * 1024.0));

		if (cls.state != CA_DEDICATED)
		{
			CL_Init();
		}

		Cbuf_InsertText("exec quake.rc\n");
		Cbuf_Execute();

		NET_Init();

		if (cls.state != CA_DEDICATED)
		{
			IN_Init();
			VID_Init();
			Draw_Init();
			SCR_Init();
			S_Init();
			CLQ1_InitTEnts();
			CDAudio_Init();
			Sbar_Init();
		}

		Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
		host_hunklevel = Hunk_LowMark();

		host_initialized = true;

		Con_Printf("========Quake Initialized=========\n");
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

// keep Con_Printf from trying to update the screen
	cls.disable_screen = true;

	Host_WriteConfiguration();

	CDAudio_Shutdown();
	SVQH_Shutdown();
	CLQH_ShutdownNetwork();
	NETQH_Shutdown();
	S_Shutdown();
	IN_Shutdown();

	if (cls.state != CA_DEDICATED)
	{
		R_Shutdown(true);
	}
}
