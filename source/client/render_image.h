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

void R_InitQ1Palette();
void R_InitQ2Palette();
bool R_ScrapAllocBlock(int w, int h, int* x, int* y);

void R_LoadBMP(const char* FileName, byte** Pic, int* Width, int* Height);

void R_LoadJPG(const char* FileName, byte** Pic, int* Width, int* Height);
void R_SaveJPG(const char* FileName, int Quality, int Width, int Height, byte* Buffer);

void R_LoadPCX(const char* FileName, byte** Pic, byte** Palette, int* Width, int* Height);
void R_LoadPCX32(const char* filename, byte** pic, int* width, int* height);
void R_SavePCXMem(QArray<byte>& buffer, byte* data, int width, int height, byte* palette);

void R_LoadTGA(const char* FileName, byte** Pic, int* Width, int* Height);
void R_SaveTGA(const char* FileName, byte* Data, int Width, int Height, bool HaveAlpha);

extern byte			host_basepal[768];
extern byte			r_palette[256][4];
extern unsigned*	d_8to24table;

extern byte			scrap_texels[SCRAP_BLOCK_WIDTH * SCRAP_BLOCK_HEIGHT * 4];
extern bool			scrap_dirty;
