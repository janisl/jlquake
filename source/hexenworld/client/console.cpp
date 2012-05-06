// console.c

#include "quakedef.h"

Cvar* con_notifytime;

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
		g_consoleField.widthInChars = con.linewidth;

		in_keyCatchers |= KEYCATCH_CONSOLE;
	}

	Con_ClearNotify();
}

/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f(void)
{
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
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f(void)
{
	chat_team = false;
	in_keyCatchers |= KEYCATCH_MESSAGE;
	chatField.widthInChars = (viddef.width >> 3) - 6;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void)
{
	chat_team = true;
	in_keyCatchers |= KEYCATCH_MESSAGE;
	chatField.widthInChars = (viddef.width >> 3) - 12;
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

//
// register our commands
//
	con_notifytime = Cvar_Get("con_notifytime","3", 0);		//seconds

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("clear", Con_Clear_f);
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

	if (txt[0] == 1 || txt[0] == 2)
	{
		mask = 128 << 8;		// go to colored text
		txt++;
	}
	else
	{
		mask = 0;
	}

	CL_ConsolePrintCommon(txt, mask);
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
	Con_Print(msg);

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
==============================================================================

DRAWING

==============================================================================
*/

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify(void)
{
	int x, v;
	short* text;
	int i;
	int skip;

	v = 0;
	for (i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++)
	{
		if (i < 0)
		{
			continue;
		}
		int time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
		{
			continue;
		}
		time = realtime * 1000 - time;
		if (time > con_notifytime->value * 1000)
		{
			continue;
		}
		text = con.text + (i % con.totallines) * con.linewidth;

		for (x = 0; x < con.linewidth; x++)
			UI_DrawChar((x + 1) << 3, v, text[x]);

		v += 8;
	}


	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		if (chat_team)
		{
			UI_DrawString(8, v, "say_team:");
			skip = 11;
		}
		else
		{
			UI_DrawString(8, v, "say:");
			skip = 5;
		}

		Field_Draw(&chatField, skip << 3, v, true);
	}
}

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
