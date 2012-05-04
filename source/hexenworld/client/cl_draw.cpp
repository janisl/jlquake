
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

image_t* draw_backtile;

image_t* cs_texture;	// crosshair texture
image_t* char_smalltexture;
image_t* char_menufonttexture;

image_t* conback;

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


//==========================================================================
//
// Draw_SmallCharacter
//
// Draws a small character that is clipped at the bottom edge of the
// screen.
//
//==========================================================================
void Draw_SmallCharacter(int x, int y, int num)
{
	if (num < 32)
	{
		num = 0;
	}
	else if (num >= 'a' && num <= 'z')
	{
		num -= 64;
	}
	else if (num > '_')
	{
		num = 0;
	}
	else
	{
		num -= 32;
	}

	if (num == 0)
	{
		return;
	}

	UI_DrawCharBase(x, y, num, 8, 8, char_smalltexture, 16, 4);
}

//==========================================================================
//
// Draw_SmallString
//
//==========================================================================
void Draw_SmallString(int x, int y, const char* str)
{
	while (*str)
	{
		Draw_SmallCharacter(x, y, *str);
		str++;
		x += 6;
	}
}

int M_DrawBigCharacter(int x, int y, int num, int numNext)
{
	int add;

	if (num == ' ')
	{
		return 32;
	}

	if (num == '/')
	{
		num = 26;
	}
	else
	{
		num -= 65;
	}

	if (num < 0 || num >= 27)	// only a-z and /
	{
		return 0;
	}

	if (numNext == '/')
	{
		numNext = 26;
	}
	else
	{
		numNext -= 65;
	}

	UI_DrawCharBase(x, y, num, 20, 20, char_menufonttexture, 8, 4);

	if (numNext < 0 || numNext >= 27)
	{
		return 0;
	}

	add = 0;
	if (num == (int)'C' - 65 && numNext == (int)'P' - 65)
	{
		add = 3;
	}

	return BigCharWidth[num][numNext] + add;
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines)
{
	int y = (viddef.height * 3) >> 2;
	if (lines > y)
	{
		UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback);
	}
	else
	{
		UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback, (float)(1.2 * lines) / y);
	}

	y = lines - 14;
	if (!clc.download)
	{
		char ver[80];
		sprintf(ver, "JLHexenWorld %s", JLQUAKE_VERSION_STRING);
		int x = viddef.width - (String::Length(ver) * 8 + 11);
		UI_DrawString(x, y, ver, 256);
	}
}

//=============================================================================

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

void R_DrawName(vec3_t origin, char* Name, int Red)
{
	if (!Name)
	{
		return;
	}

	int u;
	int v;
	if (!R_GetScreenPosFromWorldPos(origin, u, v))
	{
		return;
	}

	u -= String::Length(Name) * 4;

	if (cl_siege)
	{
		if (Red > 10)
		{
			Red -= 10;
			UI_DrawChar(u, v, 145);	//key
			u += 8;
		}
		if (Red > 0 && Red < 3)	//def
		{
			if (Red == true)
			{
				UI_DrawChar(u, v, 143);	//shield
			}
			else
			{
				UI_DrawChar(u, v, 130);	//crown
			}
			UI_DrawString(u + 8, v, Name, 256);
		}
		else if (!Red)
		{
			UI_DrawChar(u, v, 144);	//sword
			UI_DrawString(u + 8, v, Name);
		}
		else
		{
			UI_DrawString(u + 8, v, Name);
		}
	}
	else
	{
		UI_DrawString(u, v, Name);
	}
}
