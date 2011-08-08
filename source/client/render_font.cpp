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
//
// The font system uses FreeType 2.x to render TrueType fonts for use within the game.
// As of this writing ( Nov, 2000 ) Team Arena uses these fonts for all of the ui and 
// about 90% of the cgame presentation. A few areas of the CGAME were left uses the old 
// fonts since the code is shared with standard Q3A.
//
// If you include this font rendering code in a commercial product you MUST include the
// following somewhere with your product, see www.freetype.org for specifics or changes.
// The Freetype code also uses some hinting techniques that MIGHT infringe on patents 
// held by apple so be aware of that also.
//
// As of Q3A 1.25+ and Team Arena, we are shipping the game with the font rendering code
// disabled. This removes any potential patent issues and it keeps us from having to 
// distribute an actual TrueTrype font which is 1. expensive to do and 2. seems to require
// an act of god to accomplish. 
//
// What we did was pre-render the fonts using FreeType ( which is why we leave the FreeType
// credit in the credits ) and then saved off the glyph data and then hand touched up the 
// font bitmaps so they scale a bit better in GL.
//
// There are limitations in the way fonts are saved and reloaded in that it is based on 
// point size and not name. So if you pre-render Helvetica in 18 point and Impact in 18 point
// you will end up with a single 18 point data file and image set. Typically you will want to 
// choose 3 sizes to best approximate the scaling you will be doing in the ui scripting system
// 
// In the UI Scripting code, a scale of 1.0 is equal to a 48 point font. In Team Arena, we
// use three or four scales, most of them exactly equaling the specific rendered size. We 
// rendered three sizes in Team Arena, 12, 16, and 20. 
//
// To generate new font data you need to go through the following steps.
// 1. delete the fontImage_x_xx.tga files and fontImage_xx.dat files from the fonts path.
// 2. in a ui script, specificy a font, smallFont, and bigFont keyword with font name and 
//    point size. the original TrueType fonts must exist in fonts at this point.
// 3. run the game, you should see things normally.
// 4. Exit the game and there will be three dat files and at least three tga files. The 
//    tga's are in 256x256 pages so if it takes three images to render a 24 point font you 
//    will end up with fontImage_0_24.tga through fontImage_2_24.tga
// 5. You will need to flip the tga's in Photoshop as the tga output code writes them upside
//    down.
// 6. In future runs of the game, the system looks for these images and data files when a s
//    specific point sized font is rendered and loads them for use. 
// 7. Because of the original beta nature of the FreeType code you will probably want to hand
//    touch the font bitmaps.
// 
// Currently a define in the project turns on or off the FreeType code which is currently 
// defined out. To pre-render new fonts you need enable the define ( BUILD_FREETYPE ) and 
// uncheck the exclude from build check box in the FreeType2 area of the Renderer project.
//
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"
#include <freetype/ftsystem.h>
#include <freetype/ftimage.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>

// MACROS ------------------------------------------------------------------

#define MAX_FONTS	6

#define _FLOOR(x)	((x) & -64)
#define _CEIL(x)	(((x)+63) & -64)
#define _TRUNC(x)	((x) >> 6)

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static FT_Library	ftLibrary = NULL;

static int			registeredFontCount = 0;
static fontInfo_t	registeredFont[MAX_FONTS];

static int			fdOffset;
static byte*		fdFile;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_GetGlyphInfo
//
//==========================================================================

static void R_GetGlyphInfo(FT_GlyphSlot glyph, int* left, int* right, int* width,
	int* top, int* bottom, int* height, int* pitch)
{
	*left = _FLOOR(glyph->metrics.horiBearingX);
	*right = _CEIL(glyph->metrics.horiBearingX + glyph->metrics.width);
	*width = _TRUNC(*right - *left);

	*top = _CEIL(glyph->metrics.horiBearingY);
	*bottom = _FLOOR(glyph->metrics.horiBearingY - glyph->metrics.height);
	*height = _TRUNC(*top - *bottom);
	*pitch  = (*width + 3) & -4;
}

