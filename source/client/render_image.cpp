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

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

image_t*		ImageHashTable[FILE_HASH_SIZE];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte			s_gammatable[256];
static byte			s_intensitytable[256];

static byte mipBlendColors[16][4] =
{
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};

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
		if (r_palette[i][0] == 0 && r_palette[i][1] == 0 && r_palette[i][2] == 0 && r_palette[i][3] == 255)
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
//	R_ResampleTexture
//
//	Used to resample images in a more general than quartering fashion.
//
//	This will only be filtered properly if the resampled size is greater
// than half the original size.
//
//	If a larger shrinking is needed, use the mipmap function before or after.
//
//==========================================================================

static void R_ResampleTexture(byte* in, int inwidth, int inheight, byte* out, int outwidth, int outheight)
{
	if (outwidth > 2048)
	{
		throw QDropException("ResampleTexture: max width");
	}
	if (outheight > 2048)
	{
		throw QDropException("ResampleTexture: max height");
	}

	unsigned fracstep = inwidth * 0x10000 / outwidth;

	unsigned p1[2048];
	unsigned frac = fracstep >> 2;
	for (int i = 0; i < outwidth; i++)
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}
	unsigned p2[2048];
	frac = 3 * (fracstep >> 2);
	for (int i = 0; i < outwidth; i++)
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for (int i = 0; i < outheight; i++)
	{
		byte* inrow = in + inwidth * (int)((i + 0.25) * inheight / outheight) * 4;
		byte* inrow2 = in + inwidth * (int)((i + 0.75) * inheight / outheight) * 4;
		for (int j = 0; j < outwidth; j++)
		{
			byte* pix1 = inrow + p1[j];
			byte* pix2 = inrow + p2[j];
			byte* pix3 = inrow2 + p1[j];
			byte* pix4 = inrow2 + p2[j];
			*out++ = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			*out++ = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			*out++ = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			*out++ = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}
}

//==========================================================================
//
//	R_MipMap2
//
//	Operates in place, quartering the size of the texture
//	Proper linear filter
//
//==========================================================================

