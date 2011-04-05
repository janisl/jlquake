/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "quakedef.h"
#include "winquake.h"

/*
===========
Force_CenterView_f
===========
*/
void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}


/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	// mouse variables
	in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE|CVAR_LATCH);

	// joystick variables
    in_joystick = Cvar_Get("joystick", "0", CVAR_ARCHIVE);
	in_debugJoystick		= Cvar_Get ("in_debugjoystick",			"0",		CVAR_TEMP);
	joy_threshold			= Cvar_Get ("joy_threshold",			"0.15",		CVAR_ARCHIVE);
	in_joyBallScale			= Cvar_Get ("in_joyBallScale",			"0.02",		CVAR_ARCHIVE);

	Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	IN_StartupMouse ();
	IN_ActivateMouse ();
	IN_StartupJoystick ();
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{

	IN_DeactivateMouse ();

	IN_ShutdownDIMouse();
}


/*
===========
IN_Move
===========
*/
void IN_Move ()
{

	if (!ActiveApp || Minimized)
	{
		return;
	}
	if (!s_wmv.mouseActive)
		return;

	IN_MouseMove();
}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void)
{

	if (s_wmv.mouseActive)
	{
		s_wmv.oldButtonState = 0;
	}
}

/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
	IN_JoyMove();
}
