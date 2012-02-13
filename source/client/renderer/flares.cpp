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
=============================================================================

LIGHT FLARES

A light flare is an effect that takes place inside the eye when bright light
sources are visible.  The size of the flare reletive to the screen is nearly
constant, irrespective of distance, but the intensity should be proportional to the
projected area of the light source.

A surface that has been flagged as having a light flare will calculate the depth
buffer value that it's midpoint should have when the surface is added.

After all opaque surfaces have been rendered, the depth buffer is read back for
each flare in view.  If the point has not been obscured by a closer surface, the
flare should be drawn.

Surfaces that have a repeated texture should never be flagged as flaring, because
there will only be a single flare added at the midpoint of the polygon.

To prevent abrupt popping, the intensity of the flare is interpolated up and
down as it changes visibility.  This involves scene to scene state, unlike almost
all other aspects of the renderer, and is complicated by the fact that a single
frame may have multiple scenes.

RB_RenderFlares() will be called once per view (twice in a mirrored scene, potentially
up to five or more times in a frame with 3D status bar icons).

=============================================================================
*/

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

#define MAX_FLARES		128

// TYPES -------------------------------------------------------------------

// flare states maintain visibility over multiple frames for fading
// layers: view, mirror, menu
struct flare_t
{
	flare_t*	next;		// for active chain

	int			addedFrame;

	bool		inPortal;				// true if in a portal view of the scene
	int			frameSceneNum;
	void		*surface;
	int			fogNum;

	int			fadeTime;

	bool		visible;			// state of last test
	float		drawIntensity;		// may be non 0 even if !visible due to fading

	int			windowX, windowY;
	float		eyeZ;

