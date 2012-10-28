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
#ifdef _WIN32
#include "../../client/windows_shared.h"
#else
#include <unistd.h>
#endif
#include <time.h>
#include "../../server/public.h"
#include "../../client/public.h"

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
