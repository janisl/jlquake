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

/*
  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

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

shaderCommands_t	tess;

bool	setArraysOnce;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_ArrayElementDiscrete
//
//	This is just for OpenGL conformance testing, it should never be the fastest
//
//==========================================================================

static void APIENTRY R_ArrayElementDiscrete(GLint index)
{
	qglColor4ubv(tess.svars.colors[index]);
	if (glState.currenttmu)
	{
		qglMultiTexCoord2fARB(0, tess.svars.texcoords[0][index][0], tess.svars.texcoords[0][index][1]);
		qglMultiTexCoord2fARB(1, tess.svars.texcoords[1][index][0], tess.svars.texcoords[1][index][1]);
	}
	else
	{
		qglTexCoord2fv(tess.svars.texcoords[0][index]);
	}
	qglVertex3fv(tess.xyz[index]);
}

//==========================================================================
//
//	R_DrawStripElements
//
//==========================================================================

static void R_DrawStripElements(int numIndexes, const glIndex_t *indexes, void (APIENTRY*element)(GLint))
{
	if (numIndexes <= 0)
	{
		return;
	}

	qglBegin(GL_TRIANGLE_STRIP);

	// prime the strip
	element(indexes[0]);
	element(indexes[1]);
	element(indexes[2]);

	glIndex_t last[3] = { -1, -1, -1 };
	last[0] = indexes[0];
	last[1] = indexes[1];
	last[2] = indexes[2];

	bool even = false;

	for (int i = 3; i < numIndexes; i += 3)
	{
		// odd numbered triangle in potential strip
		if (!even)
		{
			// check previous triangle to see if we're continuing a strip
			if ((indexes[i + 0] == last[2]) && (indexes[i + 1] == last[1]))
			{
				element(indexes[i + 2]);
				qassert(indexes[i + 2] < (glIndex_t)tess.numVertexes);
				even = true;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				qglEnd();

				qglBegin(GL_TRIANGLE_STRIP);

				element(indexes[i + 0]);
				element(indexes[i + 1]);
				element(indexes[i + 2]);

				even = false;
			}
		}
		else
		{
			// check previous triangle to see if we're continuing a strip
			if ((last[2] == indexes[i + 1]) && (last[0] == indexes[i + 0]))
			{
				element(indexes[i + 2]);

				even = false;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				qglEnd();

				qglBegin(GL_TRIANGLE_STRIP);

				element(indexes[i + 0]);
				element(indexes[i + 1]);
				element(indexes[i + 2]);

				even = false;
			}
		}

		// cache the last three vertices
		last[0] = indexes[i + 0];
		last[1] = indexes[i + 1];
		last[2] = indexes[i + 2];
	}

	qglEnd();
}

//==========================================================================
//
//	R_DrawElements
//
//	Optionally performs our own glDrawElements that looks for strip conditions
// instead of using the single glDrawElements call that may be inefficient
// without compiled vertex arrays.
//
//==========================================================================

//static 
void R_DrawElements(int numIndexes, const glIndex_t* indexes)
{
	int primitives = r_primitives->integer;

	// default is to use triangles if compiled vertex arrays are present
	if (primitives == 0)
	{
		if (qglLockArraysEXT)
		{
			primitives = 2;
		}
		else
		{
			primitives = 1;
		}
	}

	if (primitives == 2)
	{
		qglDrawElements(GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, indexes);
		return;
	}

	if (primitives == 1)
	{
		R_DrawStripElements(numIndexes,  indexes, qglArrayElement);
		return;
	}
	
	if (primitives == 3)
	{
		R_DrawStripElements(numIndexes,  indexes, R_ArrayElementDiscrete);
		return;
	}

	// anything else will cause no drawing
}

/*
=============================================================

SURFACE SHADERS

=============================================================
*/

#if 0
//==========================================================================
//
//	R_BindAnimatedImage
//
//==========================================================================

