//**************************************************************************
//**
//**	$Id$
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

byte		host_basepal[768];
byte		r_palette[256][4];
unsigned*	d_8to24table;

int ColorIndex[16] =
{
	0, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 199, 207, 223, 231
};

unsigned ColorPercent[16] =
{
	25, 51, 76, 102, 114, 127, 140, 153, 165, 178, 191, 204, 216, 229, 237, 247
};

static int			scrap_allocated[SCRAP_BLOCK_WIDTH];
byte		scrap_texels[SCRAP_BLOCK_WIDTH * SCRAP_BLOCK_HEIGHT * 4];
bool		scrap_dirty;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_SetPalette
//
//==========================================================================

static void R_SetPalette(byte* pal)
{
	//
	// 8 8 8 encoding
	//
	d_8to24table = (unsigned*)r_palette;
	for (int i = 0; i < 256; i++)
	{
		r_palette[i][0] = pal[0];
		r_palette[i][1] = pal[1];
		r_palette[i][2] = pal[2];
		r_palette[i][3] = 255;
		pal += 3;
	}
	r_palette[255][3] = 0;	// 255 is transparent
}

//==========================================================================
//
//	R_InitQ1Palette
//
//==========================================================================

void R_InitQ1Palette()
{
	QArray<byte> Pal;
	if (FS_ReadFile("gfx/palette.lmp", Pal) <= 0)
	{
		throw QException("Couldn't load gfx/palette.lmp");
	}
	R_SetPalette(Pal.Ptr());
	Com_Memcpy(host_basepal, Pal.Ptr(), 768);
}

//==========================================================================
//
//	R_InitQ2Palette
//
//==========================================================================

void R_InitQ2Palette()
{
	// get the palette

	byte* pic;
	byte* pal;
	int width;
	int height;
	R_LoadPCX("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
	{
		throw QException("Couldn't load pics/colormap.pcx");
	}

	R_SetPalette(pal);

	delete[] pic;
	delete[] pal;
}

//==========================================================================
//
//	R_ConvertImage8To32
//
//==========================================================================

byte* R_ConvertImage8To32(byte* DataIn, int Width, int Height, int Mode)
{
	byte* DataOut = new byte[Width * Height * 4];

	int Count = Width * Height;
	for (int i = 0; i < Count; i++)
	{
		int p = DataIn[i];
		DataOut[i * 4 + 0] = r_palette[p][0];
		DataOut[i * 4 + 1] = r_palette[p][1];
		DataOut[i * 4 + 2] = r_palette[p][2];
		DataOut[i * 4 + 3] = r_palette[p][3];
	}

	//	For transparent pixels scan around for another color to avoid alpha fringes
	for (int i = 0; i < Count; i++)
	{
		int p = DataIn[i];
		if (p == 255)
		{
			int u = i % Width;
			int v = i / Width;

			byte neighbors[9][3];
			int num_neighbors_valid = 0;
			for (int neighbor_u = u - 1; neighbor_u <= u + 1; neighbor_u++)
			{
				for (int neighbor_v = v - 1; neighbor_v <= v + 1; neighbor_v++)
				{
					if (neighbor_u == neighbor_v)
					{
						continue;
					}
					// Make sure  that we are accessing a texel in the image, not out of range.
					if (neighbor_u < 0 || neighbor_u > Width || neighbor_v < 0 || neighbor_v > Height)
					{
						continue;
					}
					if (DataIn[neighbor_u + neighbor_v * Width] == 255)
					{
						continue;
					}
					neighbors[num_neighbors_valid][0] = DataOut[(neighbor_u + neighbor_v * Width) * 4 + 0];
					neighbors[num_neighbors_valid][1] = DataOut[(neighbor_u + neighbor_v * Width) * 4 + 1];
					neighbors[num_neighbors_valid][2] = DataOut[(neighbor_u + neighbor_v * Width) * 4 + 2];
					num_neighbors_valid++;
				}
			}

			if (num_neighbors_valid == 0)
			{
				continue;
			}

			int r = 0, g = 0, b = 0;
			for (int n = 0; n < num_neighbors_valid; n++)
			{
				r += neighbors[n][0];
				g += neighbors[n][1];
				b += neighbors[n][2];
			}

			r /= num_neighbors_valid;
			g /= num_neighbors_valid;
			b /= num_neighbors_valid;

			if (r > 255)
			{
				r = 255;
			}
			if (g > 255)
			{
				g = 255;
			}
			if (b > 255)
			{
				b = 255;
			}

			DataOut[i * 4 + 0] = r;
			DataOut[i * 4 + 1] = g;
			DataOut[i * 4 + 2] = b;
		}
	}

	switch (Mode)
	{
	case IMG8MODE_Transparent:
		for (int i = 0; i < Count; i++)
		{
			int p = DataIn[i];
			if (p == 0)
			{
				DataOut[i * 4 + 3] = 0;
			}
			else if (p & 1)
			{
				DataOut[i * 4 + 3] = (int)(255 * r_wateralpha->value);
			}
			else
			{
				DataOut[i * 4 + 3] = 255;
			}
		}
		break;

	case IMG8MODE_Holey:
		for (int i = 0; i < Count; i++)
		{
			int p = DataIn[i];
			if (p == 0)
			{
				DataOut[i * 4 + 3] = 0;
			}
		}
		break;

	case IMG8MODE_SpecialTrans:
		for (int i = 0; i < Count; i++)
		{
			int p = DataIn[i];
			DataOut[i * 4 + 0] = r_palette[ColorIndex[p >> 4]][0];
			DataOut[i * 4 + 1] = r_palette[ColorIndex[p >> 4]][1];
			DataOut[i * 4 + 2] = r_palette[ColorIndex[p >> 4]][2];
			DataOut[i * 4 + 3] = ColorPercent[p & 15];
		}
		break;
	}

	return DataOut;
}

//==========================================================================
//
//	R_ScrapAllocBlock
//
//	scrap allocation
//
//	Allocate all the little status bar obejcts into a single texture
// to crutch up stupid hardware / drivers
//
//==========================================================================

bool R_ScrapAllocBlock(int w, int h, int* x, int* y)
{
	int best = SCRAP_BLOCK_HEIGHT;

	for (int i = 0; i < SCRAP_BLOCK_WIDTH - w; i++)
	{
		int best2 = 0;

		int j;
		for (j = 0; j < w; j++)
		{
			if (scrap_allocated[i + j] >= best)
			{
				break;
			}
			if (scrap_allocated[i + j] > best2)
			{
				best2 = scrap_allocated[i + j];
			}
		}
		if (j == w)
		{
			// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > SCRAP_BLOCK_HEIGHT)
	{
		return false;
	}

	for (int i = 0; i < w; i++)
	{
		scrap_allocated[*x + i] = best + h;
	}

	return true;
}
