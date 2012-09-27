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
// console.c

#include "client.h"

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	con.acLength = 0;

	SCR_EndLoadingPlaque();		// get rid of loading plaque

	if (cl.q2_attractloop)
	{
		Cbuf_AddText("killserver\n");
		return;
	}

	if (cls.state == CA_DISCONNECTED)
	{	// start the demo loop again
		Cbuf_AddText("d1\n");
		return;
	}

	Con_ClearTyping();
	Con_ClearNotify();

	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		UI_ForceMenuOff();
		Cvar_SetLatched("paused", "0");
	}
	else
	{
		UI_ForceMenuOff();
		in_keyCatchers |= KEYCATCH_CONSOLE;

		if (Cvar_VariableValue("maxclients") == 1 &&
			ComQ2_ServerState())
		{
			Cvar_SetLatched("paused", "1");
		}
	}
	con.desiredFrac = 0.5;
}

/*
================
Con_Init
================
*/
void Con_Init(void)
{
	common->Printf("Console initialized.\n");

	Con_InitCommon();

//
// register our commands
//
	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
}

/*
==============
Con_CenteredPrint
==============
*/
void Con_CenteredPrint(char* text)
{
	int l;
	char buffer[1024];

	l = String::Length(text);
	l = (con.linewidth - l) / 2;
	if (l < 0)
	{
		l = 0;
	}
	Com_Memset(buffer, ' ', l);
	String::Cpy(buffer + l, text);
	String::Cat(buffer, sizeof(buffer), "\n");
	Con_ConsolePrint(buffer);
}
