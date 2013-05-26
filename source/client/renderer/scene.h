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

#ifndef _R_SCENE_H
#define _R_SCENE_H

#include "public.h"

extern int r_firstSceneDrawSurf;
extern int r_numentities;
extern int r_firstSceneEntity;
extern int r_numdlights;
extern int r_firstSceneDlight;
extern int r_numpolys;
extern int r_firstScenePoly;
extern int r_numpolyverts;
extern int r_numparticles;
extern int r_firstSceneParticle;
extern int r_numDecalProjectors;

extern int skyboxportal;
extern int drawskyboxportal;

void R_ToggleSmpFrame();

#endif
