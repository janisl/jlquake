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

int			c_brush_polys;
int			c_alias_polys;
int			c_visible_textures;
int			c_visible_lightmaps;

sortedent_t     cl_transvisedicts[MAX_ENTITIES];
sortedent_t		cl_transwateredicts[MAX_ENTITIES];

int				cl_numtransvisedicts;
int				cl_numtranswateredicts;

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
surfaceType_t	entitySurface = SF_ENTITY;

float	s_flipMatrix[16] =
{
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

// speed up sin calculations - Ed
float	r_turbsin[] =
{
	0, 0.19633, 0.392541, 0.588517, 0.784137, 0.979285, 1.17384, 1.3677,
	1.56072, 1.75281, 1.94384, 2.1337, 2.32228, 2.50945, 2.69512, 2.87916,
	3.06147, 3.24193, 3.42044, 3.59689, 3.77117, 3.94319, 4.11282, 4.27998,
	4.44456, 4.60647, 4.76559, 4.92185, 5.07515, 5.22538, 5.37247, 5.51632,
	5.65685, 5.79398, 5.92761, 6.05767, 6.18408, 6.30677, 6.42566, 6.54068,
	6.65176, 6.75883, 6.86183, 6.9607, 7.05537, 7.14579, 7.23191, 7.31368,
	7.39104, 7.46394, 7.53235, 7.59623, 7.65552, 7.71021, 7.76025, 7.80562,
	7.84628, 7.88222, 7.91341, 7.93984, 7.96148, 7.97832, 7.99036, 7.99759,
	8, 7.99759, 7.99036, 7.97832, 7.96148, 7.93984, 7.91341, 7.88222,
	7.84628, 7.80562, 7.76025, 7.71021, 7.65552, 7.59623, 7.53235, 7.46394,
	7.39104, 7.31368, 7.23191, 7.14579, 7.05537, 6.9607, 6.86183, 6.75883,
	6.65176, 6.54068, 6.42566, 6.30677, 6.18408, 6.05767, 5.92761, 5.79398,
	5.65685, 5.51632, 5.37247, 5.22538, 5.07515, 4.92185, 4.76559, 4.60647,
	4.44456, 4.27998, 4.11282, 3.94319, 3.77117, 3.59689, 3.42044, 3.24193,
	3.06147, 2.87916, 2.69512, 2.50945, 2.32228, 2.1337, 1.94384, 1.75281,
	1.56072, 1.3677, 1.17384, 0.979285, 0.784137, 0.588517, 0.392541, 0.19633,
	9.79717e-16, -0.19633, -0.392541, -0.588517, -0.784137, -0.979285, -1.17384, -1.3677,
	-1.56072, -1.75281, -1.94384, -2.1337, -2.32228, -2.50945, -2.69512, -2.87916,
	-3.06147, -3.24193, -3.42044, -3.59689, -3.77117, -3.94319, -4.11282, -4.27998,
	-4.44456, -4.60647, -4.76559, -4.92185, -5.07515, -5.22538, -5.37247, -5.51632,
	-5.65685, -5.79398, -5.92761, -6.05767, -6.18408, -6.30677, -6.42566, -6.54068,
	-6.65176, -6.75883, -6.86183, -6.9607, -7.05537, -7.14579, -7.23191, -7.31368,
	-7.39104, -7.46394, -7.53235, -7.59623, -7.65552, -7.71021, -7.76025, -7.80562,
	-7.84628, -7.88222, -7.91341, -7.93984, -7.96148, -7.97832, -7.99036, -7.99759,
	-8, -7.99759, -7.99036, -7.97832, -7.96148, -7.93984, -7.91341, -7.88222,
	-7.84628, -7.80562, -7.76025, -7.71021, -7.65552, -7.59623, -7.53235, -7.46394,
	-7.39104, -7.31368, -7.23191, -7.14579, -7.05537, -6.9607, -6.86183, -6.75883,
	-6.65176, -6.54068, -6.42566, -6.30677, -6.18408, -6.05767, -5.92761, -5.79398,
	-5.65685, -5.51632, -5.37247, -5.22538, -5.07515, -4.92185, -4.76559, -4.60647,
	-4.44456, -4.27998, -4.11282, -3.94319, -3.77117, -3.59689, -3.42044, -3.24193,
	-3.06147, -2.87916, -2.69512, -2.50945, -2.32228, -2.1337, -1.94384, -1.75281,
	-1.56072, -1.3677, -1.17384, -0.979285, -0.784137, -0.588517, -0.392541, -0.19633,
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
//	myftol
//
//==========================================================================

#if id386 && !((defined __linux__ || defined __FreeBSD__) && (defined __i386__)) // rb010123

long myftol(float f)
{
	static int tmp;
	__asm fld f
	__asm fistp tmp
	__asm mov eax, tmp
}

#endif

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

//==========================================================================
//
//	R_RotateForEntity
//
//	Generates an orientation for an entity and viewParms
//	Does NOT produce any GL calls
//	Called by both the front end and the back end
//
//==========================================================================

void R_RotateForEntity(const trRefEntity_t* ent, const viewParms_t* viewParms,
	orientationr_t* orient)
{
	if (ent->e.reType != RT_MODEL)
	{
		*orient = viewParms->world;
		return;
	}

	VectorCopy(ent->e.origin, orient->origin);

	if ((GGameType & GAME_Hexen2) && (R_GetModelByHandle(ent->e.hModel)->q1_flags & H2EF_FACE_VIEW))
	{
		float fvaxis[3][3];

		VectorSubtract(viewParms->orient.origin, ent->e.origin, fvaxis[0]);
		VectorNormalize(fvaxis[0]);

		if (fvaxis[0][1] == 0 && fvaxis[0][0] == 0)
		{
			fvaxis[1][0] = 0;
			fvaxis[1][1] = 1;
			fvaxis[1][2] = 0;
			fvaxis[2][1] = 0;
			fvaxis[2][2] = 0;
			if (fvaxis[0][2] > 0)
			{
				fvaxis[2][0] = -1;
			}
			else
			{
				fvaxis[2][0] = 1;
			}
		}
		else
		{
			fvaxis[2][0] = 0;
			fvaxis[2][1] = 0;
			fvaxis[2][2] = 1;
			CrossProduct(fvaxis[2], fvaxis[0], fvaxis[1]);
			VectorNormalize(fvaxis[1]);
			CrossProduct(fvaxis[0], fvaxis[1], fvaxis[2]);
			VectorNormalize(fvaxis[2]);
		}

		MatrixMultiply(ent->e.axis, fvaxis, orient->axis);
	}
	else
	{
		VectorCopy(ent->e.axis[0], orient->axis[0]);
		VectorCopy(ent->e.axis[1], orient->axis[1]);
		VectorCopy(ent->e.axis[2], orient->axis[2]);
	}

	float glMatrix[16];

	glMatrix[0] = orient->axis[0][0];
	glMatrix[4] = orient->axis[1][0];
	glMatrix[8] = orient->axis[2][0];
	glMatrix[12] = orient->origin[0];

	glMatrix[1] = orient->axis[0][1];
	glMatrix[5] = orient->axis[1][1];
	glMatrix[9] = orient->axis[2][1];
	glMatrix[13] = orient->origin[1];

	glMatrix[2] = orient->axis[0][2];
	glMatrix[6] = orient->axis[1][2];
	glMatrix[10] = orient->axis[2][2];
	glMatrix[14] = orient->origin[2];

	glMatrix[3] = 0;
	glMatrix[7] = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	myGlMultMatrix(glMatrix, viewParms->world.modelMatrix, orient->modelMatrix);

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	vec3_t delta;
	VectorSubtract(viewParms->orient.origin, orient->origin, delta);

	// compensate for scale in the axes if necessary
	if (ent->e.nonNormalizedAxes)
	{
		//	In Hexen 2 axis can have different scales.
		for (int i = 0; i < 3; i++)
		{
			float axisLength = VectorLength(ent->e.axis[i]);
			if (!axisLength)
			{
				axisLength = 0;
			}
			else
			{
				axisLength = 1.0f / axisLength;
			}
			orient->viewOrigin[i] = DotProduct(delta, orient->axis[i]) * axisLength;
		}
	}
	else
	{
		orient->viewOrigin[0] = DotProduct(delta, orient->axis[0]);
		orient->viewOrigin[1] = DotProduct(delta, orient->axis[1]);
		orient->viewOrigin[2] = DotProduct(delta, orient->axis[2]);
	}
}

//==========================================================================
//
//	R_CullLocalBox
//
//	Returns CULL_IN, CULL_CLIP, or CULL_OUT
//
//==========================================================================

int R_CullLocalBox(vec3_t bounds[2])
{
	if (r_nocull->integer)
	{
		return CULL_CLIP;
	}

	// transform into world space
	vec3_t transformed[8];
	for (int i = 0; i < 8; i++)
	{
		vec3_t v;
		v[0] = bounds[i & 1][0];
		v[1] = bounds[(i >> 1) & 1][1];
		v[2] = bounds[(i >> 2) & 1][2];

		VectorCopy(tr.orient.origin, transformed[i]);
		VectorMA(transformed[i], v[0], tr.orient.axis[0], transformed[i]);
		VectorMA(transformed[i], v[1], tr.orient.axis[1], transformed[i]);
		VectorMA(transformed[i], v[2], tr.orient.axis[2], transformed[i]);
	}

	// check against frustum planes
	bool anyBack = false;
	for (int i = 0; i < 4; i++)
	{
		cplane_t* frust = &tr.viewParms.frustum[i];

		bool front = false;
		bool back = false;
		for (int j = 0; j < 8; j++)
		{
			float dist = DotProduct(transformed[j], frust->normal);
			if (dist > frust->dist)
			{
				front = true;
				if (back)
				{
					break;		// a point is in front
				}
			}
			else
			{
				back = true;
			}
		}
		if (!front)
		{
			// all points were behind one of the planes
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if (!anyBack)
	{
		return CULL_IN;		// completely inside frustum
	}

	return CULL_CLIP;		// partially clipped
}

//==========================================================================
//
//	R_CullPointAndRadius
//
//==========================================================================

int R_CullPointAndRadius(vec3_t pt, float radius)
{
	if (r_nocull->integer)
	{
		return CULL_CLIP;
	}

	// check against frustum planes
	bool mightBeClipped = false;
	for (int i = 0; i < 4; i++) 
	{
		cplane_t* frust = &tr.viewParms.frustum[i];

		float dist = DotProduct(pt, frust->normal) - frust->dist;
		if (dist < -radius)
		{
			return CULL_OUT;
		}
		else if (dist <= radius)
		{
			mightBeClipped = true;
		}
	}

	if (mightBeClipped)
	{
		return CULL_CLIP;
	}

	return CULL_IN;		// completely inside frustum
}

//==========================================================================
//
//	R_CullLocalPointAndRadius
//
//==========================================================================

int R_CullLocalPointAndRadius(vec3_t pt, float radius)
{
	vec3_t transformed;

	R_LocalPointToWorld(pt, transformed);

	return R_CullPointAndRadius(transformed, radius);
}

//==========================================================================
//
//	R_AddDrawSurf
//
//==========================================================================

void R_AddDrawSurf(surfaceType_t* surface, shader_t* shader, int fogIndex, int dlightMap)
{
	// instead of checking for overflow, we just mask the index
	// so it wraps around
	int index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT) |
		tr.shiftedEntityNum | (fogIndex << QSORT_FOGNUM_SHIFT) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}

//==========================================================================
//
//	R_SpriteFogNum
//
//	See if a sprite is inside a fog volume
//
//==========================================================================

static int R_SpriteFogNum(trRefEntity_t* ent)
{
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return 0;
	}

	for (int i = 1; i < tr.world->numfogs; i++)
	{
		mbrush46_fog_t* fog = &tr.world->fogs[i];
		int j;
		for (j = 0; j < 3; j++)
		{
			if (ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j])
			{
				break;
			}
			if (ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j])
			{
				break;
			}
		}
		if (j == 3)
		{
			return i;
		}
	}

	return 0;
}

