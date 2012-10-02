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

#include "../../client.h"
#include "local.h"
#include "../quake_hexen2/menu.h"

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

int scrh2_lines;
int scrh2_StartC[MAXLINES_H2], scrh2_EndC[MAXLINES_H2];

void SCRH2_FindTextBreaks(const char* message, int Width)
{
	scrh2_lines = 0;
	int pos = 0;
	int start = 0;
	int lastspace = -1;

	while (1)
	{
		if (pos - start >= Width || message[pos] == '@' ||
			message[pos] == 0)
		{
			int oldlast = lastspace;
			if (message[pos] == '@' || lastspace == -1 || message[pos] == 0)
			{
				lastspace = pos;
			}

			scrh2_StartC[scrh2_lines] = start;
			scrh2_EndC[scrh2_lines] = lastspace;
			scrh2_lines++;
			if (scrh2_lines == MAXLINES_H2)
			{
				return;
			}

			if (message[pos] == '@')
			{
				start = pos + 1;
			}
			else if (oldlast == -1)
			{
				start = lastspace;
			}
			else
			{
				start = lastspace + 1;
			}

			lastspace = -1;
		}

		if (message[pos] == 32)
		{
			lastspace = pos;
		}
		else if (message[pos] == 0)
		{
			break;
		}

		pos++;
	}
}

void MH2_Print2(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 200) >> 1), str, 256);
}

static void SCRH2_Bottom_Plaque_Draw(const char* message)
{
	if (!*message)
	{
		return;
	}

	SCRH2_FindTextBreaks(message, PLAQUE_WIDTH);

	int by = (((viddef.height) / 8) - scrh2_lines - 2) * 8;

	MQH_DrawTextBox(32, by - 16, 30, scrh2_lines + 2);

	for (int i = 0; i < scrh2_lines; i++, by += 8)
	{
		char temp[80];
		String::NCpy(temp, &message[scrh2_StartC[i]],scrh2_EndC[i] - scrh2_StartC[i]);
		temp[scrh2_EndC[i] - scrh2_StartC[i]] = 0;
		int bx = ((40 - String::Length(temp)) * 8) / 2;
		MQH_Print(bx, by, temp);
	}
}

void SCRH2_DrawCenterString(const char* message)
{
	if (h2intro_playing)
	{
		SCRH2_Bottom_Plaque_Draw(message);
		return;
	}

	SCRH2_FindTextBreaks(message, 38);

	int by = ((25 - scrh2_lines) * 8) / 2;
	for (int i = 0; i < scrh2_lines; i++, by += 8)
	{
		char temp[80];
		String::NCpy(temp,&message[scrh2_StartC[i]],scrh2_EndC[i] - scrh2_StartC[i]);
		temp[scrh2_EndC[i] - scrh2_StartC[i]] = 0;
		int bx = ((40 - String::Length(temp)) * 8) / 2;
		MH2_Print2(bx, by, temp);
	}
}
