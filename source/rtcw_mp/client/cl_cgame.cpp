/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cl_cgame.c  -- client system interaction with client game

#include "client.h"

#include "../game/botlib.h"

extern botlib_export_t *botlib_export;

extern qboolean loadCamera( int camNum, const char *name );
extern void startCamera( int camNum, int time );
extern qboolean getCameraInfo( int camNum, int time, float* origin, float* angles, float *fov );

// RF, this is only used when running a local server
extern void SV_SendMoveSpeedsToGame( int entnum, char *text );

// NERVE - SMF
void Key_GetBindingBuf( int keynum, char *buf, int buflen );
void Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
// -NERVE - SMF

/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState( wmgameState_t *gs ) {
	*gs = cl.wm_gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig( wmglconfig_t *glconfig ) {
	String::NCpyZ(glconfig->renderer_string, cls.glconfig.renderer_string, sizeof(glconfig->renderer_string));
	String::NCpyZ(glconfig->vendor_string, cls.glconfig.vendor_string, sizeof(glconfig->vendor_string));
	String::NCpyZ(glconfig->version_string, cls.glconfig.version_string, sizeof(glconfig->version_string));
	String::NCpyZ(glconfig->extensions_string, cls.glconfig.extensions_string, sizeof(glconfig->extensions_string));
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


/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd( int cmdNumber, wmusercmd_t *ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.q3_cmdNumber ) {
		Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.q3_cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.q3_cmdNumber - CMD_BACKUP_Q3 ) {
		return qfalse;
	}

	*ucmd = cl.wm_cmds[ cmdNumber & CMD_MASK_Q3 ];

	return qtrue;
}

int CL_GetCurrentCmdNumber( void ) {
	return cl.q3_cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
qboolean    CL_GetParseEntityState( int parseEntityNumber, wmentityState_t *state ) {
	// can't return anything that hasn't been parsed yet
	if ( parseEntityNumber >= cl.parseEntitiesNum ) {
		Com_Error( ERR_DROP, "CL_GetParseEntityState: %i >= %i",
				   parseEntityNumber, cl.parseEntitiesNum );
	}

	// can't return anything that has been overwritten in the circular buffer
	if ( parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES_Q3 ) {
		return qfalse;
	}

	*state = cl.wm_parseEntities[ parseEntityNumber & ( MAX_PARSE_ENTITIES_Q3 - 1 ) ];
	return qtrue;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void    CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.wm_snap.messageNum;
	*serverTime = cl.wm_snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean    CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	wmclSnapshot_t    *clSnap;
	int i, count;

	if ( snapshotNumber > cl.wm_snap.messageNum ) {
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.wm_snap.messageNum - snapshotNumber >= PACKET_BACKUP_Q3 ) {
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.wm_snapshots[snapshotNumber & PACKET_MASK_Q3];
	if ( !clSnap->valid ) {
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES_Q3 ) {
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for ( i = 0 ; i < count ; i++ ) {
		snapshot->entities[i] =
			cl.wm_parseEntities[ ( clSnap->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES_Q3 - 1 ) ];
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

/*
==============
CL_SetUserCmdValue
==============
*/
void CL_SetUserCmdValue( int userCmdValue, int holdableValue, float sensitivityScale, int mpSetup, int mpIdentClient ) {
	cl.q3_cgameUserCmdValue        = userCmdValue;
	cl.wb_cgameUserHoldableValue   = holdableValue;
	cl.q3_cgameSensitivity         = sensitivityScale;
	cl.wm_cgameMpSetup             = mpSetup;              // NERVE - SMF
	cl.wm_cgameMpIdentClient       = mpIdentClient;        // NERVE - SMF
}

/*
==================
CL_SetClientLerpOrigin
==================
*/
void CL_SetClientLerpOrigin( float x, float y, float z ) {
	cl.wm_cgameClientLerpOrigin[0] = x;
	cl.wm_cgameClientLerpOrigin[1] = y;
	cl.wm_cgameClientLerpOrigin[2] = z;
}

/*
==============
CL_AddCgameCommand
==============
*/
void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

/*
==============
CL_CgameError
==============
*/
void CL_CgameError( const char *string ) {
	Com_Error( ERR_DROP, "%s", string );
}


/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void ) {
	char        *old, *s;
	int i, index;
	char        *dup;
	wmgameState_t oldGs;
	int len;

	index = String::Atoi( Cmd_Argv( 1 ) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS_WM ) {
		Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS_WM" );
	}
//	s = Cmd_Argv(2);
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom( 2 );

	old = cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[ index ];
	if ( !String::Cmp( old, s ) ) {
		return;     // unchanged
	}

	// build the new wmgameState_t
	oldGs = cl.wm_gameState;

	memset( &cl.wm_gameState, 0, sizeof( cl.wm_gameState ) );

	// leave the first 0 for uninitialized strings
	cl.wm_gameState.dataCount = 1;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS_WM ; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;       // leave with the default empty string
		}

		len = String::Length( dup );

		if ( len + 1 + cl.wm_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3 ) {
			Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS_Q3 exceeded" );
		}

		// append it to the gameState string buffer
		cl.wm_gameState.stringOffsets[ i ] = cl.wm_gameState.dataCount;
		memcpy( cl.wm_gameState.stringData + cl.wm_gameState.dataCount, dup, len + 1 );
		cl.wm_gameState.dataCount += len + 1;
	}

	if ( index == Q3CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand( int serverCommandNumber ) {
	char    *s;
	char    *cmd;
	static char bigConfigString[BIG_INFO_STRING];
	int argc;

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.q3_serverCommandSequence - MAX_RELIABLE_COMMANDS_WM ) {
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying ) {
			return qfalse;
		}
		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
		return qfalse;
	}

	if ( serverCommandNumber > clc.q3_serverCommandSequence ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return qfalse;
	}

	s = clc.q3_serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS_WM - 1 ) ];
	clc.q3_lastExecutedServerCommand = serverCommandNumber;

	if ( cl_showServerCommands->integer ) {         // NERVE - SMF
		Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );
	}

rescan:
	Cmd_TokenizeString( s );
	cmd = Cmd_Argv( 0 );
	argc = Cmd_Argc();

	if ( !String::Cmp( cmd, "disconnect" ) ) {
		// NERVE - SMF - allow server to indicate why they were disconnected
		if ( argc >= 2 ) {
			Com_Error( ERR_SERVERDISCONNECT, va( "Server Disconnected - %s", Cmd_Argv( 1 ) ) );
		} else {
			Com_Error( ERR_SERVERDISCONNECT,"Server disconnected\n" );
		}
	}

	if ( !String::Cmp( cmd, "bcs0" ) ) {
		String::Sprintf( bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
		return qfalse;
	}

	if ( !String::Cmp( cmd, "bcs1" ) ) {
		s = Cmd_Argv( 2 );
		if ( String::Length( bigConfigString ) + String::Length( s ) >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		return qfalse;
	}

	if ( !String::Cmp( cmd, "bcs2" ) ) {
		s = Cmd_Argv( 2 );
		if ( String::Length( bigConfigString ) + String::Length( s ) + 1 >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		strcat( bigConfigString, "\"" );
		s = bigConfigString;
		goto rescan;
	}

	if ( !String::Cmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	if ( !String::Cmp( cmd, "map_restart" ) ) {
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		memset( cl.wm_cmds, 0, sizeof( cl.wm_cmds ) );
		return qtrue;
	}

	if ( !String::Cmp( cmd, "popup" ) ) { // direct server to client popup request, bypassing cgame
//		trap_UI_Popup(Cmd_Argv(1));
//		if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
//			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_CLIPBOARD);
//			Menus_OpenByName(Cmd_Argv(1));
//		}
		return qfalse;
	}


	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if ( !String::Cmp( cmd, "clientLevelShot" ) ) {
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if ( !com_sv_running->integer ) {
			return qfalse;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText( "wait ; wait ; wait ; wait ; screenshot levelshot\n" );
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}

// DHM - Nerve :: Copied from server to here
/*
====================
CL_SetExpectedHunkUsage

  Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
void CL_SetExpectedHunkUsage( const char *mapname ) {
	int handle;
	const char *memlistfile = "hunkusage.dat";
	char *buf;
	const char *buftrav;
	char *token;
	int len;

	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );
	if ( len >= 0 ) { // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value

		buf = (char *)Z_Malloc( len + 1 );
		memset( buf, 0, len + 1 );

		FS_Read( (void *)buf, len, handle );
		FS_FCloseFile( handle );

		// now parse the file, filtering out the current map
		buftrav = buf;
		while ( ( token = String::Parse3( &buftrav ) ) && token[0] ) {
			if ( !String::ICmp( token, (char *)mapname ) ) {
				// found a match
				token = String::Parse3( &buftrav );  // read the size
				if ( token && token[0] ) {
					// this is the usage
					Cvar_Set( "com_expectedhunkusage", token );
					Z_Free( buf );
					return;
				}
			}
		}

		Z_Free( buf );
	}
	// just set it to a negative number,so the cgame knows not to draw the percent bar
	Cvar_Set( "com_expectedhunkusage", "-1" );
}

// dhm - nerve

/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap( const char *mapname ) {
	int checksum;

	// DHM - Nerve :: If we are not running the server, then set expected usage here
	if ( !com_sv_running->integer ) {
		CL_SetExpectedHunkUsage( mapname );
	} else
	{
		// TTimo
		// catch here when a local server is started to avoid outdated com_errorDiagnoseIP
		Cvar_Set( "com_errorDiagnoseIP", "" );
	}

	CM_LoadMap( mapname, qtrue, &checksum );
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame( void ) {
	in_keyCatchers &= ~KEYCATCH_CGAME;
	cls.q3_cgameStarted = qfalse;
	if ( !cgvm ) {
		return;
	}
	VM_Call( cgvm, CG_SHUTDOWN );
	VM_Free( cgvm );
	cgvm = NULL;
}

static refEntityType_t gameRefEntTypeToEngine[] =
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

static void CL_GameRefEntToEngine(const wmrefEntity_t* gameRefent, refEntity_t* refent)
{
	Com_Memset(refent, 0, sizeof(*refent));
	if (gameRefent->reType < 0 || gameRefent->reType >= 10)
	{
		refent->reType = RT_MAX_REF_ENTITY_TYPE;
	}
	else
	{
		refent->reType = gameRefEntTypeToEngine[gameRefent->reType];
	}
	refent->renderfx = gameRefent->renderfx & (RF_MINLIGHT | RF_THIRD_PERSON |
		RF_FIRST_PERSON | RF_DEPTHHACK | RF_NOSHADOW | RF_LIGHTING_ORIGIN |
		RF_SHADOW_PLANE | RF_WRAP_FRAMES);
	if (gameRefent->renderfx & WMRF_BLINK)
	{
		refent->renderfx |= RF_BLINK;
	}
	refent->hModel = gameRefent->hModel;
	VectorCopy(gameRefent->lightingOrigin, refent->lightingOrigin);
	refent->shadowPlane = gameRefent->shadowPlane;
	AxisCopy(gameRefent->axis, refent->axis);
	AxisCopy(gameRefent->torsoAxis, refent->torsoAxis);
	refent->nonNormalizedAxes = gameRefent->nonNormalizedAxes;
	VectorCopy(gameRefent->origin, refent->origin);
	refent->frame = gameRefent->frame;
	refent->torsoFrame = gameRefent->torsoFrame;
	VectorCopy(gameRefent->oldorigin, refent->oldorigin);
	refent->oldframe = gameRefent->oldframe;
	refent->oldTorsoFrame = gameRefent->oldTorsoFrame;
	refent->backlerp = gameRefent->backlerp;
	refent->torsoBacklerp = gameRefent->torsoBacklerp;
	refent->skinNum = gameRefent->skinNum;
	refent->customSkin = gameRefent->customSkin;
	refent->customShader = gameRefent->customShader;
	refent->shaderRGBA[0] = gameRefent->shaderRGBA[0];
	refent->shaderRGBA[1] = gameRefent->shaderRGBA[1];
	refent->shaderRGBA[2] = gameRefent->shaderRGBA[2];
	refent->shaderRGBA[3] = gameRefent->shaderRGBA[3];
	refent->shaderTexCoord[0] = gameRefent->shaderTexCoord[0];
	refent->shaderTexCoord[1] = gameRefent->shaderTexCoord[1];
	refent->shaderTime = gameRefent->shaderTime;
	refent->radius = gameRefent->radius;
	refent->rotation = gameRefent->rotation;
	VectorCopy(gameRefent->fireRiseDir, refent->fireRiseDir);
	refent->fadeStartTime = gameRefent->fadeStartTime;
	refent->fadeEndTime = gameRefent->fadeEndTime;
	refent->hilightIntensity = gameRefent->hilightIntensity;
	refent->reFlags = gameRefent->reFlags & (REFLAG_ONLYHAND |
		REFLAG_FORCE_LOD | REFLAG_ORIENT_LOD | REFLAG_DEAD_LOD);
	refent->entityNum = gameRefent->entityNum;
}

void CL_AddRefEntityToScene(const wmrefEntity_t* ent)
{
	refEntity_t refent;
	CL_GameRefEntToEngine(ent, &refent);
	R_AddRefEntityToScene(&refent);
}

void CL_RenderScene(const wmrefdef_t* gameRefdef)
{
	refdef_t rd;
	Com_Memset(&rd, 0, sizeof(rd));
	rd.x = gameRefdef->x;
	rd.y = gameRefdef->y;
	rd.width = gameRefdef->width;
	rd.height = gameRefdef->height;
	rd.fov_x = gameRefdef->fov_x;
	rd.fov_y = gameRefdef->fov_y;
	VectorCopy(gameRefdef->vieworg, rd.vieworg);
	AxisCopy(gameRefdef->viewaxis, rd.viewaxis);
	rd.time = gameRefdef->time;
	rd.rdflags = gameRefdef->rdflags & (RDF_NOWORLDMODEL | RDF_HYPERSPACE |
		RDF_SKYBOXPORTAL | RDF_UNDERWATER | RDF_DRAWINGSKY | RDF_SNOOPERVIEW);
	Com_Memcpy(rd.areamask, gameRefdef->areamask, sizeof(rd.areamask));
	Com_Memcpy(rd.text, gameRefdef->text, sizeof(rd.text));
	rd.glfog.mode = gameRefdef->glfog.mode;
	rd.glfog.hint = gameRefdef->glfog.hint;
	rd.glfog.startTime = gameRefdef->glfog.startTime;
	rd.glfog.finishTime = gameRefdef->glfog.finishTime;
	Vector4Copy(gameRefdef->glfog.color, rd.glfog.color);
	rd.glfog.start = gameRefdef->glfog.start;
	rd.glfog.end = gameRefdef->glfog.end;
	rd.glfog.useEndForClip = gameRefdef->glfog.useEndForClip;
	rd.glfog.density = gameRefdef->glfog.density;
	rd.glfog.registered = gameRefdef->glfog.registered;
	rd.glfog.drawsky = gameRefdef->glfog.drawsky;
	rd.glfog.clearscreen = gameRefdef->glfog.clearscreen;
	R_RenderScene(&rd);
}

int CL_LerpTag(orientation_t *tag,  const wmrefEntity_t *gameRefent, const char *tagName, int startIndex)
{
	refEntity_t refent;
	CL_GameRefEntToEngine(gameRefent, &refent);
	return R_LerpTag(tag, &refent, tagName, startIndex);
}

static int  FloatAsInt( float f ) {
	int temp;

	*(float *)&temp = f;

	return temp;
}

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
qintptr CL_CgameSystemCalls( qintptr* args ) {
	switch ( args[0] ) {
	case CG_PRINT:
		Com_Printf( "%s", VMA( 1 ) );
		return 0;
	case CG_ERROR:
		Com_Error( ERR_DROP, "%s", VMA( 1 ) );
		return 0;
	case CG_MILLISECONDS:
		return Sys_Milliseconds();
	case CG_CVAR_REGISTER:
		Cvar_Register( (vmCvar_t*)VMA( 1 ), (char*)VMA( 2 ), (char*)VMA( 3 ), args[4] );
		return 0;
	case CG_CVAR_UPDATE:
		Cvar_Update( (vmCvar_t*)VMA( 1 ) );
		return 0;
	case CG_CVAR_SET:
		Cvar_Set( (char*)VMA( 1 ), (char*)VMA( 2 ) );
		return 0;
	case CG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( (char*)VMA( 1 ), (char*)VMA( 2 ), args[3] );
		return 0;
	case CG_ARGC:
		return Cmd_Argc();
	case CG_ARGV:
		Cmd_ArgvBuffer( args[1], (char*)VMA( 2 ), args[3] );
		return 0;
	case CG_ARGS:
		Cmd_ArgsBuffer( (char*)VMA( 1 ), args[2] );
		return 0;
	case CG_FS_FOPENFILE:
		return FS_FOpenFileByMode( (char*)VMA( 1 ), (fileHandle_t*)VMA( 2 ), (fsMode_t)args[3] );
	case CG_FS_READ:
		FS_Read( VMA( 1 ), args[2], args[3] );
		return 0;
	case CG_FS_WRITE:
		return FS_Write( VMA( 1 ), args[2], args[3] );
	case CG_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;
	case CG_SENDCONSOLECOMMAND:
		Cbuf_AddText( (char*)VMA( 1 ) );
		return 0;
	case CG_ADDCOMMAND:
		CL_AddCgameCommand( (char*)VMA( 1 ) );
		return 0;
	case CG_REMOVECOMMAND:
		Cmd_RemoveCommand( (char*)VMA( 1 ) );
		return 0;
	case CG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand( (char*)VMA( 1 ) );
		return 0;
	case CG_UPDATESCREEN:
		// this is used during lengthy level loading, so pump message loop
//		Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
// We can't call Com_EventLoop here, a restart will crash and this _does_ happen
// if there is a map change while we are downloading at pk3.
// ZOID
		SCR_UpdateScreen();
		return 0;
	case CG_CM_LOADMAP:
		CL_CM_LoadMap( (char*)VMA( 1 ) );
		return 0;
	case CG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case CG_CM_INLINEMODEL:
		return CM_InlineModel( args[1] );
	case CG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel( (float*)VMA( 1 ), (float*)VMA( 2 ), qfalse );
	case CG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel( (float*)VMA( 1 ), (float*)VMA( 2 ), qtrue );
	case CG_CM_POINTCONTENTS:
		return CM_PointContentsQ3( (float*)VMA( 1 ), args[2] );
	case CG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContentsQ3( (float*)VMA( 1 ), args[2], (float*)VMA( 3 ), (float*)VMA( 4 ) );
	case CG_CM_BOXTRACE:
		CM_BoxTraceQ3( (q3trace_t*)VMA( 1 ), (float*)VMA( 2 ), (float*)VMA( 3 ), (float*)VMA( 4 ), (float*)VMA( 5 ), args[6], args[7], /*int capsule*/ qfalse );
		return 0;
	case CG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTraceQ3( (q3trace_t*)VMA( 1 ), (float*)VMA( 2 ), (float*)VMA( 3 ), (float*)VMA( 4 ), (float*)VMA( 5 ), args[6], args[7], (float*)VMA( 8 ), (float*)VMA( 9 ), /*int capsule*/ qfalse );
		return 0;
	case CG_CM_CAPSULETRACE:
		CM_BoxTraceQ3( (q3trace_t*)VMA( 1 ), (float*)VMA( 2 ), (float*)VMA( 3 ), (float*)VMA( 4 ), (float*)VMA( 5 ), args[6], args[7], /*int capsule*/ qtrue );
		return 0;
	case CG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTraceQ3( (q3trace_t*)VMA( 1 ), (float*)VMA( 2 ), (float*)VMA( 3 ), (float*)VMA( 4 ), (float*)VMA( 5 ), args[6], args[7], (float*)VMA( 8 ), (float*)VMA( 9 ), /*int capsule*/ qtrue );
		return 0;
	case CG_CM_MARKFRAGMENTS:
		return R_MarkFragmentsWolf( args[1], (const vec3_t*)VMA( 2 ), (float*)VMA( 3 ), args[4], (float*)VMA( 5 ), args[6], (markFragment_t*)VMA( 7 ) );
	case CG_S_STARTSOUND:
		S_StartSound( (float*)VMA( 1 ), args[2], args[3], args[4], 0.5 );
		return 0;
//----(SA)	added
	case CG_S_STARTSOUNDEX:
		S_StartSoundEx( (float*)VMA( 1 ), args[2], args[3], args[4], args[5], 127 );
		return 0;
//----(SA)	end
	case CG_S_STARTLOCALSOUND:
		S_StartLocalSound( args[1], args[2], 127 );
		return 0;
	case CG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds(true); // (SA) modified so no_pvs sounds can function
		return 0;
	case CG_S_ADDLOOPINGSOUND:
		// FIXME MrE: handling of looping sounds changed
		S_AddLoopingSound( args[1], (float*)VMA( 2 ), (float*)VMA( 3 ), args[4], args[5], args[6], 0 );
		return 0;
	case CG_S_ADDREALLOOPINGSOUND:
		S_AddLoopingSound( args[1], (float*)VMA( 2 ), (float*)VMA( 3 ), args[4], args[5], args[6], 0 );
		//S_AddRealLoopingSound( args[1], VMA(2), VMA(3), args[4], args[5] );
		return 0;
	case CG_S_STOPLOOPINGSOUND:
		// RF, not functional anymore, since we reverted to old looping code
		//S_StopLoopingSound( args[1] );
		return 0;
	case CG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition( args[1], (float*)VMA( 2 ) );
		return 0;
// Ridah, talking animations
	case CG_S_GETVOICEAMPLITUDE:
		return S_GetVoiceAmplitude( args[1] );
// done.
	case CG_S_RESPATIALIZE:
		S_Respatialize( args[1], (float*)VMA( 2 ), (vec3_t*)VMA( 3 ), args[4] );
		return 0;
	case CG_S_REGISTERSOUND:
		return S_RegisterSound( (char*)VMA( 1 ));
	case CG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( (char*)VMA( 1 ), (char*)VMA( 2 ), 0 );
		return 0;
	case CG_S_STARTSTREAMINGSOUND:
		S_StartStreamingSound( (char*)VMA( 1 ), (char*)VMA( 2 ), args[3], args[4], args[5] );
		return 0;
	case CG_R_LOADWORLDMAP:
		R_LoadWorld( (char*)VMA( 1 ) );
		return 0;
	case CG_R_REGISTERMODEL:
		return R_RegisterModel( (char*)VMA( 1 ) );
	case CG_R_REGISTERSKIN:
		return R_RegisterSkin( (char*)VMA( 1 ) );

		//----(SA)	added
	case CG_R_GETSKINMODEL:
		return R_GetSkinModel( args[1], (char*)VMA( 2 ), (char*)VMA( 3 ) );
	case CG_R_GETMODELSHADER:
		return R_GetShaderFromModel( args[1], args[2], args[3] );
		//----(SA)	end

	case CG_R_REGISTERSHADER:
		return R_RegisterShader( (char*)VMA( 1 ) );
	case CG_R_REGISTERFONT:
		R_RegisterFont( (char*)VMA( 1 ), args[2], (fontInfo_t*)VMA( 3 ) );
	case CG_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip( (char*)VMA( 1 ) );
	case CG_R_CLEARSCENE:
		R_ClearScene();
		return 0;
	case CG_R_ADDREFENTITYTOSCENE:
		CL_AddRefEntityToScene( (wmrefEntity_t*)VMA( 1 ) );
		return 0;
	case CG_R_ADDPOLYTOSCENE:
		R_AddPolyToScene( args[1], args[2], (polyVert_t*)VMA( 3 ), 1 );
		return 0;
		// Ridah
	case CG_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene( args[1], args[2], (polyVert_t*)VMA( 3 ), args[4] );
		return 0;
		// done.
	case CG_R_ADDLIGHTTOSCENE:
		R_AddLightToScene( (float*)VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), args[6] );
		return 0;
	case CG_R_ADDCORONATOSCENE:
		R_AddCoronaToScene( (float*)VMA( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), args[6], args[7] );
		return 0;
	case CG_R_SETFOG:
		R_SetFog( args[1], args[2], args[3], VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ) );
		return 0;
	case CG_R_RENDERSCENE:
		CL_RenderScene( (wmrefdef_t*)VMA( 1 ) );
		return 0;
	case CG_R_SETCOLOR:
		R_SetColor( (float*)VMA( 1 ) );
		return 0;
	case CG_R_DRAWSTRETCHPIC:
		R_StretchPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[9] );
		return 0;
	case CG_R_DRAWROTATEDPIC:
		R_RotatedPic( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[9], VMF( 10 ) );
		return 0;
	case CG_R_DRAWSTRETCHPIC_GRADIENT:
		R_StretchPicGradient( VMF( 1 ), VMF( 2 ), VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ), VMF( 7 ), VMF( 8 ), args[9], (float*)VMA( 10 ), args[11] );
		return 0;
	case CG_R_MODELBOUNDS:
		R_ModelBounds( args[1], (float*)VMA( 2 ), (float*)VMA( 3 ) );
		return 0;
	case CG_R_LERPTAG:
		return CL_LerpTag( (orientation_t*)VMA( 1 ), (wmrefEntity_t*)VMA( 2 ), (char*)VMA( 3 ), args[4] );
	case CG_GETGLCONFIG:
		CL_GetGlconfig( (wmglconfig_t*)VMA( 1 ) );
		return 0;
	case CG_GETGAMESTATE:
		CL_GetGameState( (wmgameState_t*)VMA( 1 ) );
		return 0;
	case CG_GETCURRENTSNAPSHOTNUMBER:
		CL_GetCurrentSnapshotNumber( (int*)VMA( 1 ), (int*)VMA( 2 ) );
		return 0;
	case CG_GETSNAPSHOT:
		return CL_GetSnapshot( args[1], (snapshot_t*)VMA( 2 ) );
	case CG_GETSERVERCOMMAND:
		return CL_GetServerCommand( args[1] );
	case CG_GETCURRENTCMDNUMBER:
		return CL_GetCurrentCmdNumber();
	case CG_GETUSERCMD:
		return CL_GetUserCmd( args[1], (wmusercmd_t*)VMA( 2 ) );
	case CG_SETUSERCMDVALUE:
		CL_SetUserCmdValue( args[1], args[2], VMF( 3 ), args[4], args[5] );
		return 0;
	case CG_SETCLIENTLERPORIGIN:
		CL_SetClientLerpOrigin( VMF( 1 ), VMF( 2 ), VMF( 3 ) );
		return 0;
	case CG_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();
	case CG_KEY_ISDOWN:
		return Key_IsDown( args[1] );
	case CG_KEY_GETCATCHER:
		return Key_GetCatcher();
	case CG_KEY_SETCATCHER:
		Key_SetCatcher( args[1] );
		return 0;
	case CG_KEY_GETKEY:
		return Key_GetKey( (char*)VMA( 1 ) );



	case CG_MEMSET:
		return (qintptr)memset( VMA( 1 ), args[2], args[3] );
	case CG_MEMCPY:
		return (qintptr)memcpy( VMA( 1 ), VMA( 2 ), args[3] );
	case CG_STRNCPY:
		String::NCpy( (char*)VMA( 1 ), (char*)VMA( 2 ), args[3] );
		return args[1];
	case CG_SIN:
		return FloatAsInt( sin( VMF( 1 ) ) );
	case CG_COS:
		return FloatAsInt( cos( VMF( 1 ) ) );
	case CG_ATAN2:
		return FloatAsInt( atan2( VMF( 1 ), VMF( 2 ) ) );
	case CG_SQRT:
		return FloatAsInt( sqrt( VMF( 1 ) ) );
	case CG_FLOOR:
		return FloatAsInt( floor( VMF( 1 ) ) );
	case CG_CEIL:
		return FloatAsInt( ceil( VMF( 1 ) ) );
	case CG_ACOS:
		return FloatAsInt( Q_acos( VMF( 1 ) ) );

	case CG_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( (char*)VMA( 1 ) );
	case CG_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( (char*)VMA( 1 ) );
	case CG_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );
	case CG_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], (pc_token_t*)VMA( 2 ) );
	case CG_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], (char*)VMA( 2 ), (int*)VMA( 3 ) );

	case CG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case CG_REAL_TIME:
		return Com_RealTime( (qtime_t*)VMA( 1 ) );
	case CG_SNAPVECTOR:
		Sys_SnapVector( (float*)VMA( 1 ) );
		return 0;

	case CG_SENDMOVESPEEDSTOGAME:
		SV_SendMoveSpeedsToGame( args[1], (char*)VMA( 2 ) );
		return 0;

	case CG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic( (char*)VMA( 1 ), args[2], args[3], args[4], args[5], args[6] );

	case CG_CIN_STOPCINEMATIC:
		return CIN_StopCinematic( args[1] );

	case CG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic( args[1] );

	case CG_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic( args[1] );
		return 0;

	case CG_CIN_SETEXTENTS:
		CIN_SetExtents( args[1], args[2], args[3], args[4], args[5] );
		return 0;

	case CG_R_REMAP_SHADER:
		R_RemapShader( (char*)VMA( 1 ), (char*)VMA( 2 ), (char*)VMA( 3 ) );
		return 0;

	case CG_TESTPRINTINT:
		Com_Printf( "%s%i\n", VMA( 1 ), args[2] );
		return 0;
	case CG_TESTPRINTFLOAT:
		Com_Printf( "%s%f\n", VMA( 1 ), VMF( 2 ) );
		return 0;

	case CG_LOADCAMERA:
		return loadCamera( args[1], (char*)VMA( 2 ) );

	case CG_STARTCAMERA:
		startCamera( args[1], args[2] );
		return 0;

	case CG_GETCAMERAINFO:
		return getCameraInfo( args[1], args[2], (float*)VMA( 3 ), (float*)VMA( 4 ), (float*)VMA( 5 ) );

	case CG_GET_ENTITY_TOKEN:
		return R_GetEntityToken( (char*)VMA( 1 ), args[2] );

	case CG_INGAME_POPUP:
		if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
			// NERVE - SMF
			if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "UIMENU_WM_PICKTEAM" ) ) {
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_PICKTEAM );
			} else if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "UIMENU_WM_PICKPLAYER" ) )    {
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_PICKPLAYER );
			} else if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "UIMENU_WM_QUICKMESSAGE" ) )    {
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_QUICKMESSAGE );
			} else if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "UIMENU_WM_QUICKMESSAGEALT" ) )    {
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_QUICKMESSAGEALT );
			} else if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "UIMENU_WM_LIMBO" ) )    {
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_LIMBO );
			} else if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "UIMENU_WM_AUTOUPDATE" ) )    {
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_WM_AUTOUPDATE );
			}
			// -NERVE - SMF
			else if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "hbook1" ) ) {   //----(SA)
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_BOOK1 );
			} else if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "hbook2" ) )    { //----(SA)
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_BOOK2 );
			} else if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "hbook3" ) )    { //----(SA)
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_BOOK3 );
			} else {
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_CLIPBOARD );
			}
		}
		return 0;

		// NERVE - SMF
	case CG_INGAME_CLOSEPOPUP:
		// if popup menu is up, then close it
		if ( (char*)VMA( 1 ) && !String::ICmp( (char*)VMA( 1 ), "UIMENU_WM_LIMBO" ) ) {
			if ( VM_Call( uivm, UI_GET_ACTIVE_MENU ) == UIMENU_WM_LIMBO ) {
				VM_Call( uivm, UI_KEY_EVENT, K_ESCAPE, qtrue );
				VM_Call( uivm, UI_KEY_EVENT, K_ESCAPE, qtrue );
			}
		}
		return 0;

	case CG_LIMBOCHAT:
		if ( VMA( 1 ) ) {
			CL_AddToLimboChat( (char*)VMA( 1 ) );
		}
		return 0;

	case CG_KEY_GETBINDINGBUF:
		Key_GetBindingBuf( args[1], (char*)VMA( 2 ), args[3] );
		return 0;

	case CG_KEY_SETBINDING:
		Key_SetBinding( args[1], (char*)VMA( 2 ) );
		return 0;

	case CG_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf( args[1], (char*)VMA( 2 ), args[3] );
		return 0;

	case CG_TRANSLATE_STRING:
		CL_TranslateString( (char*)VMA( 1 ), (char*)VMA( 2 ) );
		return 0;
		// - NERVE - SMF
	default:
		Com_Error( ERR_DROP, "Bad cgame system trap: %i", args[0] );
	}
	return 0;
}

