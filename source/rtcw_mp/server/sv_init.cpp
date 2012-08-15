/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

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
	SVT3_AddOperatorCommands();

	// serverinfo vars
	Cvar_Get("dmflags", "0", /*CVAR_SERVERINFO*/ 0);
	Cvar_Get("fraglimit", "0", /*CVAR_SERVERINFO*/ 0);
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	// DHM - Nerve :: default to WMGT_WOLF
	svt3_gametype = Cvar_Get("g_gametype", "5", CVAR_SERVERINFO | CVAR_LATCH2);

	// Rafael gameskill
	sv_gameskill = Cvar_Get("g_gameskill", "3", CVAR_SERVERINFO | CVAR_LATCH2);
	// done

	Cvar_Get("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get("protocol", va("%i", WMPROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	svt3_mapname = Cvar_Get("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	svt3_privateClients = Cvar_Get("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("sv_hostname", "WolfHost", CVAR_SERVERINFO | CVAR_ARCHIVE);
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
	sv_tourney = Cvar_Get("g_noTeamSwitching", "0", CVAR_ARCHIVE);									// NERVE - SMF

	// systeminfo
	Cvar_Get("sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM);
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
	svt3_master[0] = Cvar_Get("sv_master1", "wolfmaster.idsoftware.com", 0);			// NERVE - SMF - wolfMP master server
	svt3_master[1] = Cvar_Get("sv_master2", "", CVAR_ARCHIVE);
	svt3_master[2] = Cvar_Get("sv_master3", "", CVAR_ARCHIVE);
	svt3_master[3] = Cvar_Get("sv_master4", "", CVAR_ARCHIVE);
	svt3_master[4] = Cvar_Get("sv_master5", "", CVAR_ARCHIVE);
	svt3_reconnectlimit = Cvar_Get("sv_reconnectlimit", "3", 0);
	sv_showloss = Cvar_Get("sv_showloss", "0", 0);
	svt3_padPackets = Cvar_Get("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get("sv_mapChecksum", "", CVAR_ROM);
	svt3_lanForceRate = Cvar_Get("sv_lanForceRate", "1", CVAR_ARCHIVE);

	svwm_onlyVisibleClients = Cvar_Get("sv_onlyVisibleClients", "0", 0);			// DHM - Nerve

	svwm_showAverageBPS = Cvar_Get("sv_showAverageBPS", "0", 0);				// NERVE - SMF - net debugging

	// NERVE - SMF - create user set cvars
	Cvar_Get("g_userTimeLimit", "0", 0);
	Cvar_Get("g_userAlliedRespawnTime", "0", 0);
	Cvar_Get("g_userAxisRespawnTime", "0", 0);
	Cvar_Get("g_maxlives", "0", 0);
	Cvar_Get("g_noTeamSwitching", "0", CVAR_ARCHIVE);
	Cvar_Get("g_altStopwatchMode", "0", CVAR_ARCHIVE);
	Cvar_Get("g_minGameClients", "8", CVAR_SERVERINFO);
	Cvar_Get("g_complaintlimit", "3", CVAR_ARCHIVE);
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
	Cvar_Get("g_voteFlags", "255", CVAR_ARCHIVE | CVAR_SERVERINFO);

	// ATVI Tracker Wolfenstein Misc #263
	Cvar_Get("g_antilag", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);

	// the download netcode tops at 18/20 kb/s, no need to make you think you can go above
	svt3_dl_maxRate = Cvar_Get("sv_dl_maxRate", "42000", CVAR_ARCHIVE);

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SVT3_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SVT3_BotInitBotLib();
}