//==========================================================================
//
//	R_DrawBeam
//
//==========================================================================

static void R_DrawBeam(trRefEntity_t* e)
{
	enum { NUM_BEAM_SEGS = 6 };

	vec3_t oldorigin;
	oldorigin[0] = e->e.oldorigin[0];
	oldorigin[1] = e->e.oldorigin[1];
	oldorigin[2] = e->e.oldorigin[2];

	vec3_t origin;
	origin[0] = e->e.origin[0];
	origin[1] = e->e.origin[1];
	origin[2] = e->e.origin[2];

	vec3_t direction, normalized_direction;
	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize(normalized_direction) == 0)
	{
		return;
	}

	vec3_t perpvec;
	PerpendicularVector(perpvec, normalized_direction);
	VectorScale(perpvec, e->e.frame / 2, perpvec);

	vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	for (int i = 0; i < 6; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec, (360.0 / NUM_BEAM_SEGS) * i);
		VectorAdd(start_points[i], origin, start_points[i]);
		VectorAdd(start_points[i], direction, end_points[i]);
	}

	qglDisable(GL_TEXTURE_2D);
	GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	float r = (d_8to24table[e->e.skinNum & 0xFF]) & 0xFF;
	float g = (d_8to24table[e->e.skinNum & 0xFF] >> 8) & 0xFF;
	float b = (d_8to24table[e->e.skinNum & 0xFF] >> 16) & 0xFF;

	r *= 1 / 255.0F;
	g *= 1 / 255.0F;
	b *= 1 / 255.0F;

	qglColor4f(r, g, b, e->e.shaderRGBA[3] / 255.0);

	qglBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < NUM_BEAM_SEGS; i++)
	{
		qglVertex3fv(start_points[i]);
		qglVertex3fv(end_points[i]);
		qglVertex3fv(start_points[(i + 1) % NUM_BEAM_SEGS]);
		qglVertex3fv(end_points[(i + 1) % NUM_BEAM_SEGS]);
	}
	qglEnd();

	qglEnable(GL_TEXTURE_2D);
	GL_State(GLS_DEPTHMASK_TRUE);
}

