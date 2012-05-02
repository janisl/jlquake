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

void Con_Linefeed(bool skipnotify)
{
	// mark time for transparent overlay
	if (con.current >= 0)
	{
		if (skipnotify)
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
	int j = (con.current % con.totallines) * con.linewidth;
	for (int i = 0; i < con.linewidth; i++)
		con.text[j + i] = (ColorIndex(COLOR_WHITE) << 8) | ' ';
}
