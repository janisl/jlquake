//**************************************************************************
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

struct vrect_t
{
	int		x;
	int		y;
	int		width;
	int		height;
};

struct viddef_t
{
	int		width;		
	int		height;
};

extern viddef_t		viddef;				// global video state

image_t* Draw_CachePic(const char* path);
image_t* Draw_CachePicWithTransPixels(const char* path, byte* TransPixels);
image_t* UI_RegisterPic(const char* name);

void UI_AdjustFromVirtualScreen(float* x, float* y, float* w, float* h);
void DoQuad(float x1, float y1, float s1, float t1,
	float x2, float y2, float s2, float t2);
void UI_DrawPic(int x, int y, image_t* pic);
void UI_DrawNamedPic(int x, int y, const char* name);