//==========================================================================
//
//	R_DrawNullModel
//
//==========================================================================

static void R_DrawNullModel()
{
	vec3_t shadelight;
	if (tr.currentEntity->e.renderfx & RF_ABSOLUTE_LIGHT)
	{
		shadelight[0] = shadelight[1] = shadelight[2] = tr.currentEntity->e.radius;
	}
	else
	{
		R_LightPointQ2(tr.currentEntity->e.origin, shadelight);
	}

    qglPushMatrix();
	qglLoadMatrixf(tr.orient.modelMatrix);

	qglDisable(GL_TEXTURE_2D);
	qglColor3fv(shadelight);

	qglBegin(GL_TRIANGLE_FAN);
	qglVertex3f(0, 0, -16);
	for (int i = 0; i <= 4; i++)
	{
		qglVertex3f(16 * cos(i * M_PI / 2), 16 * sin(i * M_PI / 2), 0);
	}
	qglEnd();

	qglBegin(GL_TRIANGLE_FAN);
	qglVertex3f(0, 0, 16);
	for (int i = 4; i >= 0; i--)
	{
		qglVertex3f(16 * cos(i * M_PI / 2), 16 * sin(i * M_PI / 2), 0);
	}
	qglEnd ();

	qglColor3f(1, 1, 1);
	qglPopMatrix();
	qglEnable(GL_TEXTURE_2D);
}

