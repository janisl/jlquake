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

#include "../client.h"

#define DEFAULT_CONSOLE_WIDTH   78

console_t con;
field_t g_consoleField;
field_t historyEditLines[COMMAND_HISTORY];

Cvar* cl_noprint;

void Con_ClearNotify()
{
	for (int i = 0; i < NUM_CON_TIMES; i++)
	{
		con.times[i] = 0;
	}
}

static void Con_ClearText()
{
	for (int i = 0; i < CON_TEXTSIZE; i++)
	{
		con.text[i] = (ColorIndex(COLOR_WHITE) << 8) | ' ';
	}
}

//	If the line width has changed, reformat the buffer.
void Con_CheckResize()
{
	int width;
	if (GGameType & GAME_Tech3)
	{
		width = (cls.glconfig.vidWidth / SMALLCHAR_WIDTH) - 2;
	}
	else
	{
		width = (viddef.width / SMALLCHAR_WIDTH) - 2;
	}

	if (width == con.linewidth)
	{
		return;
	}

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		Con_ClearText();
	}
	else
	{
		int oldwidth = con.linewidth;
		con.linewidth = width;
		int oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		int numlines = oldtotallines;

		if (con.totallines < numlines)
		{
			numlines = con.totallines;
		}

		int numchars = oldwidth;

		if (con.linewidth < numchars)
		{
			numchars = con.linewidth;
		}

		short tbuf[CON_TEXTSIZE];
		Com_Memcpy(tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		Con_ClearText();

		for (int i = 0; i < numlines; i++)
		{
			for (int j = 0; j < numchars; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
					tbuf[((con.current - i + oldtotallines) % oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}

void Con_PageUp()
{
	con.display -= 2;
	if (con.current - con.display >= con.totallines)
	{
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown()
{
	con.display += 2;
	if (con.display > con.current)
	{
		con.display = con.current;
	}
}

void Con_Top()
{
	con.display = con.totallines;
	if (con.current - con.display >= con.totallines)
	{
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom()
{
	con.display = con.current;
}

void Con_Clear_f()
{
	Con_ClearText();
	Con_Bottom();		// go to end
}

void Con_Linefeed(bool skipNotify)
{
	// mark time for transparent overlay
	if (con.current >= 0)
	{
		if (skipNotify)
		{
			con.times[con.current % NUM_CON_TIMES] = 0;
		}
		else
		{
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}

	con.x = 0;
	if (con.display == con.current)
	{
		con.display++;
	}
	con.current++;
	int startPos = (con.current % con.totallines) * con.linewidth;
	for (int i = 0; i < con.linewidth; i++)
	{
		con.text[startPos + i] = (ColorIndex(COLOR_WHITE) << 8) | ' ';
	}
}

void CL_ConsolePrintCommon(const char*& txt, int mask)
{
	// for some demos we don't want to ever show anything on the console
	if (cl_noprint && cl_noprint->integer)
	{
		return;
	}

	if (!con.initialized)
	{
		con.color[0] =
			con.color[1] =
			con.color[2] =
			con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize();
		con.initialized = true;
	}

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	bool skipnotify = false;
	if (!String::NCmp(txt, "[skipnotify]", 12))
	{
		skipnotify = true;
		txt += 12;
	}

	int color = ColorIndex(COLOR_WHITE);

	char c;
	while ((c = *txt))
	{
		if (Q_IsColorString(txt))
		{
			if (*(txt + 1) == COLOR_NULL)
			{
				color = ColorIndex(COLOR_WHITE);
			}
			else
			{
				color = ColorIndex(*(txt + 1));
			}
			txt += 2;
			continue;
		}

		// count word length
		int l;
		for (l = 0; l < con.linewidth; l++)
		{
			if (txt[l] <= ' ')
			{
				break;
			}
		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth))
		{
			Con_Linefeed(skipnotify);
		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed(skipnotify);
			break;

		case '\r':
			con.x = 0;
			break;

		default:	// display character and advance
			int y = con.current % con.totallines;
			// rain - sign extension caused the character to carry over
			// into the color info for high ascii chars; casting c to unsigned
			con.text[y * con.linewidth + con.x] = (color << 8) | (unsigned char)c | mask | con.ormask;
			con.x++;
			if (con.x >= con.linewidth)
			{
				Con_Linefeed(skipnotify);
			}
			break;
		}
	}

	// mark time for transparent overlay
	if (con.current >= 0)
	{
		// NERVE - SMF
		if (skipnotify)
		{
			int prev = con.current % NUM_CON_TIMES - 1;
			if (prev < 0)
			{
				prev = NUM_CON_TIMES - 1;
			}
			con.times[prev] = 0;
		}
		else
		{
			// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}
}
