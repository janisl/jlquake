/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

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
	Cvar_Get("dmflags", "0", /*CVAR_SERVERINFO*/ 0);
	Cvar_Get("fraglimit", "0", /*CVAR_SERVERINFO*/ 0);
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);

	// Rafael gameskill
//	sv_gameskill = Cvar_Get ("g_gameskill", "3", CVAR_SERVERINFO | CVAR_LATCH2 );
	// done

	Cvar_Get("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get("protocol", va("%i", ETPROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	svt3_mapname = Cvar_Get("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	svt3_privateClients = Cvar_Get("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("sv_hostname", "ETHost", CVAR_SERVERINFO | CVAR_ARCHIVE);
	//
#ifdef __MACOS__
	sv_maxclients = Cvar_Get("sv_maxclients", "16", CVAR_SERVERINFO | CVAR_LATCH2);					//DAJ HOG
#else
	sv_maxclients = Cvar_Get("sv_maxclients", "20", CVAR_SERVERINFO | CVAR_LATCH2);					// NERVE - SMF - changed to 20 from 8
#endif

	svt3_maxRate = Cvar_Get("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_minPing = Cvar_Get("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_maxPing = Cvar_Get("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_floodProtect = Cvar_Get("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_allowAnonymous = Cvar_Get("sv_allowAnonymous", "0", CVAR_SERVERINFO);
	sv_friendlyFire = Cvar_Get("g_friendlyFire", "1", CVAR_SERVERINFO | CVAR_ARCHIVE);				// NERVE - SMF
	sv_maxlives = Cvar_Get("g_maxlives", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_SERVERINFO);		// NERVE - SMF
	sv_needpass = Cvar_Get("g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM);

	// systeminfo
	//bani - added Cvar for sv_cheats so server engine can reference it
	sv_cheats = Cvar_Get("sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM);
	sv_serverid = Cvar_Get("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM);
	svt3_pure = Cvar_Get("sv_pure", "1", CVAR_SYSTEMINFO);
	Cvar_Get("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);

	// server vars
	sv_rconPassword = Cvar_Get("rconPassword", "", CVAR_TEMP);
	svt3_privatePassword = Cvar_Get("sv_privatePassword", "", CVAR_TEMP);
	sv_fps = Cvar_Get("sv_fps", "20", CVAR_TEMP);
	svt3_timeout = Cvar_Get("sv_timeout", "240", CVAR_TEMP);
	svt3_zombietime = Cvar_Get("sv_zombietime", "2", CVAR_TEMP);
	Cvar_Get("nextmap", "", CVAR_TEMP);

	svt3_allowDownload = Cvar_Get("sv_allowDownload", "1", CVAR_ARCHIVE);
	svt3_master[0] = Cvar_Get("sv_master1", MASTER_SERVER_NAME, 0);
	svt3_master[1] = Cvar_Get("sv_master2", "", CVAR_ARCHIVE);
	svt3_master[2] = Cvar_Get("sv_master3", "", CVAR_ARCHIVE);
	svt3_master[3] = Cvar_Get("sv_master4", "", CVAR_ARCHIVE);
	svt3_master[4] = Cvar_Get("sv_master5", "", CVAR_ARCHIVE);
	svt3_reconnectlimit = Cvar_Get("sv_reconnectlimit", "3", 0);
	svet_tempbanmessage = Cvar_Get("sv_tempbanmessage", "You have been kicked and are temporarily banned from joining this server.", 0);
	sv_showloss = Cvar_Get("sv_showloss", "0", 0);
	svt3_padPackets = Cvar_Get("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get("sv_mapChecksum", "", CVAR_ROM);

	svt3_reloading = Cvar_Get("g_reloading", "0", CVAR_ROM);

	svt3_lanForceRate = Cvar_Get("sv_lanForceRate", "1", CVAR_ARCHIVE);

	svwm_onlyVisibleClients = Cvar_Get("sv_onlyVisibleClients", "0", 0);			// DHM - Nerve

	svwm_showAverageBPS = Cvar_Get("sv_showAverageBPS", "0", 0);				// NERVE - SMF - net debugging

	// NERVE - SMF - create user set cvars
	Cvar_Get("g_userTimeLimit", "0", 0);
	Cvar_Get("g_userAlliedRespawnTime", "0", 0);
	Cvar_Get("g_userAxisRespawnTime", "0", 0);
	Cvar_Get("g_maxlives", "0", 0);
	Cvar_Get("g_altStopwatchMode", "0", CVAR_ARCHIVE);
	Cvar_Get("g_minGameClients", "8", CVAR_SERVERINFO);
	Cvar_Get("g_complaintlimit", "6", CVAR_ARCHIVE);
	Cvar_Get("gamestate", "-1", CVAR_WOLFINFO | CVAR_ROM);
	Cvar_Get("g_currentRound", "0", CVAR_WOLFINFO);
	Cvar_Get("g_nextTimeLimit", "0", CVAR_WOLFINFO);
	// -NERVE - SMF

	// TTimo - some UI additions
	// NOTE: sucks to have this hardcoded really, I suppose this should be in UI
	Cvar_Get("g_axismaxlives", "0", 0);
	Cvar_Get("g_alliedmaxlives", "0", 0);
	Cvar_Get("g_fastres", "0", CVAR_ARCHIVE);
	Cvar_Get("g_fastResMsec", "1000", CVAR_ARCHIVE);

	// ATVI Tracker Wolfenstein Misc #273
	Cvar_Get("g_voteFlags", "0", CVAR_ROM | CVAR_SERVERINFO);

	// ATVI Tracker Wolfenstein Misc #263
	Cvar_Get("g_antilag", "1", CVAR_ARCHIVE | CVAR_SERVERINFO);

	Cvar_Get("g_needpass", "0", CVAR_SERVERINFO);

	svt3_gametype = Cvar_Get("g_gametype", va("%i", comet_gameInfo.defaultGameType), CVAR_SERVERINFO | CVAR_LATCH2);

	// the download netcode tops at 18/20 kb/s, no need to make you think you can go above
	svt3_dl_maxRate = Cvar_Get("sv_dl_maxRate", "42000", CVAR_ARCHIVE);

	svet_wwwDownload = Cvar_Get("sv_wwwDownload", "0", CVAR_ARCHIVE);
	svet_wwwBaseURL = Cvar_Get("sv_wwwBaseURL", "", CVAR_ARCHIVE);
	svet_wwwDlDisconnected = Cvar_Get("sv_wwwDlDisconnected", "0", CVAR_ARCHIVE);
	svet_wwwFallbackURL = Cvar_Get("sv_wwwFallbackURL", "", CVAR_ARCHIVE);

	//bani
	sv_packetloss = Cvar_Get("sv_packetloss", "0", CVAR_CHEAT);
	sv_packetdelay = Cvar_Get("sv_packetdelay", "0", CVAR_CHEAT);

	// fretn - note: redirecting of clients to other servers relies on this,
	// ET://someserver.com
	svet_fullmsg = Cvar_Get("sv_fullmsg", "Server is full.", CVAR_ARCHIVE);

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SVT3_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SVT3_BotInitBotLib();

	svs.et_serverLoad = -1;
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

	SVT3_MasterShutdown();
	SVT3_ShutdownGameProgs();

	// free current level
	SVT3_ClearServer();

	// free server static data
	if (svs.clients)
	{
		Mem_Free(svs.clients);
	}
	Com_Memset(&svs, 0, sizeof(svs));
	svs.et_serverLoad = -1;

	Cvar_Set("sv_running", "0");
	if (GGameType & GAME_Quake3)
	{
		Cvar_Set("ui_singlePlayerActive", "0");
	}

	common->Printf("---------------------------\n");

	// disconnect any local clients
	CL_Disconnect(false);
}


