/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

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

extern int gl_filter_min;
extern int gl_filter_max;

extern float gl_anisotropy;

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
//DAJ TEST		imageBufferPtr[bufferType] = Z_Malloc( imageBufferSize[bufferType] );
	}
	if ( size > imageBufferSize[bufferType] ) {   // it needs to grow
		if ( imageBufferPtr[bufferType] ) {
			free( imageBufferPtr[bufferType] );
		}
//DAJ TEST		Z_Free( imageBufferPtr[bufferType] );
		imageBufferSize[bufferType] = size;
		imageBufferPtr[bufferType] = malloc( imageBufferSize[bufferType] );
//DAJ TEST		imageBufferPtr[bufferType] = Z_Malloc( imageBufferSize[bufferType] );
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
//DAJ TEST		Z_Free( imageBufferPtr[bufferType] );
		imageBufferSize[bufferType] = 0;
		imageBufferPtr[bufferType] = NULL;
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
		GL_Bind( glt );
		// ydnar: for allowing lightmap debugging
		if ( glt->mipmap ) {
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		} else
		{
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		}
	}
}

/*
===============
GL_TextureAnisotropy
===============
*/
void GL_TextureAnisotropy( float anisotropy ) {
	int i;
	image_t *glt;

	if ( r_ext_texture_filter_anisotropic->integer == 1 ) {
		if ( anisotropy < 1.0 || anisotropy > glConfig.maxAnisotropy ) {
			ri.Printf( PRINT_ALL, "anisotropy out of range\n" );
			return;
		}
	}

	gl_anisotropy = anisotropy;

	if ( !glConfig.anisotropicAvailable ) {
		return;
	}

	// change all the existing texture objects
	for ( i = 0 ; i < tr.numImages ; i++ ) {
		glt = tr.images[ i ];
		GL_Bind( glt );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropy );
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int total;
	int i, fc = ( tr.frameCount - 1 );

	total = 0;
	for ( i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[i]->frameUsed == fc ) {
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

	ri.Printf( PRINT_ALL, "\n      -w-- -h-- -mm- -TMU- GLname -if-- wrap --name-------\n" );
	texels = 0;

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[ i ];

		texels += image->uploadWidth * image->uploadHeight;
		ri.Printf( PRINT_ALL,  "%4i: %4i %4i  %s   %5d ",
				   i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap], image->texnum );
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
			ri.Printf( PRINT_ALL, "RGB8 " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			ri.Printf( PRINT_ALL, "DXT3 " );
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

/*
==============
RE_GetSkinModel
==============
*/
qboolean RE_GetSkinModel( qhandle_t skinid, const char *type, char *name ) {
	int i;
	int hash;
	skin_t  *skin;

	skin = tr.skins[skinid];
	hash = Com_HashKey( (char *)type, String::Length( type ) );

	for ( i = 0; i < skin->numModels; i++ ) {
		if ( hash != skin->models[i]->hash ) {
			continue;
		}
		if ( !String::ICmp( skin->models[i]->type, type ) ) {
			// (SA) whoops, should've been this way
			String::NCpyZ( name, skin->models[i]->model, sizeof( skin->models[i]->model ) );
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
			// RF, check for null shader (can happen on func_explosive's with botclips attached)
			if ( !surf->shader ) {
				return 0;
			}
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

		if ( !String::NICmp( token, "tag_", 4 ) ) {
			continue;
		}

		if ( !String::NICmp( token, "md3_", 4 ) ) {
			// this is specifying a model
			model = skin->models[ skin->numModels ] = (skinModel_t*)ri.Hunk_Alloc( sizeof( *skin->models[0] ), h_low );
			String::NCpyZ( model->type, token, sizeof( model->type ) );
			model->hash = Com_HashKey( model->type, sizeof( model->type ) );

			// get the model name
			token = CommaParse( &text_p );

			String::NCpyZ( model->model, token, sizeof( model->model ) );

			skin->numModels++;
			continue;
		}

		// parse the shader name
		token = CommaParse( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = (skinSurface_t*)ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		String::NCpyZ( surf->name, surfName, sizeof( surf->name ) );
		surf->hash = Com_HashKey( surf->name, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text );

	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;       // use default skin
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
//		ri.Free( ptr );
		Mem_Free( ptr );
//DAJ TEST		ri.Free( ptr );	//DAJ was CO
	}
}

/*
===============
R_PurgeImage
===============
*/
void R_PurgeImage( image_t *image ) {

	//%	texnumImages[image->texnum - 1024] = NULL;

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
		// TTimo: assignment used as truth value
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

//bani
/*
R_GetTextureId
*/
int R_GetTextureId( const char *name ) {
	int i;

//	ri.Printf( PRINT_ALL, "R_GetTextureId [%s].\n", name );

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		if ( !String::Cmp( name, tr.images[ i ]->imgName ) ) {
//			ri.Printf( PRINT_ALL, "Found textureid %d\n", i );
			return i;
		}
	}

//	ri.Printf( PRINT_ALL, "Image not found.\n" );
	return -1;
}
