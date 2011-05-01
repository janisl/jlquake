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

#define FILE_HASH_SIZE		1024

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

struct image_t
{
	char		imgName[MAX_QPATH];			// game path, including extension
	int			width, height;				// source image
	int			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	GLuint		texnum;						// gl texture binding

	int			frameUsed;					// for texture usage in frame statistics

	int			internalFormat;

	bool		mipmap;
	bool		allowPicmip;
	int			wrapClampMode;				// GL_CLAMP or GL_REPEAT

	image_t*	next;

	float		sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	bool		scrap;

	struct msurface_s*	texturechain;	// for sort-by-texture world drawing
};

struct textureMode_t
{
	const char*		name;
	GLenum			minimize;
	GLenum			maximize;
};

void R_InitQ1Palette();
void R_InitQ2Palette();
byte* R_ConvertImage8To32(byte* DataIn, int Width, int Height, int Mode);
void R_LoadImage(const char* FileName, byte** Pic, int* Width, int* Height, int Mode = IMG8MODE_Normal, byte* TransPixels = NULL);
void R_UploadImage(byte* Data, int Width, int Height, bool MipMap, bool PicMip, bool LightMap, int* Format, int* UploadWidth, int* UploadHeight);
image_t* R_CreateImage(const char* Name, byte* Data, int Width, int Height, bool MipMap, bool AllowPicMip, GLenum WrapClampMode, bool AllowScrap);
image_t* R_FindImage(const char* name);
image_t* R_FindImageFile(const char* name, bool mipmap, bool allowPicmip, GLenum glWrapClampMode, bool AllowScrap = false, int Mode = IMG8MODE_Normal, byte* TransPixels = NULL);
void R_SetColorMappings();
void R_GammaCorrect(byte* Buffer, int BufferSize);
void GL_TextureMode(const char* string);

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

extern	int			gl_filter_min, gl_filter_max;

extern image_t*		ImageHashTable[FILE_HASH_SIZE];

extern textureMode_t modes[];
