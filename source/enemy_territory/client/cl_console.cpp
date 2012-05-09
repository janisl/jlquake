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

// console.c

#include "client.h"

Cvar* con_autoclear;

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	con.acLength = 0;

	// ydnar: persistent console input is more useful
	// Arnout: added cvar
	if (con_autoclear->integer)
	{
		Field_Clear(&g_consoleField);
	}

	Con_ClearNotify();

	// ydnar: multiple console size support
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		con.desiredFrac = 0.0;
	}
	else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;

		// short console
		if (keys[K_CTRL].down)
		{
			con.desiredFrac = (5.0 * SMALLCHAR_HEIGHT) / cls.glconfig.vidHeight;
		}
		// full console
		else if (keys[K_ALT].down)
		{
			con.desiredFrac = 1.0;
		}
		// normal half-screen console
		else
		{
			con.desiredFrac = 0.5;
		}
	}
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f(void)
{
	chat_team = qfalse;
	chat_buddy = qtrue;
	Field_Clear(&chatField);
	chatField.widthInChars = 26;
	in_keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_Init
================
*/
void Con_Init(void)
{
	Con_InitCommon();

	con_autoclear = Cvar_Get("con_autoclear", "1", CVAR_ARCHIVE);

	Cmd_AddCommand("toggleConsole", Con_ToggleConsole_f);

	// ydnar: these are deprecated in favor of cgame/ui based version
	Cmd_AddCommand("clMessageMode3", Con_MessageMode3_f);
}

void Con_Close(void)
{
	if (!com_cl_running->integer)
	{
		return;
	}
	Field_Clear(&g_consoleField);
	Con_ClearNotify();
	in_keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}
