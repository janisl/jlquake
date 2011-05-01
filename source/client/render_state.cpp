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

glstate_t	glState;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	GL_Bind
//
//==========================================================================

void GL_Bind(image_t* image)
{
	int texnum;
	if (!image)
	{
		GLog.Write(S_COLOR_YELLOW "GL_Bind: NULL image\n");
		texnum = tr.defaultImage->texnum;
	}
	else
	{
		texnum = image->texnum;
	}

	if (r_nobind->integer && tr.dlightImage)
	{
		// performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if (glState.currenttextures[glState.currenttmu] == texnum)
	{
		return;
	}

	image->frameUsed = tr.frameCount;
	glState.currenttextures[glState.currenttmu] = texnum;
	qglBindTexture(GL_TEXTURE_2D, texnum);
}

//==========================================================================
//
//	GL_SelectTexture
//
//==========================================================================

void GL_SelectTexture(int unit)
{
	if (glState.currenttmu == unit)
	{
		return;
	}

	if (unit == 0)
	{
		qglActiveTextureARB(GL_TEXTURE0_ARB);
		QGL_LogComment("glActiveTextureARB( GL_TEXTURE0_ARB )\n");
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		QGL_LogComment("glClientActiveTextureARB( GL_TEXTURE0_ARB )\n");
	}
	else if (unit == 1)
	{
		qglActiveTextureARB(GL_TEXTURE1_ARB);
		QGL_LogComment("glActiveTextureARB( GL_TEXTURE1_ARB )\n");
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		QGL_LogComment("glClientActiveTextureARB( GL_TEXTURE1_ARB )\n");
	}
	else
	{
		throw QDropException(va("GL_SelectTexture: unit = %i", unit));
	}

	glState.currenttmu = unit;
}
