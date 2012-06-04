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
 * name:		be_aas.h
 *
 * desc:		Area Awareness System, stuff exported to the AI
 *
 *
 *****************************************************************************/

#ifndef _ET_BE_AAS_H
#define _ET_BE_AAS_H

#include "../../server/botlib/public.h"

//client movement prediction stop events, stop as soon as:
#define SE_NONE                 0
#define SE_HITGROUND            1		// the ground is hit
#define SE_LEAVEGROUND          2		// there's no ground
#define SE_ENTERWATER           4		// water is entered
#define SE_ENTERSLIME           8		// slime is entered
#define SE_ENTERLAVA            16		// lava is entered
#define SE_HITGROUNDDAMAGE      32		// the ground is hit with damage
#define SE_GAP                  64		// there's a gap
#define SE_TOUCHJUMPPAD         128		// touching a jump pad area
#define SE_TOUCHTELEPORTER      256		// touching teleporter
#define SE_ENTERAREA            512		// the given stoparea is entered
#define SE_HITGROUNDAREA        1024	// a ground face in the area is hit
#define SE_HITENT               2048	// hit specified entity
#define SE_STUCK                4096

typedef struct aas_clientmove_s
{
	vec3_t endpos;			//position at the end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	bsp_trace_t trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	float endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
} aas_clientmove_t;

#endif
