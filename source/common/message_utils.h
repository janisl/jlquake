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

void MSGQW_WriteDeltaUsercmd(QMsg* sb, qwusercmd_t* from, qwusercmd_t* cmd);
void MSGQW_ReadDeltaUsercmd(QMsg* sb, qwusercmd_t* from, qwusercmd_t* cmd);

void MSGQ2_WriteDeltaUsercmd(QMsg* sb, q2usercmd_t* from, q2usercmd_t* cmd);
void MSGQ2_WriteDeltaEntity(q2entity_state_t* from, q2entity_state_t* to, QMsg* msg, bool force, bool newentity);
void MSGQ2_ReadDeltaUsercmd(QMsg* sb, q2usercmd_t* from, q2usercmd_t* cmd);
