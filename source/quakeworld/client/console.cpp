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
// console.c

#include "quakedef.h"

qboolean con_debuglog;

void Key_ClearTyping(void)
{
	g_consoleField.buffer[0] = 0;	// clear any typing
	g_consoleField.cursor = 0;
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	con.acLength = 0;

	Key_ClearTyping();

	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		if (cls.state == CA_ACTIVE)
		{
			in_keyCatchers &= ~KEYCATCH_CONSOLE;
		}
	}
	else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;
	}

	Con_ClearNotify();
	con.desiredFrac = 0.5;
}

/*
================
Con_Init
================
*/
void Con_Init(void)
{
	con_debuglog = COM_CheckParm("-condebug");

	Con_Printf("Console initialized.\n");

	Con_InitCommon();

//
// register our commands
//
	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
}

static void Con_DebugLog(const char* file, const char* fmt, ...)
{
	va_list argptr;
	static char data[1024];
	fileHandle_t fd;

	va_start(argptr, fmt);
	Q_vsnprintf(data, sizeof(data), fmt, argptr);
	va_end(argptr);
	FS_FOpenFileByMode(file, &fd, FS_APPEND);
	FS_Write(data, String::Length(data), fd);
	FS_FCloseFile(fd);
}

/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	static qboolean inupdate;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

// also echo to debugging console
	Sys_Print(msg);	// also echo to debugging console

// log all messages to file
	if (con_debuglog)
	{
		Con_DebugLog("qconsole.log", "%s", msg);
	}

// write it to the scrollable buffer
	Con_ConsolePrint(msg);

// update the screen immediately if the console is displayed
	if (cls.state != CA_ACTIVE)
	{
		// protect against infinite loop if something in SCR_UpdateScreen calls
		// Con_Printd
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen();
			inupdate = false;
		}
	}
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!com_developer || !com_developer->value)
	{
		return;			// don't confuse non-developers with techie stuff...

	}
	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	Con_Printf("%s", msg);
}

/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	int temp;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	temp = cls.disable_screen;
	cls.disable_screen = true;
	Con_Printf("%s", msg);
	cls.disable_screen = temp;
}
