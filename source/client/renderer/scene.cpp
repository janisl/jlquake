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

int r_firstSceneDrawSurf;

int r_numentities;
int r_firstSceneEntity;

int r_numdlights;
int r_firstSceneDlight;

int r_numpolys;
int r_firstScenePoly;

int r_numpolyverts;

int r_numparticles;
int r_firstSceneParticle;

int skyboxportal;
int drawskyboxportal;

static int r_numcoronas;
static int r_firstSceneCorona;

static int r_firstScenePolybuffer;
static int r_numpolybuffers;

static int r_firstSceneDecalProjector;
int r_numDecalProjectors;
static int r_firstSceneDecal;
static int r_numDecals;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// Temp storage for saving view paramters.  Drawing the animated head in the corner
// was creaming important view info.
static viewParms_t g_oldViewParms;

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

	r_numparticles = 0;
	r_firstSceneParticle = 0;

	r_numcoronas = 0;
	r_firstSceneCorona = 0;

	r_numpolybuffers = 0;
	r_firstScenePolybuffer = 0;

	r_numDecalProjectors = 0;
	r_firstSceneDecalProjector = 0;
	r_numDecals = 0;
	r_firstSceneDecal = 0;
}

//==========================================================================
//
//	R_ClearScene
//
//==========================================================================

void R_ClearScene()
{
	// ydnar: clear model stuff for dynamic fog
	if (tr.world != NULL)
	{
		for (int i = 0; i < tr.world->numBModels; i++)
		{
			tr.world->bmodels[i].visible[tr.smpFrame] = false;
		}
	}

	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstSceneCorona = r_numcoronas;
	r_firstScenePoly = r_numpolys;
	r_firstScenePolybuffer = r_numpolybuffers;
	r_firstSceneParticle = r_numparticles;
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
		throw DropException(va("R_AddRefEntityToScene: bad reType %i", ent->reType));
	}

	backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
	backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = false;

	r_numentities++;

	// ydnar: add projected shadows for this model
	R_AddModelShadow(ent);
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
	VectorCopy(org, dl->origin);
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

// Ridah, added support for overdraw field
void R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b, int overdraw)
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
	// RF, allow us to force some dlights under all circumstances
	if (!(overdraw & REF_FORCE_DLIGHT))
	{
		if (r_dynamiclight->integer == 0)
		{
			return;
		}
		if (r_dynamiclight->integer == 2 && !(backEndData[tr.smpFrame]->dlights[r_numdlights].forced))
		{
			return;
		}
	}

	if (r_dlightScale->value <= 0)		//----(SA)	added
	{
		return;
	}

	overdraw &= ~REF_FORCE_DLIGHT;
	overdraw &= ~REF_JUNIOR_DLIGHT;

	dlight_t* dl = &backEndData[tr.smpFrame]->dlights[r_numdlights++];
	VectorCopy(org, dl->origin);
	dl->radius = intensity * r_dlightScale->value;	//----(SA)	modified
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->shader = NULL;
	dl->overdraw = 0;

	if (overdraw == 10)		// sorry, hijacking 10 for a quick hack (SA)
	{
		dl->shader = R_GetShaderByHandle(R_RegisterShader("negdlightshader"));
	}
	else if (overdraw == 11)		// 11 is flames
	{
		dl->shader = R_GetShaderByHandle(R_RegisterShader("flamedlightshader"));
	}
	else
	{
		dl->overdraw = overdraw;
	}
}

//	ydnar: modified dlight system to support seperate radius and intensity
void R_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags)
{
	// early out
	if (!tr.registered || r_numdlights >= MAX_DLIGHTS || radius <= 0 || intensity <= 0)
	{
		return;
	}

	// RF, allow us to force some dlights under all circumstances
	if (!(flags & REF_FORCE_DLIGHT))
	{
		if (r_dynamiclight->integer == 0)
		{
			return;
		}
	}

	// set up a new dlight
	dlight_t* dl = &backEndData[tr.smpFrame]->dlights[r_numdlights++];
	VectorCopy(org, dl->origin);
	VectorCopy(org, dl->transformed);
	dl->radius = radius;
	dl->radiusInverseCubed = (1.0 / dl->radius);
	dl->radiusInverseCubed = dl->radiusInverseCubed * dl->radiusInverseCubed * dl->radiusInverseCubed;
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->shader = R_GetShaderByHandle(hShader);
	if (dl->shader == tr.defaultShader)
	{
		dl->shader = NULL;
	}
	dl->flags = flags;
}

