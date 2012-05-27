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

#include "../server.h"
#include "local.h"

libvar_t* sv_maxstep;
libvar_t* sv_maxbarrier;
libvar_t* sv_gravity;
libvar_t* weapindex_rocketlauncher;
libvar_t* weapindex_bfg10k;
libvar_t* weapindex_grapple;
libvar_t* entitytypemissile;
libvar_t* offhandgrapple;
libvar_t* cmd_grappleoff;
libvar_t* cmd_grappleon;

//type of model, func_plat or func_bobbing
int modeltypes[MAX_MODELS_Q3];

bot_movestate_t* botmovestates[MAX_BOTLIB_CLIENTS_ARRAY + 1];
