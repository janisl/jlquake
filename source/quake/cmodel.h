//**************************************************************************
//**
//**	$Id: endian.cpp 101 2010-04-03 23:06:31Z dj_jl $
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

void CM_Init();
clipHandle_t CM_LoadMap(const char* name, bool clientload, int* checksum);
clipHandle_t CM_PrecacheModel(const char* name);

bool CM_HullCheck(clipHandle_t hull, vec3_t p1, vec3_t p2, q1trace_t* trace);
int CM_TraceLine(vec3_t start, vec3_t end, q1trace_t* trace);
void CM_CalcPHS();