static void R_MipMap2(byte* in, int inWidth, int inHeight)
{
	int outWidth = inWidth >> 1;
	int outHeight = inHeight >> 1;
	QArray<byte> temp;
	temp.SetNum(outWidth * outHeight * 4);

	int inWidthMask = inWidth - 1;
	int inHeightMask = inHeight - 1;

	for (int i = 0; i < outHeight; i++)
	{
		for (int j = 0; j < outWidth; j++)
		{
			byte* outpix = &temp[(i * outWidth + j) * 4];
			for (int k = 0; k < 4 ; k++)
			{
				int total = 
					1 * in[(((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)) * 4 + k] +
					2 * in[(((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)) * 4 + k] +
					2 * in[(((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)) * 4 + k] +
					1 * in[(((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)) * 4 + k] +

					2 * in[(((i * 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)) * 4 + k] +
					4 * in[(((i * 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)) * 4 + k] +
					4 * in[(((i * 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)) * 4 + k] +
					2 * in[(((i * 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)) * 4 + k] +

					2 * in[(((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)) * 4 + k] +
					4 * in[(((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)) * 4 + k] +
					4 * in[(((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)) * 4 + k] +
					2 * in[(((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)) * 4 + k] +

					1 * in[(((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)) * 4 + k] +
					2 * in[(((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)) * 4 + k] +
					2 * in[(((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)) * 4 + k] +
					1 * in[(((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)) * 4 + k];
				outpix[k] = total / 36;
			}
		}
	}

	Com_Memcpy(in, temp.Ptr(), outWidth * outHeight * 4);
}

//==========================================================================
//
//	R_MipMap
//
//	Operates in place, quartering the size of the texture
//
//==========================================================================

static void R_MipMap(byte* in, int width, int height)
{
	if (!r_simpleMipMaps->integer)
	{
		R_MipMap2(in, width, height);
		return;
	}

	if (width == 1 && height == 1)
	{
		return;
	}

	int row = width * 4;
	byte* out = in;
	width >>= 1;
	height >>= 1;

	if (width == 0 || height == 0)
	{
		width += height;	// get largest
		for (int i = 0; i < width; i++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4]) >> 1;
			out[1] = (in[1] + in[5]) >> 1;
			out[2] = (in[2] + in[6]) >> 1;
			out[3] = (in[3] + in[7]) >> 1;
		}
		return;
	}

	for (int i = 0; i < height; i++, in += row)
	{
		for (int j = 0; j < width; j++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4] + in[row+0] + in[row+4]) >> 2;
			out[1] = (in[1] + in[5] + in[row+1] + in[row+5]) >> 2;
			out[2] = (in[2] + in[6] + in[row+2] + in[row+6]) >> 2;
			out[3] = (in[3] + in[7] + in[row+3] + in[row+7]) >> 2;
		}
	}
}

//==========================================================================
//
//	R_LightScaleTexture
//
//	Scale up the pixel values in a texture to increase the lighting range
//
//==========================================================================

static void R_LightScaleTexture(byte* in, int inwidth, int inheight, qboolean only_gamma)
{
	if (only_gamma)
	{
		if (!glConfig.deviceSupportsGamma)
		{
			byte* p = in;
			int c = inwidth*inheight;
			for (int i = 0; i < c; i++, p += 4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		byte* p = in;

		int c = inwidth * inheight;

		if (glConfig.deviceSupportsGamma)
		{
			for (int i = 0; i < c; i++, p += 4)
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for (int i = 0; i < c; i++, p += 4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}

//==========================================================================
//
//	R_BlendOverTexture
//
//	Apply a color blend over a set of pixels
//
//==========================================================================

static void R_BlendOverTexture(byte* data, int pixelCount, byte blend[4])
{
	int inverseAlpha = 255 - blend[3];
	int premult[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for (int i = 0; i < pixelCount; i++, data += 4)
	{
		data[0] = (data[0] * inverseAlpha + premult[0]) >> 9;
		data[1] = (data[1] * inverseAlpha + premult[1]) >> 9;
		data[2] = (data[2] * inverseAlpha + premult[2]) >> 9;
	}
}

//==========================================================================
//
//	GL_CheckErrors
//
//==========================================================================

static void GL_CheckErrors()
{
	int err = qglGetError();
	if (err == GL_NO_ERROR)
	{
		return;
	}
	if (r_ignoreGLErrors->integer)
	{
		return;
	}
	char s[64];
	switch (err)
	{
		case GL_INVALID_ENUM:
			QStr::Cpy(s, "GL_INVALID_ENUM");
			break;
		case GL_INVALID_VALUE:
			QStr::Cpy(s, "GL_INVALID_VALUE");
			break;
		case GL_INVALID_OPERATION:
			QStr::Cpy(s, "GL_INVALID_OPERATION");
			break;
		case GL_STACK_OVERFLOW:
			QStr::Cpy(s, "GL_STACK_OVERFLOW");
			break;
		case GL_STACK_UNDERFLOW:
			QStr::Cpy(s, "GL_STACK_UNDERFLOW");
			break;
		case GL_OUT_OF_MEMORY:
			QStr::Cpy(s, "GL_OUT_OF_MEMORY");
			break;
		default:
			QStr::Sprintf(s, sizeof(s), "%i", err);
			break;
	}

	throw QException(va("GL_CheckErrors: %s", s));
}

//==========================================================================
//
//	R_UploadImage
//
//==========================================================================

void R_UploadImage(byte* data, int width, int height, bool mipmap, bool picmip, bool lightMap, int* format, int* pUploadWidth, int* pUploadHeight)
{
	//
	// convert to exact power of 2 sizes
	//
	int scaled_width = 1;
	while (scaled_width < width)
	{
		scaled_width <<= 1;
	}
	int scaled_height = 1;
	while (scaled_height < height)
	{
		scaled_height <<= 1;
	}
	if (r_roundImagesDown->integer && scaled_width > width)
	{
		scaled_width >>= 1;
	}
	if (r_roundImagesDown->integer && scaled_height > height)
	{
		scaled_height >>= 1;
	}

	byte* resampledBuffer = NULL;
	if (scaled_width != width || scaled_height != height)
	{
		resampledBuffer = new byte[scaled_width * scaled_height * 4];
		R_ResampleTexture(data, width, height, resampledBuffer, scaled_width, scaled_height);
		data = resampledBuffer;
		width = scaled_width;
		height = scaled_height;
	}

	//
	// perform optional picmip operation
	//
	if (picmip)
	{
		scaled_width >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
	}

	//
	// clamp to minimum size
	//
	if (scaled_width < 1)
	{
		scaled_width = 1;
	}
	if (scaled_height < 1)
	{
		scaled_height = 1;
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while (scaled_width > glConfig.maxTextureSize || scaled_height > glConfig.maxTextureSize)
	{
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	GLenum internalFormat;
	if (!lightMap)
	{
		// scan the texture and verify if the alpha channel is being used or not
		int c = width * height;
		int samples = 3;
		for (int i = 0; i < c; i++)
		{
			if (data[i * 4 + 3] != 255)
			{
				samples = 4;
				break;
			}
		}
		// select proper internal format
		if (samples == 3)
		{
			if (glConfig.textureCompression == TC_S3TC)
			{
				internalFormat = GL_RGB4_S3TC;
			}
			else if (r_texturebits->integer == 16)
			{
				internalFormat = GL_RGB5;
			}
			else if (r_texturebits->integer == 32)
			{
				internalFormat = GL_RGB8;
			}
			else
			{
				internalFormat = GL_RGB;
			}
		}
		else
		{
			if (r_texturebits->integer == 16)
			{
				internalFormat = GL_RGBA4;
			}
			else if (r_texturebits->integer == 32)
			{
				internalFormat = GL_RGBA8;
			}
			else
			{
				internalFormat = GL_RGBA;
			}
		}
	}
	else
	{
		internalFormat = GL_RGB;
	}

	byte* scaledBuffer;

	//	Copy or resample data as appropriate for first MIP level
	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			qglTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}
	}
	else
	{
		// use the normal mip-mapping function to go down from here
		while (width > scaled_width || height > scaled_height)
		{
			R_MipMap(data, width, height);
			width >>= 1;
			height >>= 1;
			if (width < 1)
			{
				width = 1;
			}
			if (height < 1)
			{
				height = 1;
			}
		}
	}

	scaledBuffer = new byte[4 * scaled_width * scaled_height];

	Com_Memcpy(scaledBuffer, data, width * height * 4);

	R_LightScaleTexture(scaledBuffer, scaled_width, scaled_height, !mipmap);

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	qglTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);

	if (mipmap)
	{
		int miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			R_MipMap(scaledBuffer, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
			{
				scaled_width = 1;
			}
			if (scaled_height < 1)
			{
				scaled_height = 1;
			}
			miplevel++;

			if (r_colorMipLevels->integer)
			{
				R_BlendOverTexture(scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel]);
			}

			qglTexImage2D(GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
		}
	}

	delete[] scaledBuffer;
done:

	if (resampledBuffer)
	{
		delete[] resampledBuffer;
	}

	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	GL_CheckErrors();
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

static bool R_ScrapAllocBlock(int w, int h, int* x, int* y)
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

//==========================================================================
//
//	generateHashValue
//
//	return a hash value for the filename
//
//==========================================================================

static long generateHashValue(const char* fname)
{
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0')
	{
		letter = QStr::ToLower(fname[i]);
		if (letter == '.')
		{
			break;				// don't include extension
		}
		if (letter =='\\')
		{
			letter = '/';		// damn path names
		}
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash &= (FILE_HASH_SIZE - 1);
	return hash;
}

//==========================================================================
//
//	R_CreateImage
//
//	This is the only way any image_t is created
//
//==========================================================================

image_t* R_CreateImage(const char* name, byte* data, int width, int height, bool mipmap, bool allowPicmip, GLenum glWrapClampMode, bool AllowScrap)
{
	if (QStr::Length(name) >= MAX_QPATH)
	{
		throw QDropException(va("R_CreateImage: \"%s\" is too long\n", name));
	}
	if (tr.numImages == MAX_DRAWIMAGES)
	{
		throw QDropException("R_CreateImage: MAX_DRAWIMAGES hit\n");
	}

	bool isLightmap = !QStr::NCmp(name, "*lightmap", 9);

	image_t* image = new image_t;
	Com_Memset(image, 0, sizeof(image_t));
	tr.images[tr.numImages] = image;
	tr.numImages++;

	image->mipmap = mipmap;
	image->allowPicmip = allowPicmip;

	QStr::Cpy(image->imgName, name);

	image->width = width;
	image->height = height;
	image->wrapClampMode = glWrapClampMode;

	// load little ones into the scrap
	if (AllowScrap && width < 64 && height < 64)
	{
		int x, y;
		if (!R_ScrapAllocBlock(width, height, &x, &y))
		{
			goto nonscrap;
		}

		// copy the texels into the scrap block
		int k = 0;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width * 4; j++, k++)
			{
				scrap_texels[(y + i) * SCRAP_BLOCK_WIDTH * 4 + x * 4 + j] = data[k];
			}
		}
		scrap_dirty = true;

		image->texnum = tr.scrapImage->texnum;
		image->scrap = true;
		image->sl = (x + 0.01) / (float)SCRAP_BLOCK_WIDTH;
		image->sh = (x + width - 0.01) / (float)SCRAP_BLOCK_WIDTH;
		image->tl = (y + 0.01) / (float)SCRAP_BLOCK_WIDTH;
		image->th = (y + height - 0.01) / (float)SCRAP_BLOCK_WIDTH;
	}
	else
	{
nonscrap:
		qglGenTextures(1, &image->texnum);

		GL_Bind(image);

		R_UploadImage(data, width, height, mipmap, allowPicmip, isLightmap, &image->internalFormat, &image->uploadWidth, &image->uploadHeight);

		image->scrap = false;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode);

		qglBindTexture(GL_TEXTURE_2D, 0);
		glState.currenttextures[glState.currenttmu] = 0;
	}

	long hash = generateHashValue(name);
	image->next = ImageHashTable[hash];
	ImageHashTable[hash] = image;

	return image;
}

//==========================================================================
//
//	R_FindImage
//
//==========================================================================

image_t* R_FindImage(const char* name)
{
	long hash = generateHashValue(name);
	for (image_t* image = ImageHashTable[hash]; image; image=image->next)
	{
		if (!QStr::Cmp(name, image->imgName))
		{
			return image;
		}
	}
	return NULL;
}

//==========================================================================
//
//	R_FindImageFile
//
//	Finds or loads the given image. Returns NULL if it fails, not a default image.
//
//==========================================================================

image_t* R_FindImageFile(const char* name, bool mipmap, bool allowPicmip,
	GLenum glWrapClampMode, bool AllowScrap, int Mode, byte* TransPixels)
{
	if (!name)
	{
		return NULL;
	}

	//
	// see if the image is already loaded
	//
	image_t* image = R_FindImage(name);
	if (image)
	{
		// the white image can be used with any set of parms, but other mismatches are errors
		if (QStr::Cmp(name, "*white"))
		{
			if (image->mipmap != mipmap)
			{
				GLog.DWrite(S_COLOR_RED "WARNING: reused image %s with mixed mipmap parm\n", name);
			}
			if (image->allowPicmip != allowPicmip)
			{
				GLog.DWrite(S_COLOR_RED "WARNING: reused image %s with mixed allowPicmip parm\n", name);
			}
			if (image->wrapClampMode != glWrapClampMode)
			{
				GLog.Write("WARNING: reused image %s with mixed glWrapClampMode parm\n", name);
			}
		}
		return image;
	}

	//
	// load the pic from disk
	//
	byte* pic;
	int width;
	int height;
	R_LoadImage(name, &pic, &width, &height, Mode, TransPixels);
	if (pic == NULL)
	{
		//	If we dont get a successful load copy the name and try upper case
		// extension for unix systems, if that fails bail.
		char altname[MAX_QPATH];
		QStr::Cpy(altname, name);
		int len = QStr::Length(altname);
		altname[len - 3] = QStr::ToUpper(altname[len - 3]);
		altname[len - 2] = QStr::ToUpper(altname[len - 2]);
		altname[len - 1] = QStr::ToUpper(altname[len - 1]);
		GLog.Write("trying %s...\n", altname);
		R_LoadImage(altname, &pic, &width, &height);
		if (pic == NULL)
		{
			return NULL;
		}
	}

	image = R_CreateImage(name, pic, width, height, mipmap, allowPicmip, glWrapClampMode, AllowScrap);
	delete[] pic;
	return image;
}

//==========================================================================
//
//	R_SetColorMappings
//
//==========================================================================

void R_SetColorMappings()
{
	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if (!glConfig.deviceSupportsGamma)
	{
		tr.overbrightBits = 0;		// need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if (!glConfig.isFullscreen)
	{
		tr.overbrightBits = 0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if (glConfig.colorBits > 16)
	{
		if (tr.overbrightBits > 2)
		{
			tr.overbrightBits = 2;
		}
	}
	else
	{
		if (tr.overbrightBits > 1)
		{
			tr.overbrightBits = 1;
		}
	}
	if (tr.overbrightBits < 0)
	{
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / (1 << tr.overbrightBits);
	tr.identityLightByte = 255 * tr.identityLight;

	if (r_intensity->value <= 1)
	{
		Cvar_Set("r_intensity", "1");
	}

	if (r_gamma->value < 0.5f)
	{
		Cvar_Set("r_gamma", "0.5");
	}
	else if (r_gamma->value > 3.0f)
	{
		Cvar_Set("r_gamma", "3.0");
	}

	float g = r_gamma->value;

	int shift = tr.overbrightBits;

	for (int i = 0; i < 256; i++)
	{
		int inf;
		if (g == 1)
		{
			inf = i;
		}
		else
		{
			inf = (int)(255 * pow(i / 255.0f, 1.0f / g) + 0.5f);
		}
		inf <<= shift;
		if (inf < 0)
		{
			inf = 0;
		}
		if (inf > 255)
		{
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for (int i = 0; i < 256; i++)
	{
		int j = (int)(i * r_intensity->value);
		if (j > 255)
		{
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if (glConfig.deviceSupportsGamma)
	{
		GLimp_SetGamma(s_gammatable, s_gammatable, s_gammatable);
	}
}

//==========================================================================
//
//	R_GammaCorrect
//
//==========================================================================

void R_GammaCorrect(byte* Buffer, int BufferSize)
{
	for (int i = 0; i < BufferSize; i++)
	{
		Buffer[i] = s_gammatable[Buffer[i]];
	}
}
