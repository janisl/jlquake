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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

void Cvar_Changed(Cvar* var)
{
	if ((var->flags & CVAR_SERVERINFO))
	{
		if (sv.active)
		{
			SV_BroadcastPrintf("\"%s\" changed to \"%s\"\n", var->name, var->string);
		}
	}
}

//	{	// there is an error in C code if this happens
//		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);

const char* Cvar_TranslateString(const char* string)
{
	return string;
}
