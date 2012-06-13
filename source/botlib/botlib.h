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
 * name:		botlib.h
 *
 * desc:		bot AI library
 *
 * $Archive: /source/code/game/botai.h $
 *
 *****************************************************************************/

#define BOTLIB_API_VERSION      2

struct aas_clientmove_q3_t;
struct aas_altroutegoal_t;
struct bot_entitystate_t;
struct bsp_trace_t;

typedef struct ai_export_s
{
} ai_export_t;

//bot AI library imported functions
typedef struct botlib_export_s
{
	//AI functions
	ai_export_t ai;
} botlib_export_t;

//linking of bot library
botlib_export_t* GetBotLibAPI(int apiVersion);
