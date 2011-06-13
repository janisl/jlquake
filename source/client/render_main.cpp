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

glconfig_t	glConfig;

trGlobals_t	tr;

float	s_flipMatrix[16] =
{
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	myGlMultMatrix
//
//==========================================================================

void myGlMultMatrix(const float* a, const float* b, float* out)
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			out[i * 4 + j] =
				a [i * 4 + 0] * b [0 * 4 + j] +
				a [i * 4 + 1] * b [1 * 4 + j] +
				a [i * 4 + 2] * b [2 * 4 + j] +
				a [i * 4 + 3] * b [3 * 4 + j];
		}
	}
}

//==========================================================================
//
//	R_DecomposeSort
//
//==========================================================================

void R_DecomposeSort(unsigned sort, int* entityNum, shader_t** shader, 
	int* fogNum, int* dlightMap)
{
	*fogNum = (sort >> QSORT_FOGNUM_SHIFT) & 31;
	*shader = tr.sortedShaders[(sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];
	*entityNum = (sort >> QSORT_ENTITYNUM_SHIFT) & 1023;
	*dlightMap = sort & 3;
}

//==========================================================================
//
//	SetFarClip
//
//==========================================================================

static void SetFarClip()
{
	if (!(GGameType & GAME_Quake3))
	{
		tr.viewParms.zFar = 4096;
		return;
	}

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		tr.viewParms.zFar = 2048;
		return;
	}

	//
	// set far clipping planes dynamically
	//
	float farthestCornerDistance = 0;
	for (int i = 0; i < 8; i++)
	{
		vec3_t v;
		if ( i & 1 )
		{
			v[0] = tr.viewParms.visBounds[0][0];
		}
		else
		{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if ( i & 2 )
		{
			v[1] = tr.viewParms.visBounds[0][1];
		}
		else
		{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if ( i & 4 )
		{
			v[2] = tr.viewParms.visBounds[0][2];
		}
		else
		{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		vec3_t vecTo;
		VectorSubtract(v, tr.viewParms.orient.origin, vecTo);

		float distance = vecTo[0] * vecTo[0] + vecTo[1] * vecTo[1] + vecTo[2] * vecTo[2];

		if (distance > farthestCornerDistance)
		{
			farthestCornerDistance = distance;
		}
	}
	tr.viewParms.zFar = sqrt(farthestCornerDistance);
}

//==========================================================================
//
//	R_SetupProjection
//
//==========================================================================

void R_SetupProjection()
{
	// dynamically compute far clip plane distance
	SetFarClip();

	//
	// set up projection matrix
	//
	float zNear	= r_znear->value;
	float zFar	= tr.viewParms.zFar;

	float ymax = zNear * tan(tr.viewParms.fovY * M_PI / 360.0f);
	float ymin = -ymax;

	float xmax = zNear * tan(tr.viewParms.fovX * M_PI / 360.0f);
	float xmin = -xmax;

	float width = xmax - xmin;
	float height = ymax - ymin;
	float depth = zFar - zNear;

	tr.viewParms.projectionMatrix[0] = 2 * zNear / width;
	tr.viewParms.projectionMatrix[4] = 0;
	tr.viewParms.projectionMatrix[8] = (xmax + xmin) / width;	// normally 0
	tr.viewParms.projectionMatrix[12] = 0;

	tr.viewParms.projectionMatrix[1] = 0;
	tr.viewParms.projectionMatrix[5] = 2 * zNear / height;
	tr.viewParms.projectionMatrix[9] = (ymax + ymin) / height;	// normally 0
	tr.viewParms.projectionMatrix[13] = 0;

	tr.viewParms.projectionMatrix[2] = 0;
	tr.viewParms.projectionMatrix[6] = 0;
	tr.viewParms.projectionMatrix[10] = -(zFar + zNear) / depth;
	tr.viewParms.projectionMatrix[14] = -2 * zFar * zNear / depth;

	tr.viewParms.projectionMatrix[3] = 0;
	tr.viewParms.projectionMatrix[7] = 0;
	tr.viewParms.projectionMatrix[11] = -1;
	tr.viewParms.projectionMatrix[15] = 0;
}

//==========================================================================
//
//	R_SetupFrustum
//
//	Setup that culling frustum planes for the current view
//
//==========================================================================

void R_SetupFrustum()
{
	float ang = tr.viewParms.fovX / 180 * M_PI * 0.5f;
	float xs = sin(ang);
	float xc = cos(ang);

	VectorScale(tr.viewParms.orient.axis[0], xs, tr.viewParms.frustum[0].normal);
	VectorMA(tr.viewParms.frustum[0].normal, xc, tr.viewParms.orient.axis[1], tr.viewParms.frustum[0].normal);

	VectorScale(tr.viewParms.orient.axis[0], xs, tr.viewParms.frustum[1].normal);
	VectorMA(tr.viewParms.frustum[1].normal, -xc, tr.viewParms.orient.axis[1], tr.viewParms.frustum[1].normal);

	ang = tr.viewParms.fovY / 180 * M_PI * 0.5f;
	xs = sin(ang);
	xc = cos(ang);

	VectorScale(tr.viewParms.orient.axis[0], xs, tr.viewParms.frustum[2].normal);
	VectorMA(tr.viewParms.frustum[2].normal, xc, tr.viewParms.orient.axis[2], tr.viewParms.frustum[2].normal);

	VectorScale(tr.viewParms.orient.axis[0], xs, tr.viewParms.frustum[3].normal);
	VectorMA(tr.viewParms.frustum[3].normal, -xc, tr.viewParms.orient.axis[2], tr.viewParms.frustum[3].normal);

	for (int i = 0; i < 4; i++)
	{
		tr.viewParms.frustum[i].type = PLANE_NON_AXIAL;
		tr.viewParms.frustum[i].dist = DotProduct(tr.viewParms.orient.origin, tr.viewParms.frustum[i].normal);
		SetPlaneSignbits(&tr.viewParms.frustum[i]);
	}
}

//==========================================================================
//
//	R_RotateForViewer
//
//	Sets up the modelview matrix for a given viewParm
//
//==========================================================================

void R_RotateForViewer()
{
	Com_Memset(&tr.orient, 0, sizeof(tr.orient));
	tr.orient.axis[0][0] = 1;
	tr.orient.axis[1][1] = 1;
	tr.orient.axis[2][2] = 1;
	VectorCopy(tr.viewParms.orient.origin, tr.orient.viewOrigin);

	// transform by the camera placement
	vec3_t origin;
	VectorCopy(tr.viewParms.orient.origin, origin);

	float viewerMatrix[16];

	viewerMatrix[0] = tr.viewParms.orient.axis[0][0];
	viewerMatrix[4] = tr.viewParms.orient.axis[0][1];
	viewerMatrix[8] = tr.viewParms.orient.axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = tr.viewParms.orient.axis[1][0];
	viewerMatrix[5] = tr.viewParms.orient.axis[1][1];
	viewerMatrix[9] = tr.viewParms.orient.axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = tr.viewParms.orient.axis[2][0];
	viewerMatrix[6] = tr.viewParms.orient.axis[2][1];
	viewerMatrix[10] = tr.viewParms.orient.axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix(viewerMatrix, s_flipMatrix, tr.orient.modelMatrix);

	tr.viewParms.world = tr.orient;
}

//==========================================================================
//
//	R_LocalNormalToWorld
//
//==========================================================================

void R_LocalNormalToWorld(vec3_t local, vec3_t world)
{
	world[0] = local[0] * tr.orient.axis[0][0] + local[1] * tr.orient.axis[1][0] + local[2] * tr.orient.axis[2][0];
	world[1] = local[0] * tr.orient.axis[0][1] + local[1] * tr.orient.axis[1][1] + local[2] * tr.orient.axis[2][1];
	world[2] = local[0] * tr.orient.axis[0][2] + local[1] * tr.orient.axis[1][2] + local[2] * tr.orient.axis[2][2];
}

//==========================================================================
//
//	R_LocalPointToWorld
//
//==========================================================================

void R_LocalPointToWorld(vec3_t local, vec3_t world)
{
	world[0] = local[0] * tr.orient.axis[0][0] + local[1] * tr.orient.axis[1][0] + local[2] * tr.orient.axis[2][0] + tr.orient.origin[0];
	world[1] = local[0] * tr.orient.axis[0][1] + local[1] * tr.orient.axis[1][1] + local[2] * tr.orient.axis[2][1] + tr.orient.origin[1];
	world[2] = local[0] * tr.orient.axis[0][2] + local[1] * tr.orient.axis[1][2] + local[2] * tr.orient.axis[2][2] + tr.orient.origin[2];
}

//==========================================================================
//
//	R_TransformModelToClip
//
//==========================================================================

void R_TransformModelToClip(const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
	vec4_t eye, vec4_t dst)
{
	for (int i = 0; i < 4; i++)
	{
		eye[i] =
			src[0] * modelMatrix[i + 0 * 4] +
			src[1] * modelMatrix[i + 1 * 4] +
			src[2] * modelMatrix[i + 2 * 4] +
			1 * modelMatrix[i + 3 * 4];
	}

	for (int i = 0; i < 4; i++)
	{
		dst[i] = 
			eye[0] * projectionMatrix[i + 0 * 4] +
			eye[1] * projectionMatrix[i + 1 * 4] +
			eye[2] * projectionMatrix[i + 2 * 4] +
			eye[3] * projectionMatrix[i + 3 * 4];
	}
}

//==========================================================================
//
//	R_TransformClipToWindow
//
//==========================================================================

void R_TransformClipToWindow(const vec4_t clip, const viewParms_t* view, vec4_t normalized, vec4_t window)
{
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = (clip[2] + clip[3]) / (2 * clip[3]);

	window[0] = 0.5f * (1.0f + normalized[0]) * view->viewportWidth;
	window[1] = 0.5f * (1.0f + normalized[1]) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int)(window[0] + 0.5);
	window[1] = (int)(window[1] + 0.5);
}
