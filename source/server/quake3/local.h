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

#ifndef _QUAKE3_LOCAL_H
#define _QUAKE3_LOCAL_H

int SVQ3_NumForGentity(const q3sharedEntity_t* ent);
q3sharedEntity_t* SVQ3_GentityNum(int num);
q3playerState_t* SVQ3_GameClientNum(int num);
q3svEntity_t* SVQ3_SvEntityForGentity(const q3sharedEntity_t* gEnt);
q3sharedEntity_t* SVQ3_GEntityForSvEntity(const q3svEntity_t* svEnt);

#endif
