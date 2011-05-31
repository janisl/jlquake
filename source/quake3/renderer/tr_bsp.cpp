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
// tr_map.c

#include "tr_local.h"

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/


//==================================================================

/*
=================
R_GetEntityToken
=================
*/
qboolean R_GetEntityToken( char *buffer, int size ) {
	const char	*s;

	s = QStr::Parse3( const_cast<const char**>(&s_worldData.entityParsePoint) );
	QStr::NCpyZ( buffer, s, size );
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
void RE_LoadWorldMap(const char* name)
{
	if (tr.worldMapLoaded)
	{
		ri.Error(ERR_DROP, "ERROR: attempted to redundantly load world map\n");
	}

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45f;
	tr.sunDirection[1] = 0.3f;
	tr.sunDirection[2] = 0.9f;

	VectorNormalize(tr.sunDirection);

	tr.worldMapLoaded = true;

	// load it
	void* buffer;
	FS_ReadFile(name, &buffer);
	if (!buffer)
	{
		ri.Error(ERR_DROP, "RE_LoadWorldMap: %s not found", name);
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	Com_Memset(&s_worldData, 0, sizeof(s_worldData));
	QStr::NCpyZ(s_worldData.name, name, sizeof(s_worldData.name));

	QStr::NCpyZ(s_worldData.baseName, QStr::SkipPath(s_worldData.name), sizeof(s_worldData.name));
	QStr::StripExtension(s_worldData.baseName, s_worldData.baseName);

	byte* startMarker = (byte*)ri.Hunk_Alloc(0, h_low);

	R_LoadBrush46Model(buffer);

	s_worldData.dataSize = (byte*)ri.Hunk_Alloc(0, h_low) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	FS_FreeFile(buffer);
}

