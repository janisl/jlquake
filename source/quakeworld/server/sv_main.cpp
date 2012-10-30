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

#include "qwsvdef.h"
#ifndef _WIN32
#include <unistd.h>
#include <time.h>
#endif
#include "../../server/public.h"
#include "../../client/public.h"
#include "../../apps/main.h"

quakeparms_t host_parms;

double host_frametime;

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
		GGameType = GAME_Quake | GAME_QuakeWorld;
		Sys_SetHomePathSuffix("jlquake");

		COM_InitArgv2(parms->argc, parms->argv);
		COM_AddParm("-game");
		COM_AddParm("qw");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();

		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

		COM_Init();

		COM_InitCommonCommands();

		Cvar_Set("cl_warncmd", "1");

		SV_Init();
		Sys_Init();
		PMQH_Init();

		Cbuf_InsertText("exec server.cfg\n");

		com_fullyInitialized = true;

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

		common->Printf("\nServer Version %4.2f (Build %04d)\n\n", VERSION, build_number());

		common->Printf("======== QuakeWorld Initialized ========\n");

// process command line arguments
		Cbuf_AddLateCommands();
		Cbuf_Execute();

// if a map wasn't specified on the command line, spawn start.map
		if (!SV_IsServerActive())
		{
			Cmd_ExecuteString("map start");
		}
		if (!SV_IsServerActive())
		{
			common->Error("Couldn't spawn a server");
		}
}

#ifdef _WIN32
double oldtime;

void Com_SharedInit(int argc, char* argv[], char* cmdline)
{
	quakeparms_t parms;

	COM_InitArgv2(argc, argv);

	parms.argc = argc;
	parms.argv = argv;

	COM_InitServer(&parms);

// run one frame immediately for first heartbeat
	COM_ServerFrame(0.1);

//
// main loop
//
	oldtime = Sys_DoubleTime() - 0.1;
}

void Com_SharedFrame()
{
	// select on the net socket and stdin
	// the only reason we have a timeout at all is so that if the last
	// connected client times out, the message would not otherwise
	// be printed until the next event.
	//JL: Originally timeout was 0.1 ms
	if (!SOCK_Sleep(ip_sockets[0], 1))
	{
		return;
	}

	// find time passed since last cycle
	double newtime = Sys_DoubleTime();
	double time = newtime - oldtime;
	oldtime = newtime;

	COM_ServerFrame(time);
}
#else
static double oldtime;

void Com_SharedInit(int argc, char* argv[], char* cmdline)
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
