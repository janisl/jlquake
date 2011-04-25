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
//**
//**	PCX LOADING
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

#pragma pack(push, 1)

struct pcx_t
{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bits_per_pixel;
	quint16	xmin,ymin,xmax,ymax;
	quint16	hres,vres;
	quint8	palette[48];
	char	reserved;
	char	color_planes;
	quint16	bytes_per_line;
	quint16	palette_type;
	char	filler[58];
	quint8	data;			// unbounded
};

#pragma pack(pop)

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_LoadPCX
//
//==========================================================================

void R_LoadPCX(const char* filename, byte** pic, byte** palette, int* width, int* height)
{
	*pic = NULL;
	if (palette)
	{
		*palette = NULL;
	}

	//
	// load the file
	//
	byte* raw;
	int len = FS_ReadFile(filename, (void**)&raw);
	if (!raw)
	{
		return;
	}

	//
	// parse the PCX file
	//
	pcx_t* pcx = (pcx_t*)raw;
	raw = &pcx->data;

	int xmax = LittleShort(pcx->xmax);
	int ymax = LittleShort(pcx->ymax);

	int MaxWidth;
	int MaxHeight;
	if (GGameType & GAME_Quake3)
	{
		MaxWidth = 1024;
		MaxHeight = 1024;
	}
	else if (GGameType & GAME_Quake3)
	{
		MaxWidth = 640;
		MaxHeight = 480;
	}
	else
	{
		MaxWidth = 320;
		MaxHeight = 1024;
	}

	if (pcx->manufacturer != 0x0a ||
		pcx->version != 5 ||
		pcx->encoding != 1 ||
		pcx->bits_per_pixel != 8 ||
		xmax >= MaxWidth ||
		ymax >= MaxHeight)
	{
		GLog.Write("Bad pcx file %s (%i x %i) (%i x %i)\n", filename, xmax + 1, ymax + 1, pcx->xmax, pcx->ymax);
		return;
	}

	byte* out = new byte[(ymax + 1) * (xmax + 1)];

	*pic = out;

	if (palette)
	{
		*palette = new byte[768];
		Com_Memcpy(*palette, (byte*)pcx + len - 768, 768);
	}

	if (width)
	{
		*width = xmax + 1;
	}
	if (height)
	{
		*height = ymax + 1;
	}

	byte* pix = out;
	for (int y = 0; y <= ymax; y++, pix += xmax + 1)
	{
		for (int x = 0; x <= xmax; )
		{
			int dataByte = *raw++;

			int runLength;
			if ((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
			{
				runLength = 1;
			}

			while (runLength-- > 0 && x <= xmax)
			{
				pix[x++] = dataByte;
			}
		}
	}

	if (raw - (byte*)pcx > len)
	{
		GLog.DWrite("PCX file %s was malformed", filename);
		Mem_Free(*pic);
		*pic = NULL;
	}

	FS_FreeFile(pcx);
}

//==========================================================================
//
//	R_LoadPCX32
//
//==========================================================================

void R_LoadPCX32(const char* filename, byte** pic, int* width, int* height, int Mode)
{
	byte* palette;
	byte* pic8;
	R_LoadPCX(filename, &pic8, &palette, width, height);
	if (!pic8)
	{
		*pic = NULL;
		return;
	}

	if (GGameType & GAME_Quake3)
	{
		int c = (*width) * (*height);
		byte* pic32 = *pic = new byte[4 * c];
		for (int i = 0 ; i < c ; i++)
		{
			int p = pic8[i];
			pic32[0] = palette[p * 3];
			pic32[1] = palette[p * 3 + 1];
			pic32[2] = palette[p * 3 + 2];
			pic32[3] = 255;
			pic32 += 4;
		}
	}
	else
	{
		//	In Quake 2 palette from PCX file is ignored.
		*pic = R_ConvertImage8To32(pic8, *width, *height, Mode);
	}

	delete[] pic8;
	delete[] palette;
}

//==========================================================================
//
//	R_SavePCXMem
//
//==========================================================================

void R_SavePCXMem(QArray<byte>& buffer, byte* data, int width, int height, byte* palette)
{
	buffer.SetNum(width * height * 2 + 1000);
	pcx_t* pcx = (pcx_t*)buffer.Ptr();
 
	pcx->manufacturer = 0x0a;	// PCX id
	pcx->version = 5;			// 256 color
 	pcx->encoding = 1;		// uncompressed
	pcx->bits_per_pixel = 8;		// 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = LittleShort((short)(width - 1));
	pcx->ymax = LittleShort((short)(height - 1));
	pcx->hres = LittleShort((short)width);
	pcx->vres = LittleShort((short)height);
	Com_Memset(pcx->palette, 0, sizeof(pcx->palette));
	pcx->color_planes = 1;		// chunky image
	pcx->bytes_per_line = LittleShort((short)width);
	pcx->palette_type = LittleShort(2);		// not a grey scale
	Com_Memset(pcx->filler, 0, sizeof(pcx->filler));

	// pack the image
	byte* pack = &pcx->data;

	data += width * (height - 1);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			if ((*data & 0xc0) != 0xc0)
			{
				*pack++ = *data++;
			}
			else
			{
				*pack++ = 0xc1;
				*pack++ = *data++;
			}
		}

		data -= width * 2;
	}
			
	// write the palette
	*pack++ = 0x0c;	// palette ID byte
	for (int i = 0; i < 768; i++)
	{
		*pack++ = *palette++;
	}
		
	buffer.SetNum(pack - (byte*)pcx);
}
