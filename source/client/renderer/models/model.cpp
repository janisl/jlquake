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

#include "model.h"
#include "../main.h"
#include "../commands.h"
#include "../surfaces.h"
#include "../cvars.h"
#include "../scene.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/command_buffer.h"
#include "../../../common/endian.h"
#include "RenderModelBad.h"
#include "RenderModelBSP29.h"
#include "RenderModelBSP29NonMap.h"
#include "RenderModelBSP38.h"
#include "RenderModelSPR.h"
#include "RenderModelSP2.h"
#include "RenderModelMDL.h"
#include "RenderModelMDLNew.h"
#include "RenderModelMD2.h"
#include "RenderModelMD3.h"
#include "RenderModelMD4.h"
#include "RenderModelMDC.h"
#include "RenderModelMDS.h"
#include "RenderModelMDM.h"
#include "RenderModelMDX.h"

idRenderModel* loadmodel;

void R_AddModel( idRenderModel* model ) {
	if ( tr.numModels == MAX_MOD_KNOWN ) {
		common->Error( "R_AllModel: MAX_MOD_KNOWN hit." );
	}

	model->index = tr.numModels;
	tr.models[ tr.numModels ] = model;
	tr.numModels++;
}

void R_ModelInit() {
	// leave a space for NULL model
	tr.numModels = 0;

	idRenderModel* mod = new idRenderModelBad();
	R_AddModel( mod );
	mod->type = MOD_BAD;

	R_InitBsp29NoTextureMip();
}

static void R_FreeModel( idRenderModel* mod ) {
	if ( mod->type == MOD_SPRITE ) {
		Mod_FreeSpriteModel( mod );
	} else if ( mod->type == MOD_SPRITE2 ) {
		Mod_FreeSprite2Model( mod );
	} else if ( mod->type == MOD_MESH1 ) {
		Mod_FreeMdlModel( mod );
	} else if ( mod->type == MOD_MESH2 ) {
		Mod_FreeMd2Model( mod );
	} else if ( mod->type == MOD_MESH3 ) {
		R_FreeMd3( mod );
	} else if ( mod->type == MOD_MD4 ) {
		R_FreeMd4( mod );
	} else if ( mod->type == MOD_MDC ) {
		R_FreeMdc( mod );
	} else if ( mod->type == MOD_MDS ) {
		R_FreeMds( mod );
	} else if ( mod->type == MOD_MDM ) {
		R_FreeMdm( mod );
	} else if ( mod->type == MOD_MDX ) {
		R_FreeMdx( mod );
	} else if ( mod->type == MOD_BRUSH29 ) {
		Mod_FreeBsp29( mod );
	} else if ( mod->type == MOD_BRUSH29_NON_MAP ) {
		Mod_FreeBsp29NonMap( mod );
	} else if ( mod->type == MOD_BRUSH38 ) {
		Mod_FreeBsp38( mod );
	} else if ( mod->type == MOD_BRUSH46 ) {
		R_FreeBsp46Model( mod );
	}
	delete mod;
}

void R_FreeModels() {
	for ( int i = 0; i < tr.numModels; i++ ) {
		R_FreeModel( tr.models[ i ] );
		tr.models[ i ] = NULL;
	}
	tr.numModels = 0;

	if ( tr.world ) {
		R_FreeBsp46( tr.world );
	}
	tr.worldMapLoaded = false;

	Mem_Free( r_notexture_mip );
	r_notexture_mip = NULL;
}

