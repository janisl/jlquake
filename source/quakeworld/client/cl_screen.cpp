/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
#include "../../client/game/quake_hexen2/view.h"

void VID_Init()
{
	R_BeginRegistration(&cls.glconfig);

	Sys_ShowConsole(0, false);

	int i;
	if ((i = COM_CheckParm("-conwidth")) != 0)
	{
		viddef.width = String::Atoi(COM_Argv(i + 1));
	}
	else
	{
		viddef.width = 640;
	}

	viddef.width &= 0xfff8;	// make it a multiple of eight

	if (viddef.width < 320)
	{
		viddef.width = 320;
	}

	// pick a conheight that matches with correct aspect
	viddef.height = viddef.width / cls.glconfig.windowAspect;

	if ((i = COM_CheckParm("-conheight")) != 0)
	{
		viddef.height = String::Atoi(COM_Argv(i + 1));
	}
	if (viddef.height < 200)
	{
		viddef.height = 200;
	}

	if (viddef.height > cls.glconfig.vidHeight)
	{
		viddef.height = cls.glconfig.vidHeight;
	}
	if (viddef.width > cls.glconfig.vidWidth)
	{
		viddef.width = cls.glconfig.vidWidth;
	}
}
