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

#ifndef _SERVER_HEXEN2_LOCAL_H
#define _SERVER_HEXEN2_LOCAL_H

extern Cvar* sv_ce_scale;
extern Cvar* sv_ce_max_size;

void SVHW_SendEffect(QMsg* sb, int index);
void SVH2_UpdateEffects(QMsg* sb);
void SVH2_ParseEffect(QMsg* sb);
void SVHW_ParseMultiEffect(QMsg* sb);
float SVHW_GetMultiEffectId();
void SVH2_SaveEffects(fileHandle_t FH);
const char* SVH2_LoadEffects(const char* Data);
void SVHW_setseed(int seed);
float SVHW_seedrand();

extern unsigned int info_mask, info_mask2;

void PRH2_InitBuiltins();
void PRHW_InitBuiltins();

#endif
