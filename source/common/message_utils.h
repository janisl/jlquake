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

#ifndef __MESSAGE_UTILS_H__
#define __MESSAGE_UTILS_H__

#include "message.h"
#include "quakedefs.h"
#include "hexen2defs.h"
#include "quake2defs.h"
#include "wolfdefs.h"

void MSGQW_WriteDeltaUsercmd(QMsg* sb, qwusercmd_t* from, qwusercmd_t* cmd);
void MSGQW_ReadDeltaUsercmd(QMsg* sb, qwusercmd_t* from, qwusercmd_t* cmd);
void MSGHW_WriteUsercmd(QMsg* sb, hwusercmd_t* cmd, bool long_msg);
void MSGHW_ReadUsercmd(QMsg* sb, hwusercmd_t* cmd, bool long_msg);

void MSGQ2_WriteDeltaUsercmd(QMsg* sb, q2usercmd_t* from, q2usercmd_t* cmd);
void MSGQ2_WriteDeltaEntity(q2entity_state_t* from, q2entity_state_t* to, QMsg* msg, bool force, bool newentity);
void MSGQ2_ReadDeltaUsercmd(QMsg* sb, q2usercmd_t* from, q2usercmd_t* cmd);

void MSGQ3_WriteDeltaUsercmdKey(QMsg* msg, int key, q3usercmd_t* from, q3usercmd_t* to);
void MSGQ3_ReadDeltaUsercmdKey(QMsg* msg, int key, q3usercmd_t* from, q3usercmd_t* to);
void MSGWS_WriteDeltaUsercmdKey(QMsg* msg, int key, wsusercmd_t* from, wsusercmd_t* to);
void MSGWS_ReadDeltaUsercmdKey(QMsg* msg, int key, wsusercmd_t* from, wsusercmd_t* to);
void MSGWM_WriteDeltaUsercmdKey(QMsg* msg, int key, wmusercmd_t* from, wmusercmd_t* to);
void MSGWM_ReadDeltaUsercmdKey(QMsg* msg, int key, wmusercmd_t* from, wmusercmd_t* to);
void MSGET_WriteDeltaUsercmdKey(QMsg* msg, int key, etusercmd_t* from, etusercmd_t* to);
void MSGET_ReadDeltaUsercmdKey(QMsg* msg, int key, etusercmd_t* from, etusercmd_t* to);
void MSGQ3_WriteDeltaEntity(QMsg* msg, q3entityState_t* from, q3entityState_t* to, bool force);
void MSGQ3_ReadDeltaEntity(QMsg* msg, q3entityState_t* from, q3entityState_t* to, int number);
void MSGWS_WriteDeltaEntity(QMsg* msg, wsentityState_t* from, wsentityState_t* to, bool force);
void MSGWS_ReadDeltaEntity(QMsg* msg, wsentityState_t* from, wsentityState_t* to, int number);
void MSGWM_WriteDeltaEntity(QMsg* msg, wmentityState_t* from, wmentityState_t* to, bool force);
void MSGWM_ReadDeltaEntity(QMsg* msg, wmentityState_t* from, wmentityState_t* to, int number);
void MSGET_WriteDeltaEntity(QMsg* msg, etentityState_t* from, etentityState_t* to, bool force);
void MSGET_ReadDeltaEntity(QMsg* msg, etentityState_t* from, etentityState_t* to, int number);
void MSGQ3_WriteDeltaPlayerstate(QMsg* msg, q3playerState_t* from, q3playerState_t* to);
void MSGQ3_ReadDeltaPlayerstate(QMsg* msg, q3playerState_t* from, q3playerState_t* to);
void MSGWS_WriteDeltaPlayerstate(QMsg* msg, wsplayerState_t* from, wsplayerState_t* to);
void MSGWS_ReadDeltaPlayerstate(QMsg* msg, wsplayerState_t* from, wsplayerState_t* to);
void MSGWM_WriteDeltaPlayerstate(QMsg* msg, wmplayerState_t* from, wmplayerState_t* to);
void MSGWM_ReadDeltaPlayerstate(QMsg* msg, wmplayerState_t* from, wmplayerState_t* to);
void MSGET_WriteDeltaPlayerstate(QMsg* msg, etplayerState_t* from, etplayerState_t* to);
void MSGET_ReadDeltaPlayerstate(QMsg* msg, etplayerState_t* from, etplayerState_t* to);
void MSGET_PrioritiseEntitystateFields();
void MSGET_PrioritisePlayerStateFields();

#endif
