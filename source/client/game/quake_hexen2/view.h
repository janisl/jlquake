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

#ifndef __QUAKE_HEXEN2_VIEW_H__
#define __QUAKE_HEXEN2_VIEW_H__

#include "../../../common/message.h"

void VQH_ParseDamage(QMsg& message);
void VQH_RenderView();
void SCRQH_DrawTurtle();
void VQH_Init();
void VQH_InitCrosshairTexture();

#endif
