/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  cinematics
  full screen image for quit and victory

  end of unit intermissions

  */

#include "client.h"

void SCR_Loading_f(void);


/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void SCR_Sky_f(void)
{
	float rotate;
	vec3_t axis;

	if (Cmd_Argc() < 2)
	{
		common->Printf("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (Cmd_Argc() > 2)
	{
		rotate = String::Atof(Cmd_Argv(2));
	}
	else
	{
		rotate = 0;
	}
	if (Cmd_Argc() == 6)
	{
		axis[0] = String::Atof(Cmd_Argv(3));
		axis[1] = String::Atof(Cmd_Argv(4));
		axis[2] = String::Atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	R_SetSky(Cmd_Argv(1), rotate, axis);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init(void)
{
	scr_netgraph = Cvar_Get("netgraph", "0", 0);
	cl_timegraph = Cvar_Get("timegraph", "0", 0);
	cl_debuggraph = Cvar_Get("debuggraph", "0", 0);
	SCR_InitCommon();

	Cmd_AddCommand("loading",SCR_Loading_f);
	Cmd_AddCommand("sky",SCR_Sky_f);

	scr_initialized = true;
}

//=============================================================================

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f(void)
{
	SCRQ2_BeginLoadingPlaque(false);
}

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal(void)
{
	char_texture = R_LoadQuake2FontImage("pics/conchars.pcx");
}
