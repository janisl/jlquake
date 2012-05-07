//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#define NUM_CON_TIMES   4
#define CON_TEXTSIZE    32768
#define COMMAND_HISTORY 32

struct console_t
{
	bool initialized;

	short text[CON_TEXTSIZE];
	int current;			// line where next message will be printed
	int x;					// offset in current line for next print
	int display;			// bottom of console displays this line

	int linewidth;			// characters across screen
	int totallines;			// total lines in console scrollback

	float xadjust;			// for wide aspect screens

	float displayFrac;		// aproaches finalFrac at scr_conspeed
	float finalFrac;		// 0.0 to 1.0 lines of console to display
	float desiredFrac;		// ydnar: for variable console heights

	int times[NUM_CON_TIMES];		// cls.realtime time the line was generated
	// for transparent notify lines
	vec4_t color;

	int ormask;				// high bit mask for colored characters

	int acLength;			// Arnout: autocomplete buffer length
};

extern console_t con;
extern field_t g_consoleField;
extern field_t historyEditLines[COMMAND_HISTORY];
extern int nextHistoryLine;
extern int historyLine;

extern Cvar* cl_noprint;
extern Cvar* cl_conXOffset;
extern Cvar* con_notifytime;
extern Cvar* con_drawnotify;

extern image_t* conback;

extern field_t chatField;
extern bool chat_team;
extern bool chat_buddy;

void Con_ClearNotify();
void Con_CheckResize();
void Con_PageUp();
void Con_PageDown();
void Con_Top();
void Con_Bottom();
void Con_Clear_f();
void CL_ConsolePrintCommon(const char*& txt, int mask);
void Con_DrawFullBackground();
void Con_DrawConsole();
