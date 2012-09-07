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
#include "menu.h"

menu_state_t m_state;
menu_state_t m_return_state;

bool m_return_onerror;
char m_return_reason[32];

image_t* char_menufonttexture;
static char BigCharWidth[27][27];

float TitlePercent = 0;
float TitleTargetPercent = 1;
float LogoPercent = 0;
float LogoTargetPercent = 1;

void MQH_DrawPic(int x, int y, image_t* pic)
{
	UI_DrawPic(x + ((viddef.width - 320) >> 1), y, pic);
}

//	Draws one solid graphics character
void MQH_DrawCharacter(int cx, int line, int num)
{
	UI_DrawChar(cx + ((viddef.width - 320) >> 1), line, num);
}

void MQH_Print(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy, str, GGameType & GAME_Hexen2 ? 256 : 128);
}

void MQH_PrintWhite(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy, str);
}

void MQH_DrawTextBox(int x, int y, int width, int lines)
{
	// draw left side
	int cx = x;
	int cy = y;
	image_t* p = R_CachePic("gfx/box_tl.lmp");
	MQH_DrawPic(cx, cy, p);
	p = R_CachePic("gfx/box_ml.lmp");
	for (int n = 0; n < lines; n++)
	{
		cy += 8;
		MQH_DrawPic(cx, cy, p);
	}
	p = R_CachePic("gfx/box_bl.lmp");
	MQH_DrawPic(cx, cy + 8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = R_CachePic("gfx/box_tm.lmp");
		MQH_DrawPic(cx, cy, p);
		p = R_CachePic("gfx/box_mm.lmp");
		for (int n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
			{
				p = R_CachePic("gfx/box_mm2.lmp");
			}
			MQH_DrawPic(cx, cy, p);
		}
		p = R_CachePic("gfx/box_bm.lmp");
		MQH_DrawPic(cx, cy + 8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = R_CachePic("gfx/box_tr.lmp");
	MQH_DrawPic(cx, cy, p);
	p = R_CachePic("gfx/box_mr.lmp");
	for (int n = 0; n < lines; n++)
	{
		cy += 8;
		MQH_DrawPic(cx, cy, p);
	}
	p = R_CachePic("gfx/box_br.lmp");
	MQH_DrawPic(cx, cy + 8, p);
}

void MQH_DrawTextBox2(int x, int y, int width, int lines)
{
	MQH_DrawTextBox(x,  y + ((viddef.height - 200) >> 1), width, lines);
}

void MQH_DrawField(int x, int y, field_t* edit, bool showCursor)
{
	MQH_DrawTextBox(x - 8, y - 8, edit->widthInChars, 1);
	Field_Draw(edit, x + ((viddef.width - 320) >> 1), y, showCursor);
}

void MH2_ReadBigCharWidth()
{
	void* data;
	FS_ReadFile("gfx/menu/fontsize.lmp", &data);
	Com_Memcpy(BigCharWidth, data, sizeof(BigCharWidth));
	FS_FreeFile(data);
}

static int MH2_DrawBigCharacter(int x, int y, int num, int numNext)
{
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

	int add = 0;
	if (num == (int)'C' - 65 && numNext == (int)'P' - 65)
	{
		add = 3;
	}

	return BigCharWidth[num][numNext] + add;
}

void MH2_DrawBigString(int x, int y, const char* string)
{
	x += ((viddef.width - 320) >> 1);

	int length = String::Length(string);
	for (int c = 0; c < length; c++)
	{
		x += MH2_DrawBigCharacter(x, y, string[c], string[c + 1]);
	}
}

void MH2_ScrollTitle(const char* name)
{
	static const char* LastName = "";
	static bool CanSwitch = true;

	float delta;
	if (TitlePercent < TitleTargetPercent)
	{
		delta = ((TitleTargetPercent - TitlePercent) / 0.5) * cls.frametime * 0.001;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		TitlePercent += delta;
		if (TitlePercent > TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
		}
	}
	else if (TitlePercent > TitleTargetPercent)
	{
		delta = ((TitlePercent - TitleTargetPercent) / 0.15) * cls.frametime * 0.001;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		TitlePercent -= delta;
		if (TitlePercent <= TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
			CanSwitch = true;
		}
	}

	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent - LogoPercent) / 0.15) * cls.frametime * 0.001;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	if (String::ICmp(LastName,name) != 0 && TitleTargetPercent != 0)
	{
		TitleTargetPercent = 0;
	}

	if (CanSwitch)
	{
		LastName = name;
		CanSwitch = false;
		TitleTargetPercent = 1;
	}

	image_t* p = R_CachePic(LastName);
	int finaly = ((float)R_GetImageHeight(p) * TitlePercent) - R_GetImageHeight(p);
	MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, finaly, p);

	if (m_state != m_keys)
	{
		p = R_CachePic("gfx/menu/hplaque.lmp");
		finaly = ((float)R_GetImageHeight(p) * LogoPercent) - R_GetImageHeight(p);
		MQH_DrawPic(10, finaly, p);
	}
}
