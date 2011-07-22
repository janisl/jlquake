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

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

viddef_t	viddef;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	UI_CachePic
//
//==========================================================================

image_t* UI_CachePic(const char* path)
{
	image_t* pic = R_FindImageFile(path, false, false, GL_CLAMP);
	if (!pic)
	{
		throw QException(va("UI_CachePic: failed to load %s", path));
	}
	return pic;
}

//==========================================================================
//
//	UI_CachePicRepeat
//
//==========================================================================

image_t* UI_CachePicRepeat(const char* path)
{
	image_t* pic = R_FindImageFile(path, false, false, GL_REPEAT);
	if (!pic)
	{
		throw QException(va("UI_CachePic: failed to load %s", path));
	}
	return pic;
}

//==========================================================================
//
//	UI_CachePicWithTransPixels
//
//==========================================================================

image_t* UI_CachePicWithTransPixels(const char *path, byte* TransPixels)
{
	image_t* pic = R_FindImageFile(path, false, false, GL_CLAMP, false, IMG8MODE_Normal, TransPixels);
	if (!pic)
	{
		throw QException(va("UI_CachePic: failed to load %s", path));
	}
	return pic;
}

//==========================================================================
//
//	UI_RegisterPic
//
//==========================================================================

static image_t* UI_RegisterPic(const char* name, GLenum wrapClampMode)
{
	if (name[0] != '/' && name[0] != '\\')
	{
		char fullname[MAX_QPATH];
		QStr::Sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
		return R_FindImageFile(fullname, false, false, wrapClampMode, true);
	}
	else
	{
		return R_FindImageFile(name + 1, false, false, wrapClampMode, true);
	}
}

//==========================================================================
//
//	UI_RegisterPic
//
//==========================================================================

image_t* UI_RegisterPic(const char* name)
{
	return UI_RegisterPic(name, GL_CLAMP);
}

//==========================================================================
//
//	UI_RegisterPicRepeat
//
//==========================================================================

image_t* UI_RegisterPicRepeat(const char* name)
{
	return UI_RegisterPic(name, GL_REPEAT);
}

//==========================================================================
//
//	UI_GetImageWidth
//
//==========================================================================

int UI_GetImageWidth(image_t* pic)
{
	return pic->width;
}

//==========================================================================
//
//	UI_GetImageHeight
//
//==========================================================================

int UI_GetImageHeight(image_t* pic)
{
	return pic->height;
}

//==========================================================================
//
//	UI_GetPicSize
//
//==========================================================================

void UI_GetPicSize(int* w, int* h, const char* pic)
{
	image_t* gl = UI_RegisterPic(pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

//==========================================================================
//
//	UI_AdjustFromVirtualScreen
//
//	Adjusted for resolution and screen aspect ratio
//
//==========================================================================

void UI_AdjustFromVirtualScreen(float* x, float* y, float* w, float* h)
{
	// scale for screen sizes
	float xscale = (float)glConfig.vidWidth / viddef.width;
	float yscale = (float)glConfig.vidHeight / viddef.height;
	if (x)
	{
		*x *= xscale;
	}
	if (y)
	{
		*y *= yscale;
	}
	if (w)
	{
		*w *= xscale;
	}
	if (h)
	{
		*h *= yscale;
	}
}

//==========================================================================
//
//	DoQuad
//
//==========================================================================

void DoQuad(float x1, float y1, float s1, float t1,
	float x2, float y2, float s2, float t2)
{
	UI_AdjustFromVirtualScreen(&x1, &y1, &x2, &y2);

	qglBegin(GL_QUADS);
	qglTexCoord2f(s1, t1);
	qglVertex2f(x1, y1);
	qglTexCoord2f(s2, t1);
	qglVertex2f(x2, y1);
	qglTexCoord2f(s2, t2);
	qglVertex2f (x2, y2);
	qglTexCoord2f(s1, t2);
	qglVertex2f(x1, y2);
	qglEnd();
}

//==========================================================================
//
//	UI_DrawPic
//
//==========================================================================

void UI_DrawPic(int x, int y, image_t* pic, float alpha)
{
	UI_DrawStretchPic(x, y, pic->width, pic->height, pic, alpha);
}

//==========================================================================
//
//	UI_DrawNamedPic
//
//==========================================================================

void UI_DrawNamedPic(int x, int y, const char* pic)
{
	image_t* gl = UI_RegisterPic(pic);
	if (!gl)
	{
		GLog.Write("Can't find pic: %s\n", pic);
		return;
	}
	UI_DrawPic(x, y, gl);
}

//==========================================================================
//
//	UI_DrawStretchPic
//
//==========================================================================

void UI_DrawStretchPic(int x, int y, int w, int h, image_t* pic, float alpha)
{
	if (scrap_dirty)
	{
		R_ScrapUpload();
	}
	qglColor4f(1, 1, 1, alpha);
	GL_Bind(pic);
	GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	DoQuad(x, y, pic->sl, pic->tl, x + w, y + h, pic->sh, pic->th);
}

//==========================================================================
//
//	UI_DrawStretchNamedPic
//
//==========================================================================

void UI_DrawStretchNamedPic(int x, int y, int w, int h, const char* pic)
{
	image_t* gl = UI_RegisterPic(pic);
	if (!gl)
	{
		GLog.Write("Can't find pic: %s\n", pic);
		return;
	}
	UI_DrawStretchPic(x, y, w, h, gl);
}

//==========================================================================
//
//	UI_DrawSubPic
//
//==========================================================================

void UI_DrawSubPic(int x, int y, image_t* pic, int srcx, int srcy, int width, int height)
{
	float newsl, newtl, newsh, newth;
	float oldglwidth, oldglheight;

	if (scrap_dirty)
		R_ScrapUpload();
	
	oldglwidth = pic->sh - pic->sl;
	oldglheight = pic->th - pic->tl;

	newsl = pic->sl + (srcx*oldglwidth)/pic->width;
	newsh = newsl + (width*oldglwidth)/pic->width;

	newtl = pic->tl + (srcy*oldglheight)/pic->height;
	newth = newtl + (height*oldglheight)/pic->height;
	
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	
	qglColor4f (1,1,1,1);
	GL_Bind (pic);
	DoQuad(x, y, newsl, newtl, x + width, y + height, newsh, newth);
}

//==========================================================================
//
//	UI_TileClear
//
//	This repeats a 64*64 tile graphic to fill the screen around a sized down
// refresh window.
//
//==========================================================================

void UI_TileClear(int x, int y, int w, int h, image_t* pic)
{
	qglColor4f(1, 1, 1, 1);
	GL_Bind(pic);
	GL_State(GLS_DEPTHTEST_DISABLE);
	DoQuad(x, y, x / 64.0, y / 64.0, x + w, y + h, (x + w) / 64.0, (y + h) / 64.0);
}

//==========================================================================
//
//	UI_NamedTileClear
//
//==========================================================================

void UI_NamedTileClear(int x, int y, int w, int h, const char* pic)
{
	image_t* image = UI_RegisterPicRepeat(pic);
	if (!image)
	{
		GLog.Write("Can't find pic: %s\n", pic);
		return;
	}
	UI_TileClear(x, y, w, h, image);
}
