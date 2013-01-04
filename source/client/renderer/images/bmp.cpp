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
//**
//**	BMP LOADING
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../local.h"
#include "../../../common/endian.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct BMPHeader_t
{
	char id[2];
	quint32 fileSize;
	quint32 reserved0;
	quint32 bitmapDataOffset;
	quint32 bitmapHeaderSize;
	quint32 width;
	quint32 height;
	quint16 planes;
	quint16 bitsPerPixel;
	quint32 compression;
	quint32 bitmapDataSize;
	quint32 hRes;
	quint32 vRes;
	quint32 colors;
	quint32 importantColors;
	quint8 palette[256][4];
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_LoadBMP
//
//==========================================================================

void R_LoadBMP(const char* name, byte** pic, int* width, int* height)
{
	*pic = NULL;

	//
	// load the file
	//
	byte* buffer;
	int length = FS_ReadFile(name, (void**)&buffer);
	if (!buffer)
	{
		return;
	}

	byte* buf_p = buffer;

	BMPHeader_t bmpHeader;
	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.reserved0 = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.bitmapDataOffset = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.width = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.height = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.planes = LittleShort(*(qint16*)buf_p);
	buf_p += 2;
	bmpHeader.bitsPerPixel = LittleShort(*(qint16*)buf_p);
	buf_p += 2;
	bmpHeader.compression = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.bitmapDataSize = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.hRes = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.vRes = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.colors = LittleLong(*(qint32*)buf_p);
	buf_p += 4;
	bmpHeader.importantColors = LittleLong(*(qint32*)buf_p);
	buf_p += 4;

	Com_Memcpy(bmpHeader.palette, buf_p, sizeof(bmpHeader.palette));

	if (bmpHeader.bitsPerPixel == 8)
	{
		buf_p += 1024;
	}

	if (bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M')
	{
		common->Error("LoadBMP: only Windows-style BMP files supported (%s)\n", name);
	}
	if ((int)bmpHeader.fileSize != length)
	{
		common->Error("LoadBMP: header size does not match file size (%d vs. %d) (%s)\n", bmpHeader.fileSize, length, name);
	}
	if (bmpHeader.compression != 0)
	{
		common->Error("LoadBMP: only uncompressed BMP files supported (%s)\n", name);
	}
	if (bmpHeader.bitsPerPixel < 8)
	{
		common->Error("LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name);
	}

	int columns = bmpHeader.width;
	int rows = bmpHeader.height;
	if (rows < 0)
	{
		rows = -rows;
	}
	int numPixels = columns * rows;

	if (width)
	{
		*width = columns;
	}
	if (height)
	{
		*height = rows;
	}

	byte* bmpRGBA = new byte[numPixels * 4];
	*pic = bmpRGBA;

	for (int row = rows - 1; row >= 0; row--)
	{
		byte* pixbuf = bmpRGBA + row * columns * 4;

		for (int column = 0; column < columns; column++)
		{
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;

			switch (bmpHeader.bitsPerPixel)
			{
			case 8:
				palIndex = *buf_p++;
				*pixbuf++ = bmpHeader.palette[palIndex][2];
				*pixbuf++ = bmpHeader.palette[palIndex][1];
				*pixbuf++ = bmpHeader.palette[palIndex][0];
				*pixbuf++ = 0xff;
				break;

			case 16:
				shortPixel = *(unsigned short*)pixbuf;
				pixbuf += 2;
				*pixbuf++ = (shortPixel & (31 << 10)) >> 7;
				*pixbuf++ = (shortPixel & (31 << 5)) >> 2;
				*pixbuf++ = (shortPixel & (31)) << 3;
				*pixbuf++ = 0xff;
				break;

			case 24:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;

			case 32:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alpha = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				break;

			default:
				common->Error("LoadBMP: illegal pixel_size '%d' in file '%s'\n", bmpHeader.bitsPerPixel, name);
				break;
			}
		}
	}

	FS_FreeFile(buffer);
}
