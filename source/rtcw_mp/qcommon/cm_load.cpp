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

// cmodel.c -- model loading

#include "cm_local.h"

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum ) {
	int length;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if (cm46 && cm46->Name == name && clientload ) {
		*checksum = cm46->CheckSum;
		return;
	}

	// free old stuff
	if (cm46)
		delete cm46;
	cm46 = new QClipMap46();
	CMapShared = cm46;

	//
	// load the file
	//
	Array<quint8> buffer;
	length = FS_ReadFile( name, buffer);

	if ( !length ) {
		Com_Error( ERR_DROP, "Couldn't load %s", name );
	}

	cm46->LoadMap(name, buffer);
	*checksum = cm46->CheckSum;
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap( void ) {
	if (cm46)
		delete cm46;
	cm46 = NULL;
}

// DHM - Nerve
void CM_SetTempBoxModelContents( int contents ) {

	cm46->box_brush->contents = contents;
}
// dhm
