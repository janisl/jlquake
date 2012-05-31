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
 * name:		be_aas_move.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_move.h $
 *
 *****************************************************************************/

//movement prediction
int AAS_PredictClientMovement(struct aas_clientmove_s* move,
	int entnum, vec3_t origin,
	int presencetype, int onground,
	vec3_t velocity, vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, int visualize);
//predict movement until bounding box is hit
int AAS_ClientMovementHitBBox(struct aas_clientmove_s* move,
	int entnum, vec3_t origin,
	int presencetype, int onground,
	vec3_t velocity, vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	vec3_t mins, vec3_t maxs, int visualize);
//returns true if on the ground at the given origin
int AAS_OnGround(vec3_t origin, int presencetype, int passent);
//returns true if swimming at the given origin
int AAS_Swimming(vec3_t origin);
//returns the jump reachability run start point
void AAS_JumpReachRunStart(aas_reachability_t* reach, vec3_t runstart);
//rocket jump Z velocity when rocket-jumping at origin
float AAS_RocketJumpZVelocity(vec3_t origin);
//bfg jump Z velocity when bfg-jumping at origin
float AAS_BFGJumpZVelocity(vec3_t origin);
//
int AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs);
