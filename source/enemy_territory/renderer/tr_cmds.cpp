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

#include "tr_local.h"

/*
====================
RE_SetGlobalFog
	rgb = colour
	depthForOpaque is depth for opaque

	the restore flag can be used to restore the original level fog
	duration can be set to fade over a certain period
====================
*/
void RE_SetGlobalFog( qboolean restore, int duration, float r, float g, float b, float depthForOpaque ) {
	if ( restore ) {
		if ( duration > 0 ) {
			VectorCopy( tr.world->fogs[tr.world->globalFog].shader->fogParms.color, tr.world->globalTransStartFog );
			tr.world->globalTransStartFog[ 3 ] = tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque;

			Vector4Copy( tr.world->globalOriginalFog, tr.world->globalTransEndFog );

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime = tr.refdef.time + duration;
		} else {
			VectorCopy( tr.world->globalOriginalFog, tr.world->fogs[tr.world->globalFog].shader->fogParms.color );
			tr.world->fogs[tr.world->globalFog].shader->fogParms.colorInt = ColorBytes4( tr.world->globalOriginalFog[ 0 ] * tr.identityLight,
																						 tr.world->globalOriginalFog[ 1 ] * tr.identityLight,
																						 tr.world->globalOriginalFog[ 2 ] * tr.identityLight, 1.0 );
			tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque = tr.world->globalOriginalFog[ 3 ];
			tr.world->fogs[tr.world->globalFog].shader->fogParms.tcScale = 1.0f / ( tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque );
		}
	} else {
		if ( duration > 0 ) {
			VectorCopy( tr.world->fogs[tr.world->globalFog].shader->fogParms.color, tr.world->globalTransStartFog );
			tr.world->globalTransStartFog[ 3 ] = tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque;

			VectorSet( tr.world->globalTransEndFog, r, g, b );
			tr.world->globalTransEndFog[ 3 ] = depthForOpaque < 1 ? 1 : depthForOpaque;

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime = tr.refdef.time + duration;
		} else {
			VectorSet( tr.world->fogs[tr.world->globalFog].shader->fogParms.color, r, g, b );
			tr.world->fogs[tr.world->globalFog].shader->fogParms.colorInt = ColorBytes4( r * tr.identityLight,
																						 g * tr.identityLight,
																						 b * tr.identityLight, 1.0 );
			tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque = depthForOpaque < 1 ? 1 : depthForOpaque;
			tr.world->fogs[tr.world->globalFog].shader->fogParms.tcScale = 1.0f / ( tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque );
		}
	}
}
