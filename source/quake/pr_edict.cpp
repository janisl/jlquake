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
// sv_edict.c -- entity dictionary

#include "quakedef.h"

Cvar* nomonsters;
Cvar* gamecfg;
Cvar* scratch1;
Cvar* scratch2;
Cvar* scratch3;
Cvar* scratch4;
Cvar* savedgamecfg;
Cvar* saved1;
Cvar* saved2;
Cvar* saved3;
Cvar* saved4;

/*
===============
PR_Init
===============
*/
void PR_Init(void)
{
	PR_InitBuiltins();
	Cmd_AddCommand("edict", ED_PrintEdict_f);
	Cmd_AddCommand("edicts", ED_PrintEdicts);
	Cmd_AddCommand("edictcount", ED_Count);
	Cmd_AddCommand("profile", PR_Profile_f);
	nomonsters = Cvar_Get("nomonsters", "0", 0);
	gamecfg = Cvar_Get("gamecfg", "0", 0);
	scratch1 = Cvar_Get("scratch1", "0", 0);
	scratch2 = Cvar_Get("scratch2", "0", 0);
	scratch3 = Cvar_Get("scratch3", "0", 0);
	scratch4 = Cvar_Get("scratch4", "0", 0);
	savedgamecfg = Cvar_Get("savedgamecfg", "0", CVAR_ARCHIVE);
	saved1 = Cvar_Get("saved1", "0", CVAR_ARCHIVE);
	saved2 = Cvar_Get("saved2", "0", CVAR_ARCHIVE);
	saved3 = Cvar_Get("saved3", "0", CVAR_ARCHIVE);
	saved4 = Cvar_Get("saved4", "0", CVAR_ARCHIVE);
}
