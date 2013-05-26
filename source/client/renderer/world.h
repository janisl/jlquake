//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef _R_WORLD_H
#define _R_WORLD_H

#include "main.h"

void R_DrawBrushModelQ1( trRefEntity_t* e, int forcedSortIndex );
void R_DrawWorldQ1();
void R_DrawBrushModelQ2( trRefEntity_t* e, int forcedSortIndex );
void R_DrawWorldQ2();
void R_AddBrushModelSurfaces( trRefEntity_t* e );
void R_AddWorldSurfaces();

#endif
