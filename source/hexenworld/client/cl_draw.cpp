
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
#include "../../client/game/quake_hexen2/view.h"

//=============================================================================
/* Support Routines */

/*
===============
Draw_Init
===============
*/
void Draw_Init(void)
{
	char_texture = R_LoadRawFontImageFromFile("gfx/menu/conchars.lmp", 256, 128);
	char_smalltexture = R_LoadRawFontImageFromWad("tinyfont", 128, 32);

	draw_backtile = R_CachePicRepeat("gfx/menu/backtile.lmp");
	Con_InitBackgroundImage();
	MQH_InitImages();
	VQH_InitCrosshairTexture();
}

#define NET_GRAPHHEIGHT 32

#define NET_TIMINGS 256
static int packet_latency[NET_TIMINGS];

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

/*
==============
R_NetGraph
==============
*/
void R_NetGraph(void)
{
	static int lastOutgoingSequence = 0;

	for (int i = clc.netchan.outgoingSequence - UPDATE_BACKUP_HW + 1; i <= clc.netchan.outgoingSequence; i++)
	{
		hwframe_t* frame = &cl.hw_frames[i & UPDATE_MASK_HW];
		if (frame->receivedtime == -1)
		{
			packet_latency[i & 255] = 9999;		// dropped
		}
		else if (frame->receivedtime == -2)
		{
			packet_latency[i & 255] = 10000;	// choked
		}
		else if (frame->invalid)
		{
			packet_latency[i & 255] = 9998;		// invalid delta
		}
		else
		{
			packet_latency[i & 255] = (frame->receivedtime - frame->senttime) * 20;
		}
	}

	for (int a = lastOutgoingSequence + 1; a <= clc.netchan.outgoingSequence; a++)
	{
		int i = a & 255;
		R_LineGraph(packet_latency[i]);
	}
	lastOutgoingSequence = clc.netchan.outgoingSequence;
	SCR_DrawDebugGraph();
}
