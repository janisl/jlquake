//**************************************************************************
//**
//**	$Id$
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

#define SCRAP_BLOCK_WIDTH	256
#define SCRAP_BLOCK_HEIGHT	256

//	Hexen 2 model texture modes, plus special type for skins.
enum
{
	//	standard
	IMG8MODE_Normal,
	//	color 0 transparent
	IMG8MODE_Holey,
	//	All skin modes will fllo fill image
	IMG8MODE_Skin,
	//	color 0 transparent, odd - translucent, even - full value
	IMG8MODE_SkinTransparent,
	//	color 0 transparent
	IMG8MODE_SkinHoley,
	//	special (particle translucency table)
	IMG8MODE_SkinSpecialTrans,
};

void R_InitQ1Palette();
void R_InitQ2Palette();
byte* R_ConvertImage8To32(byte* DataIn, int Width, int Height, int Mode);
void R_LoadImage(const char* FileName, byte** Pic, int* Width, int* Height, int Mode = IMG8MODE_Normal, byte* TransPixels = NULL);
void R_ResampleTexture(byte* In, int InWidth, int InhHeight, byte* Out, int OutWidth, int OutHeight);
void R_MipMap(byte* in, int width, int height);
void R_LightScaleTexture (byte* in, int inwidth, int inheight, qboolean only_gamma );
bool R_ScrapAllocBlock(int w, int h, int* x, int* y);
void R_SetColorMappings();
void R_GammaCorrect(byte* Buffer, int BufferSize);

void R_LoadBMP(const char* FileName, byte** Pic, int* Width, int* Height);

void R_LoadJPG(const char* FileName, byte** Pic, int* Width, int* Height);
void R_SaveJPG(const char* FileName, int Quality, int Width, int Height, byte* Buffer);

void R_LoadPCX(const char* FileName, byte** Pic, byte** Palette, int* Width, int* Height);
void R_LoadPCX32(const char* filename, byte** pic, int* width, int* height, int Mode);
void R_SavePCXMem(QArray<byte>& buffer, byte* data, int width, int height, byte* palette);

void R_LoadPICMem(byte* Data, byte** Pic, int* Width, int* Height, byte* TransPixels = NULL, int Mode = IMG8MODE_Normal);
void R_LoadPIC(const char* FileName, byte** Pic, int* Width, int* Height, byte* TransPixels = NULL, int Mode = IMG8MODE_Normal);

void R_LoadTGA(const char* FileName, byte** Pic, int* Width, int* Height);
void R_SaveTGA(const char* FileName, byte* Data, int Width, int Height, bool HaveAlpha);

void R_LoadWAL(const char* FileName, byte** Pic, int* Width, int* Height);

extern byte			host_basepal[768];
extern byte			r_palette[256][4];
extern unsigned*	d_8to24table;

extern int			ColorIndex[16];
extern unsigned		ColorPercent[16];

extern byte			scrap_texels[SCRAP_BLOCK_WIDTH * SCRAP_BLOCK_HEIGHT * 4];
extern bool			scrap_dirty;
