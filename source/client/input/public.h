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

#ifndef __INPUT_PUBLIC__H__
#define __INPUT_PUBLIC__H__

#include "../../common/console_variable.h"

#define MAX_KEYS        256

// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
enum
{
	KEYCATCH_CONSOLE        = 0x0001,
	KEYCATCH_UI             = 0x0002,
	KEYCATCH_MESSAGE        = 0x0004,
	KEYCATCH_CGAME          = 0x0008,
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

struct qkey_t {
	bool down;
	int repeats;				// if > 1, it is autorepeating
	char* binding;
};

void IN_Init();
void IN_Shutdown();
void In_Restart_f();

extern int in_keyCatchers;		// bit flags
extern int anykeydown;

extern qkey_t keys[ MAX_KEYS ];
extern bool key_overstrikeMode;
extern Cvar* clwm_missionStats;
extern Cvar* clwm_waitForFire;

bool Key_GetOverstrikeMode();
void Key_SetOverstrikeMode( bool state );
bool Key_IsDown( int keynum );
const char* Key_KeynumToString( int keynum, bool translate );
void Key_SetBinding( int keynum, const char* binding );
const char* Key_GetBinding( int keynum );
int Key_GetKey( const char* binding );
void Key_GetKeysForBinding( const char* binding, int* key1, int* key2 );
void Key_UnbindCommand( const char* command );
void Key_ClearStates();

#endif
