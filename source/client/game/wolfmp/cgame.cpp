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
#include "../../client_main.h"
#include "../../cinematic/public.h"
#include "../../ui/ui.h"
#include "../../translate.h"
#include "../../splines/public.h"
#include "cg_public.h"
#include "cg_ui_shared.h"
#include "../../../server/public.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/precompiler.h"
#include "../../../common/system.h"

static refEntityType_t clwm_gameRefEntTypeToEngine[] =
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_SPLASH,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_CORE_TAPER,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,
};

bool CLWM_GetTag( int clientNum, const char* tagname, orientation_t* _or ) {
	return VM_Call( cgvm, WMCG_GET_TAG, clientNum, tagname, _or );
}

bool CLWM_CheckCenterView() {
	if ( !cgvm ) {
		return true;
	}
	return VM_Call( cgvm, WMCG_CHECKCENTERVIEW );
}

void CLWM_GetGlconfig( wmglconfig_t* glconfig ) {
	String::NCpyZ( glconfig->renderer_string, cls.glconfig.renderer_string, sizeof ( glconfig->renderer_string ) );
	String::NCpyZ( glconfig->vendor_string, cls.glconfig.vendor_string, sizeof ( glconfig->vendor_string ) );
	String::NCpyZ( glconfig->version_string, cls.glconfig.version_string, sizeof ( glconfig->version_string ) );
	String::NCpyZ( glconfig->extensions_string, cls.glconfig.extensions_string, sizeof ( glconfig->extensions_string ) );
	glconfig->maxTextureSize = cls.glconfig.maxTextureSize;
	glconfig->maxActiveTextures = cls.glconfig.maxActiveTextures;
	glconfig->colorBits = cls.glconfig.colorBits;
	glconfig->depthBits = cls.glconfig.depthBits;
	glconfig->stencilBits = cls.glconfig.stencilBits;
	glconfig->driverType = cls.glconfig.driverType;
	glconfig->hardwareType = cls.glconfig.hardwareType;
	glconfig->deviceSupportsGamma = cls.glconfig.deviceSupportsGamma;
	glconfig->textureCompression = cls.glconfig.textureCompression;
	glconfig->textureEnvAddAvailable = cls.glconfig.textureEnvAddAvailable;
	glconfig->anisotropicAvailable = cls.glconfig.anisotropicAvailable;
	glconfig->maxAnisotropy = cls.glconfig.maxAnisotropy;
	glconfig->NVFogAvailable = cls.glconfig.NVFogAvailable;
	glconfig->NVFogMode = cls.glconfig.NVFogMode;
	glconfig->ATIMaxTruformTess = cls.glconfig.ATIMaxTruformTess;
	glconfig->ATINormalMode = cls.glconfig.ATINormalMode;
	glconfig->ATIPointMode = cls.glconfig.ATIPointMode;
	glconfig->vidWidth = cls.glconfig.vidWidth;
	glconfig->vidHeight = cls.glconfig.vidHeight;
	glconfig->windowAspect = cls.glconfig.windowAspect;
	glconfig->displayFrequency = cls.glconfig.displayFrequency;
	glconfig->isFullscreen = cls.glconfig.isFullscreen;
	glconfig->stereoEnabled = cls.glconfig.stereoEnabled;
	glconfig->smpActive = cls.glconfig.smpActive;
	glconfig->textureFilterAnisotropicAvailable = cls.glconfig.textureFilterAnisotropicAvailable;
}