void R_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, int flags)
{
	if (!tr.registered)
	{
		return;
	}
	if (r_numcoronas >= MAX_CORONAS)
	{
		return;
	}

	corona_t* cor = &backEndData[tr.smpFrame]->coronas[r_numcoronas++];
	VectorCopy(org, cor->origin);
	cor->color[0] = r;
	cor->color[1] = g;
	cor->color[2] = b;
	cor->scale = scale;
	cor->id = id;
	cor->flags = flags;
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
		Log::write(S_COLOR_YELLOW "WARNING: R_AddPolyToScene: NULL poly shader\n");
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
			Log::develWrite(S_COLOR_RED "WARNING: R_AddPolyToScene: r_max_polys or r_max_polyverts reached\n");
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

//JL Saving a pointer - another reason for fucked up SMP.
void R_AddPolyBufferToScene(polyBuffer_t* pPolyBuffer)
{
	if (r_numpolybuffers >= MAX_POLYS)
	{
		return;
	}

	srfPolyBuffer_t* pPolySurf = &backEndData[tr.smpFrame]->polybuffers[r_numpolybuffers];
	r_numpolybuffers++;

	pPolySurf->surfaceType = SF_POLYBUFFER;
	pPolySurf->pPolyBuffer = pPolyBuffer;

	vec3_t bounds[2];
	VectorCopy(pPolyBuffer->xyz[0], bounds[0]);
	VectorCopy(pPolyBuffer->xyz[0], bounds[1]);
	for (int i = 1; i < pPolyBuffer->numVerts; i++)
	{
		AddPointToBounds(pPolyBuffer->xyz[i], bounds[0], bounds[1]);
	}
	int fogIndex;
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

	pPolySurf->fogIndex = fogIndex;
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
		throw DropException(va("Bad light style %i", style));
	}
	lightstyle_t* ls = &backEndData[tr.smpFrame]->lightstyles[style];
	ls->white = r + g + b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

//==========================================================================
//
//	R_AddParticleToScene
//
//==========================================================================

void R_AddParticleToScene(vec3_t org, int r, int g, int b, int a, float size, QParticleTexture Texture)
{
	if (r_numparticles >= MAX_REF_PARTICLES)
	{
		return;
	}
	particle_t* p = &backEndData[tr.smpFrame]->particles[r_numparticles++];
	VectorCopy(org, p->origin);
	p->rgba[0] = r;
	p->rgba[1] = g;
	p->rgba[2] = b;
	p->rgba[3] = a;
	p->size = size;
	p->Texture = Texture;
}

//==========================================================================
//
//	R_SetLightLevel
//
//==========================================================================

static void R_SetLightLevel()
{
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	// save off light value for server to look at (BIG HACK!)

	vec3_t shadelight;
	R_LightPointQ2(tr.refdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[0];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
	else
	{
		if (shadelight[1] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[1];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
}

//==========================================================================
//
//	R_RenderScene
//
//	Draw a 3D view into a part of the window, then return to 2D drawing.
//
//	Rendering a scene may require multiple views to be rendered to handle mirrors
//
//==========================================================================

void R_RenderScene(const refdef_t* fd)
{
	if (!tr.registered)
	{
		return;
	}
	QGL_LogComment("====== R_RenderScene =====\n");

	if (r_norefresh->integer)
	{
		return;
	}

	int startTime = CL_ScaledMilliseconds();

	if (!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL))
	{
		throw DropException("R_RenderScene: NULL worldmodel");
	}

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

	if (fd->rdflags & RDF_SKYBOXPORTAL)
	{
		skyboxportal = 1;
	}

	if (fd->rdflags & RDF_DRAWSKYBOX)
	{
		drawskyboxportal = 1;
	}
	else
	{
		drawskyboxportal = 0;
	}

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
	tr.refdef.dlightBits = 0;

	tr.refdef.num_coronas = r_numcoronas - r_firstSceneCorona;
	tr.refdef.coronas = &backEndData[tr.smpFrame]->coronas[r_firstSceneCorona];

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData[tr.smpFrame]->polys[r_firstScenePoly];

	tr.refdef.numPolyBuffers = r_numpolybuffers - r_firstScenePolybuffer;
	tr.refdef.polybuffers = &backEndData[tr.smpFrame]->polybuffers[r_firstScenePolybuffer];

	tr.refdef.numDecalProjectors = r_numDecalProjectors - r_firstSceneDecalProjector;
	tr.refdef.decalProjectors = &backEndData[tr.smpFrame]->decalProjectors[r_firstSceneDecalProjector];

	tr.refdef.numDecals = r_numDecals - r_firstSceneDecal;
	tr.refdef.decals = &backEndData[tr.smpFrame]->decals[r_firstSceneDecal];

	tr.refdef.lightstyles = backEndData[tr.smpFrame]->lightstyles;

	tr.refdef.num_particles = r_numparticles - r_firstSceneParticle;
	tr.refdef.particles = &backEndData[tr.smpFrame]->particles[r_firstSceneParticle];

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if (!(GGameType & (GAME_WolfMP | GAME_ET)) &&
		((!(GGameType & GAME_WolfSP) && r_dynamiclight->integer == 0) || r_vertexLight->integer == 1))
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

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	viewParms_t parms;
	Com_Memset(&parms, 0, sizeof(parms));
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal = false;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy(fd->vieworg, parms.orient.origin);
	VectorCopy(fd->viewaxis[0], parms.orient.axis[0]);
	VectorCopy(fd->viewaxis[1], parms.orient.axis[1]);
	VectorCopy(fd->viewaxis[2], parms.orient.axis[2]);

	VectorCopy(fd->vieworg, parms.pvsOrigin);

	if (GGameType & GAME_QuakeHexen)
	{
		R_PushDlightsQ1();
	}
	else if (GGameType & GAME_Quake2)
	{
		R_PushDlightsQ2();
	}

	R_RenderView(&parms);

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	R_ClearScene();

	if (GGameType & GAME_Quake2)
	{
		R_SetLightLevel();
	}

	tr.frontEndMsec += CL_ScaledMilliseconds() - startTime;
}

//	Save out the old render info to a temp place so we don't kill the LOD system
// when we do a second render.
void R_SaveViewParms()
{
	// save old viewParms so we can return to it after the mirror view
	g_oldViewParms = tr.viewParms;
}

//	Restore the old render info so we don't kill the LOD system
// when we do a second render.
void R_RestoreViewParms()
{
	// This was killing the LOD computation
	tr.viewParms = g_oldViewParms;
}

/*
    rgb = colour
    depthForOpaque is depth for opaque

    the restore flag can be used to restore the original level fog
    duration can be set to fade over a certain period
*/
void R_SetGlobalFog(bool restore, int duration, float r, float g, float b, float depthForOpaque)
{
	if (restore)
	{
		if (duration > 0)
		{
			VectorCopy(tr.world->fogs[tr.world->globalFog].shader->fogParms.color, tr.world->globalTransStartFog);
			tr.world->globalTransStartFog[3] = tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque;

			Vector4Copy(tr.world->globalOriginalFog, tr.world->globalTransEndFog);

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime = tr.refdef.time + duration;
		}
		else
		{
			VectorCopy(tr.world->globalOriginalFog, tr.world->fogs[tr.world->globalFog].shader->fogParms.color);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.colorInt = ColorBytes4(tr.world->globalOriginalFog[0] * tr.identityLight,
				tr.world->globalOriginalFog[1] * tr.identityLight,
				tr.world->globalOriginalFog[2] * tr.identityLight, 1.0);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque = tr.world->globalOriginalFog[3];
			tr.world->fogs[tr.world->globalFog].shader->fogParms.tcScale = 1.0f / (tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque);
		}
	}
	else
	{
		if (duration > 0)
		{
			VectorCopy(tr.world->fogs[tr.world->globalFog].shader->fogParms.color, tr.world->globalTransStartFog);
			tr.world->globalTransStartFog[3] = tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque;

			VectorSet(tr.world->globalTransEndFog, r, g, b);
			tr.world->globalTransEndFog[3] = depthForOpaque < 1 ? 1 : depthForOpaque;

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime = tr.refdef.time + duration;
		}
		else
		{
			VectorSet(tr.world->fogs[tr.world->globalFog].shader->fogParms.color, r, g, b);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.colorInt = ColorBytes4(r * tr.identityLight,
				g * tr.identityLight,
				b * tr.identityLight, 1.0);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque = depthForOpaque < 1 ? 1 : depthForOpaque;
			tr.world->fogs[tr.world->globalFog].shader->fogParms.tcScale = 1.0f / (tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque);
		}
	}
}
