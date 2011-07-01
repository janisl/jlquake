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

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	TableForFunc
//
//==========================================================================

float* TableForFunc(genFunc_t func)
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

float EvalWaveForm(const waveForm_t* wf)
{
	float* table = TableForFunc(wf->func);

	return WAVEVALUE(table, wf->base, wf->amplitude, wf->phase, wf->frequency);
}

//==========================================================================
//
//	EvalWaveFormClamped
//
//==========================================================================

float EvalWaveFormClamped(const waveForm_t* wf)
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

void RB_CalcWaveColor(const waveForm_t* wf, byte* dstColors)
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

void RB_CalcColorFromEntity(byte* dstColors)
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

void RB_CalcColorFromOneMinusEntity(byte* dstColors)
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
