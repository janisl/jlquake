//**************************************************************************
//**
//**	$Id$
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

void Sys_Mkdir(const char* path);
const char* Sys_Cwd();
void Sys_SetHomePathSuffix(const char* Name);
const char* Sys_DefaultHomePath();

char** Sys_ListFiles(const char* directory, const char* extension, const char* filter, int* numfiles, bool wantsubs);
void Sys_FreeFileList(char** list);

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int Sys_Milliseconds();

char* Sys_CommonConsoleInput();
