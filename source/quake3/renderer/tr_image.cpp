/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_image.c
#include "tr_local.h"

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int	total;
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
	int		i;
	image_t	*image;
	int		texels;
	const char *yesno[] = {
		"no ", "yes"
	};

	ri.Printf (PRINT_ALL, "\n      -w-- -h-- -mm- -if-- wrap --name-------\n");
	texels = 0;

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[ i ];

		texels += image->uploadWidth*image->uploadHeight;
		ri.Printf (PRINT_ALL,  "%4i: %4i %4i  %s ",
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
		case GL_RGB4_S3TC:
			ri.Printf( PRINT_ALL, "S3TC " );
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
	ri.Printf (PRINT_ALL, " ---------\n");
	ri.Printf (PRINT_ALL, " %i total texels (not including mipmaps)\n", texels);
	ri.Printf (PRINT_ALL, " %i total images\n\n", tr.numImages );
}

//===================================================================

/*
================
R_CreateDlightImage
================
*/
#define	DLIGHT_SIZE	16
static void R_CreateDlightImage( void ) {
	int		x,y;
	byte	data[DLIGHT_SIZE][DLIGHT_SIZE][4];
	int		b;

	// make a centered inverse-square falloff blob for dynamic lighting
	for (x=0 ; x<DLIGHT_SIZE ; x++) {
		for (y=0 ; y<DLIGHT_SIZE ; y++) {
			float	d;

			d = ( DLIGHT_SIZE/2 - 0.5f - x ) * ( DLIGHT_SIZE/2 - 0.5f - x ) +
				( DLIGHT_SIZE/2 - 0.5f - y ) * ( DLIGHT_SIZE/2 - 0.5f - y );
			b = 4000 / d;
			if (b > 255) {
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
	tr.dlightImage = R_CreateImage("*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, qfalse, qfalse, GL_CLAMP, false);
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int		i;
	float	d;
	float	exp;
	
	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow ( (float)i/(FOG_TABLE_SIZE-1), exp );

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
float	R_FogFactor( float s, float t ) {
	float	d;

	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define	FOG_S	256
#define	FOG_T	32
static void R_CreateFogImage( void ) {
	int		x,y;
	byte	*data;
	float	g;
	float	d;
	float	borderColor[4];

	data = (byte*)ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	g = 2.0;

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++) {
		for (y=0 ; y<FOG_T ; y++) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[(y*FOG_S+x)*4+0] = 
			data[(y*FOG_S+x)*4+1] = 
			data[(y*FOG_S+x)*4+2] = 255;
			data[(y*FOG_S+x)*4+3] = 255*d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", (byte *)data, FOG_S, FOG_T, qfalse, qfalse, GL_CLAMP, false);
	ri.Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	int		x,y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage("*white", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT, false);

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for (x=0 ; x<DEFAULT_SIZE ; x++) {
		for (y=0 ; y<DEFAULT_SIZE ; y++) {
			data[y][x][0] = 
			data[y][x][1] = 
			data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;			
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT, false);


	for(x=0;x<32;x++) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage("*scratch", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qtrue, GL_CLAMP, false);
	}

	R_CreateDlightImage();
	R_CreateFogImage();
}


/*
===============
R_InitImages
===============
*/
void	R_InitImages( void ) {
	R_CommonInitImages();

	// create default texture and white texture
	R_CreateBuiltinImages();
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures( void ) {
	int		i;

	for ( i=0; i<tr.numImages ; i++ ) {
		qglDeleteTextures( 1, &tr.images[i]->texnum );
	}
	Com_Memset( tr.images, 0, sizeof( tr.images ) );

	tr.numImages = 0;

	Com_Memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
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
	static	char	com_token[MAX_TOKEN_CHARS];

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
		while( (c = *data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin( const char *name ) {
	qhandle_t	hSkin;
	skin_t		*skin;
	skinSurface_t	*surf;
	char		*text, *text_p;
	char		*token;
	char		surfName[MAX_QPATH];

	if ( !name || !name[0] ) {
		Com_Printf( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( QStr::Length( name ) >= MAX_QPATH ) {
		Com_Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !QStr::ICmp( skin->name, name ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = (skin_t*)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	QStr::NCpyZ( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// If not a .skin file, load as a single shader
	if ( QStr::Cmp( name + QStr::Length( name ) - 5, ".skin" ) ) {
		skin->numSurfaces = 1;
		skin->surfaces[0] = (skinSurface_t*)ri.Hunk_Alloc( sizeof(skin->surfaces[0]), h_low );
		skin->surfaces[0]->shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
    FS_ReadFile( name, (void **)&text );
	if ( !text ) {
		return 0;
	}

	text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		QStr::NCpyZ( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		QStr::ToLower( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}
		
		// parse the shader name
		token = CommaParse( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = (skinSurface_t*)ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		QStr::NCpyZ( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	FS_FreeFile( text );


	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

	return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void	R_InitSkins( void ) {
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = (skin_t*)ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	QStr::NCpyZ( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = (skinSurface_t*)ri.Hunk_Alloc( sizeof( *skin->surfaces ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t	*R_GetSkinByHandle( qhandle_t hSkin ) {
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
void	R_SkinList_f( void ) {
	int			i, j;
	skin_t		*skin;

	ri.Printf (PRINT_ALL, "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n", 
				skin->surfaces[j]->name, skin->surfaces[j]->shader->name );
		}
	}
	ri.Printf (PRINT_ALL, "------------------\n");
}

