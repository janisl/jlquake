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

#ifndef _WOLFSP_LOCAL_H
#define _WOLFSP_LOCAL_H

int SVWS_NumForGentity(const wssharedEntity_t* ent);
wssharedEntity_t* SVWS_GentityNum(int num);
wsplayerState_t* SVWS_GameClientNum(int num);
q3svEntity_t* SVWS_SvEntityForGentity(const wssharedEntity_t* gEnt);
wssharedEntity_t* SVWS_GEntityForSvEntity(const q3svEntity_t* svEnt);

#endif
