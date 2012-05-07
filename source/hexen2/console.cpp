// console.c

#include "quakedef.h"

qboolean con_forcedup;			// because no entities to refresh

qboolean con_debuglog;

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
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f(void)
{
	in_keyCatchers |= KEYCATCH_MESSAGE;
	chat_team = false;
	chatField.widthInChars = (viddef.width - 48) / 8;
}


/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void)
{
	in_keyCatchers |= KEYCATCH_MESSAGE;
	chat_team = true;
	chatField.widthInChars = (viddef.width - 48) / 8;
}

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

	Con_Printf("Console initialized.\n");

	Con_InitCommon();

//
// register our commands
//
	con_notifytime = Cvar_Get("con_notifytime", "3", 0);		//seconds

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("clear", Con_Clear_f);

	PR_LoadStrings();
#ifdef MISSIONPACK
	PR_LoadInfoStrings();
#endif
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void Con_Print(const char* txt)
{
	int mask;

	if (txt[0] == 1)
	{
		mask = 256;		// go to colored text
		S_StartLocalSound("misc/comm.wav");
		// play talk wav
		txt++;
	}
	else if (txt[0] == 2)
	{
		mask = 256;		// go to colored text
		txt++;
	}
	else
	{
		mask = 0;
	}

	CL_ConsolePrintCommon(txt, mask);
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

	if (cls.state == CA_DEDICATED)
	{
		return;		// no graphics mode

	}
// write it to the scrollable buffer
	Con_Print(msg);

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


/*
==============================================================================

DRAWING

==============================================================================
*/

/*
==================
Con_NotifyBox
==================
*/
void Con_NotifyBox(const char* text)
{
	double t1, t2;

// during startup for sound / cd warnings
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	Con_Printf(text);

	Con_Printf("Press a key.\n");
	Con_Printf("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	key_count = -2;		// wait for a key down and up
	in_keyCatchers |= KEYCATCH_CONSOLE;

	do
	{
		t1 = Sys_DoubleTime();
		SCR_UpdateScreen();
		Sys_SendKeyEvents();
		IN_ProcessEvents();
		t2 = Sys_DoubleTime();
		realtime += t2 - t1;		// make the cursor blink
	}
	while (key_count < 0);

	Con_Printf("\n");
	in_keyCatchers &= ~KEYCATCH_CONSOLE;
	realtime = 0;				// put the cursor back to invisible
}
