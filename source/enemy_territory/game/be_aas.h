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

typedef enum
{
	SOLID_NOT,			// no interaction with other objects
	SOLID_TRIGGER,		// only touch when inside, after moving
	SOLID_BBOX,			// touch on edge
	SOLID_BSP			// bsp clip, touch on edge
} solid_t;

//a trace is returned when a box is swept through the AAS world
typedef struct aas_trace_s
{
	qboolean startsolid;	// if true, the initial point was in a solid area
	float fraction;			// time completed, 1.0 = didn't hit anything
	vec3_t endpos;			// final position
	int ent;				// entity blocking the trace
	int lastarea;			// last area the trace was in (zero if none)
	int area;				// area blocking the trace (zero if none)
	int planenum;			// number of the plane that was hit
} aas_trace_t;

/* Defined in botlib.h

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
    char name[16];
    int flags;
    int value;
} bsp_surface_t;

//a trace is returned when a box is swept through the BSP world
typedef struct bsp_trace_s
{
    qboolean		allsolid;	// if true, plane is not valid
    qboolean		startsolid;	// if true, the initial point was in a solid area
    float			fraction;	// time completed, 1.0 = didn't hit anything
    vec3_t		endpos;		// final position
    cplane_t		plane;		// surface normal at impact
    float			exp_dist;	// expanded plane distance
    int			sidenum;		// number of the brush side hit
    bsp_surface_t surface;	// hit surface
    int			contents;	// contents on other side of surface hit
    int			ent;			// number of entity hit
} bsp_trace_t;
//
*/

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

#ifndef BSPTRACE

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//remove the bsp_trace_s structure definition l8r on
//a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
{
	qboolean allsolid;			// if true, plane is not valid
	qboolean startsolid;		// if true, the initial point was in a solid area
	float fraction;				// time completed, 1.0 = didn't hit anything
	vec3_t endpos;				// final position
	cplane_t plane;				// surface normal at impact
	float exp_dist;				// expanded plane distance
	int sidenum;				// number of the brush side hit
	bsp_surface_t surface;		// the hit point surface
	int contents;				// contents on other side of surface hit
	int ent;					// number of entity hit
} bsp_trace_t;

#define BSPTRACE
#endif	// BSPTRACE

typedef struct aas_clientmove_s
{
	vec3_t endpos;			//position at the end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	struct bsp_trace_s trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	float endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
} aas_clientmove_t;

typedef struct aas_altroutegoal_s
{
	vec3_t origin;
	int areanum;
	unsigned short starttraveltime;
	unsigned short goaltraveltime;
	unsigned short extratraveltime;
} aas_altroutegoal_t;

#endif
