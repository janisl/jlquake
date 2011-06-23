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

int			r_firstSceneDrawSurf;

int			r_numentities;
int			r_firstSceneEntity;

int			r_numdlights;
int			r_firstSceneDlight;

int			r_numpolys;
int			r_firstScenePoly;

int			r_numpolyverts;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_ToggleSmpFrame
//
//==========================================================================

void R_ToggleSmpFrame()
{
	if (r_smp->integer)
	{
		// use the other buffers next frame, because another CPU
		// may still be rendering into the current ones
		tr.smpFrame ^= 1;
	}
	else
	{
		tr.smpFrame = 0;
	}

	backEndData[tr.smpFrame]->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;
}

//==========================================================================
//
//	R_ClearScene
//
//==========================================================================

void R_ClearScene()
{
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;
}

//==========================================================================
//
//	R_AddRefEntityToScene
//
//==========================================================================

void R_AddRefEntityToScene(const refEntity_t* ent)
{
	if (!tr.registered)
	{
		return;
	}
	if (r_numentities >= MAX_ENTITIES)
	{
		return;
	}
	if (ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE)
	{
		throw QDropException(va("R_AddRefEntityToScene: bad reType %i", ent->reType));
	}

	backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
	backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = false;

	r_numentities++;
}

//==========================================================================
//
//	R_AddDynamicLightToScene
//
//==========================================================================

static void R_AddDynamicLightToScene(const vec3_t org, float intensity, float r, float g, float b, bool additive)
{
	if (!tr.registered)
	{
		return;
	}
	if (r_numdlights >= MAX_DLIGHTS)
	{
		return;
	}
	if (intensity <= 0)
	{
		return;
	}
	dlight_t* dl = &backEndData[tr.smpFrame]->dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->additive = additive;
}

//==========================================================================
//
//	R_AddLightToScene
//
//==========================================================================

void R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b)
{
	R_AddDynamicLightToScene(org, intensity, r, g, b, false);
}

//==========================================================================
//
//	R_AddAdditiveLightToScene
//
//==========================================================================

void R_AddAdditiveLightToScene(const vec3_t org, float intensity, float r, float g, float b)
{
	R_AddDynamicLightToScene(org, intensity, r, g, b, true);
}

//==========================================================================
//
//	R_AddPolyToScene
//
//==========================================================================

void R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys)
{
	if (!tr.registered)
	{
		return;
	}

	if (!hShader)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: R_AddPolyToScene: NULL poly shader\n");
		return;
	}

	for (int j = 0; j < numPolys; j++)
	{
		if (r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys)
		{
			/*
			NOTE TTimo this was initially a PRINT_WARNING
			but it happens a lot with high fighting scenes and particles
			since we don't plan on changing the const and making for room for those effects
			simply cut this message to developer only
			*/
			GLog.DWrite(S_COLOR_RED "WARNING: R_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
			return;
		}

		srfPoly_t* poly = &backEndData[tr.smpFrame]->polys[r_numpolys];
		poly->surfaceType = SF_POLY;
		poly->hShader = hShader;
		poly->numVerts = numVerts;
		poly->verts = &backEndData[tr.smpFrame]->polyVerts[r_numpolyverts];

		Com_Memcpy(poly->verts, &verts[numVerts * j], numVerts * sizeof(*verts));

		// done.
		r_numpolys++;
		r_numpolyverts += numVerts;

		// if no world is loaded
		int fogIndex;
		if (tr.world == NULL)
		{
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if (tr.world->numfogs == 1)
		{
			fogIndex = 0;
		}
		else
		{
			// find which fog volume the poly is in
			vec3_t bounds[2];
			VectorCopy(poly->verts[0].xyz, bounds[0]);
			VectorCopy(poly->verts[0].xyz, bounds[1]);
			for (int i = 1; i < poly->numVerts; i++)
			{
				AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
			}
			for (fogIndex = 1; fogIndex < tr.world->numfogs; fogIndex++)
			{
				mbrush46_fog_t* fog = &tr.world->fogs[fogIndex]; 
				if (bounds[1][0] >= fog->bounds[0][0] &&
					bounds[1][1] >= fog->bounds[0][1] &&
					bounds[1][2] >= fog->bounds[0][2] &&
					bounds[0][0] <= fog->bounds[1][0] &&
					bounds[0][1] <= fog->bounds[1][1] &&
					bounds[0][2] <= fog->bounds[1][2])
				{
					break;
				}
			}
			if (fogIndex == tr.world->numfogs)
			{
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
	}
}

//==========================================================================
//
//	R_AddLightStyleToScene
//
//==========================================================================

void R_AddLightStyleToScene(int style, float r, float g, float b)
{
	if (style < 0 || style > MAX_LIGHTSTYLES)
	{
		throw QDropException(va("Bad light style %i", style));
	}
	lightstyle_t* ls = &backEndData[tr.smpFrame]->lightstyles[style];
	ls->white = r + g + b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

//==========================================================================
//
//	R_CommonRenderScene
//
//==========================================================================

void R_CommonRenderScene(const refdef_t* fd)
{
	Com_Memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy(fd->vieworg, tr.refdef.vieworg);
	VectorCopy(fd->viewaxis[0], tr.refdef.viewaxis[0]);
	VectorCopy(fd->viewaxis[1], tr.refdef.viewaxis[1]);
	VectorCopy(fd->viewaxis[2], tr.refdef.viewaxis[2]);

	tr.refdef.time = fd->time;
	tr.refdef.rdflags = fd->rdflags;

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = false;
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		// compare the area bits
		int areaDiff = 0;
		for (int i = 0; i < MAX_MAP_AREA_BYTES / 4; i++)
		{
			areaDiff |= ((int*)tr.refdef.areamask)[i] ^ ((int*)fd->areamask)[i];
			((int*)tr.refdef.areamask)[i] = ((int*)fd->areamask)[i];
		}

		if (areaDiff)
		{
			// a door just opened or something
			tr.refdef.areamaskModified = true;
		}
	}


	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData[tr.smpFrame]->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData[tr.smpFrame]->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData[tr.smpFrame]->dlights[r_firstSceneDlight];

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData[tr.smpFrame]->polys[r_firstScenePoly];

	tr.refdef.lightstyles = backEndData[tr.smpFrame]->lightstyles;

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if (r_dynamiclight->integer == 0 || r_vertexLight->integer == 1)
	{
		tr.refdef.num_dlights = 0;
	}

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;
}
