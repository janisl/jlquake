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

void SCRQ1_DrawPause()
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

void SCRQ1_DrawLoading()
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

void CLQW_NetGraph()
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
