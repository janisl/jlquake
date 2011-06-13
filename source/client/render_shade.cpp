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

	int last[3] = { -1, -1, -1 };
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
				qassert(indexes[i + 2] < tess.numVertexes);
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
