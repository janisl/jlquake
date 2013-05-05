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

#include "local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"

//	This is unfortunate, but the skin files aren't compatable with our normal
// parsing rules.
static const char* CommaParse( char** data_p ) {
	int c = 0, len;
	char* data;
	static char com_token[ MAX_TOKEN_CHARS_Q3 ];

	data = *data_p;
	len = 0;
	com_token[ 0 ] = 0;

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
		if ( c == '/' && data[ 1 ] == '/' ) {
			while ( *data && *data != '\n' ) {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c == '/' && data[ 1 ] == '*' ) {
			while ( *data && ( *data != '*' || data[ 1 ] != '/' ) ) {
				data++;
			}
			if ( *data ) {
				data += 2;
			}
		} else {
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if ( c == '\"' ) {
		data++;
		while ( 1 ) {
			c = *data++;
			if ( c == '\"' || !c ) {
				com_token[ len ] = 0;
				*data_p = data;
				return com_token;
			}
			if ( len < MAX_TOKEN_CHARS_Q3 ) {
				com_token[ len ] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do {
		if ( len < MAX_TOKEN_CHARS_Q3 ) {
			com_token[ len ] = c;
			len++;
		}
		data++;
		c = *data;
	} while ( c > 32 && c != ',' );

	if ( len == MAX_TOKEN_CHARS_Q3 ) {
//		common->Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS_Q3);
		len = 0;
	}
	com_token[ len ] = 0;

	*data_p = data;
	return com_token;
}

qhandle_t R_RegisterSkin( const char* name ) {
	if ( !name || !name[ 0 ] ) {
		common->Printf( "Empty name passed to R_RegisterSkin\n" );
		return 0;
	}

	if ( String::Length( name ) >= MAX_QPATH ) {
		common->Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}

	// see if the skin is already loaded
	qhandle_t hSkin;
	for ( hSkin = 1; hSkin < tr.numSkins; hSkin++ ) {
		skin_t* skin = tr.skins[ hSkin ];
		if ( !String::ICmp( skin->name, name ) ) {
			if ( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		common->Printf( S_COLOR_YELLOW "WARNING: R_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}

	//----(SA)	moved things around slightly to fix the problem where you restart
	//			a map that has ai characters who had invalid skin names entered
	//			in thier "skin" or "head" field

	skin_t* skin;
	if ( !( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) ) {
		tr.numSkins++;
		skin = new skin_t;
		Com_Memset( skin, 0, sizeof ( skin_t ) );
		tr.skins[ hSkin ] = skin;
		String::NCpyZ( skin->name, name, sizeof ( skin->name ) );
		skin->numSurfaces = 0;
		skin->numModels = 0;
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// If not a .skin file, load as a single shader
	if ( !( GGameType & GAME_ET ) && String::Cmp( name + String::Length( name ) - 5, ".skin" ) ) {
		if ( GGameType & ( GAME_WolfSP | GAME_WolfMP ) ) {
			tr.numSkins++;
			skin = new skin_t;
			Com_Memset( skin, 0, sizeof ( skin_t ) );
			tr.skins[ hSkin ] = skin;
			String::NCpyZ( skin->name, name, sizeof ( skin->name ) );
			skin->numSurfaces = 0;
			skin->numModels = 0;
		}
		skin->numSurfaces = 1;
		skin->surfaces[ 0 ] = new skinSurface_t;
		skin->surfaces[ 0 ]->name[ 0 ] = 0;
		skin->surfaces[ 0 ]->shader = R_FindShader( name, LIGHTMAP_NONE, true );
		return hSkin;
	}

	// load and parse the skin file
	char* text;
	FS_ReadFile( name, ( void** )&text );
	if ( !text ) {
		return 0;
	}

	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) {
		tr.numSkins++;
		skin = new skin_t;
		Com_Memset( skin, 0, sizeof ( skin_t ) );
		tr.skins[ hSkin ] = skin;
		String::NCpyZ( skin->name, name, sizeof ( skin->name ) );
		skin->numSurfaces = 0;
		skin->numModels = 0;
	}

	char* text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		const char* token = CommaParse( &text_p );
		char surfName[ MAX_QPATH ];
		String::NCpyZ( surfName, token, sizeof ( surfName ) );

		if ( !token[ 0 ] ) {
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

		if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) && !String::NICmp( token, "md3_", 4 ) ) {
			// this is specifying a model
			skinModel_t* model = skin->models[ skin->numModels ] = new skinModel_t;
			String::NCpyZ( model->type, token, sizeof ( model->type ) );
			model->hash = Com_HashKey( model->type, sizeof ( model->type ) );

			// get the model name
			token = CommaParse( &text_p );

			String::NCpyZ( model->model, token, sizeof ( model->model ) );

			skin->numModels++;
			continue;
		}

		if ( GGameType & ( GAME_WolfSP | GAME_WolfMP ) && strstr( token, "playerscale" ) ) {
			token = CommaParse( &text_p );
			skin->scale[ 0 ] = String::Atof( token );	// uniform scaling for now
			skin->scale[ 1 ] = String::Atof( token );
			skin->scale[ 2 ] = String::Atof( token );
			continue;
		}

		// parse the shader name
		token = CommaParse( &text_p );

		skinSurface_t* surf = new skinSurface_t;
		skin->surfaces[ skin->numSurfaces ] = surf;
		String::NCpyZ( surf->name, surfName, sizeof ( surf->name ) );
		surf->hash = Com_HashKey( surf->name, sizeof ( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, true );
		skin->numSurfaces++;
	}

	FS_FreeFile( text );

	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		//----(SA)	allow this for the (current) special case of the loper's upper body
		//			(it's upper body has no surfaces, only tags)
		if ( !( GGameType & ( GAME_WolfSP | GAME_WolfMP ) ) || !( strstr( name, "loper" ) && strstr( name, "upper" ) ) ) {
			return 0;		// use default skin
		}
	}

	return hSkin;
}

void R_InitSkins() {
	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin_t* skin = tr.skins[ 0 ] = new skin_t;
	Com_Memset( skin, 0, sizeof ( skin_t ) );
	String::NCpyZ( skin->name, "<default skin>", sizeof ( skin->name ) );
	skin->numSurfaces = 1;
	skin->surfaces[ 0 ] = new skinSurface_t;
	skin->surfaces[ 0 ]->name[ 0 ] = 0;
	skin->surfaces[ 0 ]->shader = tr.defaultShader;
}

skin_t* R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[ 0 ];
	}
	return tr.skins[ hSkin ];
}

void R_SkinList_f() {
	common->Printf( "------------------\n" );

	for ( int i = 0; i < tr.numSkins; i++ ) {
		skin_t* skin = tr.skins[ i ];

		common->Printf( "%3i:%s\n", i, skin->name );
		for ( int j = 0; j < skin->numSurfaces; j++ ) {
			common->Printf( "       %s = %s\n", skin->surfaces[ j ]->name, skin->surfaces[ j ]->shader->name );
		}
	}
	common->Printf( "------------------\n" );
}

image_t* R_RegisterSkinQ2( const char* name ) {
	return R_FindImageFile( name, true, true, GL_CLAMP, IMG8MODE_Skin );
}

static void R_CreateOrUpdateTranslatedModelSkin( image_t*& image, const char* name, qhandle_t modelHandle, byte* pixels, byte* translation ) {
	if ( !modelHandle ) {
		return;
	}
	model_t* model = R_GetModelByHandle( modelHandle );
	if ( model->type != MOD_MESH1 ) {
		// only translate skins on alias models
		return;
	}

	mesh1hdr_t* mesh1Header = ( mesh1hdr_t* )model->q1_cache;
	int width = mesh1Header->skinwidth;
	int height = mesh1Header->skinheight;

	R_CreateOrUpdateTranslatedSkin( image, name, pixels, translation, width, height );
}

void R_CreateOrUpdateTranslatedModelSkinQ1( image_t*& image, const char* name, qhandle_t modelHandle, byte* translation ) {
	R_CreateOrUpdateTranslatedModelSkin( image, name, modelHandle, q1_player_8bit_texels, translation );
}

byte* R_LoadQuakeWorldSkinData( const char* name ) {
	int width;
	int height;
	byte* pixels;
	R_LoadPCX( name, &pixels, NULL, &width, &height );
	if ( !pixels ) {
		return NULL;
	}

	byte* out = new byte[ QUAKEWORLD_SKIN_WIDTH * QUAKEWORLD_SKIN_HEIGHT ];
	Com_Memset( out, 0, QUAKEWORLD_SKIN_WIDTH * QUAKEWORLD_SKIN_HEIGHT );

	byte* outp = out;
	byte* pixp = pixels;
	int copyWidth = Min( width, static_cast<int>( QUAKEWORLD_SKIN_WIDTH ) );
	int copyHeight = Min ( height , static_cast<int>( QUAKEWORLD_SKIN_HEIGHT ) );
	for ( int y = 0; y < copyHeight; y++, outp += QUAKEWORLD_SKIN_WIDTH, pixp += width ) {
		Com_Memcpy( outp, pixp, copyWidth );
	}
	delete[] pixels;

	return out;
}

//----(SA) added so client can see what model or scale for the model was specified in a skin
bool R_GetSkinModel( qhandle_t skinid, const char* type, char* name ) {
	skin_t* skin = tr.skins[ skinid ];
	int hash = Com_HashKey( ( char* )type, String::Length( type ) );

	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP ) && !String::ICmp( type, "playerscale" ) ) {
		// client is requesting scale from the skin rather than a model
		String::Sprintf( name, MAX_QPATH, "%.2f %.2f %.2f", skin->scale[ 0 ], skin->scale[ 1 ], skin->scale[ 2 ] );
		return true;
	}

	for ( int i = 0; i < skin->numModels; i++ ) {
		if ( GGameType & GAME_ET && hash != skin->models[ i ]->hash ) {
			continue;
		}
		if ( !String::ICmp( skin->models[ i ]->type, type ) ) {
			// (SA) whoops, should've been this way
			String::NCpyZ( name, skin->models[ i ]->model, sizeof ( skin->models[ i ]->model ) );
			return true;
		}
	}
	return false;
}

//	Return a shader index for a given model's surface
// 'withlightmap' set to '0' will create a new shader that is a copy of the one found
// on the model, without the lighmap stage, if the shader has a lightmap stage
//	NOTE: only works for bmodels right now.  Could modify for other models (md3's etc.)
qhandle_t R_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap ) {
	if ( surfnum < 0 ) {
		surfnum = 0;
	}

	model_t* model = R_GetModelByHandle( modelid );		// (SA) should be correct now

	if ( model ) {
		mbrush46_model_t* bmodel  = model->q3_bmodel;
		if ( bmodel && bmodel->firstSurface ) {
			if ( surfnum >= bmodel->numSurfaces ) {		// if it's out of range, return the first surface
				surfnum = 0;
			}

			mbrush46_surface_t* surf = bmodel->firstSurface + surfnum;
			// RF, check for null shader (can happen on func_explosive's with botclips attached)
			if ( !surf->shader ) {
				return 0;
			}
			shader_t* shd;
			if ( surf->shader->lightmapIndex > LIGHTMAP_NONE ) {
				bool mip = true;	// mip generation on by default

				// get mipmap info for original texture
				image_t* image = R_FindImage( surf->shader->name );
				if ( image ) {
					mip = image->mipmap;
				}
				shd = R_FindShader( surf->shader->name, LIGHTMAP_NONE, mip );
				shd->stages[ 0 ]->rgbGen = CGEN_LIGHTING_DIFFUSE;	// (SA) new
			} else {
				shd = surf->shader;
			}

			return shd->index;
		}
	}

	return 0;
}
