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

#ifndef _SERVER_PUBLIC_H
#define _SERVER_PUBLIC_H

extern int svh2_kingofhill;
extern Cvar* svqh_coop;
extern Cvar* svqh_teamplay;
extern Cvar* qh_skill;
extern Cvar* qh_timelimit;
extern Cvar* qh_fraglimit;
extern Cvar* h2_randomclass;

bool SV_IsServerActive();
void SV_CvarChanged(Cvar* var);
int SVQH_GetMaxClients();
int SVQH_GetMaxClientsLimit();
void SVH2_RemoveGIPFiles(const char* path);

#endif