static void R_BindAnimatedImage(textureBundle_t* bundle)
{
	if (bundle->isVideoMap)
	{
		CIN_RunCinematic(bundle->videoMapHandle);
		CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if (bundle->numImageAnimations <= 1)
	{
		GL_Bind(bundle->image[0]);
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	int index = Q_ftol(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	index >>= FUNCTABLE_SIZE2;

	if (index < 0)
	{
		index = 0;	// may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	GL_Bind(bundle->image[index]);
}

//==========================================================================
//
//	DrawTris
//
//	Draws triangle outlines for debugging
//
//==========================================================================

static void DrawTris(shaderCommands_t* input)
{
	GL_Bind(tr.whiteImage);
	qglColor3f(1, 1, 1);

	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);
	qglDepthRange(0, 0);

	qglDisableClientState(GL_COLOR_ARRAY);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	qglVertexPointer(3, GL_FLOAT, 16, input->xyz);	// padded for SIMD

	if (qglLockArraysEXT)
	{
		qglLockArraysEXT(0, input->numVertexes);
		QGL_LogComment("glLockArraysEXT\n");
	}

	R_DrawElements(input->numIndexes, input->indexes);

	if (qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		QGL_LogComment("glUnlockArraysEXT\n");
	}
	qglDepthRange(0, 1);
}

//==========================================================================
//
//	DrawNormals
//
//	Draws vertex normals for debugging
//
//==========================================================================

static void DrawNormals(shaderCommands_t* input)
{
	GL_Bind(tr.whiteImage);
	qglColor3f(1, 1, 1);
	qglDepthRange(0, 0);	// never occluded
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);

	qglBegin(GL_LINES);
	for (int i = 0 ; i < input->numVertexes ; i++)
	{
		qglVertex3fv(input->xyz[i]);
		vec3_t temp;
		VectorMA(input->xyz[i], 2, input->normal[i], temp);
		qglVertex3fv(temp);
	}
	qglEnd();

	qglDepthRange(0, 1);
}

//==========================================================================
//
//	RB_BeginSurface
//
//	We must set some things up before beginning any tesselation, because a
// surface may be forced to perform a RB_End due to overflow.
//
//==========================================================================

void RB_BeginSurface(shader_t* shader, int fogNum)
{
	shader_t* state = shader->remappedShader ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.dlightBits = 0;		// will be OR'd in by surface functions
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime)
	{
		tess.shaderTime = tess.shader->clampTime;
	}
}

//==========================================================================
//
//	DrawMultitextured
//
//	output = t0 * t1 or t0 + t1
//
//	t0 = most upstream according to spec
//	t1 = most downstream according to spec
//
//==========================================================================

static void DrawMultitextured(shaderCommands_t* input, int stage)
{
	shaderStage_t* pStage = tess.xstages[stage];

	GL_State(pStage->stateBits);

	// this is an ugly hack to work around a GeForce driver
	// bug with multitexture and clip planes
	if (backEnd.viewParms.isPortal)
	{
		qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	//
	// base
	//
	GL_SelectTexture(0);
	qglTexCoordPointer(2, GL_FLOAT, 0, input->svars.texcoords[0]);
	R_BindAnimatedImage(&pStage->bundle[0]);

	//
	// lightmap/secondary pass
	//
	GL_SelectTexture(1);
	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (r_lightmap->integer)
	{
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_TexEnv(tess.shader->multitextureEnv);
	}

	qglTexCoordPointer(2, GL_FLOAT, 0, input->svars.texcoords[1]);

	R_BindAnimatedImage(&pStage->bundle[1]);

	R_DrawElements(input->numIndexes, input->indexes);

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	//qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisable(GL_TEXTURE_2D);

	GL_SelectTexture(0);
}

//==========================================================================
//
//	RB_IterateStagesGeneric
//
//==========================================================================

static void RB_IterateStagesGeneric(shaderCommands_t* input)
{
	for (int stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t* pStage = tess.xstages[stage];

		if (!pStage)
		{
			break;
		}

		ComputeColors(pStage);
		ComputeTexCoords(pStage);

		if (!setArraysOnce)
		{
			qglEnableClientState(GL_COLOR_ARRAY);
			qglColorPointer(4, GL_UNSIGNED_BYTE, 0, input->svars.colors);
		}

		//
		// do multitexture
		//
		if (pStage->bundle[1].image[0] != 0)
		{
			DrawMultitextured(input, stage);
		}
		else
		{
			if (!setArraysOnce)
			{
				qglTexCoordPointer(2, GL_FLOAT, 0, input->svars.texcoords[0]);
			}

			//
			// set state
			//
			if (pStage->bundle[0].vertexLightmap && r_vertexLight->integer && !r_uiFullScreen->integer && r_lightmap->integer)
			{
				GL_Bind(tr.whiteImage);
			}
			else
			{
				R_BindAnimatedImage(&pStage->bundle[0]);
			}

			GL_State(pStage->stateBits);

			//
			// draw
			//
			R_DrawElements(input->numIndexes, input->indexes);
		}
		// allow skipping out to show just lightmaps during development
		if (r_lightmap->integer && (pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap))
		{
			break;
		}
	}
}

//==========================================================================
//
//	ProjectDlightTexture
//
//	Perform dynamic lighting with another rendering pass
//
//==========================================================================

static void ProjectDlightTexture()
{
	if (!backEnd.refdef.num_dlights)
	{
		return;
	}

	for (int l = 0; l < backEnd.refdef.num_dlights; l++)
	{
		if (!(tess.dlightBits & (1 << l)))
		{
			continue;	// this surface definately doesn't have any of this light
		}
		float texCoordsArray[SHADER_MAX_VERTEXES][2];
		float* texCoords = texCoordsArray[0];
		byte colorArray[SHADER_MAX_VERTEXES][4];
		byte* colors = colorArray[0];

		dlight_t* dl = &backEnd.refdef.dlights[l];
		vec3_t origin;
		VectorCopy(dl->transformed, origin);
		float radius = dl->radius;
		float scale = 1.0f / radius;

		vec3_t floatColor;
		floatColor[0] = dl->color[0] * 255.0f;
		floatColor[1] = dl->color[1] * 255.0f;
		floatColor[2] = dl->color[2] * 255.0f;
		byte clipBits[SHADER_MAX_VERTEXES];
		for (int i = 0; i < tess.numVertexes; i++, texCoords += 2, colors += 4)
		{
			backEnd.pc.c_dlightVertexes++;

			vec3_t dist;
			VectorSubtract(origin, tess.xyz[i], dist);
			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			int clip = 0;
			if (texCoords[0] < 0.0f)
			{
				clip |= 1;
			}
			else if (texCoords[0] > 1.0f)
			{
				clip |= 2;
			}
			if (texCoords[1] < 0.0f)
			{
				clip |= 4;
			}
			else if (texCoords[1] > 1.0f)
			{
				clip |= 8;
			}
			// modulate the strength based on the height and color
			float modulate;
			if (dist[2] > radius)
			{
				clip |= 16;
				modulate = 0.0f;
			}
			else if (dist[2] < -radius)
			{
				clip |= 32;
				modulate = 0.0f;
			}
			else
			{
				dist[2] = Q_fabs(dist[2]);
				if (dist[2] < radius * 0.5f)
				{
					modulate = 1.0f;
				}
				else
				{
					modulate = 2.0f * (radius - dist[2]) * scale;
				}
			}
			clipBits[i] = clip;

			colors[0] = Q_ftol(floatColor[0] * modulate);
			colors[1] = Q_ftol(floatColor[1] * modulate);
			colors[2] = Q_ftol(floatColor[2] * modulate);
			colors[3] = 255;
		}

		// build a list of triangles that need light
		int numIndexes = 0;
		unsigned hitIndexes[SHADER_MAX_INDEXES];
		for (int i = 0; i < tess.numIndexes; i += 3)
		{
			int a = tess.indexes[i];
			int b = tess.indexes[i + 1];
			int c = tess.indexes[i + 2];
			if (clipBits[a] & clipBits[b] & clipBits[c])
			{
				continue;	// not lighted
			}
			hitIndexes[numIndexes] = a;
			hitIndexes[numIndexes + 1] = b;
			hitIndexes[numIndexes + 2] = c;
			numIndexes += 3;
		}

		if (!numIndexes)
		{
			continue;
		}

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, 0, texCoordsArray[0]);

		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, 0, colorArray);

		GL_Bind(tr.dlightImage);
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		if (dl->additive)
		{
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
		}
		else
		{
			GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
		}
		R_DrawElements(numIndexes, hitIndexes);
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
	}
}

