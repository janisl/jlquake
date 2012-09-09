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

float h2_introTime = 0.0;

void SCR_InitCommon()
{
	cl_graphheight = Cvar_Get("graphheight", "32", CVAR_CHEAT);
	cl_graphscale = Cvar_Get("graphscale", "1", CVAR_CHEAT);
	cl_graphshift = Cvar_Get("graphshift", "0", CVAR_CHEAT);
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
	int a, x, y, w, i, h;
	float v;
	int color;

	//
	// draw the graph
	//
	x = 0;
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

	for (a = 0; a < w; a++)
	{
		i = (current - 1 - a + 1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v * cl_graphscale->integer + cl_graphshift->integer;

		if (v < 0)
		{
			v += cl_graphheight->integer * (1 + (int)(-v / cl_graphheight->integer));
		}
		h = (int)v % cl_graphheight->integer;
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