//==========================================================================
//
//	R_RenderGlyph
//
//==========================================================================

static FT_Bitmap* R_RenderGlyph(FT_GlyphSlot glyph, glyphInfo_t* glyphOut)
{
	int left, right, width, top, bottom, height, pitch, size;

	R_GetGlyphInfo(glyph, &left, &right, &width, &top, &bottom, &height, &pitch);

	if (glyph->format != ft_glyph_format_outline)
	{
		Log::write("Non-outline fonts are not supported\n");
		return NULL;
	}

	size   = pitch*height; 

	FT_Bitmap* bit2 = new FT_Bitmap;

	bit2->width = width;
	bit2->rows = height;
	bit2->pitch = pitch;
	bit2->pixel_mode = ft_pixel_mode_grays;
	bit2->buffer = new unsigned char[pitch * height];
	bit2->num_grays = 256;

	Com_Memset(bit2->buffer, 0, size);

	FT_Outline_Translate(&glyph->outline, -left, -bottom);

	FT_Outline_Get_Bitmap(ftLibrary, &glyph->outline, bit2);

	glyphOut->height = height;
	glyphOut->pitch = pitch;
	glyphOut->top = (glyph->metrics.horiBearingY >> 6) + 1;
	glyphOut->bottom = bottom;

	return bit2;
}

//==========================================================================
//
//	RE_ConstructGlyphInfo
//
//==========================================================================

static glyphInfo_t* RE_ConstructGlyphInfo(unsigned char* imageOut, int* xOut, int* yOut,
	int* maxHeight, FT_Face face, const unsigned char c, bool calcHeight)
{
	static glyphInfo_t glyph;

	Com_Memset(&glyph, 0, sizeof(glyphInfo_t));
	// make sure everything is here
	if (face != NULL)
	{
		FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FT_LOAD_DEFAULT);
		FT_Bitmap* bitmap = R_RenderGlyph(face->glyph, &glyph);
		if (bitmap)
		{
			glyph.xSkip = (face->glyph->metrics.horiAdvance >> 6) + 1;
		}
		else
		{
			return &glyph;
		}

		if (glyph.height > *maxHeight)
		{
			*maxHeight = glyph.height;
		}

		if (calcHeight)
		{
			delete[] bitmap->buffer;
			delete bitmap;
			return &glyph;
		}

		int scaled_width = glyph.pitch;
		int scaled_height = glyph.height;

		// we need to make sure we fit
		if (*xOut + scaled_width + 1 >= 255)
		{
			if (*yOut + *maxHeight + 1 >= 255)
			{
				*yOut = -1;
				*xOut = -1;
				delete[] bitmap->buffer;
				delete bitmap;
				return &glyph;
			}
			else
			{
				*xOut = 0;
				*yOut += *maxHeight + 1;
			}
		}
		else if (*yOut + *maxHeight + 1 >= 255)
		{
			*yOut = -1;
			*xOut = -1;
			delete[] bitmap->buffer;
			delete bitmap;
			return &glyph;
		}


		unsigned char* src = bitmap->buffer;
		unsigned char* dst = imageOut + (*yOut * 256) + *xOut;

		if (bitmap->pixel_mode == ft_pixel_mode_mono)
		{
			for (int i = 0; i < glyph.height; i++)
			{
				int j;
				unsigned char *_src = src;
				unsigned char *_dst = dst;
				unsigned char mask = 0x80;
				unsigned char val = *_src;
				for (j = 0; j < glyph.pitch; j++)
				{
					if (mask == 0x80)
					{
						val = *_src++;
					}
					if (val & mask)
					{
						*_dst = 0xff;
					}
					mask >>= 1;
		
					if (mask == 0)
					{
						mask = 0x80;
					}
					_dst++;
				}

				src += glyph.pitch;
				dst += 256;

			}
		}
		else
		{
			for (int i = 0; i < glyph.height; i++)
			{
				Com_Memcpy(dst, src, glyph.pitch);
				src += glyph.pitch;
				dst += 256;
			}
		}

		// we now have an 8 bit per pixel grey scale bitmap 
		// that is width wide and pf->ftSize->metrics.y_ppem tall

		glyph.imageHeight = scaled_height;
		glyph.imageWidth = scaled_width;
		glyph.s = (float)*xOut / 256;
		glyph.t = (float)*yOut / 256;
		glyph.s2 = glyph.s + (float)scaled_width / 256;
		glyph.t2 = glyph.t + (float)scaled_height / 256;

		*xOut += scaled_width + 1;

		delete[] bitmap->buffer;
		delete bitmap;
	}

	return &glyph;
}

