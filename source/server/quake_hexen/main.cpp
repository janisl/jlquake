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

#include "../server.h"
//#include "../progsvm/progsvm.h"
#include "local.h"

Cvar* svqh_deathmatch;			// 0, 1, or 2
Cvar* svqh_coop;				// 0 or 1
Cvar* svqh_teamplay;

Cvar* svqh_highchars;

int svqh_current_skill;

fileHandle_t svqhw_fraglogfile;