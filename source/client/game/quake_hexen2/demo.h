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

#ifndef __QUAKE_HEXEN2_DEMO_H__
#define __QUAKE_HEXEN2_DEMO_H__

#include "../../../common/socket.h"
#include "../../../common/quakedefs.h"
#include "../../../common/hexen2defs.h"
#include "../../../common/message.h"

void CLQW_WriteDemoCmd( qwusercmd_t* pcmd );
void CLHW_WriteDemoCmd( hwusercmd_t* pcmd );
void CLWQ_WriteServerDataToDemo();
void CLQH_StopPlayback();
int CLQH_GetMessage( QMsg& message );
bool CLQHW_GetMessage( QMsg& message, netadr_t& from );
void CLQH_Stop_f();
void CLQH_Record_f();
void CLQW_Record_f();
void CLHW_Record_f();
void CLQH_PlayDemo_f();
void CLQHW_PlayDemo_f();
void CLQH_TimeDemo_f();
void CLQHW_ReRecord_f();
void CLQH_NextDemo();
void CLQH_Startdemos_f();
void CLQH_Demos_f();
void CLQH_Stopdemo_f();

#endif