static void CLWM_GameRefEntToEngine( const wmrefEntity_t* gameRefent, refEntity_t* refent ) {
	Com_Memset( refent, 0, sizeof ( *refent ) );
	if ( gameRefent->reType < 0 || gameRefent->reType >= 10 ) {
		refent->reType = RT_MAX_REF_ENTITY_TYPE;
	} else {
		refent->reType = clwm_gameRefEntTypeToEngine[ gameRefent->reType ];
	}
	refent->renderfx = gameRefent->renderfx & ( RF_MINLIGHT | RF_THIRD_PERSON |
												RF_FIRST_PERSON | RF_DEPTHHACK | RF_NOSHADOW | RF_LIGHTING_ORIGIN |
												RF_SHADOW_PLANE | RF_WRAP_FRAMES );
	if ( gameRefent->renderfx & WMRF_BLINK ) {
		refent->renderfx |= RF_BLINK;
	}
	refent->hModel = gameRefent->hModel;
	VectorCopy( gameRefent->lightingOrigin, refent->lightingOrigin );
	refent->shadowPlane = gameRefent->shadowPlane;
	AxisCopy( gameRefent->axis, refent->axis );
	AxisCopy( gameRefent->torsoAxis, refent->torsoAxis );
	refent->nonNormalizedAxes = gameRefent->nonNormalizedAxes;
	VectorCopy( gameRefent->origin, refent->origin );
	refent->frame = gameRefent->frame;
	refent->torsoFrame = gameRefent->torsoFrame;
	VectorCopy( gameRefent->oldorigin, refent->oldorigin );
	refent->oldframe = gameRefent->oldframe;
	refent->oldTorsoFrame = gameRefent->oldTorsoFrame;
	refent->backlerp = gameRefent->backlerp;
	refent->torsoBacklerp = gameRefent->torsoBacklerp;
	refent->skinNum = gameRefent->skinNum;
	refent->customSkin = gameRefent->customSkin;
	refent->customShader = gameRefent->customShader;
	refent->shaderRGBA[ 0 ] = gameRefent->shaderRGBA[ 0 ];
	refent->shaderRGBA[ 1 ] = gameRefent->shaderRGBA[ 1 ];
	refent->shaderRGBA[ 2 ] = gameRefent->shaderRGBA[ 2 ];
	refent->shaderRGBA[ 3 ] = gameRefent->shaderRGBA[ 3 ];
	refent->shaderTexCoord[ 0 ] = gameRefent->shaderTexCoord[ 0 ];
	refent->shaderTexCoord[ 1 ] = gameRefent->shaderTexCoord[ 1 ];
	refent->shaderTime = gameRefent->shaderTime;
	refent->radius = gameRefent->radius;
	refent->rotation = gameRefent->rotation;
	VectorCopy( gameRefent->fireRiseDir, refent->fireRiseDir );
	refent->fadeStartTime = gameRefent->fadeStartTime;
	refent->fadeEndTime = gameRefent->fadeEndTime;
	refent->hilightIntensity = gameRefent->hilightIntensity;
	refent->reFlags = gameRefent->reFlags & ( REFLAG_ONLYHAND |
											  REFLAG_FORCE_LOD | REFLAG_ORIENT_LOD | REFLAG_DEAD_LOD );
	refent->entityNum = gameRefent->entityNum;
}

void CLWM_AddRefEntityToScene( const wmrefEntity_t* ent ) {
	refEntity_t refent;
	CLWM_GameRefEntToEngine( ent, &refent );
	R_AddRefEntityToScene( &refent );
}

void CLWM_RenderScene( const wmrefdef_t* gameRefdef ) {
	refdef_t rd;
	Com_Memset( &rd, 0, sizeof ( rd ) );
	rd.x = gameRefdef->x;
	rd.y = gameRefdef->y;
	rd.width = gameRefdef->width;
	rd.height = gameRefdef->height;
	rd.fov_x = gameRefdef->fov_x;
	rd.fov_y = gameRefdef->fov_y;
	VectorCopy( gameRefdef->vieworg, rd.vieworg );
	AxisCopy( gameRefdef->viewaxis, rd.viewaxis );
	rd.time = gameRefdef->time;
	rd.rdflags = gameRefdef->rdflags & ( RDF_NOWORLDMODEL | RDF_HYPERSPACE |
										 RDF_SKYBOXPORTAL | RDF_UNDERWATER | RDF_DRAWINGSKY | RDF_SNOOPERVIEW );
	Com_Memcpy( rd.areamask, gameRefdef->areamask, sizeof ( rd.areamask ) );
	Com_Memcpy( rd.text, gameRefdef->text, sizeof ( rd.text ) );
	rd.glfog.mode = gameRefdef->glfog.mode;
	rd.glfog.hint = gameRefdef->glfog.hint;
	rd.glfog.startTime = gameRefdef->glfog.startTime;
	rd.glfog.finishTime = gameRefdef->glfog.finishTime;
	Vector4Copy( gameRefdef->glfog.color, rd.glfog.color );
	rd.glfog.start = gameRefdef->glfog.start;
	rd.glfog.end = gameRefdef->glfog.end;
	rd.glfog.useEndForClip = gameRefdef->glfog.useEndForClip;
	rd.glfog.density = gameRefdef->glfog.density;
	rd.glfog.registered = gameRefdef->glfog.registered;
	rd.glfog.drawsky = gameRefdef->glfog.drawsky;
	rd.glfog.clearscreen = gameRefdef->glfog.clearscreen;
	R_RenderScene( &rd );
}

