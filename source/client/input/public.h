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

// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
enum
{
	KEYCATCH_CONSOLE		= 0x0001,
	KEYCATCH_UI				= 0x0002,
	KEYCATCH_MESSAGE		= 0x0004,
	KEYCATCH_CGAME			= 0x0008,
};

enum
{
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
};

struct keyname_t
{
	const char* name;
	int keynum;
};

void IN_Init();
void IN_Shutdown();
void IN_Frame();

void Sys_SendKeyEvents();

extern int in_keyCatchers;		// bit flags
extern int anykeydown;

extern keyname_t keynames[];