void R_LoadWorld( const char* name ) {
	if ( tr.worldMapLoaded ) {
		common->Error( "ERROR: attempted to redundantly load world map\n" );
	}

	skyboxportal = 0;

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[ 0 ] = 0.45f;
	tr.sunDirection[ 1 ] = 0.3f;
	tr.sunDirection[ 2 ] = 0.9f;

	VectorNormalize( tr.sunDirection );

	tr.sunShader = 0;	// clear sunshader so it's not there if the level doesn't specify it

	// invalidate fogs (likely to be re-initialized to new values by the current map)
	// TODO:(SA)this is sort of silly.  I'm going to do a general cleanup on fog stuff
	//			now that I can see how it's been used.  (functionality can narrow since
	//			it's not used as much as it's designed for.)
	R_SetFog( FOG_SKY, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_PORTALVIEW, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_HUD, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_MAP, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_CURRENT, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_TARGET, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_WATER, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_SERVER, 0, 0, 0, 0, 0, 0 );

	tr.worldMapLoaded = true;
	tr.worldDir = NULL;

	// load it
	idList<byte> buffer;
	if ( FS_ReadFile( name, buffer ) < 0 ) {
		common->Error( "R_LoadWorld: %s not found", name );
	}

	// ydnar: set map meta dir
	tr.worldDir = __CopyString( name );
	String::StripExtension( tr.worldDir, tr.worldDir );

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	Com_Memset( &s_worldData, 0, sizeof ( s_worldData ) );
	String::NCpyZ( s_worldData.name, name, sizeof ( s_worldData.name ) );

	String::NCpyZ( s_worldData.baseName, String::SkipPath( s_worldData.name ), sizeof ( s_worldData.name ) );
	String::StripExtension( s_worldData.baseName, s_worldData.baseName );

	idRenderModel* mod = NULL;

	if ( GGameType & GAME_QuakeHexen ) {
		R_BeginBuildingLightmapsQ1();

		mod = new idRenderModelBSP29();
		R_AddModel( mod );
		String::NCpyZ( mod->name, name, sizeof ( mod->name ) );
		loadmodel = mod;
		mod->Load( buffer, NULL );

		// identify sky texture
		skytexturenum = -1;
		for ( int i = 0; i < mod->brush29_numtextures; i++ ) {
			if ( !mod->brush29_textures[ i ] ) {
				continue;
			}
			if ( !String::NCmp( mod->brush29_textures[ i ]->name, "sky", 3 ) ) {
				skytexturenum = i;
			}
		}
	} else if ( GGameType & GAME_Quake2 ) {
		mod = new idRenderModelBSP38();
		R_AddModel( mod );
		String::NCpyZ( mod->name, name, sizeof ( mod->name ) );
		loadmodel = mod;
		switch ( LittleLong( *( unsigned* )buffer.Ptr() ) ) {
		case BSP38_HEADER:
			Mod_LoadBrush38Model( mod, buffer.Ptr() );
			break;

		default:
			common->Error( "Mod_NumForName: unknown fileid for %s", name );
			break;
		}
	} else {
		R_LoadBrush46Model( buffer.Ptr() );
	}

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;
	tr.worldModel = mod;

	if ( GGameType & ( GAME_WolfMP | GAME_ET ) ) {
		// reset fog to world fog (if present)
		R_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0 );
	}

	//	set the sun shader if there is one
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) && tr.sunShaderName ) {
		tr.sunShader = R_FindShader( tr.sunShaderName, LIGHTMAP_NONE, true );
	}
}

bool R_GetEntityToken( char* buffer, int size ) {
	const char* s = String::Parse3( const_cast<const char**>( &s_worldData.entityParsePoint ) );
	String::NCpyZ( buffer, s, size );
	if ( !s_worldData.entityParsePoint || !s[ 0 ] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return false;
	} else {
		return true;
	}
}

idRenderModel* R_GetModelByHandle( qhandle_t index ) {
	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[ 0 ];
	}

	return tr.models[ index ];
}

//	loads a model's shadow script
static void R_LoadModelShadow( idRenderModel* mod ) {
	// set default shadow
	mod->q3_shadowShader = 0;

	// build name
	char filename[ 1024 ];
	String::StripExtension2( mod->name, filename, sizeof ( filename ) );
	String::DefaultExtension( filename, 1024, ".shadow" );

	// load file
	char* buf;
	FS_ReadFile( filename, ( void** )&buf );
	if ( !buf ) {
		return;
	}

	char* shadowBits = strchr( buf, ' ' );
	if ( shadowBits != NULL ) {
		*shadowBits = '\0';
		shadowBits++;

		if ( String::Length( buf ) >= MAX_QPATH ) {
			common->Printf( "R_LoadModelShadow: Shader name exceeds MAX_QPATH\n" );
			mod->q3_shadowShader = 0;
		} else {
			shader_t* sh = R_FindShader( buf, LIGHTMAP_NONE, true );

			if ( sh->defaultShader ) {
				mod->q3_shadowShader = 0;
			} else {
				mod->q3_shadowShader = sh->index;
			}
		}
		sscanf( shadowBits, "%f %f %f %f %f %f",
			&mod->q3_shadowParms[ 0 ], &mod->q3_shadowParms[ 1 ], &mod->q3_shadowParms[ 2 ],
			&mod->q3_shadowParms[ 3 ], &mod->q3_shadowParms[ 4 ], &mod->q3_shadowParms[ 5 ] );
	}
	FS_FreeFile( buf );
}

