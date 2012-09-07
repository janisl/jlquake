
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

image_t* cs_texture;	// crosshair texture

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
	char_menufonttexture = R_LoadBigFontImage("gfx/menu/bigfont2.lmp");

	cs_texture = R_CreateCrosshairImage();

	conback = R_CachePic("gfx/menu/conback.lmp");

	draw_backtile = R_CachePicRepeat("gfx/menu/backtile.lmp");
}

void Draw_Crosshair(void)
{
	extern Cvar* crosshair;
	extern Cvar* cl_crossx;
	extern Cvar* cl_crossy;
	int x, y;
	extern vrect_t scr_vrect;

	if (crosshair->value == 2)
	{
		x = scr_vrect.x + scr_vrect.width / 2 - 3 + cl_crossx->value;
		y = scr_vrect.y + scr_vrect.height / 2 - 3 + cl_crossy->value;

		UI_DrawStretchPic(x - 4, y - 4, 16, 16, cs_texture);
	}
	else if (crosshair->value)
	{
		UI_DrawChar(scr_vrect.x + scr_vrect.width / 2 - 4 + cl_crossx->value,
			scr_vrect.y + scr_vrect.height / 2 - 4 + cl_crossy->value,
			'+');
	}
}


/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void)
{
	UI_Fill(0, 0, viddef.width, viddef.height, 208.0 / 255.0, 180.0 / 255.0, 80.0 / 255.0, 0.2);
	for (int c = 0; c < 40; c++)
	{
		int x = rand() % viddef.width - 20;
		int y = rand() % viddef.height - 20;
		int w = (rand() % 40) + 20;
		int h = (rand() % 40) + 20;
		UI_Fill(x, y, w, h, 208.0 / 255.0, 180.0 / 255.0, 80.0 / 255.0, 0.035);
	}
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