int CLWM_LerpTag( orientation_t* tag,  const wmrefEntity_t* gameRefent, const char* tagName, int startIndex ) {
	refEntity_t refent;
	CLWM_GameRefEntToEngine( gameRefent, &refent );
	return R_LerpTag( tag, &refent, tagName, startIndex );
}

static void CLWM_GetGameState( wmgameState_t* gs ) {
	*gs = cl.wm_gameState;
}

static bool CLWM_GetUserCmd( int cmdNumber, wmusercmd_t* ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.q3_cmdNumber ) {
		common->Error( "CLWM_GetUserCmd: %i >= %i", cmdNumber, cl.q3_cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.q3_cmdNumber - CMD_BACKUP_Q3 ) {
		return false;
	}

	*ucmd = cl.wm_cmds[ cmdNumber & CMD_MASK_Q3 ];

	return true;
}

static void CLWM_GetCurrentSnapshotNumber( int* snapshotNumber, int* serverTime ) {
	*snapshotNumber = cl.wm_snap.messageNum;
	*serverTime = cl.wm_snap.serverTime;
}

static bool CLWM_GetSnapshot( int snapshotNumber, wmsnapshot_t* snapshot ) {
	if ( snapshotNumber > cl.wm_snap.messageNum ) {
		common->Error( "CLWM_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.wm_snap.messageNum - snapshotNumber >= PACKET_BACKUP_Q3 ) {
		return false;
	}

	// if the frame is not valid, we can't return it
	wmclSnapshot_t* clSnap = &cl.wm_snapshots[ snapshotNumber & PACKET_MASK_Q3 ];
	if ( !clSnap->valid ) {
		return false;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES_Q3 ) {
		return false;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy( snapshot->areamask, clSnap->areamask, sizeof ( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	int count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT_WM ) {
		common->DPrintf( "CLWM_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT_WM );
		count = MAX_ENTITIES_IN_SNAPSHOT_WM;
	}
	snapshot->numEntities = count;
	for ( int i = 0; i < count; i++ ) {
		snapshot->entities[ i ] =
			cl.wm_parseEntities[ ( clSnap->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES_Q3 - 1 ) ];
	}

	// FIXME: configstring changes and server commands!!!

	return true;
}

static void CLWM_SetUserCmdValue( int userCmdValue, int holdableValue, float sensitivityScale, int mpSetup, int mpIdentClient ) {
	cl.q3_cgameUserCmdValue = userCmdValue;
	cl.wb_cgameUserHoldableValue = holdableValue;
	cl.q3_cgameSensitivity = sensitivityScale;
	cl.wm_cgameMpSetup = mpSetup;				// NERVE - SMF
	cl.wm_cgameMpIdentClient = mpIdentClient;	// NERVE - SMF
}

static void CLWM_SetClientLerpOrigin( float x, float y, float z ) {
	cl.wm_cgameClientLerpOrigin[ 0 ] = x;
	cl.wm_cgameClientLerpOrigin[ 1 ] = y;
	cl.wm_cgameClientLerpOrigin[ 2 ] = z;
}

//	The cgame module is making a system call
qintptr CLWM_CgameSystemCalls( qintptr* args ) {
	switch ( args[ 0 ] ) {
	case WMCG_PRINT:
		common->Printf( "%s", ( char* )VMA( 1 ) );
		return 0;
	case WMCG_ERROR:
		common->Error( "%s", ( char* )VMA( 1 ) );
		return 0;
	case WMCG_MILLISECONDS:
		return Sys_Milliseconds();
	case WMCG_CVAR_REGISTER:
		Cvar_Register( ( vmCvar_t* )VMA( 1 ), ( char* )VMA( 2 ), ( char* )VMA( 3 ), args[ 4 ] );
		return 0;
	case WMCG_CVAR_UPDATE:
		Cvar_Update( ( vmCvar_t* )VMA( 1 ) );
		return 0;
	case WMCG_CVAR_SET:
		Cvar_Set( ( char* )VMA( 1 ), ( char* )VMA( 2 ) );
		return 0;
	case WMCG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( ( char* )VMA( 1 ), ( char* )VMA( 2 ), args[ 3 ] );
		return 0;
	case WMCG_ARGC:
		return Cmd_Argc();
	case WMCG_ARGV:
		Cmd_ArgvBuffer( args[ 1 ], ( char* )VMA( 2 ), args[ 3 ] );
		return 0;
	case WMCG_ARGS:
		Cmd_ArgsBuffer( ( char* )VMA( 1 ), args[ 2 ] );
		return 0;
	case WMCG_FS_FOPENFILE:
		return FS_FOpenFileByMode( ( char* )VMA( 1 ), ( fileHandle_t* )VMA( 2 ), ( fsMode_t )args[ 3 ] );
	case WMCG_FS_READ:
		FS_Read( VMA( 1 ), args[ 2 ], args[ 3 ] );
		return 0;
	case WMCG_FS_WRITE:
		return FS_Write( VMA( 1 ), args[ 2 ], args[ 3 ] );
	case WMCG_FS_FCLOSEFILE:
		FS_FCloseFile( args[ 1 ] );
		return 0;
	case WMCG_SENDCONSOLECOMMAND:
		Cbuf_AddText( ( char* )VMA( 1 ) );
		return 0;
	case WMCG_ADDCOMMAND:
		CLT3_AddCgameCommand( ( char* )VMA( 1 ) );
		return 0;
	case WMCG_REMOVECOMMAND:
		Cmd_RemoveCommand( ( char* )VMA( 1 ) );
		return 0;
	case WMCG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand( ( char* )VMA( 1 ) );
		return 0;
	case WMCG_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;
	case WMCG_CM_LOADMAP:
		CLT3_CM_LoadMap( ( char* )VMA( 1 ) );
		return 0;
	case WMCG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case WMCG_CM_INLINEMODEL:
		return CM_InlineModel( args[ 1 ] );
	case WMCG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel( ( float* )VMA( 1 ), ( float* )VMA( 2 ), false );
	case WMCG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel( ( float* )VMA( 1 ), ( float* )VMA( 2 ), true );
	case WMCG_CM_POINTCONTENTS:
		return CM_PointContentsQ3( ( float* )VMA( 1 ), args[ 2 ] );
	case WMCG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContentsQ3( ( float* )VMA( 1 ), args[ 2 ], ( float* )VMA( 3 ), ( float* )VMA( 4 ) );
	case WMCG_CM_BOXTRACE:
		CM_BoxTraceQ3( ( q3trace_t* )VMA( 1 ), ( float* )VMA( 2 ), ( float* )VMA( 3 ), ( float* )VMA( 4 ), ( float* )VMA( 5 ), args[ 6 ], args[ 7 ], /*int capsule*/ false );
		return 0;
	case WMCG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTraceQ3( ( q3trace_t* )VMA( 1 ), ( float* )VMA( 2 ), ( float* )VMA( 3 ), ( float* )VMA( 4 ), ( float* )VMA( 5 ), args[ 6 ], args[ 7 ], ( float* )VMA( 8 ), ( float* )VMA( 9 ),	/*int capsule*/ false );
		return 0;
	case WMCG_CM_CAPSULETRACE:
		CM_BoxTraceQ3( ( q3trace_t* )VMA( 1 ), ( float* )VMA( 2 ), ( float* )VMA( 3 ), ( float* )VMA( 4 ), ( float* )VMA( 5 ), args[ 6 ], args[ 7 ], /*int capsule*/ true );
		return 0;
	case WMCG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTraceQ3( ( q3trace_t* )VMA( 1 ), ( float* )VMA( 2 ), ( float* )VMA( 3 ), ( float* )VMA( 4 ), ( float* )VMA( 5 ), args[ 6 ], args[ 7 ], ( float* )VMA( 8 ), ( float* )VMA( 9 ),	/*int capsule*/ true );
		return 0;
	case WMCG_CM_MARKFRAGMENTS:
		return R_MarkFragmentsWolf( args[ 1 ], ( const vec3_t* )VMA( 2 ), ( float* )VMA( 3 ), args[ 4 ], ( float* )VMA( 5 ), args[ 6 ], ( markFragment_t* )VMA( 7 ) );
	case WMCG_S_STARTSOUND:
		S_StartSound( ( float* )VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ], 0.5 );
		return 0;
	case WMCG_S_STARTSOUNDEX:
		S_StartSoundEx( ( float* )VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ], 127 );
		return 0;
	case WMCG_S_STARTLOCALSOUND:
		S_StartLocalSound( args[ 1 ], args[ 2 ], 127 );
		return 0;
	case WMCG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds( true );	// (SA) modified so no_pvs sounds can function
		return 0;
	case WMCG_S_ADDLOOPINGSOUND:
		// FIXME MrE: handling of looping sounds changed
		S_AddLoopingSound( args[ 1 ], ( float* )VMA( 2 ), ( float* )VMA( 3 ), args[ 4 ], args[ 5 ], args[ 6 ], 0 );
		return 0;
	case WMCG_S_ADDREALLOOPINGSOUND:
		S_AddLoopingSound( args[ 1 ], ( float* )VMA( 2 ), ( float* )VMA( 3 ), args[ 4 ], args[ 5 ], args[ 6 ], 0 );
		return 0;
	case WMCG_S_STOPLOOPINGSOUND:
		// RF, not functional anymore, since we reverted to old looping code
		return 0;
	case WMCG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition( args[ 1 ], ( float* )VMA( 2 ) );
		return 0;
	case WMCG_S_GETVOICEAMPLITUDE:
		return S_GetVoiceAmplitude( args[ 1 ] );
	case WMCG_S_RESPATIALIZE:
		S_Respatialize( args[ 1 ], ( float* )VMA( 2 ), ( vec3_t* )VMA( 3 ), args[ 4 ] );
		return 0;
	case WMCG_S_REGISTERSOUND:
		return S_RegisterSound( ( char* )VMA( 1 ) );
	case WMCG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( ( char* )VMA( 1 ), ( char* )VMA( 2 ), 0 );
		return 0;
	case WMCG_S_STARTSTREAMINGSOUND:
		S_StartStreamingSound( ( char* )VMA( 1 ), ( char* )VMA( 2 ), args[ 3 ], args[ 4 ], args[ 5 ] );
		return 0;
	case WMCG_R_LOADWORLDMAP:
		R_LoadWorld( ( char* )VMA( 1 ) );
		return 0;
	case WMCG_R_REGISTERMODEL:
		return R_RegisterModel( ( char* )VMA( 1 ) );
	case WMCG_R_REGISTERSKIN:
		return R_RegisterSkin( ( char* )VMA( 1 ) );
	case WMCG_R_GETSKINMODEL:
		return R_GetSkinModel( args[ 1 ], ( char* )VMA( 2 ), ( char* )VMA( 3 ) );
	case WMCG_R_GETMODELSHADER:
		return R_GetShaderFromModel( args[ 1 ], args[ 2 ], args[ 3 ] );
	case WMCG_R_REGISTERSHADER:
		return R_RegisterShader( ( char* )VMA( 1 ) );
	case WMCG_R_REGISTERFONT:
		R_RegisterFont( ( char* )VMA( 1 ), args[ 2 ], ( fontInfo_t* )VMA( 3 ) );
	case WMCG_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip( ( char* )VMA( 1 ) );
	case WMCG_R_CLEARSCENE:
		R_ClearScene();
		return 0;
	case WMCG_R_ADDREFENTITYTOSCENE:
		CLWM_AddRefEntityToScene( ( wmrefEntity_t* )VMA( 1 ) );
		return 0;
	case WMCG_R_ADDPOLYTOSCENE:
		R_AddPolyToScene( args[ 1 ], args[ 2 ], ( polyVert_t* )VMA( 3 ), 1 );
		return 0;
	case WMCG_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene( args[ 1 ], args[ 2 ], ( polyVert_t* )VMA( 3 ), args[ 4 ] );
		return 0;
	case WMCG_R_ADDLIGHTTOSCENE:
		R_AddLightToScene( ( float* )VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), args[ 6 ] );
		return 0;
	case WMCG_R_ADDCORONATOSCENE:
		R_AddCoronaToScene( ( float* )VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), args[ 6 ], args[ 7 ] );
		return 0;
	case WMCG_R_SETFOG:
		R_SetFog( args[ 1 ], args[ 2 ], args[ 3 ], VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ) );
		return 0;
	case WMCG_R_RENDERSCENE:
		CLWM_RenderScene( ( wmrefdef_t* )VMA( 1 ) );
		return 0;
	case WMCG_R_SETCOLOR:
		R_SetColor( ( float* )VMA( 1 ) );
		return 0;
	case WMCG_R_DRAWSTRETCHPIC:
		R_StretchPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ] );
		return 0;
	case WMCG_R_DRAWROTATEDPIC:
		R_RotatedPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ], VMF( 10 ) );
		return 0;
	case WMCG_R_DRAWSTRETCHPIC_GRADIENT:
		R_StretchPicGradient( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[ 9 ], ( float* )VMA( 10 ), args[ 11 ] );
		return 0;
	case WMCG_R_MODELBOUNDS:
		R_ModelBounds( args[ 1 ], ( float* )VMA( 2 ), ( float* )VMA( 3 ) );
		return 0;
	case WMCG_R_LERPTAG:
		return CLWM_LerpTag( ( orientation_t* )VMA( 1 ), ( wmrefEntity_t* )VMA( 2 ), ( char* )VMA( 3 ), args[ 4 ] );
	case WMCG_GETGLCONFIG:
		CLWM_GetGlconfig( ( wmglconfig_t* )VMA( 1 ) );
		return 0;
	case WMCG_GETGAMESTATE:
		CLWM_GetGameState( ( wmgameState_t* )VMA( 1 ) );
		return 0;
	case WMCG_GETCURRENTSNAPSHOTNUMBER:
		CLWM_GetCurrentSnapshotNumber( ( int* )VMA( 1 ), ( int* )VMA( 2 ) );
		return 0;
	case WMCG_GETSNAPSHOT:
		return CLWM_GetSnapshot( args[ 1 ], ( wmsnapshot_t* )VMA( 2 ) );
	case WMCG_GETSERVERCOMMAND:
		return CLT3_GetServerCommand( args[ 1 ] );
	case WMCG_GETCURRENTCMDNUMBER:
		return CLT3_GetCurrentCmdNumber();
	case WMCG_GETUSERCMD:
		return CLWM_GetUserCmd( args[ 1 ], ( wmusercmd_t* )VMA( 2 ) );
	case WMCG_SETUSERCMDVALUE:
		CLWM_SetUserCmdValue( args[ 1 ], args[ 2 ], VMF( 3 ), args[ 4 ], args[ 5 ] );
		return 0;
	case WMCG_SETCLIENTLERPORIGIN:
		CLWM_SetClientLerpOrigin( VMF( 1 ), VMF( 2 ), VMF( 3 ) );
		return 0;
	case WMCG_MEMORY_REMAINING:
		return 0x4000000;
	case WMCG_KEY_ISDOWN:
		return Key_IsDown( args[ 1 ] );
	case WMCG_KEY_GETCATCHER:
		return Key_GetCatcher();
	case WMCG_KEY_SETCATCHER:
		KeyWM_SetCatcher( args[ 1 ] );
		return 0;
	case WMCG_KEY_GETKEY:
		return Key_GetKey( ( char* )VMA( 1 ) );

	case WMCG_MEMSET:
		return ( qintptr )memset( VMA( 1 ), args[ 2 ], args[ 3 ] );
	case WMCG_MEMCPY:
		return ( qintptr )memcpy( VMA( 1 ), VMA( 2 ), args[ 3 ] );
	case WMCG_STRNCPY:
		String::NCpy( ( char* )VMA( 1 ), ( char* )VMA( 2 ), args[ 3 ] );
		return args[ 1 ];
	case WMCG_SIN:
		return FloatAsInt( sin( VMF( 1 ) ) );
	case WMCG_COS:
		return FloatAsInt( cos( VMF( 1 ) ) );
	case WMCG_ATAN2:
		return FloatAsInt( atan2( VMF( 1 ), VMF( 2 ) ) );
	case WMCG_SQRT:
		return FloatAsInt( sqrt( VMF( 1 ) ) );
	case WMCG_FLOOR:
		return FloatAsInt( floor( VMF( 1 ) ) );
	case WMCG_CEIL:
		return FloatAsInt( ceil( VMF( 1 ) ) );
	case WMCG_ACOS:
		return FloatAsInt( idMath::ACos( VMF( 1 ) ) );

	case WMCG_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine( ( char* )VMA( 1 ) );
	case WMCG_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle( ( char* )VMA( 1 ) );
	case WMCG_PC_FREE_SOURCE:
		return PC_FreeSourceHandle( args[ 1 ] );
	case WMCG_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3( args[ 1 ], ( q3pc_token_t* )VMA( 2 ) );
	case WMCG_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine( args[ 1 ], ( char* )VMA( 2 ), ( int* )VMA( 3 ) );

	case WMCG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case WMCG_REAL_TIME:
		return Com_RealTime( ( qtime_t* )VMA( 1 ) );
	case WMCG_SNAPVECTOR:
		Sys_SnapVector( ( float* )VMA( 1 ) );
		return 0;

	case WMCG_SENDMOVESPEEDSTOGAME:
		SVWM_SendMoveSpeedsToGame( args[ 1 ], ( char* )VMA( 2 ) );
		return 0;

	case WMCG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematicStretched( ( char* )VMA( 1 ), args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ], args[ 6 ] );
	case WMCG_CIN_STOPCINEMATIC:
		return CIN_StopCinematic( args[ 1 ] );
	case WMCG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic( args[ 1 ] );
	case WMCG_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic( args[ 1 ] );
		return 0;
	case WMCG_CIN_SETEXTENTS:
		CIN_SetExtents( args[ 1 ], args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ] );
		return 0;

	case WMCG_R_REMAP_SHADER:
		R_RemapShader( ( char* )VMA( 1 ), ( char* )VMA( 2 ), ( char* )VMA( 3 ) );
		return 0;

	case WMCG_TESTPRINTINT:
		common->Printf( "%s%i\n", ( char* )VMA( 1 ), static_cast<int>( args[ 2 ] ) );
		return 0;
	case WMCG_TESTPRINTFLOAT:
		common->Printf( "%s%f\n", ( char* )VMA( 1 ), VMF( 2 ) );
		return 0;

	case WMCG_LOADCAMERA:
		return loadCamera( args[ 1 ], ( char* )VMA( 2 ) );
	case WMCG_STARTCAMERA:
		startCamera( args[ 1 ], args[ 2 ] );
		return 0;
	case WMCG_GETCAMERAINFO:
		return getCameraInfo( args[ 1 ], args[ 2 ], ( float* )VMA( 3 ), ( float* )VMA( 4 ), ( float* )VMA( 5 ) );

	case WMCG_GET_ENTITY_TOKEN:
		return R_GetEntityToken( ( char* )VMA( 1 ), args[ 2 ] );

	case WMCG_INGAME_POPUP:
		CLWM_InGamePopup( ( char* )VMA( 1 ) );
		return 0;

	case WMCG_INGAME_CLOSEPOPUP:
		CLWM_InGameClosePopup( ( char* )VMA( 1 ) );
		return 0;

	case WMCG_LIMBOCHAT:
		CLT3_AddToLimboChat( ( char* )VMA( 1 ) );
		return 0;

	case WMCG_KEY_GETBINDINGBUF:
		Key_GetBindingBuf( args[ 1 ], ( char* )VMA( 2 ), args[ 3 ] );
		return 0;
	case WMCG_KEY_SETBINDING:
		Key_SetBinding( args[ 1 ], ( char* )VMA( 2 ) );
		return 0;
	case WMCG_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf( args[ 1 ], ( char* )VMA( 2 ), args[ 3 ] );
		return 0;

	case WMCG_TRANSLATE_STRING:
		CL_TranslateString( ( char* )VMA( 1 ), ( char* )VMA( 2 ) );
		return 0;

	default:
		common->Error( "Bad cgame system trap: %i", static_cast<int>( args[ 0 ] ) );
	}
	return 0;
}
