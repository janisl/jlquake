/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

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


/*****************************************************************************
 * name:		be_aas_bsp.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
//loads the given BSP file
int AAS_LoadBSPFile(void);
//dump the loaded BSP data
void AAS_DumpBSPData(void);

//calculates collision with given entity
qboolean AAS_EntityCollision(int entnum,
	vec3_t start,
	vec3_t boxmins,
	vec3_t boxmaxs,
	vec3_t end,
	int contentmask,
	bsp_trace_t* trace);
//for debugging
void AAS_PrintFreeBSPLinks(char* str);
//
#endif	//AASINTERN

//trace through the world
bsp_trace_t AAS_Trace(vec3_t start,
	vec3_t mins,
	vec3_t maxs,
	vec3_t end,
	int passent,
	int contentmask);
//returns the contents at the given point
int AAS_PointContents(vec3_t point);
//returns true when p2 is in the PVS of p1
qboolean AAS_inPVS(vec3_t p1, vec3_t p2);
//returns true if the given areas are connected
qboolean AAS_AreasConnected(int area1, int area2);
//gets the mins, maxs and origin of a BSP model
void AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
