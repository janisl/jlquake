/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "../client/client.h"
#include "winquake.h"

QCvar	*in_mouse;

qboolean	in_appactive;

/*
============================================================

  MOUSE CONTROL

============================================================
*/

int			mouse_buttons;

qboolean	restore_spi;
int		originalmouseparms[3], newmouseparms[3] = {0, 0, 1};
qboolean	mouseparmsvalid;

RECT		window_rect;


/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse (void)
{
	if (!s_wmv.mouseInitialized)
		return;
	if (!in_mouse->value)
	{
		s_wmv.mouseActive = false;
		return;
	}
	if (s_wmv.mouseActive)
		return;

	s_wmv.mouseActive = true;

	if (mouseparmsvalid)
		restore_spi = SystemParametersInfo (SPI_SETMOUSE, 0, newmouseparms, 0);

	GetWindowRect ( GMainWindow, &window_rect);

	IN_ActivateWin32Mouse();
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse (void)
{
	if (!s_wmv.mouseInitialized)
		return;
	if (!s_wmv.mouseActive)
		return;

	if (restore_spi)
		SystemParametersInfo (SPI_SETMOUSE, 0, originalmouseparms, 0);

	s_wmv.mouseActive = false;

	IN_DeactivateWin32Mouse();
}



/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse (void)
{
	QCvar		*cv;

	cv = Cvar_Get ("in_initmouse", "1", CVAR_INIT);
	if ( !cv->value ) 
		return; 

	s_wmv.mouseInitialized = true;
	mouseparmsvalid = SystemParametersInfo (SPI_GETMOUSE, 0, originalmouseparms, 0);
	mouse_buttons = 3;
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate)
{
	int		i;

	if (!s_wmv.mouseInitialized)
		return;

// perform button actions
	for (i=0 ; i<mouse_buttons ; i++)
	{
		if ( (mstate & (1<<i)) &&
			!(s_wmv.oldButtonState & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, true, sysMsgTime);
		}

		if ( !(mstate & (1<<i)) &&
			(s_wmv.oldButtonState & (1<<i)) )
		{
				Key_Event (K_MOUSE1 + i, false, sysMsgTime);
		}
	}	
		
	s_wmv.oldButtonState = mstate;
}


/*
=========================================================================

VIEW CENTERING

=========================================================================
*/

QCvar	*v_centermove;
QCvar	*v_centerspeed;


/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	// mouse variables
    in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE);

	// joystick variables
	in_joystick				= Cvar_Get ("in_joystick",				"0",		CVAR_ARCHIVE);
	in_debugJoystick		= Cvar_Get ("in_debugjoystick",			"0",		CVAR_TEMP);
	joy_threshold			= Cvar_Get ("joy_threshold",			"0.15",		CVAR_ARCHIVE);
	in_joyBallScale			= Cvar_Get ("in_joyBallScale",			"0.02",		CVAR_ARCHIVE);

	// centering
	v_centermove			= Cvar_Get ("v_centermove",				"0.15",		0);
	v_centerspeed			= Cvar_Get ("v_centerspeed",			"500",		0);

	IN_StartupMouse ();
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
}


/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active)
{
	in_appactive = active;
	s_wmv.mouseActive = !active;		// force a new window check or turn off
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void)
{
	if (!s_wmv.mouseInitialized)
		return;

	if (!in_mouse || !in_appactive)
	{
		IN_DeactivateMouse ();
		return;
	}

	if ( !cl.refresh_prepped
		|| (in_keyCatchers & KEYCATCH_CONSOLE)
		|| (in_keyCatchers & KEYCATCH_UI))
	{
		// temporarily deactivate if in fullscreen
		if (Cvar_VariableValue ("vid_fullscreen") == 0)
		{
			IN_DeactivateMouse ();
			return;
		}
	}

	IN_ActivateMouse ();
}

/*
===========
IN_Move
===========
*/
void IN_Move ()
{
	int		mx, my;

	if (!s_wmv.mouseActive)
		return;

	IN_Win32Mouse(&mx, &my);

	CL_MouseEvent(mx, my);
}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void)
{
	s_wmv.oldButtonState = 0;
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