//==========================================================================
//
//	RB_FogPass
//
//	Blends a fog texture on top of everything else
//
//==========================================================================

static void RB_FogPass()
{
	qglEnableClientState(GL_COLOR_ARRAY);
	qglColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texcoords[0]);

	mbrush46_fog_t* fog = tr.world->fogs + tess.fogNum;

	for (int i = 0; i < tess.numVertexes; i++)
	{
		*(int*)&tess.svars.colors[i] = fog->colorInt;
	}

	RB_CalcFogTexCoords((float*)tess.svars.texcoords[0]);

	GL_Bind(tr.fogImage);

	if (tess.shader->fogPass == FP_EQUAL)
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);
	}
	else
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}

	R_DrawElements(tess.numIndexes, tess.indexes);
}

//==========================================================================
//
//	RB_StageIteratorGeneric
//
//==========================================================================

void RB_StageIteratorGeneric()
{
	shaderCommands_t* input = &tess;

	RB_DeformTessGeometry();

	//
	// log this call
	//
	if (r_logFile->integer) 
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment(va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name));
	}

	//
	// set face culling appropriately
	//
	GL_Cull(input->shader->cullType);

	// set polygon offset if necessary
	if (input->shader->polygonOffset)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	//
	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	//
	if (tess.numPasses > 1 || input->shader->multitextureEnv)
	{
		setArraysOnce = false;
		qglDisableClientState(GL_COLOR_ARRAY);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		setArraysOnce = true;

		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texcoords[0]);
	}

	//
	// lock XYZ
	//
	qglVertexPointer(3, GL_FLOAT, 16, input->xyz);	// padded for SIMD
	if (qglLockArraysEXT)
	{
		qglLockArraysEXT(0, input->numVertexes);
		QGL_LogComment("glLockArraysEXT\n");
	}

	//
	// enable color and texcoord arrays after the lock if necessary
	//
	if (!setArraysOnce)
	{
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglEnableClientState(GL_COLOR_ARRAY);
	}

	//
	// call shader function
	//
	RB_IterateStagesGeneric(input);

	// 
	// now do any dynamic lighting needed
	//
	if (tess.dlightBits && tess.shader->sort <= SS_OPAQUE &&
		!(tess.shader->surfaceFlags & (BSP46SURF_NODLIGHT | BSP46SURF_SKY)))
	{
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if (tess.fogNum && tess.shader->fogPass)
	{
		RB_FogPass();
	}

	// 
	// unlock arrays
	//
	if (qglUnlockArraysEXT) 
	{
		qglUnlockArraysEXT();
		QGL_LogComment("glUnlockArraysEXT\n");
	}

	//
	// reset polygon offset
	//
	if (input->shader->polygonOffset)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}

