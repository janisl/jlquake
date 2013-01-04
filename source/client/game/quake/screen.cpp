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
#include "../../ui/ui.h"
#include "../quake_hexen2/view.h"
#include "../quake_hexen2/main.h"
#include "../../ui/console.h"
#include "../../../common/common_defs.h"
#include <time.h>

static Cvar* scrqw_allowsnap;

static void SCRQ1_DrawPause()
{
	if (!scr_showpause->value)		// turn off for screenshots
	{
		return;
	}

	if (!cl.qh_paused)
	{
		return;
	}

	image_t* pic = R_CachePic("gfx/pause.lmp");
	UI_DrawPic((viddef.width - R_GetImageWidth(pic)) / 2,
		(viddef.height - 48 - R_GetImageHeight(pic)) / 2, pic);
}

static void SCRQ1_DrawLoading()
{
	if (!scr_draw_loading)
	{
		return;
	}

	image_t* pic = R_CachePic("gfx/loading.lmp");
	UI_DrawPic((viddef.width - R_GetImageWidth(pic)) / 2,
		(viddef.height - 48 - R_GetImageHeight(pic)) / 2, pic);
}

#define NET_GRAPHHEIGHT 32

static void R_LineGraph(int h)
{
	int colour;
	if (h == 10000)
	{
		// yellow
		colour = 0x00ffff;
	}
	else if (h == 9999)
	{
		// red
		colour = 0x0000ff;
	}
	else if (h == 9998)
	{
		// blue
		colour = 0xff0000;
	}
	else
	{
		// white
		colour = 0xffffff;
	}

	if (h > NET_GRAPHHEIGHT)
	{
		h = NET_GRAPHHEIGHT;
	}

	SCR_DebugGraph(h, colour);
}

static void CLQW_NetGraph()
{
	static int lastOutgoingSequence = 0;

	CLQW_CalcNet();

	for (int a = lastOutgoingSequence + 1; a <= clc.netchan.outgoingSequence; a++)
	{
		int i = a & NET_TIMINGSMASK_QH;
		R_LineGraph(clqh_packet_latency[i]);
	}
	lastOutgoingSequence = clc.netchan.outgoingSequence;
	SCR_DrawDebugGraph();
}

void SCRQ1_DrawScreen(stereoFrame_t stereoFrame)
{
	if (cls.disable_screen)
	{
		if (cls.realtime - cls.disable_screen > 60000)
		{
			cls.disable_screen = 0;
			common->Printf("load failed.\n");
		}
		else
		{
			return;
		}
	}

	if (!scr_initialized || !cls.glconfig.vidWidth)
	{
		return;				// not initialized yet

	}
	R_BeginFrame(stereoFrame);

	//
	// determine size of refresh window
	//
	SCR_CalcVrect();

//
// do 3D refresh drawing, and then update the screen
//
	VQH_RenderView();

	//
	// draw any areas not covered by the refresh
	//
	SCR_TileClear();

	if (GGameType & GAME_QuakeWorld && scr_netgraph->value)
	{
		CLQW_NetGraph();
	}

	if (scr_draw_loading)
	{
		SCRQ1_DrawLoading();
		SbarQ1_Draw();
	}
	else if (cl.qh_intermission == 1 && in_keyCatchers == 0)
	{
		SbarQ1_IntermissionOverlay();
	}
	else if (cl.qh_intermission == 2 && in_keyCatchers == 0)
	{
		SbarQ1_FinaleOverlay();
		SCR_CheckDrawCenterString();
	}
	else
	{
		SCR_DrawNet();
		SCR_DrawFPS();
		SCRQH_DrawTurtle();
		SCRQ1_DrawPause();
		SCR_CheckDrawCenterString();
		SbarQ1_Draw();
		Con_DrawConsole();
		UI_DrawMenu();
	}

	R_EndFrame(NULL, NULL);
}

static void SCRQW_RSShot_f()
{
	if (CLQW_IsUploading())
	{
		return;	// already one pending
	}

	if (cls.state < CA_LOADING)
	{
		return;	// gotta be connected
	}

	if (!scrqw_allowsnap->integer)
	{
		CL_AddReliableCommand("snap\n");
		common->Printf("Refusing remote screen shot request.\n");
		return;
	}

	common->Printf("Remote screen shot requested.\n");

	time_t now;
	time(&now);

	Array<byte> buffer;
	R_CaptureRemoteScreenShot(ctime(&now), cls.servername, clqh_name->string, buffer);
	CLQW_StartUpload(buffer.Ptr(), buffer.Num());
}

void SCRQ1_Init()
{
	if (GGameType & GAME_QuakeWorld)
	{
		Cmd_AddCommand("snap", SCRQW_RSShot_f);

		scrqw_allowsnap = Cvar_Get("scr_allowsnap", "1", 0);
		clqh_sbar = Cvar_Get("cl_sbar", "0", CVAR_ARCHIVE);
	}
}
