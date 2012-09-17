/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../client/client.h"

void CL_Shutdown(void)
{
}

void CL_Init(void)
{
}

void CL_MouseEvent(int dx, int dy, int time)
{
}

void CL_Frame(int msec)
{
}

void CL_PacketEvent(netadr_t from, QMsg* msg)
{
}

void CL_CharEvent(int key)
{
}

void CL_Disconnect(qboolean showMainMenu)
{
}

void CL_MapLoading(void)
{
}

void CL_KeyEvent(int key, qboolean down, unsigned time)
{
}

void CL_ForwardCommandToServer(const char* string)
{
}

void Con_ConsolePrint(const char* txt)
{
}

void CL_JoystickEvent(int axis, int value, int time)
{
}

void CL_CDDialog(void)
{
}

void CL_FlushMemory(void)
{
}

void CL_StartHunkUsers(void)
{
}

// bk001119 - added new dummy for sv_init.c
void CL_ShutdownAll(void) {};

// bk001208 - added new dummy (RC4)
qboolean CL_CDKeyValidate(const char* key, const char* checksum) { return true; }
