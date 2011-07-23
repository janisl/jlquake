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

image_t* UI_CachePic(const char* path);
image_t* UI_CachePicRepeat(const char* path);
image_t* UI_CachePicWithTransPixels(const char* path, byte* TransPixels);
image_t* UI_RegisterPic(const char* name);
image_t* UI_RegisterPicRepeat(const char* name);

int UI_GetImageWidth(image_t* pic);
int UI_GetImageHeight(image_t* pic);
void UI_GetPicSize(int* w, int* h, const char* name);

void UI_AdjustFromVirtualScreen(float* x, float* y, float* w, float* h);
void DoQuad(float x1, float y1, float s1, float t1,
	float x2, float y2, float s2, float t2);
void UI_DrawPic(int x, int y, image_t* pic, float alpha = 1);
void UI_DrawNamedPic(int x, int y, const char* name);
void UI_DrawStretchPic(int x, int y, int w, int h, image_t* gl, float alpha = 1);
void UI_DrawStretchNamedPic(int x, int y, int w, int h, const char *name);
void UI_DrawStretchPicWithColour(int x, int y, int w, int h, image_t* pic, byte* colour);
void UI_DrawSubPic(int x, int y, image_t *pic, int srcx, int srcy, int width, int height);
void UI_TileClear(int x, int y, int w, int h, image_t* pic);
void UI_NamedTileClear(int x, int y, int w, int h, const char *name);
void UI_Fill(int x, int y, int w, int h, float r, float g, float b, float a);
void UI_FillPal(int x, int y, int w, int h, int c);
void UI_DrawChar(int x, int y, int num, int w, int h, image_t* image, int numberOfColumns, int numberOfRows);
