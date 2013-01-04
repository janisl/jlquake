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

#ifndef __QUAKE_HEXEN2_CONNECTION_H__
#define __QUAKE_HEXEN2_CONNECTION_H__

#include "../../../common/socket.h"
#include "../../../common/message.h"

extern Cvar* clqw_localid;

void CLQH_KeepaliveMessage();
void CLQH_SendCmd();
void CLQH_Disconnect();
void CLQHW_Disconnect();
void CLQH_Connect_f();
void CLQH_Reconnect_f();
void CLQHW_SendConnectPacket();
void CLQHW_CheckForResend();
void CLQHW_BeginServerConnect();
void CLQHW_Connect_f();
void CLQHW_Reconnect_f();
void CLQHW_ConnectionlessPacket(QMsg& message, netadr_t& net_from);
void CLQH_ReadFromServer();
void CLQHW_ReadPackets();

#endif
