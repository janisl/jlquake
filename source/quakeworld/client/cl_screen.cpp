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

#include <time.h>

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
common->Printf ();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
    notify lines
    half
    full


*/

Cvar* scr_allowsnap;

void SCR_RSShot_f(void);

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	scr_allowsnap = Cvar_Get("scr_allowsnap", "1", 0);
	SCR_InitCommon();

	Cmd_AddCommand("snap",SCR_RSShot_f);

	clqh_sbar     = Cvar_Get("cl_sbar", "0", CVAR_ARCHIVE);

	scr_netgraph = Cvar_Get("cl_netgraph", "0", 0);

	scr_initialized = true;
}

/*
==================
SCR_RSShot_f
==================
*/
void SCR_RSShot_f(void)
{
	if (CL_IsUploading())
	{
		return;	// already one pending
	}

	if (cls.state < CA_LOADING)
	{
		return;	// gotta be connected
	}

	if (!scr_allowsnap->integer)
	{
		CL_AddReliableCommand("snap\n");
		common->Printf("Refusing remote screen shot request.\n");
		return;
	}

	common->Printf("Remote screen shot requested.\n");

	time_t now;
	time(&now);

	Array<byte> buffer;
	R_CaptureRemoteScreenShot(ctime(&now), cls.servername, clqh_name->string, buffer);
	CL_StartUpload(buffer.Ptr(), buffer.Num());
}

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
