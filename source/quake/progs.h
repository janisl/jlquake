/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "../server/progsvm/progsvm.h"	// defs shared with qcc

#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,qhedict_t,area)

void PR_Init(void);

void PR_LoadProgs(void);

qhedict_t* ED_Alloc(void);
void ED_Free(qhedict_t* ed);

void ED_LoadFromFile(const char* data);
void PR_InitBuiltins();

//============================================================================

extern unsigned short pr_crc;
