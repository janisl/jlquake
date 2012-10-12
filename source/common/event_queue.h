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

enum sysEventType_t
{
	// bk001129 - make sure SE_NONE is zero
	SE_NONE = 0,		// evTime is still valid
	SE_KEY,				// evValue is a key code, evValue2 is the down flag
	SE_CHAR,			// evValue is an ascii char
	SE_MOUSE,			// evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,	// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE,			// evPtr is a char*
	SE_PACKET			// evPtr is a netadr_t followed by data bytes to evPtrLength
};

struct sysEvent_t
{
	int evTime;
	sysEventType_t evType;
	int evValue;
	int evValue2;
	int evPtrLength;				// bytes of data pointed to by evPtr, for journaling
	void* evPtr;					// this must be manually freed if not NULL
};

void Com_InitEventQueue();
void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void* ptr);
sysEvent_t Com_GetEvent();
int Com_Milliseconds();