/*
====================
CL_UpdateLevelHunkUsage

  This updates the "hunkusage.dat" file with the current map and it's hunk usage count

  This is used for level loading, so we can show a percentage bar dependant on the amount
  of hunk memory allocated so far

  This will be slightly inaccurate if some settings like sound quality are changed, but these
  things should only account for a small variation (hopefully)
====================
*/
void CL_UpdateLevelHunkUsage( void ) {
	int handle;
	const char *memlistfile = "hunkusage.dat";
	char *buf, *outbuf;
	const char *buftrav;
	char *outbuftrav;
	char *token;
	char outstr[256];
	int len, memusage;

	memusage = Cvar_VariableIntegerValue( "com_hunkused" ) + Cvar_VariableIntegerValue( "hunk_soundadjust" );

	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );
	if ( len >= 0 ) { // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value

		buf = (char *)Z_Malloc( len + 1 );
		memset( buf, 0, len + 1 );
		outbuf = (char *)Z_Malloc( len + 1 );
		memset( outbuf, 0, len + 1 );

		FS_Read( (void *)buf, len, handle );
		FS_FCloseFile( handle );

		// now parse the file, filtering out the current map
		buftrav = buf;
		outbuftrav = outbuf;
		outbuftrav[0] = '\0';
		while ( ( token = String::Parse3( &buftrav ) ) && token[0] ) {
			if ( !String::ICmp( token, cl.q3_mapname ) ) {
				// found a match
				token = String::Parse3( &buftrav );  // read the size
				if ( token && token[0] ) {
					if ( String::Atoi( token ) == memusage ) {  // if it is the same, abort this process
						Z_Free( buf );
						Z_Free( outbuf );
						return;
					}
				}
			} else {    // send it to the outbuf
				String::Cat( outbuftrav, len + 1, token );
				String::Cat( outbuftrav, len + 1, " " );
				token = String::Parse3( &buftrav );  // read the size
				if ( token && token[0] ) {
					String::Cat( outbuftrav, len + 1, token );
					String::Cat( outbuftrav, len + 1, "\n" );
				} else {
					Com_Error( ERR_DROP, "hunkusage.dat file is corrupt\n" );
				}
			}
		}

#ifdef __MACOS__    //DAJ MacOS file typing
		{
			extern _MSL_IMP_EXP_C long _fcreator, _ftype;
			_ftype = 'TEXT';
			_fcreator = 'WlfS';
		}
#endif
		handle = FS_FOpenFileWrite( memlistfile );
		if ( handle < 0 ) {
			Com_Error( ERR_DROP, "cannot create %s\n", memlistfile );
		}
		// input file is parsed, now output to the new file
		len = String::Length( outbuf );
		if ( FS_Write( (void *)outbuf, len, handle ) != len ) {
			Com_Error( ERR_DROP, "cannot write to %s\n", memlistfile );
		}
		FS_FCloseFile( handle );

		Z_Free( buf );
		Z_Free( outbuf );
	}
	// now append the current map to the current file
	FS_FOpenFileByMode( memlistfile, &handle, FS_APPEND );
	if ( handle < 0 ) {
		Com_Error( ERR_DROP, "cannot write to hunkusage.dat, check disk full\n" );
	}
	String::Sprintf( outstr, sizeof( outstr ), "%s %i\n", cl.q3_mapname, memusage );
	FS_Write( outstr, String::Length( outstr ), handle );
	FS_FCloseFile( handle );

	// now just open it and close it, so it gets copied to the pak dir
	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );
	if ( len >= 0 ) {
		FS_FCloseFile( handle );
	}
}

