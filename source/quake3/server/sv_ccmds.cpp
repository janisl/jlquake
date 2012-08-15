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

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

#include "../../server/quake3/local.h"
#include "../../server/wolfsp/local.h"
#include "../../server/wolfmp/local.h"
#include "../../server/et/local.h"

void SVT3_KillServer_f()
{
	SVT3_Shutdown("killserver");
}

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands(void)
{
	static qboolean initialized;

	if (initialized)
	{
		return;
	}
	initialized = true;

	Cmd_AddCommand("heartbeat", SVT3_Heartbeat_f);
	Cmd_AddCommand("kick", SVT3_Kick_f);
	Cmd_AddCommand("banUser", SVT3_Ban_f);
	Cmd_AddCommand("banClient", SVT3_BanNum_f);
	Cmd_AddCommand("clientkick", SVT3_KickNum_f);
	Cmd_AddCommand("status", SVT3_Status_f);
	Cmd_AddCommand("serverinfo", SVT3_Serverinfo_f);
	Cmd_AddCommand("systeminfo", SVT3_Systeminfo_f);
	Cmd_AddCommand("dumpuser", SVT3_DumpUser_f);
	Cmd_AddCommand("map_restart", SVT3_MapRestart_f);
	Cmd_AddCommand("sectorlist", SVT3_SectorList_f);
	Cmd_AddCommand("map", SVT3_Map_f);
	Cmd_AddCommand("devmap", SVT3_Map_f);
	Cmd_AddCommand("spmap", SVT3_Map_f);
	Cmd_AddCommand("spdevmap", SVT3_Map_f);
	Cmd_AddCommand("killserver", SVT3_KillServer_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", SVT3_ConSay_f);
	}
}
