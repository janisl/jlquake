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
// server.h

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../server/quake3/g_public.h"
#include "../../server/server.h"
#include "../../server/tech3/local.h"
#include "../../server/quake3/local.h"

//=============================================================================

extern Cvar* sv_fps;
extern Cvar* sv_rconPassword;
extern Cvar* sv_showloss;
extern Cvar* sv_killserver;
extern Cvar* sv_mapChecksum;
extern Cvar* sv_serverid;

//===========================================================

//
// sv_main.c
//
void SV_AddOperatorCommands(void);
