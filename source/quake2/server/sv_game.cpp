/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// sv_game.c -- interface to the game dll

#include "server.h"

/*
===============
SV_InitGameProgs

Init the game subsystem for a new map
===============
*/
void SCR_DebugGraph(float value, int color);

void SV_InitGameProgs(void)
{
	q2game_import_t import;

	// unload anything we have now
	if (ge)
	{
		SVQ2_ShutdownGameProgs();
	}


	// load a new game dll
	import.multicast = SVQ2_Multicast;
	import.unicast = SVQ2_Unicast;
	import.bprintf = SVQ2_BroadcastPrintf;
	import.dprintf = SVQ2_dprintf;
	import.cprintf = SVQ2_cprintf;
	import.centerprintf = SVQ2_centerprintf;
	import.error = SVQ2_error;

	import.linkentity = SVQ2_LinkEdict;
	import.unlinkentity = SVQ2_UnlinkEdict;
	import.BoxEdicts = SVQ2_AreaEdicts;
	import.trace = SVQ2_Trace;
	import.pointcontents = SVQ2_PointContents;
	import.setmodel = SVQ2_setmodel;
	import.inPVS = SVQ2_inPVS;
	import.inPHS = SVQ2_inPHS;
	import.Pmove = PMQ2_Pmove;

	import.modelindex = SVQ2_ModelIndex;
	import.soundindex = SVQ2_SoundIndex;
	import.imageindex = SVQ2_ImageIndex;

	import.configstring = SVQ2_Configstring;
	import.sound = SVQ2_sound;
	import.positioned_sound = SVQ2_StartSound;

	import.WriteChar = SVQ2_WriteChar;
	import.WriteByte = SVQ2_WriteByte;
	import.WriteShort = SVQ2_WriteShort;
	import.WriteLong = SVQ2_WriteLong;
	import.WriteFloat = SVQ2_WriteFloat;
	import.WriteString = SVQ2_WriteString;
	import.WritePosition = SVQ2_WritePos;
	import.WriteDir = SVQ2_WriteDir;
	import.WriteAngle = SVQ2_WriteAngle;

	import.TagMalloc = Z_TagMalloc;
	import.TagFree = Z_Free;
	import.FreeTags = Z_FreeTags;

	import.cvar = Cvar_Get;
	import.cvar_set = Cvar_SetLatched;
	import.cvar_forceset = Cvar_Set;

	import.argc = Cmd_Argc;
	import.argv = Cmd_Argv;
	import.args = Cmd_ArgsUnmodified;
	import.AddCommandString = Cbuf_AddText;

	import.DebugGraph = SCR_DebugGraph;
	import.SetAreaPortalState = CM_SetAreaPortalState;
	import.AreasConnected = CM_AreasConnected;

	ge = (q2game_export_t*)SVQ2_GetGameAPI(&import);

	if (!ge)
	{
		Com_Error(ERR_DROP, "failed to load game DLL");
	}
	if (ge->apiversion != Q2GAME_API_VERSION)
	{
		Com_Error(ERR_DROP, "game is version %i, not %i", ge->apiversion,
			Q2GAME_API_VERSION);
	}

	ge->Init();
}