/*
====================
CL_InitCGame

Should only by called by CL_StartHunkUsers
====================
*/
void CL_InitCGame( void ) {
	const char          *info;
	const char          *mapname;
	int t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[ Q3CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	String::Sprintf( cl.q3_mapname, sizeof( cl.q3_mapname ), "maps/%s.bsp", mapname );

	// load the dll
	cgvm = VM_Create( "cgame", CL_CgameSystemCalls, VMI_NATIVE );
	if ( !cgvm ) {
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	VM_Call( cgvm, CG_INIT, clc.q3_serverMessageSequence, clc.q3_lastExecutedServerCommand, clc.q3_clientNum );

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf( "CL_InitCGame: %5.2f seconds\n", ( t2 - t1 ) / 1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	R_EndRegistration();

	// make sure everything is paged in
	Com_TouchMemory();

	// clear anything that got printed
	Con_ClearNotify();

	// Ridah, update the memory usage file
	CL_UpdateLevelHunkUsage();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand( void ) {
	if ( !cgvm ) {
		return qfalse;
	}

	return VM_Call( cgvm, CG_CONSOLE_COMMAND );
}



/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying );
	VM_Debug( 0 );
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define RESET_TIME  500

void CL_AdjustTimeDelta( void ) {
	int resetTime;
	int newDelta;
	int deltaDelta;

	cl.q3_newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if ( clc.demoplaying ) {
		return;
	}

	// if the current time is WAY off, just correct to the current value
	if ( com_sv_running->integer ) {
		resetTime = 100;
	} else {
		resetTime = RESET_TIME;
	}

	newDelta = cl.wm_snap.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.q3_serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.q3_serverTimeDelta = newDelta;
		cl.q3_oldServerTime = cl.wm_snap.serverTime;  // FIXME: is this a problem for cgame?
		cl.serverTime = cl.wm_snap.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl.q3_serverTimeDelta = ( cl.q3_serverTimeDelta + newDelta ) >> 1;
	} else {
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl.q3_extrapolatedSnapshot ) {
				cl.q3_extrapolatedSnapshot = qfalse;
				cl.q3_serverTimeDelta -= 2;
			} else {
				// otherwise, move our sense of time forward to minimize total latency
				cl.q3_serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.q3_serverTimeDelta );
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot( void ) {
	// ignore snapshots that don't have entities
	if ( cl.wm_snap.snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}
	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.q3_serverTimeDelta = cl.wm_snap.serverTime - cls.realtime;
	cl.q3_oldServerTime = cl.wm_snap.serverTime;

	clc.q3_timeDemoBaseTime = cl.wm_snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Cvar_Set( "activeAction", "" );
	}

	Sys_BeginProfiling();
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void ) {
	// getting a valid frame message ends the connection process
	if ( cls.state != CA_ACTIVE ) {
		if ( cls.state != CA_PRIMED ) {
			return;
		}
		if ( clc.demoplaying ) {
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.q3_firstDemoFrameSkipped ) {
				clc.q3_firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if ( cl.q3_newSnapshots ) {
			cl.q3_newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if ( cls.state != CA_ACTIVE ) {
			return;
		}
	}

	// if we have gotten to this point, cl.wm_snap is guaranteed to be valid
	if ( !cl.wm_snap.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.wm_snap.valid" );
	}

	// allow pause in single player
	if ( sv_paused->integer && cl_paused->integer && com_sv_running->integer ) {
		// paused
		return;
	}

	if ( cl.wm_snap.serverTime < cl.q3_oldFrameServerTime ) {
		// Ridah, if this is a localhost, then we are probably loading a savegame
		if ( !String::ICmp( cls.servername, "localhost" ) ) {
			// do nothing?
			CL_FirstSnapshot();
		} else {
			Com_Error( ERR_DROP, "cl.wm_snap.serverTime < cl.oldFrameServerTime" );
		}
	}
	cl.q3_oldFrameServerTime = cl.wm_snap.serverTime;


	// get our current view of time

	if ( clc.demoplaying && cl_freezeDemo->integer ) {
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	} else {
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int tn;

		tn = cl_timeNudge->integer;
		if ( tn < -30 ) {
			tn = -30;
		} else if ( tn > 30 ) {
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.q3_serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if ( cl.serverTime < cl.q3_oldServerTime ) {
			cl.serverTime = cl.q3_oldServerTime;
		}
		cl.q3_oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if ( cls.realtime + cl.q3_serverTimeDelta >= cl.wm_snap.serverTime - 5 ) {
			cl.q3_extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.q3_newSnapshots ) {
		CL_AdjustTimeDelta();
	}

	if ( !clc.demoplaying ) {
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if ( cl_timedemo->integer ) {
		if ( !clc.q3_timeDemoStart ) {
			clc.q3_timeDemoStart = Sys_Milliseconds();
		}
		clc.q3_timeDemoFrames++;
		cl.serverTime = clc.q3_timeDemoBaseTime + clc.q3_timeDemoFrames * 50;
	}

	while ( cl.serverTime >= cl.wm_snap.serverTime ) {
		// feed another messag, which should change
		// the contents of cl.wm_snap
		CL_ReadDemoMessage();
		if ( cls.state != CA_ACTIVE ) {
			return;     // end of demo
		}
	}

}

/*
====================
CL_GetTag
====================
*/
qboolean CL_GetTag( int clientNum, char *tagname, orientation_t *_or ) {
	if ( !cgvm ) {
		return qfalse;
	}

	return VM_Call( cgvm, CG_GET_TAG, clientNum, tagname, _or );
}
