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

// tr_map.c

#include "tr_local.h"

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

/*
=================
R_GetEntityToken
=================
*/
qboolean R_GetEntityToken( char *buffer, int size ) {
	const char  *s;

	s = String::Parse3(const_cast<const char**>(&s_worldData.entityParsePoint));
	String::NCpyZ( buffer, s, size );
	if ( !s_worldData.entityParsePoint || !s[0] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	} else {
		return qtrue;
	}
}

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap( const char *name ) {
	byte        *buffer;
	byte        *startMarker;

	skyboxportal = 0;

	if ( tr.worldMapLoaded ) {
		ri.Error( ERR_DROP, "ERROR: attempted to redundantly load world map\n" );
	}

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45;
	tr.sunDirection[1] = 0.3;
	tr.sunDirection[2] = 0.9;

	tr.sunShader = 0;   // clear sunshader so it's not there if the level doesn't specify it

	// inalidate fogs (likely to be re-initialized to new values by the current map)
	// TODO:(SA)this is sort of silly.  I'm going to do a general cleanup on fog stuff
	//			now that I can see how it's been used.  (functionality can narrow since
	//			it's not used as much as it's designed for.)
	R_SetFog( FOG_SKY,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_PORTALVIEW,0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_HUD,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_MAP,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_CURRENT,   0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_TARGET,    0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_WATER,     0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_SERVER,    0, 0, 0, 0, 0, 0 );

	VectorNormalize( tr.sunDirection );

	tr.worldMapLoaded = qtrue;
	tr.worldDir = NULL;

	// load it
	ri.FS_ReadFile( name, (void **)&buffer );
	if ( !buffer ) {
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s not found", name );
	}

	// ydnar: set map meta dir
	tr.worldDir = CopyString( name );
	String::StripExtension( tr.worldDir, tr.worldDir );

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	memset( &s_worldData, 0, sizeof( s_worldData ) );
	String::NCpyZ( s_worldData.name, name, sizeof( s_worldData.name ) );

	String::NCpyZ( s_worldData.baseName, String::SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	String::StripExtension( s_worldData.baseName, s_worldData.baseName );

	startMarker = (byte*)ri.Hunk_Alloc( 0, h_low );

	R_LoadBrush46Model(buffer);

	s_worldData.dataSize = (byte *)ri.Hunk_Alloc( 0, h_low ) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	// reset fog to world fog (if present)
	R_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP,20,0,0,0,0 );

//----(SA)	set the sun shader if there is one
	if ( tr.sunShaderName ) {
		tr.sunShader = R_FindShader( tr.sunShaderName, LIGHTMAP_NONE, qtrue );
	}

//----(SA)	end
	ri.FS_FreeFile( buffer );
}