	vec3_t		color;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static flare_t		r_flareStructs[MAX_FLARES];
static flare_t*		r_activeFlares;
static flare_t*		r_inactiveFlares;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_ClearFlares
//
//==========================================================================

void R_ClearFlares()
{
	Com_Memset(r_flareStructs, 0, sizeof(r_flareStructs));
	r_activeFlares = NULL;
	r_inactiveFlares = NULL;

	for (int i = 0; i < MAX_FLARES; i++)
	{
		r_flareStructs[i].next = r_inactiveFlares;
		r_inactiveFlares = &r_flareStructs[i];
	}
}

//==========================================================================
//
//	RB_AddFlare
//
//	This is called at surface tesselation time
//
//==========================================================================

static void RB_AddFlare(void* surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal)
{
	backEnd.pc.c_flareAdds++;

	// if the point is off the screen, don't bother adding it
	// calculate screen coordinates and depth
	vec4_t eye, clip;
	R_TransformModelToClip(point, backEnd.orient.modelMatrix, 
		backEnd.viewParms.projectionMatrix, eye, clip);

	// check to see if the point is completely off screen
	for (int i = 0; i < 3; i++)
	{
		if (clip[i] >= clip[3] || clip[i] <= -clip[3])
		{
			return;
		}
	}

	vec4_t normalized, window;
	R_TransformClipToWindow(clip, &backEnd.viewParms, normalized, window);

	if (window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth ||
		window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight)
	{
		return;	// shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	// see if a flare with a matching surface, scene, and view exists
	flare_t* f;
	for (f = r_activeFlares; f; f = f->next)
	{
		if (f->surface == surface && f->frameSceneNum == backEnd.viewParms.frameSceneNum &&
			f->inPortal == backEnd.viewParms.isPortal)
		{
			break;
		}
	}

	// allocate a new one
	if (!f)
	{
		if (!r_inactiveFlares)
		{
			// the list is completely full
			return;
		}
		f = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next = r_activeFlares;
		r_activeFlares = f;

		f->surface = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->inPortal = backEnd.viewParms.isPortal;
		f->addedFrame = -1;
	}

	if (f->addedFrame != backEnd.viewParms.frameCount - 1)
	{
		f->visible = false;
		f->fadeTime = backEnd.refdef.time - 2000;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum = fogNum;

	VectorCopy(color, f->color);

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	if (normal)
	{
		vec3_t local;
		VectorSubtract(backEnd.viewParms.orient.origin, point, local);
		VectorNormalizeFast(local);
		float d = DotProduct(local, normal);
		VectorScale(f->color, d, f->color);
	}

	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[0];
	f->windowY = backEnd.viewParms.viewportY + window[1];

	f->eyeZ = eye[2];
}

//==========================================================================
//
//	RB_AddDlightFlares
//
//==========================================================================

static void RB_AddDlightFlares()
{
	if (!r_flares->integer)
	{
		return;
	}

	dlight_t* l = backEnd.refdef.dlights;
	mbrush46_fog_t* fog = tr.world->fogs;
	for (int i = 0 ; i < backEnd.refdef.num_dlights; i++, l++)
	{
		// find which fog volume the light is in 
		int j;
		for (j = 1; j < tr.world->numfogs; j++)
		{
			fog = &tr.world->fogs[j];
			int k;
			for (k = 0; k < 3; k++)
			{
				if (l->origin[k] < fog->bounds[0][k] || l->origin[k] > fog->bounds[1][k])
				{
					break;
				}
			}
			if (k == 3)
			{
				break;
			}
		}
		if (j == tr.world->numfogs)
		{
			j = 0;
		}

		RB_AddFlare((void*)l, j, l->origin, l->color, NULL);
	}
}

//==========================================================================
//
//	RB_TestFlare
//
//==========================================================================

static void RB_TestFlare(flare_t* f)
{
	backEnd.pc.c_flareTests++;

	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = false;

	// read back the z buffer contents
	float depth;
	qglReadPixels(f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	float screenZ = backEnd.viewParms.projectionMatrix[14] / 
		((2 * depth - 1) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10]);

	bool visible = (-f->eyeZ - -screenZ) < 24;

	float fade;
	if (visible)
	{
		if (!f->visible)
		{
			f->visible = true;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = ((backEnd.refdef.time - f->fadeTime) /1000.0f) * r_flareFade->value;
	}
	else
	{
		if (f->visible)
		{
			f->visible = false;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = 1.0f - ((backEnd.refdef.time - f->fadeTime) / 1000.0f) * r_flareFade->value;
	}

	if (fade < 0)
	{
		fade = 0;
	}
	if (fade > 1)
	{
		fade = 1;
	}

	f->drawIntensity = fade;
}

//==========================================================================
//
//	RB_RenderFlare
//
//==========================================================================

static void RB_RenderFlare(flare_t* f)
{
	backEnd.pc.c_flareRenders++;

	vec3_t color;
	VectorScale(f->color, f->drawIntensity * tr.identityLight, color);
	int iColor[3];
	iColor[0] = color[0] * 255;
	iColor[1] = color[1] * 255;
	iColor[2] = color[2] * 255;

	float size = backEnd.viewParms.viewportWidth * (r_flareSize->value / 640.0f + 8 / -f->eyeZ);

	RB_BeginSurface(tr.flareShader, f->fogNum);

	// FIXME: use quadstamp?
	tess.xyz[tess.numVertexes][0] = f->windowX - size;
	tess.xyz[tess.numVertexes][1] = f->windowY - size;
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = 255;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX - size;
	tess.xyz[tess.numVertexes][1] = f->windowY + size;
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = 255;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX + size;
	tess.xyz[tess.numVertexes][1] = f->windowY + size;
	tess.texCoords[tess.numVertexes][0][0] = 1;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = 255;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX + size;
	tess.xyz[tess.numVertexes][1] = f->windowY - size;
	tess.texCoords[tess.numVertexes][0][0] = 1;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = 255;
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	RB_EndSurface();
}

//==========================================================================
//
//	RB_RenderFlares
//
//	Because flares are simulating an occular effect, they should be drawn after
// everything (all views) in the entire frame has been drawn.
//
//	Because of the way portals use the depth buffer to mark off areas, the
// needed information would be lost after each view, so we are forced to draw
// flares after each view.
//
// The resulting artifact is that flares in mirrors or portals don't dim properly
// when occluded by something in the main view, and portal flares that should
// extend past the portal edge will be overwritten.
//
//==========================================================================

void RB_RenderFlares()
{
	if (!r_flares->integer)
	{
		return;
	}

	RB_AddDlightFlares();

	// perform z buffer readback on each flare in this view
	bool draw = false;
	flare_t** prev = &r_activeFlares;
	flare_t* f;
	while ((f = *prev) != NULL)
	{
		// throw out any flares that weren't added last frame
		if (f->addedFrame < backEnd.viewParms.frameCount - 1)
		{
			*prev = f->next;
			f->next = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		// don't draw any here that aren't from this scene / portal
		f->drawIntensity = 0;
		if (f->frameSceneNum == backEnd.viewParms.frameSceneNum &&
			f->inPortal == backEnd.viewParms.isPortal)
		{
			RB_TestFlare(f);
			if (f->drawIntensity)
			{
				draw = true;
			}
			else
			{
				// this flare has completely faded out, so remove it from the chain
				*prev = f->next;
				f->next = r_inactiveFlares;
				r_inactiveFlares = f;
				continue;
			}
		}

		prev = &f->next;
	}

	if (!draw)
	{
		return;		// none visible
	}

	if (backEnd.viewParms.isPortal)
	{
		qglDisable(GL_CLIP_PLANE0);
	}

	qglPushMatrix();
    qglLoadIdentity();
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
    qglLoadIdentity();
	qglOrtho(backEnd.viewParms.viewportX, backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
		backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
		-99999, 99999);

	for (f = r_activeFlares; f; f = f->next)
	{
		if (f->frameSceneNum == backEnd.viewParms.frameSceneNum &&
			f->inPortal == backEnd.viewParms.isPortal && f->drawIntensity)
		{
			RB_RenderFlare(f);
		}
	}

	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();
}
