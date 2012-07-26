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

void PF_setpuzzlemodel();
void PF_precache_sound2();
void PF_precache_sound3();
void PF_precache_sound4();
void PF_precache_model2();
void PF_precache_model3();
void PF_precache_model4();
void PF_StopSound();
void PF_UpdateSoundPos();
void PF_tracearea();
void PF_SpawnTemp();
void PF_FindFloat();
void PF_precache_puzzle_model();
void PFHW_lightstylevalue();
void PFH2_lightstylestatic();
void PFHW_lightstylestatic();
void PF_movestep();
void PFH2_AdvanceFrame();
void PFHW_AdvanceFrame();
void PF_RewindFrame();
void PF_advanceweaponframe();
void PF_matchAngleToSlope();
void PF_updateInfoPlaque();

#endif
