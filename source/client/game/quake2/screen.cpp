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
#include "../quake_hexen2/main.h"

Cvar* scrq2_debuggraph;
Cvar* scrq2_timegraph;

void SCRQ2_DrawPause()
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

void SCRQ2_DrawLoading()
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

#define NET_GRAPHHEIGHT 32

static void CLHW_LineGraph(int h)
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

void CLHW_NetGraph()
{
	static int lastOutgoingSequence = 0;

	for (int i = clc.netchan.outgoingSequence - UPDATE_BACKUP_HW + 1; i <= clc.netchan.outgoingSequence; i++)
	{
		hwframe_t* frame = &cl.hw_frames[i & UPDATE_MASK_HW];
		if (frame->receivedtime == -1)
		{
			clqh_packet_latency[i & 255] = 9999;		// dropped
		}
		else if (frame->receivedtime == -2)
		{
			clqh_packet_latency[i & 255] = 10000;	// choked
		}
		else if (frame->invalid)
		{
			clqh_packet_latency[i & 255] = 9998;		// invalid delta
		}
		else
		{
			clqh_packet_latency[i & 255] = (frame->receivedtime - frame->senttime) * 20;
		}
	}

	for (int a = lastOutgoingSequence + 1; a <= clc.netchan.outgoingSequence; a++)
	{
		int i = a & 255;
		CLHW_LineGraph(clqh_packet_latency[i]);
	}
	lastOutgoingSequence = clc.netchan.outgoingSequence;
	SCR_DrawDebugGraph();
}

//	A new packet was just parsed
void CLQ2_AddNetgraph()
{
	// if using the debuggraph for something else, don't
	// add the net lines
	if (scrq2_debuggraph->value || scrq2_timegraph->value)
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
