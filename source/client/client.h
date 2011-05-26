//**************************************************************************
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

#ifndef _CLIENT_H
#define _CLIENT_H

#include "../core/core.h"

#include "sound_public.h"
#include "render_public.h"
#include "input_keycodes.h"
#include "input_public.h"
#include "cinematic_public.h"

extern QCvar*		cl_inGameVideo;

void CL_SharedInit();
int CL_ScaledMilliseconds();

//	Called by Windows driver.
void Key_ClearStates();
//	For QuakeWorld
void CL_SendModelChecksum(const char* name, const void* buffer);

#endif