//==========================================================================
//
//	R_AddEntitySurfaces
//
//==========================================================================

void R_AddEntitySurfaces(bool TranslucentPass)
{
	cl_numtransvisedicts = 0;
	cl_numtranswateredicts = 0;

	if (!r_drawentities->integer)
	{
		return;
	}

	for (tr.currentEntityNum = 0; tr.currentEntityNum < tr.refdef.num_entities; tr.currentEntityNum++)
	{
		tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

		trRefEntity_t* ent = tr.currentEntity;
		ent->needDlights = false;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

		if ((GGameType & GAME_Quake2) && !(tr.currentEntity->e.renderfx & RF_TRANSLUCENT) != TranslucentPass)
		{
			continue;
		}

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in 
		// mirrors, because the true body position will already be drawn
		//
		if ((ent->e.renderfx & RF_FIRST_PERSON) && tr.viewParms.isPortal)
		{
			continue;
		}

		bool item_trans = false;

		// simple generated models, like sprites and beams, are not culled
		switch (ent->e.reType)
		{
		case RT_PORTALSURFACE:
			break;		// don't draw anything

		case RT_SPRITE:
		case RT_BEAM:
		case RT_LIGHTNING:
		case RT_RAIL_CORE:
		case RT_RAIL_RINGS:
			if (GGameType & GAME_Quake3)
			{
				// self blood sprites, talk balloons, etc should not be drawn in the primary
				// view.  We can't just do this check for all entities, because md3
				// entities may still want to cast shadows from them
				if ((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
				{
					continue;
				}
				shader_t* shader = R_GetShaderByHandle(ent->e.customShader);
				R_AddDrawSurf(&entitySurface, shader, R_SpriteFogNum(ent), 0);
			}
			else
			{
				R_DrawBeam(tr.currentEntity);
			}
			break;

		case RT_MODEL:
			// we must set up parts of tr.or for model culling
			R_RotateForEntity(ent, &tr.viewParms, &tr.orient);

			tr.currentModel = R_GetModelByHandle(ent->e.hModel);
			if (!tr.currentModel)
			{
				if (GGameType & GAME_Quake3)
				{
					R_AddDrawSurf(&entitySurface, tr.defaultShader, 0, 0);
				}
				else
				{
					R_DrawNullModel();
				}
			}
			else
			{
				switch (tr.currentModel->type)
				{
				case MOD_BAD:
					if (GGameType & GAME_Quake3)
					{
						if ((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
						{
							break;
						}
						R_AddDrawSurf(&entitySurface, tr.defaultShader, 0, 0);
					}
					else
					{
						R_DrawNullModel();
					}

				case MOD_MESH1:
					if (GGameType & GAME_Hexen2)
					{
						item_trans = (ent->e.renderfx & RF_WATERTRANS) ||
							R_MdlHasHexen2Transparency(tr.currentModel);
					}
					if (!item_trans)
					{
						R_DrawMdlModel(ent);
					}
					break;

				case MOD_BRUSH29:
					if (GGameType & GAME_Hexen2)
					{
						item_trans = ((ent->e.renderfx & RF_WATERTRANS)) != 0;
					}
					if (!item_trans)
					{
						R_DrawBrushModelQ1(ent, false);
					}
					break;

				case MOD_SPRITE:
					if (GGameType & GAME_Hexen2)
					{
						item_trans = true;
					}
					break;

				case MOD_MESH2:
					R_DrawMd2Model(ent);
					break;

				case MOD_BRUSH38:
					R_DrawBrushModelQ2(ent);
					break;

				case MOD_SPRITE2:
					R_DrawSp2Model(ent);
					break;

				case MOD_MESH3:
					R_AddMD3Surfaces(ent);
					break;

				case MOD_MD4:
					R_AddAnimSurfaces(ent);
					break;

				case MOD_BRUSH46:
					R_AddBrushModelSurfaces(ent);
					break;

				default:
					throw QDropException("R_AddEntitySurfaces: Bad modeltype");
				}
			}
			break;

		default:
			throw QDropException("R_AddEntitySurfaces: Bad reType");
		}

		if ((GGameType & GAME_Hexen2) && item_trans)
		{
			mbrush29_leaf_t* pLeaf = Mod_PointInLeafQ1 (tr.currentEntity->e.origin, tr.worldModel);
			if (pLeaf->contents != BSP29CONTENTS_WATER)
			{
				cl_transvisedicts[cl_numtransvisedicts++].ent = tr.currentEntity;
			}
			else
			{
				cl_transwateredicts[cl_numtranswateredicts++].ent = tr.currentEntity;
			}
		}
	}

	if (GGameType & GAME_Quake)
	{
		for (tr.currentEntityNum = 0; tr.currentEntityNum < tr.refdef.num_entities; tr.currentEntityNum++)
		{
			tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];
			tr.currentModel = R_GetModelByHandle(tr.currentEntity->e.hModel);
			if (tr.currentModel->type == MOD_SPRITE)
			{
				R_DrawSprModel(tr.currentEntity);
			}
		}
	}
}
