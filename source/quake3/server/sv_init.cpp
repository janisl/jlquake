/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
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
	Cvar_Get("dmflags", "0", CVAR_SERVERINFO);
	Cvar_Get("fraglimit", "20", CVAR_SERVERINFO);
	Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	svt3_gametype = Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH2);
	Cvar_Get("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get("protocol", va("%i", Q3PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	svt3_mapname = Cvar_Get("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	svt3_privateClients = Cvar_Get("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("sv_hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
	sv_maxclients = Cvar_Get("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH2);

	svt3_maxRate = Cvar_Get("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_minPing = Cvar_Get("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_maxPing = Cvar_Get("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO);
	svt3_floodProtect = Cvar_Get("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO);

	// systeminfo
	Cvar_Get("sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM);
	sv_serverid = Cvar_Get("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM);
	svt3_pure = Cvar_Get("sv_pure", "1", CVAR_SYSTEMINFO);
	Cvar_Get("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM);
	Cvar_Get("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM);

	// server vars
	svt3_rconPassword = Cvar_Get("rconPassword", "", CVAR_TEMP);
	svt3_privatePassword = Cvar_Get("sv_privatePassword", "", CVAR_TEMP);
	sv_fps = Cvar_Get("sv_fps", "20", CVAR_TEMP);
	svt3_timeout = Cvar_Get("sv_timeout", "200", CVAR_TEMP);
	svt3_zombietime = Cvar_Get("sv_zombietime", "2", CVAR_TEMP);
	Cvar_Get("nextmap", "", CVAR_TEMP);

	svt3_allowDownload = Cvar_Get("sv_allowDownload", "0", CVAR_SERVERINFO);
	svt3_master[0] = Cvar_Get("sv_master1", MASTER_SERVER_NAME, 0);
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
	svq3_strictAuth = Cvar_Get("sv_strictAuth", "1", CVAR_ARCHIVE);

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SVT3_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SVT3_BotInitBotLib();
}
