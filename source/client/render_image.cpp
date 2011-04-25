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
//	R_FloodFillSkin
//
//	Fill background pixels so mipmapping doesn't have haloes
//
//==========================================================================

static void R_FloodFillSkin(byte* skin, int skinwidth, int skinheight)
{
	int filledcolor = 0;
	// attempt to find opaque black
	for (int i = 0; i < 256; ++i)
		if (r_palette[i][0] == 0 && r_palette[i][1] == 0 && r_palette[i][2] == 0 && r_palette[i][0] == 255)
		{
			filledcolor = i;
			break;
		}

	byte fillcolor = *skin; // assume this is the pixel to fill

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	enum
	{
		// must be a power of 2
		FLOODFILL_FIFO_SIZE = 0x1000,
		FLOODFILL_FIFO_MASK = (FLOODFILL_FIFO_SIZE - 1)
	};

	struct floodfill_t
	{
		short		x, y;
	};

	int inpt = 0;
	int outpt = 0;
	floodfill_t fifo[FLOODFILL_FIFO_SIZE];
	fifo[inpt].x = 0;
	fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int x = fifo[outpt].x;
		int y = fifo[outpt].y;
		int fdc = filledcolor;
		byte* pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx); \
		fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) \
	{ \
		fdc = pos[off]; \
	} \
}

		if (x > 0)
		{
			FLOODFILL_STEP(-1, -1, 0);
		}
		if (x < skinwidth - 1)
		{
			FLOODFILL_STEP(1, 1, 0);
		}
		if (y > 0)
		{
			FLOODFILL_STEP(-skinwidth, 0, -1);
		}
		if (y < skinheight - 1)
		{
			FLOODFILL_STEP(skinwidth, 0, 1);
		}
		skin[x + skinwidth * y] = fdc;

#undef FLOODFILL_STEP
	}
}

//==========================================================================
//
//	R_ConvertImage8To32
//
//==========================================================================

byte* R_ConvertImage8To32(byte* DataIn, int Width, int Height, int Mode)
{
	if (Mode >= IMG8MODE_Skin)
	{
		R_FloodFillSkin(DataIn, Width, Height);
	}

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
		if (p == 255 || (p == 0 && (Mode == IMG8MODE_Holey || Mode == IMG8MODE_SkinHoley || Mode == IMG8MODE_SkinTransparent)))
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
	case IMG8MODE_SkinTransparent:
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
	case IMG8MODE_SkinHoley:
		for (int i = 0; i < Count; i++)
		{
			int p = DataIn[i];
			if (p == 0)
			{
				DataOut[i * 4 + 3] = 0;
			}
		}
		break;

	case IMG8MODE_SkinSpecialTrans:
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
//	R_LoadImage
//
//	Loads any of the supported image types into a cannonical 32 bit format.
//
//==========================================================================

void R_LoadImage(const char* name, byte** pic, int* width, int* height, int Mode, byte* TransPixels)
{
	*pic = NULL;
	*width = 0;
	*height = 0;

	int len = QStr::Length(name);
	if (len < 5)
	{
		return;
	}

	if (!QStr::ICmp(name + len - 4, ".tga"))
	{
		R_LoadTGA(name, pic, width, height);            // try tga first
		if (!*pic)
		{                                    //
			char altname[MAX_QPATH];                      // try jpg in place of tga 
			QStr::Cpy(altname, name);                      
			len = QStr::Length(altname);                  
			altname[len - 3] = 'j';
			altname[len - 2] = 'p';
			altname[len - 1] = 'g';
			R_LoadJPG(altname, pic, width, height);
		}
	}
	else if (!QStr::ICmp(name + len - 4, ".pcx"))
	{
		R_LoadPCX32(name, pic, width, height, Mode);
	}
	else if (!QStr::ICmp(name + len - 4, ".bmp"))
	{
		R_LoadBMP(name, pic, width, height);
	}
	else if (!QStr::ICmp(name + len - 4, ".jpg"))
	{
		R_LoadJPG(name, pic, width, height);
	}
	else if (!QStr::ICmp(name + len - 4, ".wal"))
	{
		R_LoadWAL(name, pic, width, height);
	}
	else if (!QStr::ICmp(name + len - 4, ".lmp"))
	{
		R_LoadPIC(name, pic, width, height, TransPixels, Mode);
	}
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
