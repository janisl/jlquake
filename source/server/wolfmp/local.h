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

#ifndef _WOLFMP_LOCAL_H
#define _WOLFMP_LOCAL_H

int SVWM_NumForGentity(const wmsharedEntity_t* ent);
wmsharedEntity_t* SVWM_GentityNum(int num);
wmplayerState_t* SVWM_GameClientNum(int num);
q3svEntity_t* SVWM_SvEntityForGentity(const wmsharedEntity_t* gEnt);
wmsharedEntity_t* SVWM_GEntityForSvEntity(const q3svEntity_t* svEnt);

#endif
