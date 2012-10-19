/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// client.h -- primary header for client

#include "../../client/client.h"
#include "../../client/game/quake2/local.h"
#include "../../client/game/quake2/menu.h"

//define	PARANOID			// speed sapping error checking

#include "../qcommon/qcommon.h"

//
// cvars
//
extern Cvar* cl_autoskins;

void CL_ParseConfigString(void);

//=================================================

void IN_Accumulate(void);

void CL_ParseLayout(void);

//
// cl_main
//
void CL_Init(void);

void CL_GetChallengePacket(void);
void CL_Snd_Restart_f(void);

void CL_ReadPackets(void);

void CL_WriteToServer(q2usercmd_t* cmd);

//
// cl_parse.c
//
void CL_Download_f(void);

//
// cl_pred.c
//
void CL_PredictMovement(void);
