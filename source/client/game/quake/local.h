//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

extern Cvar* cl_doubleeyes;

void CLQ1_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles);

void CLQ1_InitTEnts();
void CLQ1_ClearTEnts();
void CLQ1_ParseBeam(QMsg& message, qhandle_t model);
void CLQ1_ParseWizSpike(QMsg& message);
void CLQ1_ParseKnightSpike(QMsg& message);
void CLQ1_ParseSpike(QMsg& message);
void CLQ1_ParseSuperSpike(QMsg& message);
void CLQ1_ParseExplosion(QMsg& message);
void CLQ1_ParseTarExplosion(QMsg& message);
void CLQ1_ParseExplosion2(QMsg& message);
void CLQ1_ParseLavaSplash(QMsg& message);
void CLQ1_ParseTeleportSplash(QMsg& message);
void CLQ1_ParseGunShot(QMsg& message);
void CLQ1_ParseBlood(QMsg& message);
void CLQ1_ParseLightningBlood(QMsg& message);
void CLQ1_UpdateTEnts();
