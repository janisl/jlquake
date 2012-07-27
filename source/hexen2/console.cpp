// console.c

#include "quakedef.h"

qboolean con_forcedup;			// because no entities to refresh

qboolean con_debuglog;

#ifndef DEDICATED
extern void M_Menu_Main_f(void);

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	con.acLength = 0;

	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		if (cls.state == CA_ACTIVE)
		{
			g_consoleField.buffer[0] = 0;	// clear any typing
			g_consoleField.cursor = 0;
		}
		else
		{
			M_Menu_Main_f();
		}
	}
	else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;
	}

	SCR_EndLoadingPlaque();
	Con_ClearNotify();
	con.desiredFrac = 0.5;
}
#endif

/*
================
Con_Init
================
*/
void Con_Init(void)
{
	const char* t2 = "qconsole.log";

	con_debuglog = COM_CheckParm("-condebug");

	if (con_debuglog)
	{
		FS_FCloseFile(FS_FOpenFileWrite(t2));
	}

	common->Printf("Console initialized.\n");

#ifndef DEDICATED
	Con_InitCommon();

//
// register our commands
//
	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
#endif
}

/*
================
Con_DebugLog
================
*/
void Con_DebugLog(const char* file, const char* fmt, ...)
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

#ifndef DEDICATED
	if (cls.state == CA_DEDICATED)
	{
		return;		// no graphics mode

	}
// write it to the scrollable buffer
	Con_ConsolePrint(msg);

// update the screen if the console is displayed
	if (clc.qh_signon != SIGNONS && !cls.disable_screen)
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
#endif
}

/*
================
Con_DPrintf

A common->Printf that only shows up if the "developer" cvar is set
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

	common->Printf("%s", msg);
}
