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

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_InitCommandBuffers
//
//==========================================================================

void R_InitCommandBuffers()
{
	glConfig.smpActive = false;
	if ((GGameType & GAME_Quake3) && r_smp->integer)
	{
		GLog.Write("Trying SMP acceleration...\n");
		if (GLimp_SpawnRenderThread(RB_RenderThread))
		{
			GLog.Write("...succeeded.\n");
			glConfig.smpActive = true;
		}
		else
		{
			GLog.Write("...failed.\n");
		}
	}
}

//==========================================================================
//
//	R_ShutdownCommandBuffers
//
//==========================================================================

void R_ShutdownCommandBuffers()
{
	// kill the rendering thread
	if (glConfig.smpActive)
	{
		GLimp_WakeRenderer(NULL);
		glConfig.smpActive = false;
	}
}
