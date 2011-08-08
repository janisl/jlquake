//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	TARGA LOADING
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct TargaHeader
{
	quint8		id_length;
	quint8		colormap_type;
	quint8		image_type;
	quint16		colormap_index;
	quint16		colormap_length;
	quint8		colormap_size;
	quint16		x_origin;
	quint16		y_origin;
	quint16		width;
	quint16		height;
	quint8		pixel_size;
	quint8		attributes;
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
//	R_LoadTGA
//
//==========================================================================

void R_LoadTGA(const char* name, byte** pic, int* width, int* height)
{
	*pic = NULL;

	//
	// load the file
	//
	byte* buffer;
	FS_ReadFile(name, (void **)&buffer);
	if (!buffer)
	{
		return;
	}

	byte* buf_p = buffer;

	TargaHeader targa_header;
	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	targa_header.colormap_index = LittleShort(*(qint16*)buf_p);
	buf_p += 2;
	targa_header.colormap_length = LittleShort(*(qint16*)buf_p);
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort(*(qint16*)buf_p);
	buf_p += 2;
	targa_header.y_origin = LittleShort(*(qint16*)buf_p);
	buf_p += 2;
	targa_header.width = LittleShort(*(qint16*)buf_p);
	buf_p += 2;
	targa_header.height = LittleShort(*(qint16*)buf_p);
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type != 2 && targa_header.image_type != 10 && targa_header.image_type != 3)
	{
		throw QDropException("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n");
	}

	if (targa_header.colormap_type != 0)
	{
		throw QDropException("LoadTGA: colormaps not supported\n");
	}

	if (targa_header.pixel_size != 32 && targa_header.pixel_size != 24 && targa_header.image_type != 3)
	{
		throw QDropException("LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
	}

	int columns = targa_header.width;
	int rows = targa_header.height;
	int numPixels = columns * rows;

	if (width)
	{
		*width = columns;
	}
	if (height)
	{
		*height = rows;
	}

	byte* targa_rgba = new byte[numPixels * 4];
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
	{
		buf_p += targa_header.id_length;  // skip TARGA image comment
	}
	
	if (targa_header.image_type==2 || targa_header.image_type == 3)
	{ 
		// Uncompressed RGB or gray scale image
		for (int row = rows - 1; row >= 0; row--) 
		{
			byte* pixbuf = targa_rgba + row * columns * 4;
			for (int column = 0; column < columns; column++) 
			{
				byte red, green, blue, alphabyte;
				switch (targa_header.pixel_size) 
				{
				case 8:
					blue = *buf_p++;
					green = blue;
					red = blue;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
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
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;

				default:
					throw QDropException(va("LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name));
				}
			}
		}
	}
	else if (targa_header.image_type == 10)
	{
		// Runlength encoded RGB images
		byte red = 0;
		byte green = 0;
		byte blue = 0;
		byte alphabyte = 0xff;

		for (int row = rows - 1; row >= 0; row--)
		{
			byte* pixbuf = targa_rgba + row * columns * 4;
			for (int column = 0; column < columns;)
			{
				byte packetHeader = *buf_p++;
				byte packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80)
				{
					// run-length packet
					switch (targa_header.pixel_size)
					{
					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = 255;
						break;

					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						break;

					default:
						throw QDropException(va("LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name));
					}
	
					for (int j = 0; j < packetSize; j++)
					{
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if (column == columns)
						{
							// run spans across rows
							column = 0;
							if (row > 0)
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else
				{
					// non run-length packet
					for (int j = 0; j < packetSize; j++)
					{
						switch (targa_header.pixel_size)
						{
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
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;

						default:
							throw QDropException(va("LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name));
						}
						column++;
						if (column == columns)
						{
							// pixel packet run spans across rows
							column = 0;
							if (row > 0)
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
			breakOut:;
		}
	}

#if 0 
  // TTimo: this is the chunk of code to ensure a behavior that meets TGA specs 
  // bk0101024 - fix from Leonardo
  // bit 5 set => top-down
  if (targa_header.attributes & 0x20) {
    unsigned char *flip = (unsigned char*)malloc (columns*4);
    unsigned char *src, *dst;

    for (row = 0; row < rows/2; row++) {
      src = targa_rgba + row * 4 * columns;
      dst = targa_rgba + (rows - row - 1) * 4 * columns;

      Com_Memcpy(flip, src, columns*4);
      Com_Memcpy(src, dst, columns*4);
      Com_Memcpy(dst, flip, columns*4);
    }
    free (flip);
  }
#endif
	// instead we just print a warning
	if (targa_header.attributes & 0x20)
	{
		Log::write(S_COLOR_YELLOW "WARNING: '%s' TGA file header declares top-down image, ignoring\n", name);
	}

	FS_FreeFile(buffer);
}

//==========================================================================
//
//	R_SaveTGA
//
//==========================================================================

void R_SaveTGA(const char* FileName, byte* Data, int Width, int Height, bool HaveAlpha)
{
	int Bpp = HaveAlpha ? 4 : 3;
	int Length = 18 + Width * Height * Bpp;
	byte* Buffer = new byte[Length];

	Com_Memset(Buffer, 0, 18);
	Buffer[2] = 2;			// uncompressed type
	Buffer[12] = Width & 255;
	Buffer[13] = Width >> 8;
	Buffer[14] = Height & 255;
	Buffer[15] = Height >> 8;
	Buffer[16] = 8 * Bpp;	// pixel size

	// swap rgb to bgr
	for (int i = 18; i < Length; i += Bpp)
	{
		Buffer[i] = Data[i - 18 + 2];			// blue
		Buffer[i + 1] = Data[i - 18 + 1];		// green
		Buffer[i + 2] = Data[i - 18 + 0];		// red
		if (HaveAlpha)
		{
			Buffer[i + 3] = Data[i - 18 + 3];	// alpha
		}
	}

	FS_WriteFile(FileName, Buffer, Length);

	delete[] Buffer;
}
