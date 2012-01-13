
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"

image_t*	draw_backtile;

image_t*	char_texture;
image_t*	cs_texture; // crosshair texture
image_t*	char_smalltexture;
image_t*	char_menufonttexture;

image_t		*conback;

//=============================================================================
/* Support Routines */

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	char_texture = R_LoadRawFontImageFromFile("gfx/menu/conchars.lmp", 256, 128);
	char_smalltexture = R_LoadRawFontImageFromWad("tinyfont", 128, 32);
	char_menufonttexture = R_LoadBigFontImage("gfx/menu/bigfont2.lmp");

	cs_texture = R_CreateCrosshairImage();

	conback = R_CachePic("gfx/menu/conback.lmp");

	draw_backtile = R_CachePicRepeat("gfx/menu/backtile.lmp");
}



/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, unsigned int num)
{
	num &= 511;

	if (num == 32)
		return;		// space

	UI_DrawChar(x, y, num, 8, 8, char_texture, 32, 16);
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, const char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

void Draw_RedString (int x, int y, const char *str)
{
	while (*str)
	{
		Draw_Character (x, y, ((unsigned char)(*str))+256);
		str++;
		x += 8;
	}
}

void Draw_Crosshair(void)
{
	extern Cvar* crosshair;
	extern Cvar* cl_crossx;
	extern Cvar* cl_crossy;
	int x, y;
	extern vrect_t		scr_vrect;

	if (crosshair->value == 2) {
		x = scr_vrect.x + scr_vrect.width/2 - 3 + cl_crossx->value; 
		y = scr_vrect.y + scr_vrect.height/2 - 3 + cl_crossy->value;

		UI_DrawStretchPic(x - 4, y - 4, 16, 16, cs_texture);
	} else if (crosshair->value)
		Draw_Character (scr_vrect.x + scr_vrect.width/2-4 + cl_crossx->value, 
			scr_vrect.y + scr_vrect.height/2-4 + cl_crossy->value, 
			'+');
}


//==========================================================================
//
// Draw_SmallCharacter
//
// Draws a small character that is clipped at the bottom edge of the
// screen.
//
//==========================================================================
void Draw_SmallCharacter (int x, int y, int num)
{
	if(num < 32)
	{
		num = 0;
	}
	else if(num >= 'a' && num <= 'z')
	{
		num -= 64;
	}
	else if(num > '_')
	{
		num = 0;
	}
	else
	{
		num -= 32;
	}

	if (num == 0) return;

	UI_DrawChar(x, y, num, 8, 8, char_smalltexture, 16, 4);
}

//==========================================================================
//
// Draw_SmallString
//
//==========================================================================
void Draw_SmallString(int x, int y, const char *str)
{
	while (*str)
	{
		Draw_SmallCharacter (x, y, *str);
		str++;
		x += 6;
	}
}

int M_DrawBigCharacter (int x, int y, int num, int numNext)
{
	int				add;

	if (num == ' ') return 32;

	if (num == '/') num = 26;
	else num -= 65;

	if (num < 0 || num >= 27)  // only a-z and /
		return 0;

	if (numNext == '/') numNext = 26;
	else numNext -= 65;

	UI_DrawChar(x, y, num, 20, 20, char_menufonttexture, 8, 4);

	if (numNext < 0 || numNext >= 27) return 0;

	add = 0;
	if (num == (int)'C'-65 && numNext == (int)'P'-65)
		add = 3;

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
		Draw_RedString(x, y, ver);
	}
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	UI_Fill(0, 0, viddef.width, viddef.height, 208.0/255.0, 180.0/255.0, 80.0/255.0, 0.2);
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

#define	NET_TIMINGS	256
static	int	packet_latency[NET_TIMINGS];

static void R_LineGraph(int x, int y, int h)
{
	int r, g, b;
	if (h == 10000)
	{
		// yellow
		r = 1;
		g = 1;
		b = 0;
	}
	else if (h == 9999)
	{
		// red
		r = 1;
		g = 0;
		b = 0;
	}
	else if (h == 9998)
	{
		// blue
		r = 0;
		g = 0;
		b = 1;
	}
	else
	{
		// white
		r = 1;
		g = 1;
		b = 1;
	}

	if (h > NET_GRAPHHEIGHT)
	{
		h = NET_GRAPHHEIGHT;
	}

	UI_Fill(8 + x, y + NET_GRAPHHEIGHT - h, 1, h, r, g, b, 1);
}

/*
==============
R_NetGraph
==============
*/
void R_NetGraph (void)
{
	int		x, y;
	hwframe_t	*frame;
	char st[80];

	for (int i = clc.netchan.outgoingSequence - UPDATE_BACKUP_HW + 1; i <= clc.netchan.outgoingSequence; i++)
	{
		frame = &cl.hw_frames[i&UPDATE_MASK_HW];
		if (frame->receivedtime == -1)
			packet_latency[i&255] = 9999;	// dropped
		else if (frame->receivedtime == -2)
			packet_latency[i&255] = 10000;	// choked
		else if (frame->invalid)
			packet_latency[i&255] = 9998;	// invalid delta
		else
			packet_latency[i&255] = (frame->receivedtime - frame->senttime)*20;
	}

	x = 0;
	int lost = 0;
	for (int a = 0; a < NET_TIMINGS; a++)
	{
		int i = (clc.netchan.outgoingSequence - a) & 255;
		if (packet_latency[i] == 9999)
		{
			lost++;
		}
	}

	x =	-((viddef.width - 320) >> 1);
	y = viddef.height - sb_lines - 24 - NET_GRAPHHEIGHT - 1;

	M_DrawTextBox(x, y, NET_TIMINGS / 8, NET_GRAPHHEIGHT / 8 + 1);
	y += 8;

	sprintf(st, "%3i%% packet loss", lost * 100 / NET_TIMINGS);
	Draw_String(8, y, st);
	y += 8;

	for (int a = 0; a < NET_TIMINGS; a++)
	{
		int i = (clc.netchan.outgoingSequence - a) & 255;
		R_LineGraph(NET_TIMINGS - 1 - a, y, packet_latency[i]);
	}
}

void R_DrawName(vec3_t origin, char *Name, int Red)
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

	if(cl_siege)
	{
		if(Red>10)
		{
			Red-=10;
			Draw_Character (u, v, 145);//key
			u+=8;
		}
		if(Red>0&&Red<3)//def
		{
			if(Red==true)
				Draw_Character (u, v, 143);//shield
			else
				Draw_Character (u, v, 130);//crown
			Draw_RedString(u+8, v, Name);
		}
		else if(!Red)
		{
			Draw_Character (u, v, 144);//sword
			Draw_String (u+8, v, Name);
		}
		else
			Draw_String (u+8, v, Name);
	}
	else
		Draw_String (u, v, Name);
}
