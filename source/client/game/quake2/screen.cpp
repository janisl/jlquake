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

#include "local.h"
#include "../../client_main.h"
#include "../../public.h"
#include "../../cinematic/public.h"
#include "../../ui/ui.h"
#include "../../ui/console.h"
#include "../quake_hexen2/main.h"

static void SCRQ2_DrawPause()
{
	if (!scr_showpause->value)		// turn off for screenshots
	{
		return;
	}

	if (!cl_paused->value)
	{
		return;
	}

	int w, h;
	R_GetPicSize(&w, &h, "pause");
	UI_DrawNamedPic((viddef.width - w) / 2, viddef.height / 2 + 8, "pause");
}

static void SCRQ2_DrawLoading()
{
	if (!scr_draw_loading)
	{
		return;
	}

	scr_draw_loading = false;
	int w, h;
	R_GetPicSize(&w, &h, "loading");
	UI_DrawNamedPic((viddef.width - w) / 2, (viddef.height - h) / 2, "loading");
}

//	A new packet was just parsed
void CLQ2_AddNetgraph()
{
	// if using the debuggraph for something else, don't
	// add the net lines
	if (cl_debuggraph->value || cl_timegraph->value)
	{
		return;
	}

	for (int i = 0; i < clc.netchan.dropped; i++)
	{
		SCR_DebugGraph(30, 0x40);
	}

	for (int i = 0; i < cl.q2_surpressCount; i++)
	{
		SCR_DebugGraph(30, 0xdf);
	}

	// see what the latency was on this packet
	int in = clc.netchan.incomingAcknowledged & (CMD_BACKUP_Q2 - 1);
	int ping = cls.realtime - cl.q2_cmd_time[in];
	ping /= 30;
	if (ping > 30)
	{
		ping = 30;
	}
	SCR_DebugGraph(ping, 0xd0);
}

void SCRQ2_DrawScreen(stereoFrame_t stereoFrame, float separation)
{
	R_BeginFrame(stereoFrame);

	if (scr_draw_loading == 2)
	{	//  loading plaque over black screen
		int w, h;

		UI_Fill(0, 0, viddef.width, viddef.height, 0, 0, 0, 1);
		scr_draw_loading = false;
		R_GetPicSize(&w, &h, "loading");
		UI_DrawNamedPic((viddef.width - w) / 2, (viddef.height - h) / 2, "loading");
	}
	// if a cinematic is supposed to be running, handle menus
	// and console specially
	else if (SCR_DrawCinematic())
	{
		if (in_keyCatchers & KEYCATCH_UI)
		{
			UI_DrawMenu();
		}
		else if (in_keyCatchers & KEYCATCH_CONSOLE)
		{
			Con_DrawConsole();
		}
	}
	else
	{
		// do 3D refresh drawing, and then update the screen
		SCR_CalcVrect();

		// clear any dirty part of the background
		SCR_TileClear();

		VQ2_RenderView(separation);

		SCRQ2_DrawHud();

		if (cl_timegraph->value)
		{
			SCR_DebugGraph(cls.frametime * 0.25, 0);
		}

		if (cl_debuggraph->value || cl_timegraph->value || scr_netgraph->value)
		{
			SCR_DrawDebugGraph();
		}

		SCRQ2_DrawPause();

		Con_DrawConsole();

		UI_DrawMenu();

		SCRQ2_DrawLoading();
	}
	SCR_DrawFPS();
}

//	Set a specific sky and rotation speed
static void SCR_Sky_f()
{
	if (Cmd_Argc() < 2)
	{
		common->Printf("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	float rotate;
	if (Cmd_Argc() > 2)
	{
		rotate = String::Atof(Cmd_Argv(2));
	}
	else
	{
		rotate = 0;
	}
	vec3_t axis;
	if (Cmd_Argc() == 6)
	{
		axis[0] = String::Atof(Cmd_Argv(3));
		axis[1] = String::Atof(Cmd_Argv(4));
		axis[2] = String::Atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	R_SetSky(Cmd_Argv(1), rotate, axis);
}

static void SCR_Loading_f()
{
	SCRQ2_BeginLoadingPlaque(false);
}

void SCRQ2_Init()
{
	Cmd_AddCommand("loading", SCR_Loading_f);
	Cmd_AddCommand("sky", SCR_Sky_f);
}
