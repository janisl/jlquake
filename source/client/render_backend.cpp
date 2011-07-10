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

backEndData_t*	backEndData[SMP_FRAMES];
backEndState_t	backEnd;

int				max_polys;
int				max_polyverts;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_InitBackEndData
//
//==========================================================================

void R_InitBackEndData()
{
	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
	{
		max_polys = MAX_POLYS;
	}

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
	{
		max_polyverts = MAX_POLYVERTS;
	}

	byte* ptr = (byte*)Mem_Alloc(sizeof(*backEndData[0]) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts);
	backEndData[0] = (backEndData_t*)ptr;
	backEndData[0]->polys = (srfPoly_t*)((char*)ptr + sizeof(*backEndData[0]));
	backEndData[0]->polyVerts = (polyVert_t*)((char*)ptr + sizeof(*backEndData[0]) + sizeof(srfPoly_t) * max_polys);
	if (r_smp->integer)
	{
		ptr = (byte*)Mem_Alloc(sizeof(*backEndData[1]) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts);
		backEndData[1] = (backEndData_t*)ptr;
		backEndData[1]->polys = (srfPoly_t*)((char*)ptr + sizeof(*backEndData[1]));
		backEndData[1]->polyVerts = (polyVert_t*)((char*)ptr + sizeof(*backEndData[1]) + sizeof(srfPoly_t) * max_polys);
	}
	else
	{
		backEndData[1] = NULL;
	}
	R_ToggleSmpFrame();
}

//==========================================================================
//
//	R_FreeBackEndData
//
//==========================================================================

void R_FreeBackEndData()
{
	Mem_Free(backEndData[0]);
	backEndData[0] = NULL;

	if (backEndData[1])
	{
		Mem_Free(backEndData[1]);
		backEndData[1] = NULL;
	}
}

//==========================================================================
//
//	RB_SetColor
//
//==========================================================================

const void* RB_SetColor(const void* data)
{
	const setColorCommand_t* cmd = (const setColorCommand_t*)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void*)(cmd + 1);
}

//==========================================================================
//
//	RB_SetColor
//
//==========================================================================

const void* RB_DrawBuffer(const void* data)
{
	const drawBufferCommand_t* cmd = (const drawBufferCommand_t*)data;

	qglDrawBuffer(cmd->buffer);

	// clear screen for debugging
	if (r_clear->integer)
	{
		qglClearColor(1, 0, 0.5, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return (const void*)(cmd + 1);
}

//==========================================================================
//
//	SetViewportAndScissor
//
//==========================================================================

static void SetViewportAndScissor()
{
	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf(backEnd.viewParms.projectionMatrix);
	qglMatrixMode(GL_MODELVIEW);

	// set the window clipping
	qglViewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
	qglScissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
}

//==========================================================================
//
//	RB_Hyperspace
//
//	A player has predicted a teleport, but hasn't arrived yet
//
//==========================================================================

static void RB_Hyperspace()
{
	if (!backEnd.isHyperspace)
	{
		// do initialization shit
	}

	float c = (backEnd.refdef.time & 255) / 255.0f;
	qglClearColor(c, c, c, 1);
	qglClear(GL_COLOR_BUFFER_BIT);

	backEnd.isHyperspace = true;
}

//==========================================================================
//
//	RB_BeginDrawingView
//
//	Any mirrored or portaled views have already been drawn, so prepare
// to actually render the visible surfaces for this view
//
//==========================================================================

void RB_BeginDrawingView()
{
	// sync with gl if needed
	if (r_finish->integer == 1 && !glState.finishCalled)
	{
		qglFinish();
		glState.finishCalled = true;
	}
	if (r_finish->integer == 0)
	{
		glState.finishCalled = true;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = false;

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State(GLS_DEFAULT);
	// clear relevant buffers
	int clearBits = GL_DEPTH_BUFFER_BIT;

	if (r_measureOverdraw->integer || ((GGameType & GAME_Quake3) && r_shadows->integer == 2))
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	if (r_fastsky->integer && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
#ifdef _DEBUG
		qglClearColor(0.8f, 0.7f, 0.4f, 1.0f);	// FIXME: get color of sky
#else
		qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// FIXME: get color of sky
#endif
	}
	qglClear(clearBits);

	if (backEnd.refdef.rdflags & RDF_HYPERSPACE)
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = false;
	}

	glState.faceCulling = -1;		// force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = false;

	// clip to the plane of the portal
	if (backEnd.viewParms.isPortal)
	{
		float	plane[4];
		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		double	plane2[4];
		plane2[0] = DotProduct(backEnd.viewParms.orient.axis[0], plane);
		plane2[1] = DotProduct(backEnd.viewParms.orient.axis[1], plane);
		plane2[2] = DotProduct(backEnd.viewParms.orient.axis[2], plane);
		plane2[3] = DotProduct(plane, backEnd.viewParms.orient.origin) - plane[3];

		qglLoadMatrixf(s_flipMatrix);
		qglClipPlane(GL_CLIP_PLANE0, plane2);
		qglEnable(GL_CLIP_PLANE0);
	}
	else
	{
		qglDisable(GL_CLIP_PLANE0);
	}
}