//==========================================================================
//
//	readInt
//
//==========================================================================

static int readInt()
{
	int i = fdFile[fdOffset] + (fdFile[fdOffset + 1] << 8) + (fdFile[fdOffset + 2] << 16) + (fdFile[fdOffset + 3] << 24);
	fdOffset += 4;
	return i;
}

//==========================================================================
//
//	readFloat
//
//==========================================================================

static float readFloat()
{
	float f = LittleFloat(*(float*)(fdFile + fdOffset));
	fdOffset += 4;
	return f;
}

//==========================================================================
//
//	R_RegisterFont
//
//==========================================================================

void R_RegisterFont(const char* fontName, int pointSize, fontInfo_t* font)
{
	float dpi = 72;
	float glyphScale =  72.0f / dpi;	// change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )

	if (pointSize <= 0)
	{
		pointSize = 12;
	}
	// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
	glyphScale *= 48.0f / pointSize;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	if (registeredFontCount >= MAX_FONTS)
	{
		Log::write("R_RegisterFont: Too many fonts registered already.\n");
		return;
	}

	char name[1024];
	String::Sprintf(name, sizeof(name), "fonts/fontImage_%i.dat", pointSize);
	for (int i = 0; i < registeredFontCount; i++)
	{
		if (String::ICmp(name, registeredFont[i].name) == 0)
		{
			Com_Memcpy(font, &registeredFont[i], sizeof(fontInfo_t));
			return;
		}
	}

	int len = FS_ReadFile(name, NULL);
	if (len == sizeof(fontInfo_t))
	{
		void* faceData;
		FS_ReadFile(name, &faceData);
		fdOffset = 0;
		fdFile = (byte*)faceData;
		for (int i = 0; i < GLYPHS_PER_FONT; i++)
		{
			font->glyphs[i].height = readInt();
			font->glyphs[i].top = readInt();
			font->glyphs[i].bottom = readInt();
			font->glyphs[i].pitch = readInt();
			font->glyphs[i].xSkip = readInt();
			font->glyphs[i].imageWidth = readInt();
			font->glyphs[i].imageHeight = readInt();
			font->glyphs[i].s = readFloat();
			font->glyphs[i].t = readFloat();
			font->glyphs[i].s2 = readFloat();
			font->glyphs[i].t2 = readFloat();
			font->glyphs[i].glyph = readInt();
			Com_Memcpy(font->glyphs[i].shaderName, &fdFile[fdOffset], 32);
			fdOffset += 32;
		}
		font->glyphScale = readFloat();
		Com_Memcpy(font->name, &fdFile[fdOffset], MAX_QPATH);

		String::NCpyZ(font->name, name, sizeof(font->name));
		for (int i = GLYPH_START; i < GLYPH_END; i++)
		{
			font->glyphs[i].glyph = R_RegisterShaderNoMip(font->glyphs[i].shaderName);
		}
		Com_Memcpy(&registeredFont[registeredFontCount++], font, sizeof(fontInfo_t));
		return;
	}

	if (ftLibrary == NULL)
	{
		Log::write("R_RegisterFont: FreeType not initialized.\n");
		return;
	}

	void *faceData;
	len = FS_ReadFile(fontName, &faceData);
	if (len <= 0)
	{
		Log::write("R_RegisterFont: Unable to read font file\n");
		return;
	}

	// allocate on the stack first in case we fail
	FT_Face face;
	if (FT_New_Memory_Face(ftLibrary, (FT_Byte*)faceData, len, 0, &face))
	{
		Log::write("R_RegisterFont: FreeType2, unable to allocate new face.\n");
		return;
	}

	if (FT_Set_Char_Size(face, pointSize << 6, pointSize << 6, dpi, dpi))
	{
		Log::write("R_RegisterFont: FreeType2, Unable to set face char size.\n");
		return;
	}

	// make a 256x256 image buffer, once it is full, register it, clean it and keep going 
	// until all glyphs are rendered

	byte* out = new byte[1024 * 1024];
	Com_Memset(out, 0, 1024 * 1024);

	int maxHeight = 0;

	for (int i = GLYPH_START; i < GLYPH_END; i++)
	{
		int xOut;
		int yOut;
		RE_ConstructGlyphInfo(out, &xOut, &yOut, &maxHeight, face, (unsigned char)i, true);
	}

	int xOut = 0;
	int yOut = 0;
	int i = GLYPH_START;
	int lastStart = i;
	int imageNumber = 0;

	while (i <= GLYPH_END)
	{
		glyphInfo_t* glyph = RE_ConstructGlyphInfo(out, &xOut, &yOut, &maxHeight, face, (unsigned char)i, false);

		if (xOut == -1 || yOut == -1 || i == GLYPH_END)
		{
			// ran out of room
			// we need to create an image from the bitmap, set all the handles in the glyphs to this point
			// 

			int scaledSize = 256 * 256;
			int newSize = scaledSize * 4;
			byte* imageBuff = new byte[newSize];
			int left = 0;
			float max = 0;
			for (int k = 0; k < scaledSize; k++)
			{
				if (max < out[k])
				{
					max = out[k];
				}
			}

			if (max > 0)
			{
				max = 255 / max;
			}

			for (int k = 0; k < scaledSize; k++)
			{
				imageBuff[left++] = 255;
				imageBuff[left++] = 255;
				imageBuff[left++] = 255;

				imageBuff[left++] = ((float)out[k] * max);
			}

			String::Sprintf (name, sizeof(name), "fonts/fontImage_%i_%i.tga", imageNumber++, pointSize);
			if (r_saveFontData->integer)
			{
				R_SaveTGA(name, imageBuff, 256, 256, true);
			}

			image_t* image = R_CreateImage(name, imageBuff, 256, 256, false, false, GL_CLAMP, false);
			qhandle_t h = R_RegisterShaderFromImage(name, LIGHTMAP_2D, image, false);
			for (int j = lastStart; j < i; j++)
			{
				font->glyphs[j].glyph = h;
				String::NCpyZ(font->glyphs[j].shaderName, name, sizeof(font->glyphs[j].shaderName));
			}
			lastStart = i;
			Com_Memset(out, 0, 1024 * 1024);
			xOut = 0;
			yOut = 0;
			delete[] imageBuff;
			i++;
		}
		else
		{
			Com_Memcpy(&font->glyphs[i], glyph, sizeof(glyphInfo_t));
			i++;
		}
	}

	registeredFont[registeredFontCount].glyphScale = glyphScale;
	font->glyphScale = glyphScale;
	Com_Memcpy(&registeredFont[registeredFontCount++], font, sizeof(fontInfo_t));

	if (r_saveFontData->integer)
	{
		FS_WriteFile(va("fonts/fontImage_%i.dat", pointSize), font, sizeof(fontInfo_t));
	}

	delete[] out;
	
	FS_FreeFile(faceData);
}

//==========================================================================
//
//	R_InitFreeType
//
//==========================================================================

void R_InitFreeType()
{
	if (FT_Init_FreeType(&ftLibrary))
	{
		Log::write("R_InitFreeType: Unable to initialize FreeType.\n");
	}
	registeredFontCount = 0;
}

//==========================================================================
//
//	R_DoneFreeType
//
//==========================================================================

void R_DoneFreeType()
{
	if (ftLibrary)
	{
		FT_Done_FreeType(ftLibrary);
		ftLibrary = NULL;
	}
	registeredFontCount = 0;
}
