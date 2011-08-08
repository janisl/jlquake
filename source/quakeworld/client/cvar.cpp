/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
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

#ifdef SERVERONLY 
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif

#ifdef SERVERONLY
void SV_SendServerInfoChange(char *key, char *value);
#endif

void Cvar_Changed(QCvar* var)
{
#ifdef SERVERONLY
	if (var->flags & CVAR_SERVERINFO && var->name[0] != '*')
	{
		Info_SetValueForKey(svs.info, var->name, var->string, MAX_SERVERINFO_STRING,
			64, 64, !sv_highchars || !sv_highchars->value, false);
		SV_SendServerInfoChange(var->name, var->string);
//		SV_BroadcastCommand ("fullserverinfo \"%s\"\n", svs.info);
	}
#else
	if (var->flags & CVAR_USERINFO && var->name[0] != '*')
	{
		Info_SetValueForKey(cls.userinfo, var->name, var->string, MAX_INFO_STRING, 64, 64,
			String::ICmp(var->name, "name") != 0, String::ICmp(var->name, "team") == 0);
		if (cls.state >= ca_connected)
		{
			cls.netchan.message.WriteByte(clc_stringcmd);
			cls.netchan.message.WriteString2(va("setinfo \"%s\" \"%s\"\n", var->name, var->string));
		}
	}
#endif
}
