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

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

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

//==========================================================================
//
//	RB_RenderDrawSurfList
//
//==========================================================================

void RB_RenderDrawSurfList(drawSurf_t* drawSurfs, int numDrawSurfs)
{
	// save original time for entity shader offsets
	float originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView();

	// draw everything
	int oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	shader_t* oldShader = NULL;
	int oldFogNum = -1;
	bool oldDepthRange = false;
	int oldDlighted = false;
	unsigned int oldSort = -1;
	bool depthRange = false;

	backEnd.pc.c_surfaces += numDrawSurfs;

	drawSurf_t* drawSurf = drawSurfs;
	for (int i = 0; i < numDrawSurfs; i++, drawSurf++)
	{
		if (drawSurf->sort == oldSort)
		{
			// fast path, same as previous sort
			rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
			continue;
		}
		oldSort = drawSurf->sort;
		int entityNum;
		shader_t* shader;
		int fogNum;
		int dlighted;
		R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted);

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if (shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted ||
			(entityNum != oldEntityNum && !shader->entityMergable))
		{
			if (oldShader != NULL)
			{
				RB_EndSurface();
			}
			RB_BeginSurface(shader, fogNum);
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
		}

		//
		// change the modelview matrix if needed
		//
		if (entityNum != oldEntityNum)
		{
			depthRange = false;

			if (entityNum != REF_ENTITYNUM_WORLD)
			{
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orient);

				// set up the dynamic lighting if needed
				if (backEnd.currentEntity->needDlights)
				{
					R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orient);
				}

				if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = true;
				}
			}
			else
			{
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.orient = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orient);
			}

			qglLoadMatrixf(backEnd.orient.modelMatrix);

			//
			// change depthrange if needed
			//
			if (oldDepthRange != depthRange)
			{
				if (depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL)
	{
		RB_EndSurface();
	}

	// go back to the world modelview matrix
	qglLoadMatrixf(backEnd.viewParms.world.modelMatrix);
	if (depthRange)
	{
		qglDepthRange(0, 1);
	}

#if 0
	RB_DrawSun();
#endif
	// darken down any stencil shadows
	RB_ShadowFinish();		

	// add light flares on lights that aren't obscured
	RB_RenderFlares();
}

//==========================================================================
//
//	RB_SetGL2D
//
//==========================================================================

void RB_SetGL2D()
{
	backEnd.projection2D = true;

	// set 2D virtual screen size
	qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	qglScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity();
	qglOrtho(0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();

	GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	qglDisable(GL_CULL_FACE);
	qglDisable(GL_CLIP_PLANE0);

	// set time for 2D shaders
	backEnd.refdef.time = CL_ScaledMilliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}
