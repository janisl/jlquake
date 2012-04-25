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

#define NOISE_SIZE          256
#define NOISE_MASK          (NOISE_SIZE - 1)

#define VAL(a)              s_noise_perm[(a) & (NOISE_MASK)]
#define INDEX(x, y, z, t)   VAL(x + VAL(y + VAL(z + VAL(t))))

#define LERP(a, b, w)       (a * (1.0f - w) + b * w)

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static float s_noise_table[NOISE_SIZE];
static int s_noise_perm[NOISE_SIZE];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	GetNoiseValue
//
//==========================================================================

static float GetNoiseValue(int x, int y, int z, int t)
{
	int index = INDEX(x, y, z, t);

	return s_noise_table[index];
}

//==========================================================================
//
//	R_NoiseInit
//
//==========================================================================

void R_NoiseInit()
{
	srand(1001);

	for (int i = 0; i < NOISE_SIZE; i++)
	{
		s_noise_table[i] = (float)(((rand() / (float)RAND_MAX) * 2.0 - 1.0));
		s_noise_perm[i] = (unsigned char)(rand() / (float)RAND_MAX * 255);
	}
}

//==========================================================================
//
//	R_NoiseGet4f
//
//==========================================================================

float R_NoiseGet4f(float x, float y, float z, float t)
{
	int ix = (int)floor(x);
	float fx = x - ix;
	int iy = (int)floor(y);
	float fy = y - iy;
	int iz = (int)floor(z);
	float fz = z - iz;
	int it = (int)floor(t);
	float ft = t - it;

	float value[2];
	for (int i = 0; i < 2; i++)
	{
		float front[4];
		front[0] = GetNoiseValue(ix, iy, iz, it + i);
		front[1] = GetNoiseValue(ix + 1, iy, iz, it + i);
		front[2] = GetNoiseValue(ix, iy + 1, iz, it + i);
		front[3] = GetNoiseValue(ix + 1, iy + 1, iz, it + i);

		float back[4];
		back[0] = GetNoiseValue(ix, iy, iz + 1, it + i);
		back[1] = GetNoiseValue(ix + 1, iy, iz + 1, it + i);
		back[2] = GetNoiseValue(ix, iy + 1, iz + 1, it + i);
		back[3] = GetNoiseValue(ix + 1, iy + 1, iz + 1, it + i);

		float fvalue = LERP(LERP(front[0], front[1], fx), LERP(front[2], front[3], fx), fy);
		float bvalue = LERP(LERP(back[0], back[1], fx), LERP(back[2], back[3], fx), fy);

		value[i] = LERP(fvalue, bvalue, fz);
	}

	float finalvalue = LERP(value[0], value[1], ft);

	return finalvalue;
}
