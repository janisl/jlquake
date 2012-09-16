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

#ifndef _CGAME_WOLFMP_LOCAL_H
#define _CGAME_WOLFMP_LOCAL_H

#include "../tech3/local.h"

bool CLWM_GetTag(int clientNum, const char* tagname, orientation_t* _or);
bool CLWM_CheckCenterView();

#endif
