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

extern q1entity_state_t clq1_baselines[MAX_EDICTS_Q1];

void CLQ1_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles);

void CLQ1_InitTEnts();
void CLQ1_ClearTEnts();
void CLQ1_ParseTEnt(QMsg& message);
void CLQW_ParseTEnt(QMsg& message);
void CLQ1_UpdateTEnts();
