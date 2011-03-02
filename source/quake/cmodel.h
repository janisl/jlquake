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

void CM_CalcPHS();