//==========================================================================
//
//	RB_StageIteratorVertexLitTexture
//
//==========================================================================

void RB_StageIteratorVertexLitTexture()
{
	shaderCommands_t* input = &tess;

	//
	// compute colors
	//
	RB_CalcDiffuseColor((byte*)tess.svars.colors);

	//
	// log this call
	//
	if (r_logFile->integer) 
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment(va("--- RB_StageIteratorVertexLitTexturedUnfogged( %s ) ---\n", tess.shader->name));
	}

	//
	// set face culling appropriately
	//
	GL_Cull(input->shader->cullType);

	//
	// set arrays and lock
	//
	qglEnableClientState(GL_COLOR_ARRAY);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

	qglColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);
	qglTexCoordPointer(2, GL_FLOAT, 16, tess.texCoords[0][0]);
	qglVertexPointer(3, GL_FLOAT, 16, input->xyz);

	if (qglLockArraysEXT)
	{
		qglLockArraysEXT(0, input->numVertexes);
		QGL_LogComment("glLockArraysEXT\n");
	}

	//
	// call special shade routine
	//
	R_BindAnimatedImage(&tess.xstages[0]->bundle[0]);
	GL_State(tess.xstages[0]->stateBits);
	R_DrawElements(input->numIndexes, input->indexes);

	// 
	// now do any dynamic lighting needed
	//
	if (tess.dlightBits && tess.shader->sort <= SS_OPAQUE)
	{
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if (tess.fogNum && tess.shader->fogPass)
	{
		RB_FogPass();
	}

	// 
	// unlock arrays
	//
	if (qglUnlockArraysEXT) 
	{
		qglUnlockArraysEXT();
		QGL_LogComment("glUnlockArraysEXT\n");
	}
}

