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

#define WAVEVALUE(table, base, amplitude, phase, freq)  ((base) + table[myftol((((phase) + tess.shaderTime * (freq)) * FUNCTABLE_SIZE)) & FUNCTABLE_MASK] * (amplitude))

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static vec3_t lightOrigin = { -960, 1980, 96 };		// FIXME: track dynamically

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	TableForFunc
//
//==========================================================================

static float* TableForFunc(genFunc_t func)
{
	switch (func)
	{
	case GF_SIN:
		return tr.sinTable;
	case GF_TRIANGLE:
		return tr.triangleTable;
	case GF_SQUARE:
		return tr.squareTable;
	case GF_SAWTOOTH:
		return tr.sawToothTable;
	case GF_INVERSE_SAWTOOTH:
		return tr.inverseSawToothTable;
	case GF_NONE:
	default:
		break;
	}

	throw QDropException(va("TableForFunc called with invalid function '%d' in shader '%s'\n", func, tess.shader->name));
}

//==========================================================================
//
//	EvalWaveForm
//
//	Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
//
//==========================================================================

static float EvalWaveForm(const waveForm_t* wf)
{
	float* table = TableForFunc(wf->func);

	return WAVEVALUE(table, wf->base, wf->amplitude, wf->phase, wf->frequency);
}

//==========================================================================
//
//	EvalWaveFormClamped
//
//==========================================================================

static float EvalWaveFormClamped(const waveForm_t* wf)
{
	float glow = EvalWaveForm(wf);

	if (glow < 0)
	{
		return 0;
	}

	if (glow > 1)
	{
		return 1;
	}

	return glow;
}

/*
====================================================================

DEFORMATIONS

====================================================================
*/

//==========================================================================
//
//	RB_CalcDeformNormals
//
//	Wiggle the normals for wavy environment mapping
//
//==========================================================================

