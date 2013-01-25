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

#include "public.h"
#include "../public.h"

int in_keyCatchers;		// bit flags
int anykeydown;

int CL_GetKeyCatchers() {
	return in_keyCatchers;
}

//	Restart the input subsystem
void In_Restart_f() {
	IN_Shutdown();
	IN_Init();
}
