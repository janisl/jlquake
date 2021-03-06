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

#ifndef _CGAME_QUAKE3_LOCAL_H
#define _CGAME_QUAKE3_LOCAL_H

#include "../tech3/local.h"

qintptr CLQ3_CgameSystemCalls( qintptr* args );

bool UIQ3_ConsoleCommand( int realTime );
void UIQ3_DrawConnectScreen( bool overlay );
bool UIQ3_HasUniqueCDKey();
qintptr CLQ3_UISystemCalls( qintptr* args );

#endif