//	Loads in a model for the given name
//	Zero will be returned if the model fails to load. An entry will be
// retained for failed models as an optimization to prevent disk rescanning
// if they are asked for again.
qhandle_t R_RegisterModel( const char* name, idSkinTranslation* skinTranslation ) {
	if ( !name || !name[ 0 ] ) {
		common->Printf( "R_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( String::Length( name ) >= MAX_QPATH ) {
		common->Printf( "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

	// Ridah, caching
	if ( r_cacheGathering->integer ) {
		Cbuf_ExecuteText( EXEC_NOW, va( "cache_usedfile model %s\n", name ) );
	}

	//
	// search the currently loaded models
	//
	for ( int hModel = 1; hModel < tr.numModels; hModel++ ) {
		idRenderModel* mod = tr.models[ hModel ];
		if ( !String::ICmp( mod->name, name ) ) {
			if ( mod->type == MOD_BAD ) {
				return 0;
			}
			return hModel;
		}
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	idList<byte> buf;
	int modfilelen = FS_ReadFile( name, buf );
	if ( modfilelen < 0 && ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) ) {
		char filename[ 1024 ];
		String::Cpy( filename, name );

		//	try MDC first
		filename[ String::Length( filename ) - 1 ] = 'c';
		modfilelen = FS_ReadFile( filename, buf );

		if ( modfilelen < 0 ) {
			// try MD3 second
			filename[ String::Length( filename ) - 1 ] = '3';
			modfilelen = FS_ReadFile( filename, buf );
		}
	}
	if ( modfilelen < 0 ) {
		common->Printf( S_COLOR_YELLOW "R_RegisterModel: couldn't load %s\n", name );
		// we still keep the idRenderModel around, so if the model name is asked for
		// again, we won't bother scanning the filesystem
		idRenderModel* mod = new idRenderModelBad();
		R_AddModel( mod );
		mod->type = MOD_BAD;
		String::NCpyZ( mod->name, name, sizeof ( mod->name ) );
		return 0;
	}

	// allocate a new idRenderModel
	idRenderModel* mod;
	switch ( LittleLong( *( qint32* )buf.Ptr() ) ) {
	case BSP29_VERSION:
		mod = new idRenderModelBSP29NonMap();
		break;

	case IDSPRITE1HEADER:
		mod = new idRenderModelSPR();
		break;

	case IDSPRITE2HEADER:
		mod = new idRenderModelSP2();
		break;

	case IDPOLYHEADER:
		mod = new idRenderModelMDL();
		break;

	case RAPOLYHEADER:
		mod = new idRenderModelMDLNew();
		break;

	case IDMESH2HEADER:
		mod = new idRenderModelMD2();
		break;

	case MD3_IDENT:
		mod = new idRenderModelMD3();
		break;

	case MD4_IDENT:
		mod = new idRenderModelMD4();
		break;

	case MDC_IDENT:
		mod = new idRenderModelMDC();
		break;

	case MDS_IDENT:
		mod = new idRenderModelMDS();
		break;

	case MDM_IDENT:
		mod = new idRenderModelMDM();
		break;

	case MDX_IDENT:
		mod = new idRenderModelMDX();
		break;

	default:
		common->Printf( S_COLOR_YELLOW "R_RegisterModel: unknown fileid for %s\n", name );
		mod = new idRenderModelBad;
		break;
	}
	R_AddModel( mod );

	// only set the name after the model has been successfully loaded
	String::NCpyZ( mod->name, name, sizeof ( mod->name ) );

	loadmodel = mod;

	//	call the apropriate loader
	bool loaded = mod->Load( buf, skinTranslation );

	R_LoadModelShadow( mod );

	// GR - by default models are not tessellated
	mod->q3_ATI_tess = false;
	// GR - check if can be tessellated...
	//		make sure to tessellate model heads
	if ( GGameType & GAME_WolfSP && strstr( name, "head" ) ) {
		mod->q3_ATI_tess = true;
	}

	if ( !loaded ) {
		common->Printf( S_COLOR_YELLOW "R_RegisterModel: couldn't load %s\n", name );
		// we still keep the idRenderModel around, so if the model name is asked for
		// again, we won't bother scanning the filesystem
		delete mod;
		mod = new idRenderModelBad();
		R_AddModel( mod );
		mod->type = MOD_BAD;
		String::NCpyZ( mod->name, name, sizeof ( mod->name ) );
		return 0;
	}

	return mod->index;
}

void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {
	idRenderModel* model = R_GetModelByHandle( handle );

	switch ( model->type ) {
	case MOD_BRUSH29:
	case MOD_BRUSH29_NON_MAP:
	case MOD_SPRITE:
	case MOD_MESH1:
		VectorCopy( model->q1_mins, mins );
		VectorCopy( model->q1_maxs, maxs );
		break;

	case MOD_BRUSH38:
	case MOD_MESH2:
		VectorCopy( model->q2_mins, mins );
		VectorCopy( model->q2_maxs, maxs );
		break;

	case MOD_BRUSH46:
		VectorCopy( model->q3_bmodel->bounds[ 0 ], mins );
		VectorCopy( model->q3_bmodel->bounds[ 1 ], maxs );
		break;

	case MOD_MESH3:
	{
		md3Header_t* header = model->q3_md3[ 0 ].header;

		md3Frame_t* frame = ( md3Frame_t* )( ( byte* )header + header->ofsFrames );

		VectorCopy( frame->bounds[ 0 ], mins );
		VectorCopy( frame->bounds[ 1 ], maxs );
	}
	break;

	case MOD_MDC:
	{
		md3Frame_t* frame = ( md3Frame_t* )( ( byte* )model->q3_mdc[ 0 ].header + model->q3_mdc[ 0 ].header->ofsFrames );

		VectorCopy( frame->bounds[ 0 ], mins );
		VectorCopy( frame->bounds[ 1 ], maxs );
	}
	break;

	default:
		VectorClear( mins );
		VectorClear( maxs );
		break;
	}
}

int R_ModelNumFrames( qhandle_t Handle ) {
	idRenderModel* Model = R_GetModelByHandle( Handle );
	switch ( Model->type ) {
	case MOD_BRUSH29:
	case MOD_BRUSH29_NON_MAP:
	case MOD_SPRITE:
	case MOD_MESH1:
		return Model->q1_numframes;

	case MOD_BRUSH38:
	case MOD_SPRITE2:
	case MOD_MESH2:
		return Model->q2_numframes;

	case MOD_BRUSH46:
		return 1;

	case MOD_MESH3:
		return Model->q3_md3[ 0 ].header->numFrames;

	case MOD_MD4:
		return Model->q3_md4->numFrames;

	case MOD_MDC:
		return Model->q3_mdc[ 0 ].header->numFrames;

	case MOD_MDS:
		return Model->q3_mds->numFrames;

	case MOD_MDM:
		return 1;

	case MOD_MDX:
		return Model->q3_mdx->numFrames;

	default:
		return 0;
	}
}

//	Use only by Quake and Hexen 2, only .MDL files will have meaningfull flags.
int R_ModelFlags( qhandle_t Handle ) {
	idRenderModel* Model = R_GetModelByHandle( Handle );
	if ( Model->type == MOD_MESH1 ) {
		return Model->q1_flags;
	}
	return 0;
}

bool R_IsMeshModel( qhandle_t Handle ) {
	idRenderModel* Model = R_GetModelByHandle( Handle );
	return Model->type == MOD_MESH1 || Model->type == MOD_MESH2 ||
		   Model->type == MOD_MESH3 || Model->type == MOD_MD4 ||
		   Model->type == MOD_MDC || Model->type == MOD_MDS || Model->type == MOD_MDM;
}

const char* R_ModelName( qhandle_t Handle ) {
	return R_GetModelByHandle( Handle )->name;
}

int R_ModelSyncType( qhandle_t Handle ) {
	idRenderModel* Model = R_GetModelByHandle( Handle );
	switch ( Model->type ) {
	case MOD_SPRITE:
	case MOD_MESH1:
		return Model->q1_synctype;

	default:
		return 0;
	}
}

void R_CalculateModelScaleOffset( qhandle_t Handle, float ScaleX, float ScaleY, float ScaleZ, float ScaleZOrigin, vec3_t Out ) {
	idRenderModel* Model = R_GetModelByHandle( Handle );
	if ( Model->type != MOD_MESH1 ) {
		VectorClear( Out );
		return;
	}

	idSurfaceMDL* AliasHdr = Model->q1_mdl;

	Out[ 0 ] = -( ScaleX - 1.0 ) * ( AliasHdr->header.scale[ 0 ] * 127.95 + AliasHdr->header.scale_origin[ 0 ] );
	Out[ 1 ] = -( ScaleY - 1.0 ) * ( AliasHdr->header.scale[ 1 ] * 127.95 + AliasHdr->header.scale_origin[ 1 ] );
	Out[ 2 ] = -( ScaleZ - 1.0 ) * ( ScaleZOrigin * 2.0 * AliasHdr->header.scale[ 2 ] * 127.95 + AliasHdr->header.scale_origin[ 2 ] );
}

static md3Tag_t* R_GetTag( md3Header_t* mod, int frame, const char* tagName ) {
	md3Tag_t* tag;
	int i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = ( md3Tag_t* )( ( byte* )mod + mod->ofsTags ) + frame * mod->numTags;
	for ( i = 0; i < mod->numTags; i++, tag++ ) {
		if ( !String::Cmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

bool R_LerpTag( orientation_t* tag, qhandle_t handle, int startFrame, int endFrame,
	float frac, const char* tagName ) {
	int i;
	float frontLerp, backLerp;

	idRenderModel* model = R_GetModelByHandle( handle );
	if ( !model->q3_md3[ 0 ].header ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return false;
	}

	md3Tag_t* start = R_GetTag( model->q3_md3[ 0 ].header, startFrame, tagName );
	md3Tag_t* end = R_GetTag( model->q3_md3[ 0 ].header, endFrame, tagName );
	if ( !start || !end ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return false;
	}

	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0; i < 3; i++ ) {
		tag->origin[ i ] = start->origin[ i ] * backLerp +  end->origin[ i ] * frontLerp;
		tag->axis[ 0 ][ i ] = start->axis[ 0 ][ i ] * backLerp +  end->axis[ 0 ][ i ] * frontLerp;
		tag->axis[ 1 ][ i ] = start->axis[ 1 ][ i ] * backLerp +  end->axis[ 1 ][ i ] * frontLerp;
		tag->axis[ 2 ][ i ] = start->axis[ 2 ][ i ] * backLerp +  end->axis[ 2 ][ i ] * frontLerp;
	}
	VectorNormalize( tag->axis[ 0 ] );
	VectorNormalize( tag->axis[ 1 ] );
	VectorNormalize( tag->axis[ 2 ] );
	return true;
}

static int R_GetTag( byte* mod, int frame, const char* tagName, int startTagIndex, md3Tag_t** outTag ) {
	md3Header_t* md3 = ( md3Header_t* )mod;

	if ( frame >= md3->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = md3->numFrames - 1;
	}

	if ( startTagIndex > md3->numTags ) {
		*outTag = NULL;
		return -1;
	}

	md3Tag_t* tag = ( md3Tag_t* )( ( byte* )mod + md3->ofsTags ) + frame * md3->numTags;
	for ( int i = 0; i < md3->numTags; i++, tag++ ) {
		if ( ( i >= startTagIndex ) && !String::Cmp( tag->name, tagName ) ) {
			*outTag = tag;
			return i;	// found it
		}
	}

	*outTag = NULL;
	return -1;
}

static int R_GetMDCTag( byte* mod, int frame, const char* tagName, int startTagIndex, mdcTag_t** outTag ) {
	mdcHeader_t* mdc = ( mdcHeader_t* )mod;

	if ( frame >= mdc->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mdc->numFrames - 1;
	}

	if ( startTagIndex > mdc->numTags ) {
		*outTag = NULL;
		return -1;
	}

	mdcTagName_t* pTagName = ( mdcTagName_t* )( ( byte* )mod + mdc->ofsTagNames );
	for ( int i = 0; i < mdc->numTags; i++, pTagName++ ) {
		if ( ( i >= startTagIndex ) && !String::Cmp( pTagName->name, tagName ) ) {
			mdcTag_t* tag = ( mdcTag_t* )( ( byte* )mod + mdc->ofsTags ) + frame * mdc->numTags + i;
			*outTag = tag;
			return i;
		}
	}

	*outTag = NULL;
	return -1;
}

//	returns the index of the tag it found, for cycling through tags with the same name
int R_LerpTag( orientation_t* tag, const refEntity_t* refent, const char* tagName, int startIndex ) {

	qhandle_t handle = refent->hModel;
	int startFrame = refent->oldframe;
	int endFrame = refent->frame;
	float frac = 1.0 - refent->backlerp;

	idRenderModel* model = R_GetModelByHandle( handle );
	if ( !model->q3_md3[ 0 ].header && !model->q3_mdc[ 0 ].header && !model->q3_mds && !model->q3_mdm ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	float frontLerp = frac;
	float backLerp = 1.0 - frac;

	int retval;
	md3Tag_t* start;
	md3Tag_t* end;
	md3Tag_t ustart, uend;
	if ( model->type == MOD_MESH3 ) {
		// old MD3 style
		retval = R_GetTag( ( byte* )model->q3_md3[ 0 ].header, startFrame, tagName, startIndex, &start );
		retval = R_GetTag( ( byte* )model->q3_md3[ 0 ].header, endFrame, tagName, startIndex, &end );

	} else if ( model->type == MOD_MDS ) {
		// use bone lerping
		retval = R_GetBoneTagMds( tag, model->q3_mds, startIndex, refent, tagName );

		if ( retval >= 0 ) {
			return retval;
		}

		// failed
		return -1;
	} else if ( model->type == MOD_MDM ) {
		// use bone lerping
		retval = R_MDM_GetBoneTag( tag, model->q3_mdm, startIndex, refent, tagName );

		if ( retval >= 0 ) {
			return retval;
		}

		// failed
		return -1;
	} else {
		// psuedo-compressed MDC tags
		mdcTag_t* cstart;
		mdcTag_t* cend;

		retval = R_GetMDCTag( ( byte* )model->q3_mdc[ 0 ].header, startFrame, tagName, startIndex, &cstart );
		retval = R_GetMDCTag( ( byte* )model->q3_mdc[ 0 ].header, endFrame, tagName, startIndex, &cend );

		// uncompress the MDC tags into MD3 style tags
		if ( cstart && cend ) {
			vec3_t sangles, eangles;
			for ( int i = 0; i < 3; i++ ) {
				ustart.origin[ i ] = ( float )cstart->xyz[ i ] * MD3_XYZ_SCALE;
				uend.origin[ i ] = ( float )cend->xyz[ i ] * MD3_XYZ_SCALE;
				sangles[ i ] = ( float )cstart->angles[ i ] * MDC_TAG_ANGLE_SCALE;
				eangles[ i ] = ( float )cend->angles[ i ] * MDC_TAG_ANGLE_SCALE;
			}

			AnglesToAxis( sangles, ustart.axis );
			AnglesToAxis( eangles, uend.axis );

			start = &ustart;
			end = &uend;
		} else {
			start = NULL;
			end = NULL;
		}
	}

	if ( !start || !end ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	for ( int i = 0; i < 3; i++ ) {
		tag->origin[ i ] = start->origin[ i ] * backLerp +  end->origin[ i ] * frontLerp;
		tag->axis[ 0 ][ i ] = start->axis[ 0 ][ i ] * backLerp +  end->axis[ 0 ][ i ] * frontLerp;
		tag->axis[ 1 ][ i ] = start->axis[ 1 ][ i ] * backLerp +  end->axis[ 1 ][ i ] * frontLerp;
		tag->axis[ 2 ][ i ] = start->axis[ 2 ][ i ] * backLerp +  end->axis[ 2 ][ i ] * frontLerp;
	}

	VectorNormalize( tag->axis[ 0 ] );
	VectorNormalize( tag->axis[ 1 ] );
	VectorNormalize( tag->axis[ 2 ] );

	return retval;
}

void R_Modellist_f() {
	int total = 0;
	for ( int i = 0; i < tr.numModels; i++ ) {
		idRenderModel* mod = tr.models[ i ];
		int lods = 1;
		int DataSize = 0;
		switch ( mod->type ) {
		case MOD_BRUSH38:
		case MOD_SPRITE2:
		case MOD_MESH2:
			DataSize = tr.models[ i ]->q2_extradatasize;
			break;

		case MOD_BRUSH46:
		case MOD_MD4:
		case MOD_MDS:
		case MOD_MDM:
		case MOD_MDX:
			DataSize = mod->q3_dataSize;
			break;

		case MOD_MESH3:
			DataSize = mod->q3_dataSize;
			for ( int j = 1; j < MD3_MAX_LODS; j++ ) {
				if ( mod->q3_md3[ j ].header && mod->q3_md3[ j ].header != mod->q3_md3[ j - 1 ].header ) {
					lods++;
				}
			}
			break;

		case MOD_MDC:
			DataSize = mod->q3_dataSize;
			for ( int j = 1; j < MD3_MAX_LODS; j++ ) {
				if ( mod->q3_mdc[ j ].header && mod->q3_mdc[ j ].header != mod->q3_mdc[ j - 1 ].header ) {
					lods++;
				}
			}
			break;

		default:
			break;
		}
		common->Printf( "%8i : (%i) %s\n", DataSize, lods, mod->name );
		total += DataSize;
	}
	common->Printf( "%8i : Total models\n", total );
}
