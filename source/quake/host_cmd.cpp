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

#include "quakedef.h"
#include "../server/public.h"

void Com_Quit_f()
{
	CL_Disconnect(true);
	SV_Shutdown("");

	Sys_Quit();
}

void Host_Version_f()
{
	common->Printf("Version %4.2f\n", VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
}

void Host_InitCommands()
{
	Cmd_AddCommand("quit", Com_Quit_f);
	Cmd_AddCommand("version", Host_Version_f);
}
