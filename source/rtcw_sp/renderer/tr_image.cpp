/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		tr_image.c
 *
 * desc:
 *
*/

#include "tr_local.h"

long generateHashValue( const char *fname );

extern byte s_intensitytable[256];
extern unsigned char s_gammatable[256];

extern int gl_filter_min;
extern int gl_filter_max;

#define IMAGE_HASH_SIZE      4096
extern image_t*        ImageHashTable[IMAGE_HASH_SIZE];

// Ridah, in order to prevent zone fragmentation, all images will
// be read into this buffer. In order to keep things as fast as possible,
// we'll give it a starting value, which will account for the majority of
// images, but allow it to grow if the buffer isn't big enough
#define R_IMAGE_BUFFER_SIZE     ( 512 * 512 * 4 )     // 512 x 512 x 32bit

typedef enum {
	BUFFER_IMAGE,
	BUFFER_SCALED,
	BUFFER_RESAMPLED,
	BUFFER_MAX_TYPES
} bufferMemType_t;

int imageBufferSize[BUFFER_MAX_TYPES] = {0,0,0};
void        *imageBufferPtr[BUFFER_MAX_TYPES] = {NULL,NULL,NULL};

void *R_GetImageBuffer( int size, bufferMemType_t bufferType ) {
	if ( imageBufferSize[bufferType] < R_IMAGE_BUFFER_SIZE && size <= imageBufferSize[bufferType] ) {
		imageBufferSize[bufferType] = R_IMAGE_BUFFER_SIZE;
		imageBufferPtr[bufferType] = malloc( imageBufferSize[bufferType] );
	}
	if ( size > imageBufferSize[bufferType] ) {   // it needs to grow
		if ( imageBufferPtr[bufferType] ) {
			free( imageBufferPtr[bufferType] );
		}
		imageBufferSize[bufferType] = size;
		imageBufferPtr[bufferType] = malloc( imageBufferSize[bufferType] );
	}

	return imageBufferPtr[bufferType];
}

void R_FreeImageBuffer( void ) {
	int bufferType;
	for ( bufferType = 0; bufferType < BUFFER_MAX_TYPES; bufferType++ ) {
		if ( !imageBufferPtr[bufferType] ) {
			return;
		}
		free( imageBufferPtr[bufferType] );
		imageBufferSize[bufferType] = 0;
		imageBufferPtr[bufferType] = NULL;
	}
}

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

struct textureMode_t
{
	char *name;
	int minimize, maximize;
};

