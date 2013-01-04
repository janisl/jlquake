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

#ifndef __QUAKE_HEXEN2_PARSE_H__
#define __QUAKE_HEXEN2_PARSE_H__

#include "../../../common/qcommon.h"

void CLQH_ParseTime(QMsg& message);
void CLQH_ParseDisconnect();
void CLQH_ParseSetAngle(QMsg& message);
void CLQH_ParseSetView(QMsg& message);
void CLQH_ParseLightStyle(QMsg& message);
void CLQH_ParseStartSoundPacket(QMsg& message, int overflowMask);
void CLQHW_ParseStartSoundPacket(QMsg& message, float attenuationScale);
void CLQH_ParseStopSound(QMsg& message);
void CLQH_ParseSetPause(QMsg& message);
void CLQH_ParseKilledMonster();
void CLQH_ParseFoundSecret();
void CLQH_ParseUpdateStat(QMsg& message);
void CLQH_ParseStaticSound(QMsg& message);
void CLQHW_ParseCDTrack(QMsg& message);
void CLQH_ParseSellScreen();
void CLQHW_ParseFinale(QMsg& message);
void CLQHW_ParseSmallKick();
void CLQHW_ParseBigKick();
void CLQHW_ParseMaxSpeed(QMsg& message);
void CLQHW_ParseEntGravity(QMsg& message);

#endif
