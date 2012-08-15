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
	SV_Shutdown("killserver");
}

/*
=================
SV_GameCompleteStatus_f

NERVE - SMF
=================
*/
void SV_GameCompleteStatus_f(void)
{
	SVT3_MasterGameCompleteStatus();
}

//===========================================================

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
	Cmd_AddCommand("status", SVT3_Status_f);
	Cmd_AddCommand("serverinfo", SVT3_Serverinfo_f);
	Cmd_AddCommand("systeminfo", SVT3_Systeminfo_f);
	Cmd_AddCommand("dumpuser", SVT3_DumpUser_f);
	Cmd_AddCommand("map_restart", SVT3_MapRestart_f);
	Cmd_AddCommand("fieldinfo", SVET_FieldInfo_f);
	Cmd_AddCommand("sectorlist", SVT3_SectorList_f);
	Cmd_AddCommand("map", SVT3_Map_f);
	Cmd_AddCommand("gameCompleteStatus", SV_GameCompleteStatus_f);			// NERVE - SMF
	Cmd_AddCommand("devmap", SVT3_Map_f);
	Cmd_AddCommand("spmap", SVT3_Map_f);
	Cmd_AddCommand("spdevmap", SVT3_Map_f);
	Cmd_AddCommand("loadgame", SVT3_LoadGame_f);
	Cmd_AddCommand("killserver", SVT3_KillServer_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", SVT3_ConSay_f);
	}
}
