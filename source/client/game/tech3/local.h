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

#ifndef _CGAME_TECH3_LOCAL_H
#define _CGAME_TECH3_LOCAL_H

void CLT3_ShutdownCGame();
void CLT3_CGameRendering(stereoFrame_t stereo);
int CLT3_CrosshairPlayer();
int CLT3_LastAttacker();
void CLT3_KeyEvent(int key, bool down);
void CLT3_MouseEvent(int dx, int dy);
void CLT3_EventHandling();

#endif
