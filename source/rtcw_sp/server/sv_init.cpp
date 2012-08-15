/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		sv_init.c
 *
 * desc:
 *
*/

#include "server.h"
#include "../../server/quake3/local.h"
#include "../../server/wolfsp/local.h"
#include "../../server/wolfmp/local.h"
#include "../../server/et/local.h"

/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_Init(void)
{
	SV_AddOperatorCommands();

	// serverinfo vars
	Cvar_Get("dmflags", "0", CVAR_SERVERINFO);
	Cvar_Get("fraglimit", "20", CVAR_SERVERINFO);
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	svt3_gametype = Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH2);

	// Rafael gameskill
	sv_gameskill = Cvar_Get("g_gameskill", "1", CVAR_SERVERINFO | CVAR_LATCH2);
	// done

	Cvar_Get("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get("protocol", va("%i", WSPROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	svt3_mapname = Cvar_Get("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	svt3_privateClients = Cvar_Get("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("sv_hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
	sv_maxclients = Cvar_Get("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH2);
	svt3_maxRate = Cvar_Get("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_minPing = Cvar_Get("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_maxPing = Cvar_Get("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_floodProtect = Cvar_Get("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_allowAnonymous = Cvar_Get("sv_allowAnonymous", "0", CVAR_SERVERINFO);

	// systeminfo
	Cvar_Get("sv_cheats", "0", CVAR_SYSTEMINFO | CVAR_ROM);
	sv_serverid = Cvar_Get("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM);
//----(SA) VERY VERY TEMPORARY!!!!!!!!!!!
//----(SA) this is so Activision can test milestones with
//----(SA) the default config.  remember to change this back when shipping!!!
	svt3_pure = Cvar_Get("sv_pure", "0", CVAR_SYSTEMINFO);
//	svt3_pure = Cvar_Get ("sv_pure", "1", CVAR_SYSTEMINFO );
	Cvar_Get("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);

	// server vars
	sv_rconPassword = Cvar_Get("rconPassword", "", CVAR_TEMP);
	svt3_privatePassword = Cvar_Get("sv_privatePassword", "", CVAR_TEMP);
	sv_fps = Cvar_Get("sv_fps", "20", CVAR_TEMP);
	svt3_timeout = Cvar_Get("sv_timeout", "120", CVAR_TEMP);
	svt3_zombietime = Cvar_Get("sv_zombietime", "2", CVAR_TEMP);
	Cvar_Get("nextmap", "", CVAR_TEMP);

	svt3_allowDownload = Cvar_Get("sv_allowDownload", "1", 0);
//----(SA)	heh, whoops.  we've been talking to id masters since we got a connection...
//	sv_master[0] = Cvar_Get ("sv_master1", "master3.idsoftware.com", 0 );
	sv_master[0] = Cvar_Get("sv_master1", "master.gmistudios.com", 0);
	sv_master[1] = Cvar_Get("sv_master2", "", CVAR_ARCHIVE);
	sv_master[2] = Cvar_Get("sv_master3", "", CVAR_ARCHIVE);
	sv_master[3] = Cvar_Get("sv_master4", "", CVAR_ARCHIVE);
	sv_master[4] = Cvar_Get("sv_master5", "", CVAR_ARCHIVE);
	svt3_reconnectlimit = Cvar_Get("sv_reconnectlimit", "3", 0);
	sv_showloss = Cvar_Get("sv_showloss", "0", 0);
	svt3_padPackets = Cvar_Get("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get("sv_mapChecksum", "", CVAR_ROM);
	svt3_lanForceRate = Cvar_Get("sv_lanForceRate", "1", CVAR_ARCHIVE);

	sv_reloading = Cvar_Get("g_reloading", "0", CVAR_ROM);		//----(SA)	added

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SVT3_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SVT3_BotInitBotLib();
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown(const char* finalmsg)
{
	if (!com_sv_running || !com_sv_running->integer)
	{
		return;
	}

	common->Printf("----- Server Shutdown -----\n");

	if (svs.clients && !com_errorEntered)
	{
		SVT3_FinalCommand(va("print \"%s\"", finalmsg), true);
	}

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	SVT3_ShutdownGameProgs();

	// free current level
	SVT3_ClearServer();

	// free server static data
	if (svs.clients)
	{
		Mem_Free(svs.clients);
	}
	memset(&svs, 0, sizeof(svs));

	Cvar_Set("sv_running", "0");

	common->Printf("---------------------------\n");

	// disconnect any local clients
	CL_Disconnect(false);
}
