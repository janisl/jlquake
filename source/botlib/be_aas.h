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
//!!!!!!!!!!!!!!! Used by game VM !!!!!!!!!!!!!!!!!!!!!!!!!!!!
//

/*****************************************************************************
 * name:		be_aas.h
 *
 * desc:		Area Awareness System, stuff exported to the AI
 *
 * $Archive: /source/code/botlib/be_aas.h $
 *
 *****************************************************************************/

// client movement prediction stop events, stop as soon as:
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
#define SE_HITBOUNDINGBOX       2048	// hit the specified bounding box
#define SE_TOUCHCLUSTERPORTAL   4096	// touching a cluster portal

typedef struct aas_clientmove_s
{
	vec3_t endpos;			//position at the end of movement prediction
	int endarea;			//area at end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	aas_trace_t trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	int endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
} aas_clientmove_t;
