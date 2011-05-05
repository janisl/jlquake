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

#ifndef _SYSTEM_WINDOWS_H
#define _SYSTEM_WINDOWS_H

#include <windows.h>

//	Copied from resources.h
#define IDI_ICON1                       1

extern HINSTANCE	global_hInstance;
extern unsigned		sysMsgTime;

void Sys_CreateConsole(const char* Title);
void Sys_DestroyConsole();
void Sys_SetErrorText(const char* text);

#endif