void RB_CalcDeformNormals(deformStage_t* ds)
{
	float* xyz = (float*)tess.xyz;
	float* normal = (float*)tess.normal;

	for (int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
	{
		float scale = 0.98f;
		scale = R_NoiseGet4f(xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[0] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f(100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[1] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f(200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency);
		normal[2] += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast(normal);
	}
}

//==========================================================================
//
//	RB_CalcDeformVertexes
//
//==========================================================================

void RB_CalcDeformVertexes(deformStage_t* ds)
{
	float* xyz = (float*)tess.xyz;
	float* normal = (float*)tess.normal;

	if (ds->deformationWave.frequency == 0)
	{
		float scale = EvalWaveForm(&ds->deformationWave);

		for (int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			vec3_t offset;
			VectorScale(normal, scale, offset);

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
	else
	{
		float* table = TableForFunc(ds->deformationWave.func);

		for (int i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4)
		{
			float off = (xyz[0] + xyz[1] + xyz[2]) * ds->deformationSpread;

			float scale = WAVEVALUE(table, ds->deformationWave.base, 
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency);

			vec3_t offset;
			VectorScale(normal, scale, offset);

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
}

//==========================================================================
//
//	RB_CalcBulgeVertexes
//
//==========================================================================

void RB_CalcBulgeVertexes(deformStage_t* ds)
{
	const float* st = (const float*)tess.texCoords[0];
	float* xyz = (float*) tess.xyz;
	float* normal = (float*)tess.normal;

	float now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

	for (int i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4)
	{
		int off = (int)((float)(FUNCTABLE_SIZE / (M_PI * 2)) * (st[0] * ds->bulgeWidth + now));

		float scale = tr.sinTable[off & FUNCTABLE_MASK] * ds->bulgeHeight;

		xyz[0] += normal[0] * scale;
		xyz[1] += normal[1] * scale;
		xyz[2] += normal[2] * scale;
	}
}

//==========================================================================
//
//	RB_CalcMoveVertexes
//
//	A deformation that can move an entire surface along a wave path
//
//==========================================================================

void RB_CalcMoveVertexes(deformStage_t* ds)
{
	float* table = TableForFunc(ds->deformationWave.func);

	float scale = WAVEVALUE(table, ds->deformationWave.base, 
		ds->deformationWave.amplitude,
		ds->deformationWave.phase,
		ds->deformationWave.frequency);

	vec3_t offset;
	VectorScale(ds->moveVector, scale, offset);

	float* xyz = (float*)tess.xyz;
	for (int i = 0; i < tess.numVertexes; i++, xyz += 4)
	{
		VectorAdd(xyz, offset, xyz);
	}
}

//==========================================================================
//
//	DeformText
//
//	Change a polygon into a bunch of text polygons
//
//==========================================================================

void DeformText(const char* text)
{
	vec3_t height;
	height[0] = 0;
	height[1] = 0;
	height[2] = -1;
	vec3_t width;
	CrossProduct(tess.normal[0], height, width);

	// find the midpoint of the box
	vec3_t mid;
	VectorClear(mid);
	float bottom = 999999;
	float top = -999999;
	for (int i = 0; i < 4; i++)
	{
		VectorAdd(tess.xyz[i], mid, mid);
		if (tess.xyz[i][2] < bottom)
		{
			bottom = tess.xyz[i][2];
		}
		if (tess.xyz[i][2] > top)
		{
			top = tess.xyz[i][2];
		}
	}
	vec3_t origin;
	VectorScale(mid, 0.25f, origin);

	// determine the individual character size
	height[0] = 0;
	height[1] = 0;
	height[2] = (top - bottom) * 0.5f;

	VectorScale(width, height[2] * -0.75f, width);

	// determine the starting position
	int len = QStr::Length(text);
	VectorMA(origin, (len - 1), width, origin);

	// clear the shader indexes
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	byte color[4];
	color[0] = color[1] = color[2] = color[3] = 255;

	// draw each character
	for (int i = 0; i < len; i++)
	{
		int ch = text[i];
		ch &= 255;

		if (ch != ' ')
		{
			int row = ch >> 4;
			int col = ch & 15;

			float frow = row * 0.0625f;
			float fcol = col * 0.0625f;
			float size = 0.0625f;

			RB_AddQuadStampExt(origin, width, height, color, fcol, frow, fcol + size, frow + size);
		}
		VectorMA(origin, -2, width, origin);
	}
}

/*
====================================================================

COLORS

====================================================================
*/

//==========================================================================
//
//	RB_CalcDiffuseColor
//
//	The basic vertex lighting calc
//
//==========================================================================

void RB_CalcDiffuseColor(byte* colors)
{
	trRefEntity_t* ent = backEnd.currentEntity;
	int ambientLightInt = ent->ambientLightInt;
	vec3_t ambientLight;
	VectorCopy(ent->ambientLight, ambientLight);
	vec3_t directedLight;
	VectorCopy(ent->directedLight, directedLight);
	vec3_t lightDir;
	VectorCopy(ent->lightDir, lightDir);

	float* v = tess.xyz[0];
	float* normal = tess.normal[0];

	int numVertexes = tess.numVertexes;
	for (int i = 0; i < numVertexes; i++, v += 4, normal += 4)
	{
		float incoming = DotProduct(normal, lightDir);
		if (incoming <= 0)
		{
			*(int*)&colors[i * 4] = ambientLightInt;
			continue;
		} 
		int j = myftol(ambientLight[0] + incoming * directedLight[0]);
		if (j > 255)
		{
			j = 255;
		}
		colors[i * 4 + 0] = j;

		j = myftol(ambientLight[1] + incoming * directedLight[1]);
		if (j > 255)
		{
			j = 255;
		}
		colors[i * 4 + 1] = j;

		j = myftol(ambientLight[2] + incoming * directedLight[2]);
		if (j > 255)
		{
			j = 255;
		}
		colors[i * 4 + 2] = j;

		colors[i * 4 + 3] = 255;
	}
}

//==========================================================================
//
//	RB_CalcWaveColor
//
//==========================================================================

static void RB_CalcWaveColor(const waveForm_t* wf, byte* dstColors)
{
	float glow;
	if (wf->func == GF_NOISE)
	{
		glow = wf->base + R_NoiseGet4f(0, 0, 0, (tess.shaderTime + wf->phase) * wf->frequency) * wf->amplitude;
	}
	else
	{
		glow = EvalWaveForm(wf) * tr.identityLight;
	}
	
	if (glow < 0)
	{
		glow = 0;
	}
	else if (glow > 1)
	{
		glow = 1;
	}

	int v = myftol(255 * glow);
	byte color[4];
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	v = *(int*)color;

	int* colors = (int*)dstColors;
	for (int i = 0; i < tess.numVertexes; i++, colors++)
	{
		*colors = v;
	}
}

//==========================================================================
//
//	RB_CalcColorFromEntity
//
//==========================================================================

static void RB_CalcColorFromEntity(byte* dstColors)
{
	if (!backEnd.currentEntity)
	{
		return;
	}

	int c = *(int*)backEnd.currentEntity->e.shaderRGBA;
	int* pColors = (int*)dstColors;

	for (int i = 0; i < tess.numVertexes; i++, pColors++)
	{
		*pColors = c;
	}
}

//==========================================================================
//
//	RB_CalcColorFromOneMinusEntity
//
//==========================================================================

static void RB_CalcColorFromOneMinusEntity(byte* dstColors)
{
	if (!backEnd.currentEntity)
	{
		return;
	}

	byte invModulate[4];
	invModulate[0] = 255 - backEnd.currentEntity->e.shaderRGBA[0];
	invModulate[1] = 255 - backEnd.currentEntity->e.shaderRGBA[1];
	invModulate[2] = 255 - backEnd.currentEntity->e.shaderRGBA[2];
	invModulate[3] = 255 - backEnd.currentEntity->e.shaderRGBA[3];	// this trashes alpha, but the AGEN block fixes it

	int c = *(int*)invModulate;
	int* pColors = (int*)dstColors;

	for (int i = 0; i < tess.numVertexes; i++, pColors++)
	{
		*pColors = c;
	}
}

//==========================================================================
//
//	RB_CalcWaveAlpha
//
//==========================================================================

static void RB_CalcWaveAlpha(const waveForm_t* wf, byte* dstColors)
{
	float glow = EvalWaveFormClamped(wf);

	int v = 255 * glow;

	for (int i = 0; i < tess.numVertexes; i++, dstColors += 4)
	{
		dstColors[3] = v;
	}
}

//==========================================================================
//
//	RB_CalcSpecularAlpha
//
//	Calculates specular coefficient and places it in the alpha channel
//
//==========================================================================

static void RB_CalcSpecularAlpha(byte* alphas)
{
	float* v = tess.xyz[0];
	float* normal = tess.normal[0];

	alphas += 3;

	int numVertexes = tess.numVertexes;
	for (int i = 0; i < numVertexes; i++, v += 4, normal += 4, alphas += 4)
	{
		vec3_t lightDir;
		VectorSubtract(lightOrigin, v, lightDir);
		VectorNormalizeFast(lightDir);

		// calculate the specular color
		float d = DotProduct(normal, lightDir);

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		vec3_t reflected;
		reflected[0] = normal[0] * 2 * d - lightDir[0];
		reflected[1] = normal[1] * 2 * d - lightDir[1];
		reflected[2] = normal[2] * 2 * d - lightDir[2];

		vec3_t viewer;
		VectorSubtract(backEnd.orient.viewOrigin, v, viewer);
		float ilength = Q_rsqrt(DotProduct(viewer, viewer));
		float l = DotProduct(reflected, viewer);
		l *= ilength;

		int b;
		if (l < 0)
		{
			b = 0;
		}
		else
		{
			l = l * l;
			l = l * l;
			b = l * 255;
			if (b > 255)
			{
				b = 255;
			}
		}

		*alphas = b;
	}
}

//==========================================================================
//
//	RB_CalcAlphaFromEntity
//
//==========================================================================

static void RB_CalcAlphaFromEntity(byte* dstColors)
{
	if (!backEnd.currentEntity)
	{
		return;
	}

	dstColors += 3;

	for (int i = 0; i < tess.numVertexes; i++, dstColors += 4)
	{
		*dstColors = backEnd.currentEntity->e.shaderRGBA[3];
	}
}

//==========================================================================
//
//	RB_CalcAlphaFromOneMinusEntity
//
//==========================================================================

static void RB_CalcAlphaFromOneMinusEntity(byte* dstColors)
{
	if (!backEnd.currentEntity)
	{
		return;
	}

	dstColors += 3;

	for (int i = 0; i < tess.numVertexes; i++, dstColors += 4)
	{
		*dstColors = 0xff - backEnd.currentEntity->e.shaderRGBA[3];
	}
}

//==========================================================================
//
//	RB_CalcModulateColorsByFog
//
//==========================================================================

static void RB_CalcModulateColorsByFog(byte* colors)
{
	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[SHADER_MAX_VERTEXES][2];
	RB_CalcFogTexCoords(texCoords[0]);

	for (int i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
	}
}

//==========================================================================
//
//	RB_CalcModulateAlphasByFog
//
//==========================================================================

static void RB_CalcModulateAlphasByFog(byte* colors)
{
	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[SHADER_MAX_VERTEXES][2];
	RB_CalcFogTexCoords(texCoords[0]);

	for (int i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		colors[3] *= f;
	}
}

//==========================================================================
//
//	RB_CalcModulateRGBAsByFog
//
//==========================================================================

static void RB_CalcModulateRGBAsByFog(byte* colors)
{
	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	float texCoords[SHADER_MAX_VERTEXES][2];
	RB_CalcFogTexCoords(texCoords[0]);

	for (int i = 0; i < tess.numVertexes; i++, colors += 4)
	{
		float f = 1.0 - R_FogFactor(texCoords[i][0], texCoords[i][1]);
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
		colors[3] *= f;
	}
}

//==========================================================================
//
//	ComputeColors
//
//==========================================================================

void ComputeColors(shaderStage_t* pStage)
{
	//
	// rgbGen
	//
	switch (pStage->rgbGen)
	{
	case CGEN_IDENTITY:
		Com_Memset(tess.svars.colors, 0xff, tess.numVertexes * 4);
		break;

	default:
	case CGEN_IDENTITY_LIGHTING:
		Com_Memset(tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4);
		break;

	case CGEN_LIGHTING_DIFFUSE:
		RB_CalcDiffuseColor((byte*)tess.svars.colors);
		break;

	case CGEN_EXACT_VERTEX:
		Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
		break;

	case CGEN_CONST:
		for (int i = 0; i < tess.numVertexes; i++)
		{
			*(int*)tess.svars.colors[i] = *(int*)pStage->constantColor;
		}
		break;

	case CGEN_VERTEX:
		if (tr.identityLight == 1)
		{
			Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
		}
		else
		{
			for (int i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
				tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
				tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
		break;

	case CGEN_ONE_MINUS_VERTEX:
		if (tr.identityLight == 1)
		{
			for (int i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
				tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
				tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
			}
		}
		else
		{
			for (int i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][0] = (255 - tess.vertexColors[i][0]) * tr.identityLight;
				tess.svars.colors[i][1] = (255 - tess.vertexColors[i][1]) * tr.identityLight;
				tess.svars.colors[i][2] = (255 - tess.vertexColors[i][2]) * tr.identityLight;
			}
		}
		break;

	case CGEN_FOG:
		{
			mbrush46_fog_t* fog = tr.world->fogs + tess.fogNum;

			for (int i = 0; i < tess.numVertexes; i++)
			{
				*(int*)&tess.svars.colors[i] = fog->colorInt;
			}
		}
		break;

	case CGEN_WAVEFORM:
		RB_CalcWaveColor(&pStage->rgbWave, (byte*)tess.svars.colors);
		break;

	case CGEN_ENTITY:
		RB_CalcColorFromEntity((byte*)tess.svars.colors);
		break;

	case CGEN_ONE_MINUS_ENTITY:
		RB_CalcColorFromOneMinusEntity((byte*)tess.svars.colors);
		break;
	}

	//
	// alphaGen
	//
	switch (pStage->alphaGen)
	{
	case AGEN_SKIP:
		break;

	case AGEN_IDENTITY:
		if (pStage->rgbGen != CGEN_IDENTITY)
		{
			if ((pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1) ||
				pStage->rgbGen != CGEN_VERTEX)
			{
				for (int i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.colors[i][3] = 0xff;
				}
			}
		}
		break;

	case AGEN_CONST:
		if (pStage->rgbGen != CGEN_CONST)
		{
			for (int i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][3] = pStage->constantColor[3];
			}
		}
		break;

	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha(&pStage->alphaWave, (byte*)tess.svars.colors);
		break;

	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha((byte*)tess.svars.colors);
		break;

	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity(( byte*)tess.svars.colors);
		break;

	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity((byte*)tess.svars.colors);
		break;

    case AGEN_VERTEX:
		if (pStage->rgbGen != CGEN_VERTEX)
		{
			for (int i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][3] = tess.vertexColors[i][3];
			}
		}
		break;

	case AGEN_ONE_MINUS_VERTEX:
		for (int i = 0; i < tess.numVertexes; i++)
		{
			tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
		}
		break;

	case AGEN_PORTAL:
		for (int i = 0; i < tess.numVertexes; i++ )
		{
			vec3_t v;
			VectorSubtract(tess.xyz[i], backEnd.viewParms.orient.origin, v);
			float len = VectorLength(v);

			len /= tess.shader->portalRange;

			byte alpha;
			if (len < 0)
			{
				alpha = 0;
			}
			else if (len > 1)
			{
				alpha = 0xff;
			}
			else
			{
				alpha = len * 0xff;
			}

			tess.svars.colors[i][3] = alpha;
		}
		break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	if (tess.fogNum)
	{
		switch (pStage->adjustColorsForFog)
		{
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog((byte*)tess.svars.colors);
			break;

		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog((byte*)tess.svars.colors);
			break;

		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog((byte*)tess.svars.colors);
			break;

		case ACFF_NONE:
			break;
		}
	}
}

/*
====================================================================

TEX COORDS

====================================================================
*/

//==========================================================================
//
//	RB_CalcFogTexCoords
//
//	To do the clipped fog plane really correctly, we should use projected
// textures, but I don't trust the drivers and it doesn't fit our shader data.
//
//==========================================================================

void RB_CalcFogTexCoords(float* st)
{
	mbrush46_fog_t* fog = tr.world->fogs + tess.fogNum;

	// all fogging distance is based on world Z units
	vec3_t local;
	VectorSubtract(backEnd.orient.origin, backEnd.viewParms.orient.origin, local);
	vec4_t fogDistanceVector;
	fogDistanceVector[0] = -backEnd.orient.modelMatrix[2];
	fogDistanceVector[1] = -backEnd.orient.modelMatrix[6];
	fogDistanceVector[2] = -backEnd.orient.modelMatrix[10];
	fogDistanceVector[3] = DotProduct(local, backEnd.viewParms.orient.axis[0]);

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[0] *= fog->tcScale;
	fogDistanceVector[1] *= fog->tcScale;
	fogDistanceVector[2] *= fog->tcScale;
	fogDistanceVector[3] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	vec4_t fogDepthVector;
	float eyeT;
	if (fog->hasSurface)
	{
		fogDepthVector[0] = fog->surface[0] * backEnd.orient.axis[0][0] + 
			fog->surface[1] * backEnd.orient.axis[0][1] + fog->surface[2] * backEnd.orient.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.orient.axis[1][0] + 
			fog->surface[1] * backEnd.orient.axis[1][1] + fog->surface[2] * backEnd.orient.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.orient.axis[2][0] + 
			fog->surface[1] * backEnd.orient.axis[2][1] + fog->surface[2] * backEnd.orient.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct(backEnd.orient.origin, fog->surface);

		eyeT = DotProduct(backEnd.orient.viewOrigin, fogDepthVector) + fogDepthVector[3];
	}
	else
	{
		eyeT = 1;	// non-surface fog always has eye inside

		//JL Don't leave memory uninitialised.
		fogDepthVector[0] = 0;
		fogDepthVector[1] = 0;
		fogDepthVector[2] = 0;
		fogDepthVector[3] = 0;
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	bool eyeOutside;
	if (eyeT < 0)
	{
		eyeOutside = true;
	}
	else
	{
		eyeOutside = false;
	}

	fogDistanceVector[3] += 1.0 / 512;

	// calculate density for each point
	float* v = tess.xyz[0];
	for (int i = 0; i < tess.numVertexes; i++, v += 4)
	{
		// calculate the length in fog
		float s = DotProduct(v, fogDistanceVector) + fogDistanceVector[3];
		float t = DotProduct(v, fogDepthVector) + fogDepthVector[3];

		// partially clipped fogs use the T axis		
		if (eyeOutside)
		{
			if (t < 1.0)
			{
				t = 1.0 / 32;	// point is outside, so no fogging
			}
			else
			{
				t = 1.0 / 32 + 30.0 / 32 * t / (t - eyeT);	// cut the distance at the fog plane
			}
		}
		else
		{
			if (t < 0)
			{
				t = 1.0 / 32;	// point is outside, so no fogging
			}
			else
			{
				t = 31.0 / 32;
			}
		}

		st[0] = s;
		st[1] = t;
		st += 2;
	}
}

//==========================================================================
//
//	RB_CalcEnvironmentTexCoords
//
//==========================================================================

static void RB_CalcEnvironmentTexCoords(float* st) 
{
	float* v = tess.xyz[0];
	float* normal = tess.normal[0];

	for (int i = 0; i < tess.numVertexes; i++, v += 4, normal += 4, st += 2)
	{
		vec3_t viewer;
		VectorSubtract(backEnd.orient.viewOrigin, v, viewer);
		VectorNormalizeFast(viewer);

		float d = DotProduct(normal, viewer);

		vec3_t reflected;
		reflected[0] = normal[0] * 2 * d - viewer[0];
		reflected[1] = normal[1] * 2 * d - viewer[1];
		reflected[2] = normal[2] * 2 * d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

//==========================================================================
//
//	RB_CalcTurbulentTexCoords
//
//==========================================================================

static void RB_CalcTurbulentTexCoords(const waveForm_t* wf, float* st)
{
	float now = wf->phase + tess.shaderTime * wf->frequency;

	for (int i = 0; i < tess.numVertexes; i++, st += 2)
	{
		float s = st[0];
		float t = st[1];

		st[0] = s + tr.sinTable[((int)(((tess.xyz[i][0] + tess.xyz[i][2]) * 1.0 / 128 * 0.125 + now) * FUNCTABLE_SIZE)) & FUNCTABLE_MASK] * wf->amplitude;
		st[1] = t + tr.sinTable[((int)((tess.xyz[i][1] * 1.0 / 128 * 0.125 + now) * FUNCTABLE_SIZE)) & FUNCTABLE_MASK] * wf->amplitude;
	}
}

//==========================================================================
//
//	RB_CalcScrollTexCoords
//
//==========================================================================

static void RB_CalcScrollTexCoords(const float scrollSpeed[2], float* st)
{
	float timeScale = tess.shaderTime;

	float adjustedScrollS = scrollSpeed[0] * timeScale;
	float adjustedScrollT = scrollSpeed[1] * timeScale;

	// clamp so coordinates don't continuously get larger, causing problems
	// with hardware limits
	adjustedScrollS = adjustedScrollS - floor(adjustedScrollS);
	adjustedScrollT = adjustedScrollT - floor(adjustedScrollT);

	for (int i = 0; i < tess.numVertexes; i++, st += 2)
	{
		st[0] += adjustedScrollS;
		st[1] += adjustedScrollT;
	}
}

//==========================================================================
//
//	RB_CalcScaleTexCoords
//
//==========================================================================

static void RB_CalcScaleTexCoords(const float scale[2], float* st)
{
	for (int i = 0; i < tess.numVertexes; i++, st += 2)
	{
		st[0] *= scale[0];
		st[1] *= scale[1];
	}
}

//==========================================================================
//
//	RB_CalcTransformTexCoords
//
//==========================================================================

static void RB_CalcTransformTexCoords(const texModInfo_t* tmi, float* st)
{
	for (int i = 0; i < tess.numVertexes; i++, st += 2)
	{
		float s = st[0];
		float t = st[1];

		st[0] = s * tmi->matrix[0][0] + t * tmi->matrix[1][0] + tmi->translate[0];
		st[1] = s * tmi->matrix[0][1] + t * tmi->matrix[1][1] + tmi->translate[1];
	}
}

//==========================================================================
//
//	RB_CalcStretchTexCoords
//
//==========================================================================

static void RB_CalcStretchTexCoords(const waveForm_t* wf, float* st)
{
	float p = 1.0f / EvalWaveForm(wf);

	texModInfo_t tmi;
	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords(&tmi, st);
}

//==========================================================================
//
//	RB_CalcRotateTexCoords
//
//==========================================================================

static void RB_CalcRotateTexCoords(float degsPerSecond, float* st)
{
	float timeScale = tess.shaderTime;
	texModInfo_t tmi;

	float degs = -degsPerSecond * timeScale;
	int index = (int)(degs * (FUNCTABLE_SIZE / 360.0f));

	float sinValue = tr.sinTable[index & FUNCTABLE_MASK];
	float cosValue = tr.sinTable[(index + FUNCTABLE_SIZE / 4) & FUNCTABLE_MASK];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5 - 0.5 * cosValue + 0.5 * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5 - 0.5 * sinValue - 0.5 * cosValue;

	RB_CalcTransformTexCoords(&tmi, st);
}

//==========================================================================
//
//	ComputeTexCoords
//
//==========================================================================

void ComputeTexCoords(shaderStage_t* pStage)
{
	for (int b = 0; b < NUM_TEXTURE_BUNDLES; b++)
	{
		//
		// generate the texture coordinates
		//
		switch (pStage->bundle[b].tcGen)
		{
		case TCGEN_IDENTITY:
			Com_Memset(tess.svars.texcoords[b], 0, sizeof(float) * 2 * tess.numVertexes);
			break;

		case TCGEN_TEXTURE:
			for (int i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
			}
			break;

		case TCGEN_LIGHTMAP:
			for (int i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
				tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
			}
			break;

		case TCGEN_VECTOR:
			for (int i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.texcoords[b][i][0] = DotProduct(tess.xyz[i], pStage->bundle[b].tcGenVectors[0]);
				tess.svars.texcoords[b][i][1] = DotProduct(tess.xyz[i], pStage->bundle[b].tcGenVectors[1]);
			}
			break;

		case TCGEN_FOG:
			RB_CalcFogTexCoords((float*)tess.svars.texcoords[b]);
			break;

		case TCGEN_ENVIRONMENT_MAPPED:
			RB_CalcEnvironmentTexCoords((float*)tess.svars.texcoords[b]);
			break;

		case TCGEN_BAD:
			return;
		}

		//
		// alter texture coordinates
		//
		for (int tm = 0; tm < pStage->bundle[b].numTexMods ; tm++)
		{
			switch (pStage->bundle[b].texMods[tm].type)
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;		// break out of for loop
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords(&pStage->bundle[b].texMods[tm].wave, 
					(float*)tess.svars.texcoords[b]);
				break;

			case TMOD_ENTITY_TRANSLATE:
				RB_CalcScrollTexCoords(backEnd.currentEntity->e.shaderTexCoord,
					(float*)tess.svars.texcoords[b]);
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords(pStage->bundle[b].texMods[tm].scroll,
					(float*)tess.svars.texcoords[b]);
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords(pStage->bundle[b].texMods[tm].scale,
					(float*)tess.svars.texcoords[b]);
				break;
			
			case TMOD_STRETCH:
				RB_CalcStretchTexCoords(&pStage->bundle[b].texMods[tm].wave, 
					(float*)tess.svars.texcoords[b]);
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords(&pStage->bundle[b].texMods[tm],
					(float*)tess.svars.texcoords[b]);
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords(pStage->bundle[b].texMods[tm].rotateSpeed,
					(float*)tess.svars.texcoords[b]);
				break;

			default:
				throw QDropException(va("ERROR: unknown texmod '%d' in shader '%s'\n",
					pStage->bundle[b].texMods[tm].type, tess.shader->name));
				break;
			}
		}
	}
}
