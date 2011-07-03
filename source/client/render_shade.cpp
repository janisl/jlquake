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

shaderCommands_t	tess;

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
		throw QDropException("RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}	
	if (input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0)
	{
		throw QDropException("RB_EndSurface() - SHADER_MAX_VERTEXES hit");
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
