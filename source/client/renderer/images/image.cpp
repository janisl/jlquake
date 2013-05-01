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

#include "../local.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/command_buffer.h"

//#define IMAGE_HASH_SIZE		1024
#define IMAGE_HASH_SIZE     4096

#define DEFAULT_SIZE        16

struct textureMode_t {
	const char* name;
	GLenum minimize;
	GLenum maximize;
};

byte host_basepal[ 768 ];
byte r_palette[ 256 ][ 4 ];
unsigned* d_8to24table;

int ColorIndex[ 16 ] =
{
	0, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 199, 207, 223, 231
};

unsigned ColorPercent[ 16 ] =
{
	25, 51, 76, 102, 114, 127, 140, 153, 165, 178, 191, 204, 216, 229, 237, 247
};

static byte s_gammatable[ 256 ];
static byte s_intensitytable[ 256 ];

static image_t* ImageHashTable[ IMAGE_HASH_SIZE ];

static int numBackupImages = 0;
static image_t* backupHashTable[ IMAGE_HASH_SIZE ];

static int gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
static int gl_filter_max = GL_LINEAR;

static float gl_anisotropy = 1.0;

static byte mipBlendColors[ 16 ][ 4 ] =
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

static textureMode_t modes[] =
{
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

static byte cs_data[ 64 ] =
{
	0, 0, 0, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 255, 0, 0, 0, 0,
	255, 0, 255, 0, 255, 0, 255, 0,
	0, 0, 0, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

static void R_SetPalette( byte* pal ) {
	//
	// 8 8 8 encoding
	//
	d_8to24table = ( unsigned* )r_palette;
	for ( int i = 0; i < 256; i++ ) {
		r_palette[ i ][ 0 ] = pal[ 0 ];
		r_palette[ i ][ 1 ] = pal[ 1 ];
		r_palette[ i ][ 2 ] = pal[ 2 ];
		r_palette[ i ][ 3 ] = 255;
		pal += 3;
	}
	r_palette[ 255 ][ 3 ] = 0;	// 255 is transparent
}

void R_InitQ1Palette() {
	idList<byte> Pal;
	if ( FS_ReadFile( "gfx/palette.lmp", Pal ) <= 0 ) {
		common->FatalError( "Couldn't load gfx/palette.lmp" );
	}
	R_SetPalette( Pal.Ptr() );
	Com_Memcpy( host_basepal, Pal.Ptr(), 768 );
}

void R_InitQ2Palette() {
	// get the palette

	byte* pic;
	byte* pal;
	int width;
	int height;
	R_LoadPCX( "pics/colormap.pcx", &pic, &pal, &width, &height );
	if ( !pal ) {
		common->FatalError( "Couldn't load pics/colormap.pcx" );
	}

	R_SetPalette( pal );

	delete[] pic;
	delete[] pal;
}

//	Fill background pixels so mipmapping doesn't have haloes
static void R_FloodFillSkin( byte* skin, int skinwidth, int skinheight ) {
	int filledcolor = 0;
	// attempt to find opaque black
	for ( int i = 0; i < 256; ++i )
		if ( r_palette[ i ][ 0 ] == 0 && r_palette[ i ][ 1 ] == 0 && r_palette[ i ][ 2 ] == 0 && r_palette[ i ][ 3 ] == 255 ) {
			filledcolor = i;
			break;
		}

	byte fillcolor = *skin;	// assume this is the pixel to fill

	// can't fill to filled color or to transparent color (used as visited marker)
	if ( ( fillcolor == filledcolor ) || ( fillcolor == 255 ) ) {
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	enum
	{
		// must be a power of 2
		FLOODFILL_FIFO_SIZE = 0x1000,
		FLOODFILL_FIFO_MASK = ( FLOODFILL_FIFO_SIZE - 1 )
	};

	struct floodfill_t {
		short x, y;
	};

	int inpt = 0;
	int outpt = 0;
	floodfill_t fifo[ FLOODFILL_FIFO_SIZE ];
	fifo[ inpt ].x = 0;
	fifo[ inpt ].y = 0;
	inpt = ( inpt + 1 ) & FLOODFILL_FIFO_MASK;

	while ( outpt != inpt ) {
		int x = fifo[ outpt ].x;
		int y = fifo[ outpt ].y;
		int fdc = filledcolor;
		byte* pos = &skin[ x + skinwidth * y ];

		outpt = ( outpt + 1 ) & FLOODFILL_FIFO_MASK;

#define FLOODFILL_STEP( off, dx, dy ) \
	{ \
		if ( pos[ off ] == fillcolor ) \
		{ \
			pos[ off ] = 255; \
			fifo[ inpt ].x = x + ( dx ); \
			fifo[ inpt ].y = y + ( dy ); \
			inpt = ( inpt + 1 ) & FLOODFILL_FIFO_MASK; \
		} \
		else if ( pos[ off ] != 255 ) \
		{ \
			fdc = pos[ off ]; \
		} \
	}

		if ( x > 0 ) {
			FLOODFILL_STEP( -1, -1, 0 );
		}
		if ( x < skinwidth - 1 ) {
			FLOODFILL_STEP( 1, 1, 0 );
		}
		if ( y > 0 ) {
			FLOODFILL_STEP( -skinwidth, 0, -1 );
		}
		if ( y < skinheight - 1 ) {
			FLOODFILL_STEP( skinwidth, 0, 1 );
		}
		skin[ x + skinwidth * y ] = fdc;

#undef FLOODFILL_STEP
	}
}

byte* R_ConvertImage8To32( byte* DataIn, int Width, int Height, int Mode ) {
	if ( Mode >= IMG8MODE_Skin ) {
		R_FloodFillSkin( DataIn, Width, Height );
	}

	byte* DataOut = new byte[ Width * Height * 4 ];

	int Count = Width * Height;
	for ( int i = 0; i < Count; i++ ) {
		int p = DataIn[ i ];
		DataOut[ i * 4 + 0 ] = r_palette[ p ][ 0 ];
		DataOut[ i * 4 + 1 ] = r_palette[ p ][ 1 ];
		DataOut[ i * 4 + 2 ] = r_palette[ p ][ 2 ];
		DataOut[ i * 4 + 3 ] = r_palette[ p ][ 3 ];
	}

	//	For transparent pixels scan around for another color to avoid alpha fringes
	for ( int i = 0; i < Count; i++ ) {
		int p = DataIn[ i ];
		if ( p == 255 || ( p == 0 && ( Mode == IMG8MODE_Holey || Mode == IMG8MODE_SkinHoley || Mode == IMG8MODE_SkinTransparent ) ) ) {
			int u = i % Width;
			int v = i / Width;

			byte neighbors[ 9 ][ 3 ];
			int num_neighbors_valid = 0;
			for ( int neighbor_u = u - 1; neighbor_u <= u + 1; neighbor_u++ ) {
				for ( int neighbor_v = v - 1; neighbor_v <= v + 1; neighbor_v++ ) {
					if ( neighbor_u == neighbor_v ) {
						continue;
					}
					// Make sure  that we are accessing a texel in the image, not out of range.
					if ( neighbor_u < 0 || neighbor_u >= Width || neighbor_v < 0 || neighbor_v >= Height ) {
						continue;
					}
					if ( DataIn[ neighbor_u + neighbor_v * Width ] == 255 ) {
						continue;
					}
					neighbors[ num_neighbors_valid ][ 0 ] = DataOut[ ( neighbor_u + neighbor_v * Width ) * 4 + 0 ];
					neighbors[ num_neighbors_valid ][ 1 ] = DataOut[ ( neighbor_u + neighbor_v * Width ) * 4 + 1 ];
					neighbors[ num_neighbors_valid ][ 2 ] = DataOut[ ( neighbor_u + neighbor_v * Width ) * 4 + 2 ];
					num_neighbors_valid++;
				}
			}

			if ( num_neighbors_valid == 0 ) {
				continue;
			}

			int r = 0, g = 0, b = 0;
			for ( int n = 0; n < num_neighbors_valid; n++ ) {
				r += neighbors[ n ][ 0 ];
				g += neighbors[ n ][ 1 ];
				b += neighbors[ n ][ 2 ];
			}

			r /= num_neighbors_valid;
			g /= num_neighbors_valid;
			b /= num_neighbors_valid;

			if ( r > 255 ) {
				r = 255;
			}
			if ( g > 255 ) {
				g = 255;
			}
			if ( b > 255 ) {
				b = 255;
			}

			DataOut[ i * 4 + 0 ] = r;
			DataOut[ i * 4 + 1 ] = g;
			DataOut[ i * 4 + 2 ] = b;
		}
	}

	switch ( Mode ) {
	case IMG8MODE_SkinTransparent:
		for ( int i = 0; i < Count; i++ ) {
			int p = DataIn[ i ];
			if ( p == 0 ) {
				DataOut[ i * 4 + 3 ] = 0;
			} else if ( p & 1 )     {
				DataOut[ i * 4 + 3 ] = ( int )( 255 * r_wateralpha->value );
			} else   {
				DataOut[ i * 4 + 3 ] = 255;
			}
		}
		break;

	case IMG8MODE_Holey:
	case IMG8MODE_SkinHoley:
		for ( int i = 0; i < Count; i++ ) {
			int p = DataIn[ i ];
			if ( p == 0 ) {
				DataOut[ i * 4 + 3 ] = 0;
			}
		}
		break;

	case IMG8MODE_SkinSpecialTrans:
		for ( int i = 0; i < Count; i++ ) {
			int p = DataIn[ i ];
			DataOut[ i * 4 + 0 ] = r_palette[ ColorIndex[ p >> 4 ] ][ 0 ];
			DataOut[ i * 4 + 1 ] = r_palette[ ColorIndex[ p >> 4 ] ][ 1 ];
			DataOut[ i * 4 + 2 ] = r_palette[ ColorIndex[ p >> 4 ] ][ 2 ];
			DataOut[ i * 4 + 3 ] = ColorPercent[ p & 15 ];
		}
		break;
	}

	return DataOut;
}

byte* R_GetFullBrightImage( byte* data8, byte* data32, int width, int height ) {
	//	Only Quake has fullbright colours in palette.
	if ( !( GGameType & GAME_Quake ) || !r_fullBrightColours->integer ) {
		return NULL;
	}

	int count = width * height;
	bool haveFullBrights = false;
	for ( int i = 0; i < count; i++ ) {
		int p = data8[ i ];
		if ( p >= 224 && p < 255 ) {
			haveFullBrights = true;
			break;
		}
	}
	if ( !haveFullBrights ) {
		return NULL;
	}

	byte* dataOut = new byte[ count * 4 ];
	Com_Memcpy( dataOut, data32, count * 4 );

	for ( int i = 0; i < count; i++ ) {
		int p = data8[ i ];
		if ( p < 224 ) {
			dataOut[ i * 4 + 3 ] = 0;
		}
	}

	return dataOut;
}

//	Loads any of the supported image types into a cannonical 32 bit format.
void R_LoadImage( const char* name, byte** pic, int* width, int* height, int Mode, byte* TransPixels ) {
	*pic = NULL;
	*width = 0;
	*height = 0;

	int len = String::Length( name );
	if ( len < 5 ) {
		return;
	}

	if ( !String::ICmp( name + len - 4, ".tga" ) ) {
		R_LoadTGA( name, pic, width, height );				// try tga first
		if ( !*pic ) {							//
			char altname[ MAX_QPATH ];							// try jpg in place of tga
			String::Cpy( altname, name );
			len = String::Length( altname );
			altname[ len - 3 ] = 'j';
			altname[ len - 2 ] = 'p';
			altname[ len - 1 ] = 'g';
			R_LoadJPG( altname, pic, width, height );
		}
	} else if ( !String::ICmp( name + len - 4, ".pcx" ) )       {
		R_LoadPCX32( name, pic, width, height, Mode );
	} else if ( !String::ICmp( name + len - 4, ".bmp" ) )       {
		R_LoadBMP( name, pic, width, height );
	} else if ( !String::ICmp( name + len - 4, ".jpg" ) )       {
		R_LoadJPG( name, pic, width, height );
	} else if ( !String::ICmp( name + len - 4, ".wal" ) )       {
		R_LoadWAL( name, pic, width, height );
	} else if ( !String::ICmp( name + len - 4, ".lmp" ) )       {
		R_LoadPIC( name, pic, width, height, TransPixels, Mode );
	}
}

//	Used to resample images in a more general than quartering fashion.
//
//	This will only be filtered properly if the resampled size is greater
// than half the original size.
//
//	If a larger shrinking is needed, use the mipmap function before or after.
static void R_ResampleTexture( const byte* in, int inwidth, int inheight, byte* out, int outwidth, int outheight ) {
	if ( outwidth > 2048 ) {
		common->Error( "ResampleTexture: max width" );
	}
	if ( outheight > 2048 ) {
		common->Error( "ResampleTexture: max height" );
	}

	unsigned fracstep = inwidth * 0x10000 / outwidth;

	unsigned p1[ 2048 ];
	unsigned frac = fracstep >> 2;
	for ( int i = 0; i < outwidth; i++ ) {
		p1[ i ] = 4 * ( frac >> 16 );
		frac += fracstep;
	}
	unsigned p2[ 2048 ];
	frac = 3 * ( fracstep >> 2 );
	for ( int i = 0; i < outwidth; i++ ) {
		p2[ i ] = 4 * ( frac >> 16 );
		frac += fracstep;
	}

	for ( int i = 0; i < outheight; i++ ) {
		const byte* inrow = in + inwidth * ( int )( ( i + 0.25 ) * inheight / outheight ) * 4;
		const byte* inrow2 = in + inwidth * ( int )( ( i + 0.75 ) * inheight / outheight ) * 4;
		for ( int j = 0; j < outwidth; j++ ) {
			const byte* pix1 = inrow + p1[ j ];
			const byte* pix2 = inrow + p2[ j ];
			const byte* pix3 = inrow2 + p1[ j ];
			const byte* pix4 = inrow2 + p2[ j ];
			*out++ = ( pix1[ 0 ] + pix2[ 0 ] + pix3[ 0 ] + pix4[ 0 ] ) >> 2;
			*out++ = ( pix1[ 1 ] + pix2[ 1 ] + pix3[ 1 ] + pix4[ 1 ] ) >> 2;
			*out++ = ( pix1[ 2 ] + pix2[ 2 ] + pix3[ 2 ] + pix4[ 2 ] ) >> 2;
			*out++ = ( pix1[ 3 ] + pix2[ 3 ] + pix3[ 3 ] + pix4[ 3 ] ) >> 2;
		}
	}
}

//	Operates in place, quartering the size of the texture
//	Proper linear filter
static void R_MipMap2( byte* in, int inWidth, int inHeight ) {
	int outWidth = inWidth >> 1;
	int outHeight = inHeight >> 1;
	idList<byte> temp;
	temp.SetNum( outWidth * outHeight * 4 );

	int inWidthMask = inWidth - 1;
	int inHeightMask = inHeight - 1;

	for ( int i = 0; i < outHeight; i++ ) {
		for ( int j = 0; j < outWidth; j++ ) {
			byte* outpix = &temp[ ( i * outWidth + j ) * 4 ];
			for ( int k = 0; k < 4; k++ ) {
				int total =
					1 * in[ ( ( ( i * 2 - 1 ) & inHeightMask ) * inWidth + ( ( j * 2 - 1 ) & inWidthMask ) ) * 4 + k ] +
					2 * in[ ( ( ( i * 2 - 1 ) & inHeightMask ) * inWidth + ( ( j * 2 ) & inWidthMask ) ) * 4 + k ] +
					2 * in[ ( ( ( i * 2 - 1 ) & inHeightMask ) * inWidth + ( ( j * 2 + 1 ) & inWidthMask ) ) * 4 + k ] +
					1 * in[ ( ( ( i * 2 - 1 ) & inHeightMask ) * inWidth + ( ( j * 2 + 2 ) & inWidthMask ) ) * 4 + k ] +

					2 * in[ ( ( ( i * 2 ) & inHeightMask ) * inWidth + ( ( j * 2 - 1 ) & inWidthMask ) ) * 4 + k ] +
					4 * in[ ( ( ( i * 2 ) & inHeightMask ) * inWidth + ( ( j * 2 ) & inWidthMask ) ) * 4 + k ] +
					4 * in[ ( ( ( i * 2 ) & inHeightMask ) * inWidth + ( ( j * 2 + 1 ) & inWidthMask ) ) * 4 + k ] +
					2 * in[ ( ( ( i * 2 ) & inHeightMask ) * inWidth + ( ( j * 2 + 2 ) & inWidthMask ) ) * 4 + k ] +

					2 * in[ ( ( ( i * 2 + 1 ) & inHeightMask ) * inWidth + ( ( j * 2 - 1 ) & inWidthMask ) ) * 4 + k ] +
					4 * in[ ( ( ( i * 2 + 1 ) & inHeightMask ) * inWidth + ( ( j * 2 ) & inWidthMask ) ) * 4 + k ] +
					4 * in[ ( ( ( i * 2 + 1 ) & inHeightMask ) * inWidth + ( ( j * 2 + 1 ) & inWidthMask ) ) * 4 + k ] +
					2 * in[ ( ( ( i * 2 + 1 ) & inHeightMask ) * inWidth + ( ( j * 2 + 2 ) & inWidthMask ) ) * 4 + k ] +

					1 * in[ ( ( ( i * 2 + 2 ) & inHeightMask ) * inWidth + ( ( j * 2 - 1 ) & inWidthMask ) ) * 4 + k ] +
					2 * in[ ( ( ( i * 2 + 2 ) & inHeightMask ) * inWidth + ( ( j * 2 ) & inWidthMask ) ) * 4 + k ] +
					2 * in[ ( ( ( i * 2 + 2 ) & inHeightMask ) * inWidth + ( ( j * 2 + 1 ) & inWidthMask ) ) * 4 + k ] +
					1 * in[ ( ( ( i * 2 + 2 ) & inHeightMask ) * inWidth + ( ( j * 2 + 2 ) & inWidthMask ) ) * 4 + k ];
				outpix[ k ] = total / 36;
			}
		}
	}

	Com_Memcpy( in, temp.Ptr(), outWidth * outHeight * 4 );
}

//	Operates in place, quartering the size of the texture
static void R_MipMap( byte* in, int width, int height ) {
	if ( !r_simpleMipMaps->integer ) {
		R_MipMap2( in, width, height );
		return;
	}

	if ( width == 1 && height == 1 ) {
		return;
	}

	int row = width * 4;
	byte* out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;	// get largest
		for ( int i = 0; i < width; i++, out += 4, in += 8 ) {
			out[ 0 ] = ( in[ 0 ] + in[ 4 ] ) >> 1;
			out[ 1 ] = ( in[ 1 ] + in[ 5 ] ) >> 1;
			out[ 2 ] = ( in[ 2 ] + in[ 6 ] ) >> 1;
			out[ 3 ] = ( in[ 3 ] + in[ 7 ] ) >> 1;
		}
		return;
	}

	for ( int i = 0; i < height; i++, in += row ) {
		for ( int j = 0; j < width; j++, out += 4, in += 8 ) {
			out[ 0 ] = ( in[ 0 ] + in[ 4 ] + in[ row + 0 ] + in[ row + 4 ] ) >> 2;
			out[ 1 ] = ( in[ 1 ] + in[ 5 ] + in[ row + 1 ] + in[ row + 5 ] ) >> 2;
			out[ 2 ] = ( in[ 2 ] + in[ 6 ] + in[ row + 2 ] + in[ row + 6 ] ) >> 2;
			out[ 3 ] = ( in[ 3 ] + in[ 7 ] + in[ row + 3 ] + in[ row + 7 ] ) >> 2;
		}
	}
}

//	Scale up the pixel values in a texture to increase the lighting range
static void R_LightScaleTexture( byte* in, int inwidth, int inheight, qboolean only_gamma ) {
	if ( only_gamma ) {
		if ( !glConfig.deviceSupportsGamma ) {
			byte* p = in;
			int c = inwidth * inheight;
			for ( int i = 0; i < c; i++, p += 4 ) {
				p[ 0 ] = s_gammatable[ p[ 0 ] ];
				p[ 1 ] = s_gammatable[ p[ 1 ] ];
				p[ 2 ] = s_gammatable[ p[ 2 ] ];
			}
		}
	} else   {
		byte* p = in;

		int c = inwidth * inheight;

		if ( glConfig.deviceSupportsGamma ) {
			for ( int i = 0; i < c; i++, p += 4 ) {
				p[ 0 ] = s_intensitytable[ p[ 0 ] ];
				p[ 1 ] = s_intensitytable[ p[ 1 ] ];
				p[ 2 ] = s_intensitytable[ p[ 2 ] ];
			}
		} else   {
			for ( int i = 0; i < c; i++, p += 4 ) {
				p[ 0 ] = s_gammatable[ s_intensitytable[ p[ 0 ] ] ];
				p[ 1 ] = s_gammatable[ s_intensitytable[ p[ 1 ] ] ];
				p[ 2 ] = s_gammatable[ s_intensitytable[ p[ 2 ] ] ];
			}
		}
	}
}

//	Apply a color blend over a set of pixels
static void R_BlendOverTexture( byte* data, int pixelCount, byte blend[ 4 ] ) {
	int inverseAlpha = 255 - blend[ 3 ];
	int premult[ 3 ];
	premult[ 0 ] = blend[ 0 ] * blend[ 3 ];
	premult[ 1 ] = blend[ 1 ] * blend[ 3 ];
	premult[ 2 ] = blend[ 2 ] * blend[ 3 ];

	for ( int i = 0; i < pixelCount; i++, data += 4 ) {
		data[ 0 ] = ( data[ 0 ] * inverseAlpha + premult[ 0 ] ) >> 9;
		data[ 1 ] = ( data[ 1 ] * inverseAlpha + premult[ 1 ] ) >> 9;
		data[ 2 ] = ( data[ 2 ] * inverseAlpha + premult[ 2 ] ) >> 9;
	}
}

static void GL_CheckErrors() {
	int err = qglGetError();
	if ( err == GL_NO_ERROR ) {
		return;
	}
	if ( r_ignoreGLErrors->integer ) {
		return;
	}
	char s[ 64 ];
	switch ( err ) {
	case GL_INVALID_ENUM:
		String::Cpy( s, "GL_INVALID_ENUM" );
		break;
	case GL_INVALID_VALUE:
		String::Cpy( s, "GL_INVALID_VALUE" );
		break;
	case GL_INVALID_OPERATION:
		String::Cpy( s, "GL_INVALID_OPERATION" );
		break;
	case GL_STACK_OVERFLOW:
		String::Cpy( s, "GL_STACK_OVERFLOW" );
		break;
	case GL_STACK_UNDERFLOW:
		String::Cpy( s, "GL_STACK_UNDERFLOW" );
		break;
	case GL_OUT_OF_MEMORY:
		String::Cpy( s, "GL_OUT_OF_MEMORY" );
		break;
	default:
		String::Sprintf( s, sizeof ( s ), "%i", err );
		break;
	}

	common->FatalError( "GL_CheckErrors: %s", s );
}

static void R_UploadImage( byte* data, int width, int height, bool mipmap,
	bool picmip, bool characterMip, bool lightMap, int* format,
	int* pUploadWidth, int* pUploadHeight, bool noCompress ) {
	//
	// convert to exact power of 2 sizes
	//
	int scaled_width = 1;
	while ( scaled_width < width ) {
		scaled_width <<= 1;
	}
	int scaled_height = 1;
	while ( scaled_height < height ) {
		scaled_height <<= 1;
	}
	if ( r_roundImagesDown->integer && scaled_width > width ) {
		scaled_width >>= 1;
	}
	if ( r_roundImagesDown->integer && scaled_height > height ) {
		scaled_height >>= 1;
	}

	byte* resampledBuffer = NULL;
	if ( scaled_width != width || scaled_height != height ) {
		resampledBuffer = new byte[ scaled_width * scaled_height * 4 ];
		R_ResampleTexture( data, width, height, resampledBuffer, scaled_width, scaled_height );
		data = resampledBuffer;
		width = scaled_width;
		height = scaled_height;
	}

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		if ( characterMip ) {
			scaled_width >>= r_picmip2->integer;
			scaled_height >>= r_picmip2->integer;
		} else   {
			scaled_width >>= r_picmip->integer;
			scaled_height >>= r_picmip->integer;
		}
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( scaled_width > glConfig.maxTextureSize || scaled_height > glConfig.maxTextureSize ) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	//
	// clamp to minimum size
	//
	if ( scaled_width < 1 ) {
		scaled_width = 1;
	}
	if ( scaled_height < 1 ) {
		scaled_height = 1;
	}

	GLenum internalFormat;
	if ( !lightMap ) {
		// scan the texture and verify if the alpha channel is being used or not
		int c = width * height;
		int samples = 3;
		for ( int i = 0; i < c; i++ ) {
			if ( data[ i * 4 + 3 ] != 255 ) {
				samples = 4;
				break;
			}
		}
		// select proper internal format
		if ( samples == 3 ) {
			if ( !noCompress && glConfig.textureCompression == TC_EXT_COMP_S3TC ) {
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			} else if ( !noCompress && glConfig.textureCompression == TC_S3TC )     {
				internalFormat = GL_RGB4_S3TC;
			} else if ( r_texturebits->integer == 16 )     {
				internalFormat = GL_RGB5;
			} else if ( r_texturebits->integer == 32 )     {
				internalFormat = GL_RGB8;
			} else   {
				internalFormat = GL_RGB;
			}
		} else   {
			if ( !noCompress && glConfig.textureCompression == TC_EXT_COMP_S3TC ) {
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			} else if ( r_texturebits->integer == 16 )     {
				internalFormat = GL_RGBA4;
			} else if ( r_texturebits->integer == 32 )     {
				internalFormat = GL_RGBA8;
			} else   {
				internalFormat = GL_RGBA;
			}
		}
	} else   {
		internalFormat = GL_RGB;
	}

	byte* scaledBuffer;

	//	Copy or resample data as appropriate for first MIP level
	if ( scaled_width == width && scaled_height == height ) {
		if ( !mipmap ) {
			qglTexImage2D( GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}
	} else   {
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height ) {
			R_MipMap( data, width, height );
			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
	}

	scaledBuffer = new byte[ 4 * scaled_width * scaled_height ];

	Com_Memcpy( scaledBuffer, data, width * height * 4 );

	R_LightScaleTexture( scaledBuffer, scaled_width, scaled_height, !mipmap );

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	qglTexImage2D( GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );

	if ( mipmap ) {
		int miplevel = 0;
		while ( scaled_width > 1 || scaled_height > 1 ) {
			R_MipMap( scaledBuffer, scaled_width, scaled_height );
			scaled_width >>= 1;
			scaled_height >>= 1;
			if ( scaled_width < 1 ) {
				scaled_width = 1;
			}
			if ( scaled_height < 1 ) {
				scaled_height = 1;
			}
			miplevel++;

			if ( r_colorMipLevels->integer ) {
				R_BlendOverTexture( scaledBuffer, scaled_width * scaled_height, mipBlendColors[ miplevel ] );
			}

			qglTexImage2D( GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
		}
	}

	delete[] scaledBuffer;
done:

	if ( resampledBuffer ) {
		delete[] resampledBuffer;
	}

	if ( mipmap ) {
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
	} else   {
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	if ( glConfig.anisotropicAvailable ) {
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropy );
	}

	GL_CheckErrors();
}

//	return a hash value for the filename
static long generateHashValue( const char* fname ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( fname[ i ] != '\0' ) {
		letter = String::ToLower( fname[ i ] );
		if ( letter == '.' ) {
			break;				// don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';		// damn path names
		}
		hash += ( long )( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( IMAGE_HASH_SIZE - 1 );
	return hash;
}

//	This is the only way any image_t is created
image_t* R_CreateImage( const char* name, byte* data, int width, int height, bool mipmap,
	bool allowPicmip, GLenum glWrapClampMode, bool characterMip ) {
	if ( String::Length( name ) >= MAX_QPATH ) {
		common->Error( "R_CreateImage: \"%s\" is too long\n", name );
	}
	if ( tr.numImages == MAX_DRAWIMAGES ) {
		common->Error( "R_CreateImage: MAX_DRAWIMAGES hit\n" );
	}

	bool isLightmap = !String::NCmp( name, "*lightmap", 9 );

	bool noCompress = false;
	if ( !String::NCmp( name, "*lightmap", 9 ) ) {
		noCompress = true;
	}
	if ( !noCompress && strstr( name, "skies" ) ) {
		noCompress = true;
	}
	if ( !noCompress && strstr( name, "weapons" ) ) {
		// don't compress view weapon skins
		noCompress = true;
	}
	// RF, if the shader hasn't specifically asked for it, don't allow compression
	if ( r_ext_compressed_textures->integer == 2 && ( tr.allowCompress != true ) ) {
		noCompress = true;
	} else if ( r_ext_compressed_textures->integer == 1 && ( tr.allowCompress < 0 ) )       {
		noCompress = true;
	}
	// ydnar: don't compress textures smaller or equal to 128x128 pixels
	else if ( ( width * height ) <= ( 128 * 128 ) ) {
		noCompress = true;
	}

	image_t* image = new image_t;
	Com_Memset( image, 0, sizeof ( image_t ) );
	tr.images[ tr.numImages ] = image;
	tr.numImages++;

	image->mipmap = mipmap;
	image->allowPicmip = allowPicmip;
	image->characterMIP = characterMip;

	String::Cpy( image->imgName, name );

	image->width = width;
	image->height = height;
	image->wrapClampMode = glWrapClampMode;

	qglGenTextures( 1, &image->texnum );

	GL_Bind( image );

	R_UploadImage( data, width, height, mipmap, allowPicmip, characterMip,
		isLightmap, &image->internalFormat, &image->uploadWidth, &image->uploadHeight, noCompress );

	if ( r_clampToEdge->integer && glWrapClampMode == GL_CLAMP ) {
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	}

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode );

	qglBindTexture( GL_TEXTURE_2D, 0 );
	glState.currenttextures[ glState.currenttmu ] = 0;

	long hash = generateHashValue( name );
	image->next = ImageHashTable[ hash ];
	ImageHashTable[ hash ] = image;
	image->hash = hash;

	return image;
}

//	Parameters of the image are the same as when it was created.
//	This is a very bad thing to do, but unfortunately it was a quite common
// practice to use this for dynamicaly changing images. Most, if not all of
// those cases can be rewritten not to do reuploading of data.
void R_ReUploadImage( image_t* image, byte* data ) {
	GL_Bind( image );
	bool isLightmap = !String::NCmp( image->imgName, "*lightmap", 9 );
	R_UploadImage( data, image->width, image->height, image->mipmap, image->allowPicmip, false,
		isLightmap, &image->internalFormat, &image->uploadWidth, &image->uploadHeight, true );
}

//  remove this image from the backupHashTable and make sure it doesn't get overwritten
bool R_TouchImage( image_t* inImage ) {
	if ( inImage == tr.dlightImage ||
		 inImage == tr.whiteImage ||
		 inImage == tr.defaultImage ||
		 inImage->imgName[ 0 ] == '*' ) {	// can't use lightmaps since they might have the same name, but different maps will have different actual lightmap pixels
		return false;
	}

	int hash = inImage->hash;

	image_t* bImage = backupHashTable[ hash ];
	image_t* bImagePrev = NULL;
	while ( bImage ) {
		if ( bImage == inImage ) {
			// add it to the current images
			if ( tr.numImages == MAX_DRAWIMAGES ) {
				common->Error( "R_CreateImage: MAX_DRAWIMAGES hit\n" );
			}

			tr.images[ tr.numImages ] = bImage;

			// remove it from the backupHashTable
			if ( bImagePrev ) {
				bImagePrev->next = bImage->next;
			} else   {
				backupHashTable[ hash ] = bImage->next;
			}

			// add it to the ImageHashTable
			bImage->next = ImageHashTable[ hash ];
			ImageHashTable[ hash ] = bImage;

			// get the new texture
			tr.numImages++;

			return true;
		}

		bImagePrev = bImage;
		bImage = bImage->next;
	}

	return true;
}

image_t* R_FindImage( const char* name ) {
	long hash = generateHashValue( name );
	for ( image_t* image = ImageHashTable[ hash ]; image; image = image->next ) {
		if ( !String::ICmp( name, image->imgName ) ) {
			return image;
		}
	}
	return NULL;
}

static image_t* R_FindCachedImage( const char* name ) {
	if ( !r_cacheShaders->integer ) {
		return NULL;
	}

	if ( !numBackupImages ) {
		return NULL;
	}

	int hash = generateHashValue( name );
	for ( image_t* bImage = backupHashTable[ hash ]; bImage; bImage = bImage->next ) {
		if ( !String::ICmp( name, bImage->imgName ) ) {
			R_TouchImage( bImage );
			return bImage;
		}
	}
	return NULL;
}

//	Finds or loads the given image. Returns NULL if it fails, not a default image.
image_t* R_FindImageFile( const char* name, bool mipmap, bool allowPicmip,
	GLenum glWrapClampMode, int Mode, byte* TransPixels,
	bool characterMIP, bool lightmap ) {
	if ( !name ) {
		return NULL;
	}

	// Ridah, caching
	if ( r_cacheGathering->integer ) {
		if ( GGameType & GAME_WolfSP ) {
			Cbuf_ExecuteText( EXEC_NOW, va( "cache_usedfile image %s %i %i %i %i\n", name, mipmap, allowPicmip, characterMIP, glWrapClampMode ) );
		} else   {
			Cbuf_ExecuteText( EXEC_NOW, va( "cache_usedfile image %s %i %i %i\n", name, mipmap, allowPicmip, glWrapClampMode ) );
		}
	}

	//
	// see if the image is already loaded
	//
	image_t* image = R_FindImage( name );
	if ( image ) {
		// the white image can be used with any set of parms, but other mismatches are errors
		if ( String::Cmp( name, "*white" ) ) {
			if ( image->mipmap != mipmap ) {
				common->DPrintf( S_COLOR_RED "WARNING: reused image %s with mixed mipmap parm\n", name );
			}
			if ( image->allowPicmip != allowPicmip ) {
				common->DPrintf( S_COLOR_RED "WARNING: reused image %s with mixed allowPicmip parm\n", name );
			}
			if ( image->characterMIP != characterMIP ) {
				common->DPrintf( S_COLOR_RED "WARNING: reused image %s with mixed characterMIP parm\n", name );
			}
			if ( image->wrapClampMode != glWrapClampMode ) {
				common->Printf( "WARNING: reused image %s with mixed glWrapClampMode parm\n", name );
			}
		}
		return image;
	}

	// Ridah, check the cache
	// ydnar: don't do this for lightmaps
	if ( !lightmap ) {
		image = R_FindCachedImage( name );
		if ( image ) {
			return image;
		}
	}

	//
	// load the pic from disk
	//
	byte* pic;
	int width;
	int height;
	R_LoadImage( name, &pic, &width, &height, Mode, TransPixels );
	if ( pic == NULL ) {
		//	If we dont get a successful load copy the name and try upper case
		// extension for unix systems, if that fails bail.
		char altname[ MAX_QPATH ];
		String::Cpy( altname, name );
		int len = String::Length( altname );
		altname[ len - 3 ] = String::ToUpper( altname[ len - 3 ] );
		altname[ len - 2 ] = String::ToUpper( altname[ len - 2 ] );
		altname[ len - 1 ] = String::ToUpper( altname[ len - 1 ] );
		common->Printf( "trying %s...\n", altname );
		R_LoadImage( altname, &pic, &width, &height );
		if ( pic == NULL ) {
			return NULL;
		}
	}

	// Arnout: apply lightmap colouring
	int allowCompress = tr.allowCompress;
	if ( lightmap ) {
		R_ProcessLightmap( pic, 4, width, height, pic );
		// ydnar: no texture compression
		tr.allowCompress = -1;
	}

	image = R_CreateImage( name, pic, width, height, mipmap, allowPicmip, glWrapClampMode, characterMIP );
	delete[] pic;

	// ydnar: no texture compression
	if ( lightmap ) {
		tr.allowCompress = allowCompress;
	}

	return image;
}

void R_SetColorMappings() {
	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if ( !glConfig.deviceSupportsGamma ) {
		tr.overbrightBits = 0;		// need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if ( !glConfig.isFullscreen ) {
		tr.overbrightBits = 0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if ( glConfig.colorBits > 16 ) {
		if ( tr.overbrightBits > 2 ) {
			tr.overbrightBits = 2;
		}
	} else   {
		if ( tr.overbrightBits > 1 ) {
			tr.overbrightBits = 1;
		}
	}
	if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	tr.identityLightByte = 255 * tr.identityLight;

	if ( r_intensity->value <= 1 ) {
		Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 3.0f )     {
		Cvar_Set( "r_gamma", "3.0" );
	}

	float g = r_gamma->value;

	int shift = tr.overbrightBits;

	for ( int i = 0; i < 256; i++ ) {
		int inf;
		if ( g == 1 ) {
			inf = i;
		} else   {
			inf = ( int )( 255 * pow( i / 255.0f, 1.0f / g ) + 0.5f );
		}
		inf <<= shift;
		if ( inf < 0 ) {
			inf = 0;
		}
		if ( inf > 255 ) {
			inf = 255;
		}
		s_gammatable[ i ] = inf;
	}

	for ( int i = 0; i < 256; i++ ) {
		int j = ( int )( i * r_intensity->value );
		if ( j > 255 ) {
			j = 255;
		}
		s_intensitytable[ i ] = j;
	}

	if ( glConfig.deviceSupportsGamma ) {
		GLimp_SetGamma( s_gammatable, s_gammatable, s_gammatable );
	}
}

void R_GammaCorrect( byte* Buffer, int BufferSize ) {
	for ( int i = 0; i < BufferSize; i++ ) {
		Buffer[ i ] = s_gammatable[ Buffer[ i ] ];
	}
}

static void R_CreateDefaultImage() {
	// the default image will be a box, to allow you to see the mapping coordinates
	byte data[ DEFAULT_SIZE ][ DEFAULT_SIZE ][ 4 ];
	Com_Memset( data, 32, sizeof ( data ) );
	for ( int x = 0; x < DEFAULT_SIZE; x++ ) {
		data[ 0 ][ x ][ 0 ] =
			data[ 0 ][ x ][ 1 ] =
				data[ 0 ][ x ][ 2 ] =
					data[ 0 ][ x ][ 3 ] = 255;

		data[ x ][ 0 ][ 0 ] =
			data[ x ][ 0 ][ 1 ] =
				data[ x ][ 0 ][ 2 ] =
					data[ x ][ 0 ][ 3 ] = 255;

		data[ DEFAULT_SIZE - 1 ][ x ][ 0 ] =
			data[ DEFAULT_SIZE - 1 ][ x ][ 1 ] =
				data[ DEFAULT_SIZE - 1 ][ x ][ 2 ] =
					data[ DEFAULT_SIZE - 1 ][ x ][ 3 ] = 255;

		data[ x ][ DEFAULT_SIZE - 1 ][ 0 ] =
			data[ x ][ DEFAULT_SIZE - 1 ][ 1 ] =
				data[ x ][ DEFAULT_SIZE - 1 ][ 2 ] =
					data[ x ][ DEFAULT_SIZE - 1 ][ 3 ] = 255;
	}
	tr.defaultImage = R_CreateImage( "*default", ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, true, false, GL_REPEAT );
}

static void R_CreateDlightImage() {
	enum { DLIGHT_SIZE = 16 };
	byte data[ DLIGHT_SIZE ][ DLIGHT_SIZE ][ 4 ];

	// make a centered inverse-square falloff blob for dynamic lighting
	for ( int x = 0; x < DLIGHT_SIZE; x++ ) {
		for ( int y = 0; y < DLIGHT_SIZE; y++ ) {
			float d = ( DLIGHT_SIZE / 2 - 0.5f - x ) * ( DLIGHT_SIZE / 2 - 0.5f - x ) +
					  ( DLIGHT_SIZE / 2 - 0.5f - y ) * ( DLIGHT_SIZE / 2 - 0.5f - y );
			int b = ( int )( 4000 / d );
			if ( b > 255 ) {
				b = 255;
			} else if ( b < 75 )     {
				b = 0;
			}
			data[ y ][ x ][ 0 ] =
				data[ y ][ x ][ 1 ] =
					data[ y ][ x ][ 2 ] = b;
			data[ y ][ x ][ 3 ] = 255;
		}
	}
	tr.dlightImage = R_CreateImage( "*dlight", ( byte* )data, DLIGHT_SIZE, DLIGHT_SIZE, false, false, GL_CLAMP );
}

void R_InitFogTable() {
	float exp = 0.5;
	for ( int i = 0; i < FOG_TABLE_SIZE; i++ ) {
		float d = pow( ( float )i / ( FOG_TABLE_SIZE - 1 ), exp );

		tr.fogTable[ i ] = d;
	}
}

//	Returns a 0.0 to 1.0 fog density value. This is called for each texel
// of the fog texture on startup and for each vertex of transparent shaders
// in fog dynamically
float R_FogFactor( float s, float t ) {
	s -= 1.0 / 512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0 / 32 ) {
		return 0;
	}
	if ( t < 31.0 / 32 ) {
		s *= ( t - 1.0f / 32.0f ) / ( 30.0f / 32.0f );
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	float d = tr.fogTable[ ( int )( s * ( FOG_TABLE_SIZE - 1 ) ) ];

	return d;
}

static void R_CreateFogImage() {
	enum { FOG_S = 256 };
	enum { FOG_T = 32 };
	byte* data = new byte[ FOG_S * FOG_T * 4 ];

	// S is distance, T is depth
	for ( int x = 0; x < FOG_S; x++ ) {
		for ( int y = 0; y < FOG_T; y++ ) {
			float d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[ ( y * FOG_S + x ) * 4 + 0 ] =
				data[ ( y * FOG_S + x ) * 4 + 1 ] =
					data[ ( y * FOG_S + x ) * 4 + 2 ] = 255;
			data[ ( y * FOG_S + x ) * 4 + 3 ] = 255 * d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage( "*fog", data, FOG_S, FOG_T, false, false, GL_CLAMP );
	delete[] data;

	float borderColor[ 4 ];
	borderColor[ 0 ] = 1.0;
	borderColor[ 1 ] = 1.0;
	borderColor[ 2 ] = 1.0;
	borderColor[ 3 ] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

static void R_CreateFogImageET() {
	enum { FOG_S = 16 };
	enum { FOG_T = 16 };
	float borderColor[ 4 ];

	// allocate table for image
	byte* data = new byte[ FOG_S * FOG_T * 4 ];

	// ydnar: new, linear fog texture generating algo for GL_CLAMP_TO_EDGE (OpenGL 1.2+)

	// S is distance, T is depth
	for ( int x = 0; x < FOG_S; x++ ) {
		for ( int y = 0; y < FOG_T; y++ ) {
			int alpha = 270 * ( ( float )x / FOG_S ) * ( ( float )y / FOG_T );		// need slop room for fp round to 0
			if ( alpha < 0 ) {
				alpha = 0;
			} else if ( alpha > 255 )     {
				alpha = 255;
			}

			// ensure edge/corner cases are fully transparent (at 0,0) or fully opaque (at 1,N where N is 0-1.0)
			if ( x == 0 ) {
				alpha = 0;
			} else if ( x == ( FOG_S - 1 ) )       {
				alpha = 255;
			}

			data[ ( y * FOG_S + x ) * 4 + 0 ] =
				data[ ( y * FOG_S + x ) * 4 + 1 ] =
					data[ ( y * FOG_S + x ) * 4 + 2 ] = 255;
			data[ ( y * FOG_S + x ) * 4 + 3 ] = alpha;
		}
	}

	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage( "*fog", ( byte* )data, FOG_S, FOG_T, false, false, GL_CLAMP, false );
	delete[] data;

	// ydnar: the following lines are unecessary for new GL_CLAMP_TO_EDGE fog
	borderColor[ 0 ] = 1.0;
	borderColor[ 1 ] = 1.0;
	borderColor[ 2 ] = 1.0;
	borderColor[ 3 ] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

static void R_CreateBuiltinImages() {
	byte data[ DEFAULT_SIZE ][ DEFAULT_SIZE ][ 4 ];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 255, sizeof ( data ) );
	tr.whiteImage = R_CreateImage( "*white", ( byte* )data, 8, 8, false, false, GL_REPEAT );

	for ( int x = 0; x < 32; x++ ) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[ x ] = R_CreateImage( "*scratch", ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, false, true, GL_CLAMP );
	}

	Com_Memset( data, 0, sizeof ( data ) );
	tr.transparentImage = R_CreateImage( "*transparent", ( byte* )data, 8, 8, false, false, GL_REPEAT );

	R_CreateDlightImage();

	if ( GGameType & GAME_ET ) {
		R_CreateFogImageET();
	} else   {
		R_CreateFogImage();
	}

	R_InitParticleTexture();
}

static void R_LoadCacheImages() {
	if ( numBackupImages ) {
		return;
	}

	int len = FS_ReadFile( "image.cache", NULL );
	if ( len <= 0 ) {
		return;
	}

	void* buf;
	FS_ReadFile( "image.cache", &buf );
	const char* pString = ( char* )buf;

	char* token;
	while ( ( token = String::ParseExt( &pString, true ) ) && token[ 0 ] ) {
		char name[ MAX_QPATH ];
		String::NCpyZ( name, token, sizeof ( name ) );
		int parms[ 4 ];
		for ( int i = 0; i < ( GGameType & ( GAME_WolfSP | GAME_ET ) ? 4 : 3 ); i++ ) {
			token = String::ParseExt( &pString, false );
			parms[ i ] = String::Atoi( token );
		}
		if ( GGameType & GAME_WolfSP ) {
			R_FindImageFile( name, parms[ 0 ], parms[ 1 ], parms[ 3 ], IMG8MODE_Normal, NULL, parms[ 2 ] );
		} else if ( GGameType & GAME_ET )     {
			R_FindImageFile( name, parms[ 0 ], parms[ 1 ], parms[ 2 ], IMG8MODE_Normal, NULL, false, parms[ 3 ] );
		} else   {
			R_FindImageFile( name, parms[ 0 ], parms[ 1 ], parms[ 2 ] );
		}
	}

	FS_FreeFile( buf );
}

void R_InitImages() {
	Com_Memset( ImageHashTable, 0, sizeof ( ImageHashTable ) );

	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();

	// Ridah, load the cache media, if they were loaded previously, they'll be restored from the backupImages
	R_LoadCacheImages();

	if ( GGameType & GAME_QuakeHexen ) {
		R_LoadWadFile();
	}
}

void R_DeleteTextures() {
	for ( int i = 0; i < tr.numImages; i++ ) {
		qglDeleteTextures( 1, &tr.images[ i ]->texnum );
		delete tr.images[ i ];
	}
	Com_Memset( tr.images, 0, sizeof ( tr.images ) );

	tr.numImages = 0;

	Com_Memset( glState.currenttextures, 0, sizeof ( glState.currenttextures ) );
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		qglBindTexture( GL_TEXTURE_2D, 0 );
		GL_SelectTexture( 0 );
		qglBindTexture( GL_TEXTURE_2D, 0 );
	} else   {
		qglBindTexture( GL_TEXTURE_2D, 0 );
	}
}

void GL_TextureMode( const char* string ) {
	int i;

	for ( i = 0; i < 6; i++ ) {
		if ( !String::ICmp( modes[ i ].name, string ) ) {
			break;
		}
	}

	if ( i == 6 ) {
		common->Printf( "bad filter name\n" );
		return;
	}

	gl_filter_min = modes[ i ].minimize;
	gl_filter_max = modes[ i ].maximize;

	// change all the existing mipmap texture objects
	for ( i = 0; i < tr.numImages; i++ ) {
		image_t* glt = tr.images[ i ];
		if ( glt->mipmap ) {
			GL_Bind( glt );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		}
	}
}

void GL_TextureAnisotropy( float anisotropy ) {
	if ( r_ext_texture_filter_anisotropic->integer == 1 ) {
		if ( anisotropy < 1.0 || anisotropy > glConfig.maxAnisotropy ) {
			common->Printf( "anisotropy out of range\n" );
			return;
		}
	}

	gl_anisotropy = anisotropy;

	if ( !glConfig.anisotropicAvailable ) {
		return;
	}

	// change all the existing texture objects
	for ( int i = 0; i < tr.numImages; i++ ) {
		image_t* glt = tr.images[ i ];
		GL_Bind( glt );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropy );
	}
}

void R_ImageList_f() {
	const char* yesno[] =
	{
		"no ", "yes"
	};

	common->Printf( "\n      -w-- -h-- -mm- -if-- wrap --name-------\n" );
	int texels = 0;

	for ( int i = 0; i < tr.numImages; i++ ) {
		image_t* image = tr.images[ i ];

		texels += image->uploadWidth * image->uploadHeight;
		common->Printf( "%4i: %4i %4i  %s ", i, image->uploadWidth, image->uploadHeight, yesno[ image->mipmap ] );
		switch ( image->internalFormat ) {
		case GL_RGB:
			common->Printf( "RGB  " );
			break;
		case GL_RGBA:
			common->Printf( "RGBA " );
			break;
		case GL_RGBA8:
			common->Printf( "RGBA8" );
			break;
		case GL_RGB8:
			common->Printf( "RGB8 " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			common->Printf( "DXT3 " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			common->Printf( "DXT5 " );
			break;
		case GL_RGB4_S3TC:
			common->Printf( "S3TC " );
			break;
		case GL_RGBA4:
			common->Printf( "RGBA4" );
			break;
		case GL_RGB5:
			common->Printf( "RGB5 " );
			break;
		default:
			common->Printf( "???? " );
		}

		switch ( image->wrapClampMode ) {
		case GL_REPEAT:
			common->Printf( "rept " );
			break;
		case GL_CLAMP:
			common->Printf( "clmp " );
			break;
		default:
			common->Printf( "%4i ", image->wrapClampMode );
			break;
		}

		common->Printf( " %s\n", image->imgName );
	}
	common->Printf( " ---------\n" );
	common->Printf( " %i total texels (not including mipmaps)\n", texels );
	common->Printf( " %i total images\n\n", tr.numImages );
}

int R_SumOfUsedImages() {
	int total = 0;
	for ( int i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[ i ]->frameUsed == tr.frameCount ) {
			total += tr.images[ i ]->uploadWidth * tr.images[ i ]->uploadHeight;
		}
	}

	return total;
}

qhandle_t R_GetImageHandle( image_t* Image ) {
	for ( int i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[ i ] == Image ) {
			return i;
		}
	}
	return 0;
}

const char* R_GetImageName( qhandle_t Handle ) {
	return tr.images[ Handle ]->imgName;
}

void R_UploadCinematic( int cols, int rows, const byte* data, int client, bool dirty ) {
	GL_Bind( tr.scratchImage[ client ] );

	int i = 1;
	while ( i < cols ) {
		i <<= 1;
	}
	int j = 1;
	while ( j < rows ) {
		j <<= 1;
	}
	byte* resampled = NULL;
	if ( i != cols || j != rows ) {
		resampled = new byte[ i * j * 4 ];
		R_ResampleTexture( data, cols, rows, resampled, i, j );
		cols = i;
		rows = j;
		data = resampled;
	}

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[ client ]->width || rows != tr.scratchImage[ client ]->height ) {
		tr.scratchImage[ client ]->width = tr.scratchImage[ client ]->uploadWidth = cols;
		tr.scratchImage[ client ]->height = tr.scratchImage[ client ]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	} else   {
		if ( dirty ) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}

	if ( resampled ) {
		delete[] resampled;
	}
}

static void R_CreateOrUpdateTranslatedImageEx( image_t*& image, const char* name, byte* pixels, byte* translation, int width, int height, bool allowPicMip, int mode ) {
	byte* translated = new byte[ width * height ];
	int c = width * height;
	for ( int i = 0; i < c; i++ ) {
		translated[ i ] = translation[ pixels[ i ] ];
	}

	byte* translated32 = R_ConvertImage8To32( translated, width, height, mode );
	delete[] translated;

	if ( !image ) {
		image = R_CreateImage( name, translated32, width, height, false, allowPicMip, GL_CLAMP );
	} else   {
		R_ReUploadImage( image, translated32 );
	}
	delete[] translated32;
}

void R_CreateOrUpdateTranslatedImage( image_t*& image, const char* name, byte* pixels, byte* translation, int width, int height ) {
	R_CreateOrUpdateTranslatedImageEx( image, name, pixels, translation, width, height, false, IMG8MODE_Normal );
}

void R_CreateOrUpdateTranslatedSkin( image_t*& image, const char* name, byte* pixels, byte* translation, int width, int height ) {
	R_CreateOrUpdateTranslatedImageEx( image, name, pixels, translation, width, height, true, IMG8MODE_Skin );
}

static image_t* R_LoadRawFontImage( const char* name, byte* data8, int width, int height ) {
	byte* data32 = R_ConvertImage8To32( data8, width, height, IMG8MODE_Holey );
	image_t* image = R_CreateImage( name, data32, width, height, false, false, GL_CLAMP );
	delete[] data32;

	//	Set nearest filtering mode to make text easier to read.
	GL_Bind( image );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	return image;
}

image_t* R_LoadRawFontImageFromFile( const char* name, int width, int height ) {
	idList<byte> data;
	FS_ReadFile( name, data );
	return R_LoadRawFontImage( name, data.Ptr(), width, height );
}

image_t* R_LoadRawFontImageFromWad( const char* name, int width, int height ) {
	byte* data = ( byte* )R_GetWadLumpByName( name );
	return R_LoadRawFontImage( name, data, width, height );
}

image_t* R_LoadBigFontImage( const char* name ) {
	return R_FindImageFile( name, false, false, GL_CLAMP, IMG8MODE_Holey );
}

image_t* R_LoadQuake2FontImage( const char* name ) {
	image_t* image = R_FindImageFile( name, false, false, GL_CLAMP );
	GL_Bind( image );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	return image;
}

image_t* R_CreateCrosshairImage() {
	byte data[ 64 * 4 ];
	Com_Memset( data, 255, 64 * 4 );
	for ( int i = 0; i < 64; i++ ) {
		data[ i * 4 + 3 ] = cs_data[ i ];
	}
	return R_CreateImage( "crosshair", data, 8, 8, false, false, GL_CLAMP );
}

image_t* R_CachePic( const char* path ) {
	image_t* pic = R_FindImageFile( path, false, false, GL_CLAMP );
	if ( !pic ) {
		common->FatalError( "R_CachePic: failed to load %s", path );
	}
	return pic;
}

qhandle_t R_CacheShaderRepeat( const char* path ) {
	image_t* pic = R_FindImageFile( path, false, false, GL_REPEAT );
	if ( !pic ) {
		common->FatalError( "R_CachePic: failed to load %s", path );
	}
	return R_Build2DShaderFromImage( pic )->index;
}

image_t* R_CachePicWithTransPixels( const char* path, byte* TransPixels ) {
	image_t* pic = R_FindImageFile( path, false, false, GL_CLAMP, IMG8MODE_Normal, TransPixels );
	if ( !pic ) {
		common->FatalError( "R_CachePic: failed to load %s", path );
	}
	return pic;
}

static image_t* R_RegisterPic( const char* name, GLenum wrapClampMode ) {
	if ( name[ 0 ] != '/' && name[ 0 ] != '\\' ) {
		char fullname[ MAX_QPATH ];
		String::Sprintf( fullname, sizeof ( fullname ), "pics/%s.pcx", name );
		return R_FindImageFile( fullname, false, false, wrapClampMode );
	} else   {
		return R_FindImageFile( name + 1, false, false, wrapClampMode );
	}
}

image_t* R_RegisterPic( const char* name ) {
	return R_RegisterPic( name, GL_CLAMP );
}

image_t* R_RegisterPicRepeat( const char* name ) {
	return R_RegisterPic( name, GL_REPEAT );
}

int R_GetImageWidth( image_t* pic ) {
	return pic->width;
}

int R_GetImageHeight( image_t* pic ) {
	return pic->height;
}

void R_GetPicSize( int* w, int* h, const char* pic ) {
	image_t* gl = R_RegisterPic( pic );
	if ( !gl ) {
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

void R_BackupImages() {
	if ( !r_cache->integer ) {
		return;
	}
	if ( !r_cacheShaders->integer ) {
		return;
	}

	// backup the ImageHashTable
	Com_Memcpy( backupHashTable, ImageHashTable, sizeof ( backupHashTable ) );

	// pretend we have cleared the list
	numBackupImages = tr.numImages;
	tr.numImages = 0;

	Com_Memset( glState.currenttextures, 0, sizeof ( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else   {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

static void R_PurgeImage( image_t* image ) {
	qglDeleteTextures( 1, &image->texnum );
	delete image;

	Com_Memset( glState.currenttextures, 0, sizeof ( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else   {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

//	Can specify the number of Images to purge this call (used for background purging)
void R_PurgeBackupImages( int purgeCount ) {
	static int lastPurged = 0;

	if ( !numBackupImages ) {
		// nothing to purge
		lastPurged = 0;
		return;
	}

	R_SyncRenderThread();

	int cnt = 0;
	for ( int i = lastPurged; i < IMAGE_HASH_SIZE; ) {
		lastPurged = i;
		image_t* image = backupHashTable[ i ];
		if ( image ) {
			// kill it
			backupHashTable[ i ] = image->next;
			R_PurgeImage( image );
			cnt++;

			if ( cnt >= purgeCount ) {
				return;
			}
		} else   {
			i++;	// no images in this slot, so move to the next one
		}
	}

	// all done
	numBackupImages = 0;
	lastPurged = 0;
}

int R_GetTextureId( const char* name ) {
	for ( int i = 0; i < tr.numImages; i++ ) {
		if ( !String::Cmp( name, tr.images[ i ]->imgName ) ) {
			return i;
		}
	}
	return -1;
}
