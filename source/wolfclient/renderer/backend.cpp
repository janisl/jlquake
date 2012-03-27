//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"

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

volatile bool	renderThreadActive;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

#if 0
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

static const void* RB_SetColor(const void* data)
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
//	RB_DrawBuffer
//
//==========================================================================

static const void* RB_DrawBuffer(const void* data)
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

static void RB_RenderDrawSurfList(drawSurf_t* drawSurfs, int numDrawSurfs)
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
//	RB_DrawSurfs
//
//==========================================================================

static const void* RB_DrawSurfs(const void* data)
{
	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		RB_EndSurface();
	}

	const drawSurfsCommand_t* cmd = (const drawSurfsCommand_t*)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList(cmd->drawSurfs, cmd->numDrawSurfs);

	return (const void*)(cmd + 1);
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

//==========================================================================
//
//	RB_StretchPic
//
//==========================================================================

static const void* RB_StretchPic(const void* data)
{
	const stretchPicCommand_t* cmd = (const stretchPicCommand_t*)data;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader_t* shader = cmd->shader;
	if (shader != tess.shader)
	{
		if (tess.numIndexes)
		{
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface(shader, 0);
	}

	RB_CHECKOVERFLOW(4, 6);
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[numIndexes ] = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;

	*(int*)tess.vertexColors[numVerts] =
		*(int*)tess.vertexColors[numVerts + 1] =
		*(int*)tess.vertexColors[numVerts + 2] =
		*(int*)tess.vertexColors[numVerts + 3] = *(int*)backEnd.color2D;

	tess.xyz[numVerts][0] = cmd->x;
	tess.xyz[numVerts][1] = cmd->y;
	tess.xyz[numVerts][2] = 0;

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 1][1] = cmd->y;
	tess.xyz[numVerts + 1][2] = 0;

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 2][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 2][2] = 0;

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = cmd->x;
	tess.xyz[numVerts + 3][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 3][2] = 0;

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;

	return (const void*)(cmd + 1);
}

//==========================================================================
//
//	RB_ShowImages
//
//	Draw all the images to the screen, on top of whatever was there.  This
// is used to test for texture thrashing.
//
//	Also called by RE_EndRegistration
//
//==========================================================================

void RB_ShowImages()
{
	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	qglClear(GL_COLOR_BUFFER_BIT);

	qglFinish();

	int start = CL_ScaledMilliseconds();

	for (int i = 0; i < tr.numImages; i++)
	{
		image_t* image = tr.images[i];

		float w = glConfig.vidWidth / 20;
		float h = glConfig.vidHeight / 15;
		float x = i % 20 * w;
		float y = i / 20 * h;

		// show in proportional size in mode 2
		if (r_showImages->integer == 2)
		{
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		GL_Bind(image);
		qglBegin(GL_QUADS);
		qglTexCoord2f(0, 0);
		qglVertex2f(x, y);
		qglTexCoord2f(1, 0);
		qglVertex2f(x + w, y);
		qglTexCoord2f(1, 1);
		qglVertex2f(x + w, y + h);
		qglTexCoord2f(0, 1);
		qglVertex2f(x, y + h);
		qglEnd();
	}

	qglFinish();

	int end = CL_ScaledMilliseconds();
	Log::write("%i msec to draw all images\n", end - start);
}

//==========================================================================
//
//	RB_SwapBuffers
//
//==========================================================================

static const void* RB_SwapBuffers(const void* data)
{
	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		RB_EndSurface();
	}

	// texture swapping test
	if (r_showImages->integer)
	{
		RB_ShowImages();
	}

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if (r_measureOverdraw->integer)
	{
		byte* stencilReadback = new byte[glConfig.vidWidth * glConfig.vidHeight];
		qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback);

		long sum = 0;
		for (int i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++)
		{
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		delete[] stencilReadback;
	}

	if (!glState.finishCalled)
	{
		qglFinish();
	}

	QGL_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	//
	// swapinterval stuff
	//
#ifdef _WIN32
	if (r_swapInterval->modified)
	{
		r_swapInterval->modified = false;

		if (!glConfig.stereoEnabled)	// why?
		{	
			if (qwglSwapIntervalEXT)
			{
				qwglSwapIntervalEXT(r_swapInterval->integer);
			}
		}
	}
#endif

	// don't flip if drawing to front buffer
	if (String::ICmp(r_drawBuffer->string, "GL_FRONT") != 0)
	{
		GLimp_SwapBuffers();
	}

	// check logging
	QGL_EnableLogging(!!r_logFile->integer);

	backEnd.projection2D = false;

	const swapBuffersCommand_t* cmd = (const swapBuffersCommand_t*)data;
	return (const void*)(cmd + 1);
}

//==========================================================================
//
//	RB_ExecuteRenderCommands
//
//	This function will be called synchronously if running without smp
// extensions, or asynchronously by another thread.
//
//==========================================================================

void RB_ExecuteRenderCommands(const void* data)
{
	int t1 = CL_ScaledMilliseconds();

	if (!r_smp->integer || data == backEndData[0]->commands.cmds)
	{
		backEnd.smpFrame = 0;
	}
	else
	{
		backEnd.smpFrame = 1;
	}

	while (1)
	{
		switch (*(const int*)data)
		{
		case RC_SET_COLOR:
			data = RB_SetColor(data);
			break;

		case RC_STRETCH_PIC:
			data = RB_StretchPic(data);
			break;

		case RC_DRAW_SURFS:
			data = RB_DrawSurfs(data);
			break;

		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer(data);
			break;

		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers(data);
			break;

		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd(data);
			break;

		case RC_END_OF_LIST:
		default:
			// stop rendering on this thread
			int t2 = CL_ScaledMilliseconds();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
}

//==========================================================================
//
//	RB_RenderThread
//
//==========================================================================

void RB_RenderThread()
{
	// wait for either a rendering command or a quit command
	while (1)
	{
		// sleep until we have work to do
		const void* data = GLimp_RendererSleep();

		if (!data)
		{
			return;	// all done, renderer is shutting down
		}

		renderThreadActive = true;

		RB_ExecuteRenderCommands(data);

		renderThreadActive = false;
	}
}

//==========================================================================
//
//	R_StretchRaw
//
//	FIXME: not exactly backend
//	Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
//	Used for cinematics.
//
//==========================================================================

void R_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, bool dirty)
{
	if (!tr.registered)
	{
		return;
	}
	R_SyncRenderThread();

	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		RB_EndSurface();
	}

	// we definately want to sync every frame for the cinematics
	qglFinish();

	int start = 0;
	if (r_speeds->integer)
	{
		start = CL_ScaledMilliseconds();
	}

	R_UploadCinematic(cols, rows, data, client, dirty);

	if (r_speeds->integer)
	{
		int end = CL_ScaledMilliseconds();
		Log::write("qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start);
	}

	RB_SetGL2D();

	qglColor3f(tr.identityLight, tr.identityLight, tr.identityLight);

	qglBegin(GL_QUADS);
	qglTexCoord2f(0, 0);
	qglVertex2f(x, y);
	qglTexCoord2f(1, 0);
	qglVertex2f(x + w, y);
	qglTexCoord2f(1, 1);
	qglVertex2f(x + w, y + h);
	qglTexCoord2f(0, 1);
	qglVertex2f(x, y + h);
	qglEnd();
}
#endif