extern textureMode_t modes[];

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string ) {
	int i;
	image_t *glt;

	for ( i = 0 ; i < 6 ; i++ ) {
		if ( !String::ICmp( modes[i].name, string ) ) {
			break;
		}
	}

	if ( i == 6 ) {
		ri.Printf( PRINT_ALL, "bad filter name\n" );
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for ( i = 0 ; i < tr.numImages ; i++ ) {
		glt = tr.images[ i ];
		if ( glt->mipmap ) {
			GL_Bind( glt );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		}
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int total;
	int i;

	total = 0;
	for ( i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[i]->frameUsed == tr.frameCount ) {
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f( void ) {
	int i;
	image_t *image;
	int texels;
	const char *yesno[] = {
		"no ", "yes"
	};

	ri.Printf( PRINT_ALL, "\n      -w-- -h-- -mm- -TMU- -if-- wrap --name-------\n" );
	texels = 0;

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[ i ];

		texels += image->uploadWidth * image->uploadHeight;
		ri.Printf( PRINT_ALL,  "%4i: %4i %4i  %s   ",
				   i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap]);
		switch ( image->internalFormat ) {
		case 1:
			ri.Printf( PRINT_ALL, "I    " );
			break;
		case 2:
			ri.Printf( PRINT_ALL, "IA   " );
			break;
		case 3:
			ri.Printf( PRINT_ALL, "RGB  " );
			break;
		case 4:
			ri.Printf( PRINT_ALL, "RGBA " );
			break;
		case GL_RGBA8:
			ri.Printf( PRINT_ALL, "RGBA8" );
			break;
		case GL_RGB8:
			ri.Printf( PRINT_ALL, "RGB8" );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			ri.Printf( PRINT_ALL, "DXT5 " );
			break;
		case GL_RGB4_S3TC:
			ri.Printf( PRINT_ALL, "S3TC4" );
			break;
		case GL_RGBA4:
			ri.Printf( PRINT_ALL, "RGBA4" );
			break;
		case GL_RGB5:
			ri.Printf( PRINT_ALL, "RGB5 " );
			break;
		default:
			ri.Printf( PRINT_ALL, "???? " );
		}

		switch ( image->wrapClampMode ) {
		case GL_REPEAT:
			ri.Printf( PRINT_ALL, "rept " );
			break;
		case GL_CLAMP:
			ri.Printf( PRINT_ALL, "clmp " );
			break;
		default:
			ri.Printf( PRINT_ALL, "%4i ", image->wrapClampMode );
			break;
		}

		ri.Printf( PRINT_ALL, " %s\n", image->imgName );
	}
	ri.Printf( PRINT_ALL, " ---------\n" );
	ri.Printf( PRINT_ALL, " %i total texels (not including mipmaps)\n", texels );
	ri.Printf( PRINT_ALL, " %i total images\n\n", tr.numImages );
}

//=======================================================================

/*
================
R_CreateDlightImage
================
*/
#define DLIGHT_SIZE 16
static void R_CreateDlightImage( void ) {
	int x,y;
	byte data[DLIGHT_SIZE][DLIGHT_SIZE][4];
	int b;

	// make a centered inverse-square falloff blob for dynamic lighting
	for ( x = 0 ; x < DLIGHT_SIZE ; x++ ) {
		for ( y = 0 ; y < DLIGHT_SIZE ; y++ ) {
			float d;

			d = ( DLIGHT_SIZE / 2 - 0.5f - x ) * ( DLIGHT_SIZE / 2 - 0.5f - x ) +
				( DLIGHT_SIZE / 2 - 0.5f - y ) * ( DLIGHT_SIZE / 2 - 0.5f - y );
			b = 4000 / d;
			if ( b > 255 ) {
				b = 255;
			} else if ( b < 75 ) {
				b = 0;
			}
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}
	tr.dlightImage = R_CreateImage( "*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, qfalse, qfalse, GL_CLAMP, false, false );
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int i;
	float d;
	float exp;

	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow( (float)i / ( FOG_TABLE_SIZE - 1 ), exp );

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float   R_FogFactor( float s, float t ) {
	float d;

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

	d = tr.fogTable[ (int)( s * ( FOG_TABLE_SIZE - 1 ) ) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define FOG_S   256
#define FOG_T   32
static void R_CreateFogImage( void ) {
	int x,y;
	byte    *data;
	float g;
	float d;
	float borderColor[4];

	data = (byte*)ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	g = 2.0;

	// S is distance, T is depth
	for ( x = 0 ; x < FOG_S ; x++ ) {
		for ( y = 0 ; y < FOG_T ; y++ ) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[( y * FOG_S + x ) * 4 + 0] =
				data[( y * FOG_S + x ) * 4 + 1] =
					data[( y * FOG_S + x ) * 4 + 2] = 255;
			data[( y * FOG_S + x ) * 4 + 3] = 255 * d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage( "*fog", (byte *)data, FOG_S, FOG_T, qfalse, qfalse, GL_CLAMP, false, false );
	ri.Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateDefaultImage
==================
*/
#define DEFAULT_SIZE    16
static void R_CreateDefaultImage( void ) {
	int x;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		data[0][x][0] =
			data[0][x][1] =
				data[0][x][2] = 0; //----(SA) to make the default grid noticable but not blinding
		data[0][x][3] = 255;

		data[x][0][0] =
			data[x][0][1] =
				data[x][0][2] = 0; //----(SA) to make the default grid noticable but not blinding
		data[x][0][3] = 255;

		data[DEFAULT_SIZE - 1][x][0] =
			data[DEFAULT_SIZE - 1][x][1] =
				data[DEFAULT_SIZE - 1][x][2] = 0; //----(SA) to make the default grid noticable but not blinding
		data[DEFAULT_SIZE - 1][x][3] = 255;

		data[x][DEFAULT_SIZE - 1][0] =
			data[x][DEFAULT_SIZE - 1][1] =
				data[x][DEFAULT_SIZE - 1][2] = 0; //----(SA) to make the default grid noticable but not blinding
		data[x][DEFAULT_SIZE - 1][3] = 255;
	}
	tr.defaultImage = R_CreateImage( "*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qfalse, GL_REPEAT, false, false );
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	int x,y;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage( "*white", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT, false, false );

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		for ( y = 0 ; y < DEFAULT_SIZE ; y++ ) {
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;
		}
	}

	for ( x = 0; x < 32; x++ ) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage( "*scratch", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qtrue, GL_CLAMP, false, false );
	}

	R_CreateDlightImage();
	R_CreateFogImage();
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	int i, j;
	float g;
	int inf;
	int shift;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if ( !glConfig.deviceSupportsGamma ) {
		tr.overbrightBits = 0;      // need hardware gamma for overbright
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
	} else {
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
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 3.0f ) {
		ri.Cvar_Set( "r_gamma", "3.0" );
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow( i / 255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		if ( inf < 0 ) {
			inf = 0;
		}
		if ( inf > 255 ) {
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for ( i = 0 ; i < 256 ; i++ ) {
		j = i * r_intensity->value;
		if ( j > 255 ) {
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if ( glConfig.deviceSupportsGamma ) {
		GLimp_SetGamma( s_gammatable, s_gammatable, s_gammatable );
	}
}

/*
===============
R_InitImages
===============
*/
void    R_InitImages( void ) {
	memset( ImageHashTable, 0, sizeof( ImageHashTable ) );

	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();

	// Ridah, load the cache media, if they were loaded previously, they'll be restored from the backupImages
	R_LoadCacheImages();
	// done.
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures( void ) {
	int i;

	for ( i = 0; i < tr.numImages ; i++ ) {
		qglDeleteTextures( 1, &tr.images[i]->texnum );
	}
	memset( tr.images, 0, sizeof( tr.images ) );

	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static char com_token[MAX_TOKEN_CHARS_Q3];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while ( ( c = *data ) <= ' ' ) {
			if ( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' ) {
			while ( *data && *data != '\n' )
				data++;
		}
		// skip /* */ comments
		else if ( c == '/' && data[1] == '*' ) {
			while ( *data && ( *data != '*' || data[1] != '/' ) )
			{
				data++;
			}
			if ( *data ) {
				data += 2;
			}
		} else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if ( c == '\"' ) {
		data++;
		while ( 1 )
		{
			c = *data++;
			if ( c == '\"' || !c ) {
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if ( len < MAX_TOKEN_CHARS_Q3 ) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if ( len < MAX_TOKEN_CHARS_Q3 ) {
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while ( c > 32 && c != ',' );

	if ( len == MAX_TOKEN_CHARS_Q3 ) {
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS_Q3);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


//----(SA) added so client can see what model or scale for the model was specified in a skin
/*
==============
RE_GetSkinModel
==============
*/
qboolean RE_GetSkinModel( qhandle_t skinid, const char *type, char *name ) {
	int i;
	skin_t      *bar;

	bar = tr.skins[skinid];

	if ( !String::ICmp( type, "playerscale" ) ) {    // client is requesting scale from the skin rather than a model
		String::Sprintf( name, MAX_QPATH, "%.2f %.2f %.2f", bar->scale[0], bar->scale[1], bar->scale[2] );
		return qtrue;
	}

	for ( i = 0; i < bar->numModels; i++ )
	{
		if ( !String::ICmp( bar->models[i]->type, type ) ) { // (SA) whoops, should've been this way
			String::NCpyZ( name, bar->models[i]->model, sizeof( bar->models[i]->model ) );
			return qtrue;
		}
	}
	return qfalse;
}

/*
==============
RE_GetShaderFromModel
	return a shader index for a given model's surface
	'withlightmap' set to '0' will create a new shader that is a copy of the one found
	on the model, without the lighmap stage, if the shader has a lightmap stage

	NOTE: only works for bmodels right now.  Could modify for other models (md3's etc.)
==============
*/
qhandle_t RE_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap ) {
	model_t     *model;
	mbrush46_model_t    *bmodel;
	mbrush46_surface_t  *surf;
	shader_t    *shd;

	if ( surfnum < 0 ) {
		surfnum = 0;
	}

	model = R_GetModelByHandle( modelid );  // (SA) should be correct now

	if ( model ) {
		bmodel  = model->q3_bmodel;
		if ( bmodel && bmodel->firstSurface ) {
			if ( surfnum >= bmodel->numSurfaces ) { // if it's out of range, return the first surface
				surfnum = 0;
			}

			surf = bmodel->firstSurface + surfnum;
//			if(surf->shader->lightmapIndex != LIGHTMAP_NONE) {
			if ( surf->shader->lightmapIndex > LIGHTMAP_NONE ) {
				image_t *image;
				long hash;
				qboolean mip = qtrue;   // mip generation on by default

				// get mipmap info for original texture
				hash = generateHashValue( surf->shader->name );
				for ( image = ImageHashTable[hash]; image; image = image->next ) {
					if ( !String::Cmp( surf->shader->name, image->imgName ) ) {
						mip = image->mipmap;
						break;
					}
				}
				shd = R_FindShader( surf->shader->name, LIGHTMAP_NONE, mip );
				shd->stages[0]->rgbGen = CGEN_LIGHTING_DIFFUSE; // (SA) new
			} else {
				shd = surf->shader;
			}

			return shd->index;
		}
	}

	return 0;
}

//----(SA) end

/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin( const char *name ) {
	qhandle_t hSkin;
	skin_t      *skin;
	skinSurface_t   *surf;
	skinModel_t *model;          //----(SA) added
	char        *text, *text_p;
	char        *token;
	char surfName[MAX_QPATH];

	if ( !name || !name[0] ) {
		Com_Printf( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( String::Length( name ) >= MAX_QPATH ) {
		Com_Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !String::ICmp( skin->name, name ) ) {
			if ( skin->numSurfaces == 0 ) {
				return 0;       // default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}


//----(SA)	moved things around slightly to fix the problem where you restart
//			a map that has ai characters who had invalid skin names entered
//			in thier "skin" or "head" field

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// If not a .skin file, load as a single shader
	if ( String::Cmp( name + String::Length( name ) - 5, ".skin" ) ) {
		tr.numSkins++;
		skin = (skin_t*)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
		tr.skins[hSkin] = skin;
		String::NCpyZ( skin->name, name, sizeof( skin->name ) );
		skin->numSurfaces   = 0;
		skin->numModels     = 0;    //----(SA) added
		skin->numSurfaces = 1;
		skin->surfaces[0] = (skinSurface_t*)ri.Hunk_Alloc( sizeof( skin->surfaces[0] ), h_low );
		skin->surfaces[0]->shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
	ri.FS_ReadFile( name, (void **)&text );
	if ( !text ) {
		return 0;
	}

	tr.numSkins++;
	skin = (skin_t*)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	String::NCpyZ( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces   = 0;
	skin->numModels     = 0;    //----(SA) added

//----(SA)	end

	text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		String::NCpyZ( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		String::ToLower( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}

		if ( strstr( token, "md3_" ) ) {  // this is specifying a model
			model = skin->models[ skin->numModels ] = (skinModel_t*)ri.Hunk_Alloc( sizeof( *skin->models[0] ), h_low );
			String::NCpyZ( model->type, token, sizeof( model->type ) );

			// get the model name
			token = CommaParse( &text_p );

			String::NCpyZ( model->model, token, sizeof( model->model ) );

			skin->numModels++;
			continue;
		}

//----(SA)	added
		if ( strstr( token, "playerscale" ) ) {
			token = CommaParse( &text_p );
			skin->scale[0] = String::Atof( token );   // uniform scaling for now
			skin->scale[1] = String::Atof( token );
			skin->scale[2] = String::Atof( token );
			continue;
		}
//----(SA) end

		// parse the shader name
		token = CommaParse( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = (skinSurface_t*)ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		String::NCpyZ( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text );


	// never let a skin have 0 shaders

	//----(SA)	allow this for the (current) special case of the loper's upper body
	//			(it's upper body has no surfaces, only tags)
	if ( skin->numSurfaces == 0 ) {
		if ( !( strstr( name, "loper" ) && strstr( name, "upper" ) ) ) {
			return 0;       // use default skin
		}
	}

	return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void    R_InitSkins( void ) {
	skin_t      *skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = (skin_t*)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	String::NCpyZ( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = (skinSurface_t*)ri.Hunk_Alloc( sizeof( *skin->surfaces ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t  *R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void    R_SkinList_f( void ) {
	int i, j;
	skin_t      *skin;

	ri.Printf( PRINT_ALL, "------------------\n" );

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n",
					   skin->surfaces[j]->name, skin->surfaces[j]->shader->name );
		}
	}
	ri.Printf( PRINT_ALL, "------------------\n" );
}

//==========================================================================================
// Ridah, caching system

extern int numBackupImages;
extern image_t  *backupHashTable[IMAGE_HASH_SIZE];

/*
===============
R_CacheImageFree
===============
*/
void R_CacheImageFree( void *ptr ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		free( ptr );
		//ri.Free( ptr );
	}
}

/*
===============
R_PurgeImage
===============
*/
void R_PurgeImage( image_t *image ) {

	qglDeleteTextures( 1, &image->texnum );

	R_CacheImageFree( image );

	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}


/*
===============
R_PurgeBackupImages

  Can specify the number of Images to purge this call (used for background purging)
===============
*/
void R_PurgeBackupImages( int purgeCount ) {
	int i, cnt;
	static int lastPurged = 0;
	image_t *image;

	if ( !numBackupImages ) {
		// nothing to purge
		lastPurged = 0;
		return;
	}

	R_SyncRenderThread();

	cnt = 0;
	for ( i = lastPurged; i < IMAGE_HASH_SIZE; ) {
		lastPurged = i;
		// TTimo: suggests () around assignment used as truth value
		if ( ( image = backupHashTable[i] ) ) {
			// kill it
			backupHashTable[i] = image->next;
			R_PurgeImage( image );
			cnt++;

			if ( cnt >= purgeCount ) {
				return;
			}
		} else {
			i++;    // no images in this slot, so move to the next one
		}
	}

	// all done
	numBackupImages = 0;
	lastPurged = 0;
}

/*
===============
R_BackupImages
===============
*/
void R_BackupImages( void ) {

	if ( !r_cache->integer ) {
		return;
	}
	if ( !r_cacheShaders->integer ) {
		return;
	}

	// backup the ImageHashTable
	memcpy( backupHashTable, ImageHashTable, sizeof( backupHashTable ) );

	// pretend we have cleared the list
	numBackupImages = tr.numImages;
	tr.numImages = 0;

	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

/*
===============
R_LoadCacheImages
===============
*/
void R_LoadCacheImages( void ) {
	int len;
	byte *buf;
	char    *token;
	const char* pString;
	char name[MAX_QPATH];
	int parms[3], i;

	if ( numBackupImages ) {
		return;
	}

	len = ri.FS_ReadFile( "image.cache", NULL );

	if ( len <= 0 ) {
		return;
	}

	buf = (byte *)ri.Hunk_AllocateTempMemory( len );
	ri.FS_ReadFile( "image.cache", (void **)&buf );
	pString = (char*)buf;   //DAJ added (char*)

	while ( ( token = String::ParseExt( &pString, qtrue ) ) && token[0] ) {
		String::NCpyZ( name, token, sizeof( name ) );
		for ( i = 0; i < 4; i++ ) {
			token = String::ParseExt( &pString, qfalse );
			parms[i] = String::Atoi( token );
		}
		R_FindImageFile( name, parms[0], parms[1], parms[3], false, IMG8MODE_Normal, NULL, parms[2] );
	}

	ri.Hunk_FreeTempMemory( buf );
}
// done.
//==========================================================================================
