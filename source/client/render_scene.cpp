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

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_CommonToggleSmpFrame
//
//==========================================================================

void R_CommonToggleSmpFrame()
{
	r_firstSceneDrawSurf = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;
}

//==========================================================================
//
//	R_CommonClearScene
//
//==========================================================================

void R_CommonClearScene()
{
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
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
}
