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
// client.h -- primary header for client

#include "../game/q_shared.h"
#include "../../client/client.h"
#include "../qcommon/qcommon.h"
#include "../../client/game/quake3/local.h"

//=============================================================================

//
// cvars
//
extern Cvar* cl_timegraph;
extern Cvar* cl_timeNudge;
extern Cvar* cl_freezeDemo;

//=================================================

//
// cl_main
//

void CL_Init(void);
void CL_AddReliableCommand(const char* cmd);

void CL_GetChallengePacket(void);
void CL_Vid_Restart_f(void);
void CL_Snd_Restart_f(void);

void CL_ShutdownRef(void);
void CL_InitRef(void);

//
// cl_cgame.c
//
void CL_SetCGameTime(void);
void CL_ShaderStateChanged(void);
