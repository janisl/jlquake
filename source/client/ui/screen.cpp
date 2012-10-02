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
#include "../game/hexen2/local.h"

struct graphsamp_t
{
	float value;
	int color;
};

static int current;
static graphsamp_t values[1024];

static Cvar* cl_graphheight;
static Cvar* cl_graphscale;
static Cvar* cl_graphshift;

Cvar* scr_viewsize;
Cvar* crosshair;

float h2_introTime = 0.0;

vrect_t scr_vrect;

static Cvar* scr_centertime;
static Cvar* scr_printspeed;

static char scr_centerstring[1024];
static float scr_centertime_start;				// for slow victory printing
static float scr_centertime_off;
static int scr_center_lines;

void SCR_InitCommon()
{
	cl_graphheight = Cvar_Get("graphheight", "32", CVAR_CHEAT);
	cl_graphscale = Cvar_Get("graphscale", "1", CVAR_CHEAT);
	cl_graphshift = Cvar_Get("graphshift", "0", CVAR_CHEAT);
	if (!(GGameType & GAME_Tech3))
	{
		scr_printspeed = Cvar_Get("scr_printspeed", "8", 0);
	}
	if (GGameType & GAME_Quake)
	{
		scr_centertime = Cvar_Get("scr_centertime", "2", 0);
	}
	else if (GGameType & GAME_Hexen2)
	{
		scr_centertime = Cvar_Get("scr_centertime", "4", 0);
	}
	else if (GGameType & GAME_Quake2)
	{
		scr_centertime = Cvar_Get("scr_centertime", "2.5", 0);
	}
}

void SCR_EndLoadingPlaque()
{
	cls.disable_screen = 0;
	Con_ClearNotify();
}

void SCR_DebugGraph(float value, int color)
{
	values[current & 1023].value = value;
	values[current & 1023].color = color;
	current++;
}

void SCR_DrawDebugGraph()
{
	//
	// draw the graph
	//
	int x = 0;
	int y, w;
	if (GGameType & GAME_Tech3)
	{
		w = cls.glconfig.vidWidth;
		y = cls.glconfig.vidHeight;
	}
	else
	{
		w = viddef.width;
		y = viddef.height;
	}
	if (GGameType & GAME_Tech3)
	{
		R_SetColor(g_color_table[0]);
		R_StretchPic(x, y - cl_graphheight->integer,
			w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader);
		R_SetColor(NULL);
	}
	else
	{
		UI_FillPal(x, y - cl_graphheight->integer,
			w, cl_graphheight->integer, 8);
	}

	for (int a = 0; a < w; a++)
	{
		int i = (current - 1 - a + 1024) & 1023;
		float v = values[i].value;
		int color = values[i].color;
		v = v * cl_graphscale->integer + cl_graphshift->integer;

		if (v < 0)
		{
			v += cl_graphheight->integer * (1 + (int)(-v / cl_graphheight->integer));
		}
		int h = (int)v % cl_graphheight->integer;
		if (GGameType & GAME_Tech3)
		{
			R_StretchPic(x + w - 1 - a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader);
		}
		else if (GGameType & GAME_Quake2)
		{
			UI_FillPal(x + w - 1 - a, y - h, 1,  h, color);
		}
		else
		{
			float r = (color & 0xff) / 255.0;
			float g = ((color >> 8) & 0xff) / 255.0;
			float b = ((color >> 16) & 0xff) / 255.0;
			UI_Fill(x + w - 1 - a, y - h, 1, h, r, g, b, 1);
		}
	}
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

//	Called for important messages that should stay in the center of the screen
// for a few moments
void SCR_CenterPrint(const char* str)
{
	String::NCpy(scr_centerstring, str, sizeof(scr_centerstring) - 1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.qh_serverTimeFloat;

	// count the number of lines for centering
	scr_center_lines = 1;
	const char* s = str;
	while (*s)
	{
		if (*s == '\n')
		{
			scr_center_lines++;
		}
		s++;
	}

	if (GGameType & GAME_Quake2)
	{
		// echo it to the console
		common->Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

		const char* s = str;
		do
		{
			// scan the width of the line
			int l;
			for (l = 0; l < 40; l++)
			{
				if (s[l] == '\n' || !s[l])
				{
					break;
				}
			}
			int i;
			char line[64];
			for (i = 0; i < (40 - l) / 2; i++)
			{
				line[i] = ' ';
			}

			for (int j = 0; j < l; j++)
			{
				line[i++] = s[j];
			}

			line[i] = '\n';
			line[i + 1] = 0;

			common->Printf("%s", line);

			while (*s && *s != '\n')
			{
				s++;
			}

			if (!*s)
			{
				break;
			}
			s++;		// skip the \n
		}
		while (1);
		common->Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Con_ClearNotify();
	}
}

static void SCR_DrawCenterString()
{
	if (GGameType & GAME_Hexen2)
	{
		SCRH2_DrawCenterString(scr_centerstring);
		return;
	}

	// the finale prints the characters one at a time
	int remaining;
	if (GGameType & GAME_Quake && cl.qh_intermission)
	{
		remaining = scr_printspeed->value * (cl.qh_serverTimeFloat - scr_centertime_start);
	}
	else
	{
		remaining = 9999;
	}

	const char* start = scr_centerstring;

	int y;
	if (scr_center_lines <= 4)
	{
		y = viddef.height * 0.35;
	}
	else
	{
		y = 48;
	}

	do
	{
		// scan the width of the line
		char buf[41];
		int l;
		for (l = 0; l < 40; l++)
		{
			if (start[l] == '\n' || !start[l])
			{
				break;
			}
			buf[l] = start[l];
			if (!remaining)
			{
				break;
			}
			remaining--;
		}
		buf[l] = 0;
		int x = (viddef.width - l * 8) / 2;
		UI_DrawString(x, y, buf);
		if (!remaining)
		{
			return;
		}

		y += 8;

		while (*start && *start != '\n')
		{
			start++;
		}

		if (!*start)
		{
			break;
		}
		start++;		// skip the \n
	}
	while (1);
}

void SCR_CheckDrawCenterString()
{
	scr_centertime_off -= cls.frametime * 0.001;

	if (scr_centertime_off <= 0 && (GGameType & GAME_Quake2 || !cl.qh_intermission))
	{
		return;
	}
	if (!(GGameType & GAME_Quake2) && in_keyCatchers != 0)
	{
		return;
	}

	SCR_DrawCenterString();
}

void SCR_ClearCenterString()
{
	scr_centertime_off = 0;
}
