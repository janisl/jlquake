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

#ifndef _R_BACKEND_H
#define _R_BACKEND_H

#include "commands.h"

#define MAX_ENTITIES            1023		// can't be increased without changing drawsurf bit packing
#define REF_ENTITYNUM_WORLD     MAX_ENTITIES

#define MAX_DRAWSURFS           0x40000
#define DRAWSURF_MASK           ( MAX_DRAWSURFS - 1 )

// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define MAX_POLYS       4096
#define MAX_POLYVERTS   8192

#define MAX_REF_PARTICLES       ( 8 * 1024 )

#define MAX_CORONAS     32			//----(SA)	not really a reason to limit this other than trying to keep a reasonable count

// ydnar: max decal projectors per frame, each can generate lots of polys
#define MAX_DECAL_PROJECTORS    32	// uses bitmasks, don't increase
#define DECAL_PROJECTOR_MASK    ( MAX_DECAL_PROJECTORS - 1 )
#define MAX_DECALS              1024

struct backEndCounters_t {
	int c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	float c_overDraw;

	int c_dlightVertexes;
	int c_dlightIndexes;

	int c_flareAdds;
	int c_flareTests;
	int c_flareRenders;

	int msec;				// total msec for backend run
};

// all state modified by the back end is seperated
// from the front end state
struct backEndState_t {
	int smpFrame;
	trRefdef_t refdef;
	viewParms_t viewParms;
	orientationr_t orient;
	backEndCounters_t pc;
	qboolean isHyperspace;
	trRefEntity_t* currentEntity;
	qboolean skyRenderedThisView;		// flag for drawing sun

	qboolean projection2D;		// if true, drawstretchpic doesn't need to change modes
	byte color2D[ 4 ];
	qboolean vertexes2D;		// shader needs to be finished
	trRefEntity_t entity2D;		// currentEntity will point at this when doing 2D rendering
};

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
struct backEndData_t {
	drawSurf_t drawSurfs[ MAX_DRAWSURFS ];
	dlight_t dlights[ MAX_DLIGHTS ];
	trRefEntity_t entities[ MAX_ENTITIES ];
	idSurfacePoly* polys;			//[MAX_POLYS];
	polyVert_t* polyVerts;			//[MAX_POLYVERTS];
	lightstyle_t lightstyles[ MAX_LIGHTSTYLES ];
	particle_t particles[ MAX_REF_PARTICLES ];
	corona_t coronas[ MAX_CORONAS ];
	idSurfacePolyBuffer* polybuffers;
	decalProjector_t decalProjectors[ MAX_DECAL_PROJECTORS ];
	idSurfaceDecal* decals;
	renderCommandList_t commands;
};

extern backEndState_t backEnd;
extern backEndData_t* backEndData[ SMP_FRAMES ];		// the second one may not be allocated

extern int max_polys;
extern int max_polyverts;

extern volatile bool renderThreadActive;

void R_InitBackEndData();
void R_FreeBackEndData();
void RB_SetGL2D();
void RB_ShowImages();
void RB_ExecuteRenderCommands( const void* data );
void RB_RenderThread();

#endif
