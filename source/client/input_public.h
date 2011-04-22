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

// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
enum
{
	KEYCATCH_CONSOLE		= 0x0001,
	KEYCATCH_UI				= 0x0002,
	KEYCATCH_MESSAGE		= 0x0004,
	KEYCATCH_CGAME			= 0x0008,
};

struct keyname_t
{
	const char*	name;
	int			keynum;
};

void IN_Init();
void IN_Shutdown();
void IN_Frame();

void Sys_SendKeyEvents();

extern int			in_keyCatchers;		// bit flags

extern keyname_t keynames[];