//==========================================================================
//
//	RB_StageIteratorLightmappedMultitexture
//
//==========================================================================

void RB_StageIteratorLightmappedMultitexture()
{
	shaderCommands_t* input = &tess;

	//
	// log this call
	//
	if (r_logFile->integer)
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment(va("--- RB_StageIteratorLightmappedMultitexture( %s ) ---\n", tess.shader->name));
	}

	//
	// set face culling appropriately
	//
	GL_Cull(input->shader->cullType);

	//
	// set color, pointers, and lock
	//
	GL_State(GLS_DEFAULT);
	qglVertexPointer(3, GL_FLOAT, 16, input->xyz);

	qglEnableClientState(GL_COLOR_ARRAY);
	qglColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.constantColor255);

	//
	// select base stage
	//
	GL_SelectTexture(0);

	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	R_BindAnimatedImage(&tess.xstages[0]->bundle[0]);
	qglTexCoordPointer(2, GL_FLOAT, 16, tess.texCoords[0][0]);

	//
	// configure second stage
	//
	GL_SelectTexture(1);
	qglEnable(GL_TEXTURE_2D);
	if (r_lightmap->integer)
	{
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_TexEnv(GL_MODULATE);
	}
	R_BindAnimatedImage(&tess.xstages[0]->bundle[1]);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 16, tess.texCoords[0][1]);

	//
	// lock arrays
	//
	if (qglLockArraysEXT)
	{
		qglLockArraysEXT(0, input->numVertexes);
		QGL_LogComment("glLockArraysEXT\n");
	}

	R_DrawElements(input->numIndexes, input->indexes);

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	qglDisable(GL_TEXTURE_2D);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	GL_SelectTexture(0);

	// 
	// now do any dynamic lighting needed
	//
	if (tess.dlightBits && tess.shader->sort <= SS_OPAQUE)
	{
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if (tess.fogNum && tess.shader->fogPass)
	{
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if (qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		QGL_LogComment("glUnlockArraysEXT\n");
	}
}

//==========================================================================
//
//	RB_EndSurface
//
//==========================================================================

void RB_EndSurface()
{
	shaderCommands_t* input = &tess;

	if (input->numIndexes == 0)
	{
		return;
	}

	if (input->indexes[SHADER_MAX_INDEXES - 1] != 0)
	{
		throw DropException("RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}	
	if (input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0)
	{
		throw DropException("RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	if (tess.shader == tr.shadowShader)
	{
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if (r_debugSort->integer && r_debugSort->integer < tess.shader->sort)
	{
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if (r_showtris->integer)
	{
		DrawTris(input);
	}
	if (r_shownormals->integer)
	{
		DrawNormals(input);
	}
	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;

	QGL_LogComment("----------\n");
}
#endif
