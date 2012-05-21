/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
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

/*****************************************************************************
 * name:		be_aas_bspq3.c
 *
 * desc:		BSP, Environment Sampling
 *
 * $Archive: /MissionPack/code/botlib/be_aas_bspq3.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "l_memory.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
// traces axial boxes of any size through the world
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bsp_trace_t AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask)
{
	bsp_trace_t bsptrace;
	botimport.Trace(&bsptrace, start, mins, maxs, end, passent, contentmask);
	return bsptrace;
}	//end of the function AAS_Trace
//===========================================================================
// returns the contents at the given point
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_PointContents(vec3_t point)
{
	return botimport.PointContents(point);
}	//end of the function AAS_PointContents
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_EntityCollision(int entnum,
	vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end,
	int contentmask, bsp_trace_t* trace)
{
	bsp_trace_t enttrace;

	botimport.EntityTrace(&enttrace, start, boxmins, boxmaxs, end, entnum, contentmask);
	if (enttrace.fraction < trace->fraction)
	{
		Com_Memcpy(trace, &enttrace, sizeof(bsp_trace_t));
		return true;
	}	//end if
	return false;
}	//end of the function AAS_EntityCollision
//===========================================================================
// returns true if in Potentially Hearable Set
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_inPVS(vec3_t p1, vec3_t p2)
{
	return botimport.inPVS(p1, p2);
}	//end of the function AAS_InPVS
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin)
{
	BotImport_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, origin);
}	//end of the function AAS_BSPModelMinsMaxs
