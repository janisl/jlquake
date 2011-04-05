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

int			mouse_buttons;

static qboolean	restore_spi;
static int		originalmouseparms[3], newmouseparms[3] = {0, 0, 1};

static qboolean	mouseparmsvalid, mouseactivatetoggle;
static qboolean	mouseshowtoggle = 1;
static qboolean	dinput_acquired;

static qboolean	dinput;

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
IN_UpdateClipCursor
===========
*/
void IN_UpdateClipCursor (void)
{

	if (s_wmv.mouseInitialized && s_wmv.mouseActive && !dinput)
	{
		ClipCursor (&window_rect);
	}
}


/*
===========
IN_ShowMouse
===========
*/
void IN_ShowMouse (void)
{

	if (!mouseshowtoggle)
	{
		ShowCursor (TRUE);
		mouseshowtoggle = 1;
	}
}


/*
===========
IN_HideMouse
===========
*/
void IN_HideMouse (void)
{

	if (mouseshowtoggle)
	{
		ShowCursor (FALSE);
		mouseshowtoggle = 0;
	}
}


/*
===========
IN_ActivateMouse
===========
*/
void IN_ActivateMouse (void)
{

	mouseactivatetoggle = true;

	if (s_wmv.mouseInitialized)
	{
		if (dinput)
		{
			if (g_pMouse)
			{
				if (!dinput_acquired)
				{
					IN_ActivateDIMouse();
					dinput_acquired = true;
				}
			}
			else
			{
				return;
			}
			IN_HideMouse ();
		}
		else
		{
			if (mouseparmsvalid)
				restore_spi = SystemParametersInfo (SPI_SETMOUSE, 0, newmouseparms, 0);

			IN_ActivateWin32Mouse();
			mouseshowtoggle = 0;
		}

		s_wmv.mouseActive = true;
	}
}


/*
===========
IN_SetQuakeMouseState
===========
*/
void IN_SetQuakeMouseState (void)
{
	if (mouseactivatetoggle)
		IN_ActivateMouse ();
}


/*
===========
IN_DeactivateMouse
===========
*/
void IN_DeactivateMouse (void)
{

	mouseactivatetoggle = false;

	if (s_wmv.mouseInitialized)
	{
		if (dinput)
		{
			if (g_pMouse)
			{
				if (dinput_acquired)
				{
					IN_DeactivateDIMouse();
					dinput_acquired = false;
				}
			}
		IN_ShowMouse();
		}
		else
		{
			if (restore_spi)
				SystemParametersInfo (SPI_SETMOUSE, 0, originalmouseparms, 0);

			IN_DeactivateWin32Mouse();
		mouseshowtoggle = 1;
		}

		s_wmv.mouseActive = false;
	}
}


/*
===========
IN_RestoreOriginalMouseState
===========
*/
void IN_RestoreOriginalMouseState (void)
{
	if (mouseactivatetoggle)
	{
		IN_DeactivateMouse ();
		mouseactivatetoggle = true;
	}

// try to redraw the cursor so it gets reinitialized, because sometimes it
// has garbage after the mode switch
	ShowCursor (TRUE);
	ShowCursor (FALSE);
}


/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse (void)
{
	HDC			hdc;

	if ( COM_CheckParm ("-nomouse") ) 
		return; 

	s_wmv.mouseInitialized = true;

	if (COM_CheckParm ("-dinput"))
	{
		dinput = IN_InitDIMouse();
	}

	if (!dinput)
	{
		mouseparmsvalid = SystemParametersInfo (SPI_GETMOUSE, 0, originalmouseparms, 0);

		if (mouseparmsvalid)
		{
			if ( COM_CheckParm ("-noforcemspd") ) 
				newmouseparms[2] = originalmouseparms[2];

			if ( COM_CheckParm ("-noforcemaccel") ) 
			{
				newmouseparms[0] = originalmouseparms[0];
				newmouseparms[1] = originalmouseparms[1];
			}

			if ( COM_CheckParm ("-noforcemparms") ) 
			{
				newmouseparms[0] = originalmouseparms[0];
				newmouseparms[1] = originalmouseparms[1];
				newmouseparms[2] = originalmouseparms[2];
			}
		}
	}

	mouse_buttons = 3;

// if a fullscreen video mode was set before the mouse was initialized,
// set the mouse state appropriately
	if (mouseactivatetoggle)
		IN_ActivateMouse ();
}


/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	// joystick variables
    in_joystick = Cvar_Get("joystick", "0", CVAR_ARCHIVE);
	in_debugJoystick		= Cvar_Get ("in_debugjoystick",			"0",		CVAR_TEMP);
	joy_threshold			= Cvar_Get ("joy_threshold",			"0.15",		CVAR_ARCHIVE);
	in_joyBallScale			= Cvar_Get ("in_joyBallScale",			"0.02",		CVAR_ARCHIVE);

	Cmd_AddCommand ("force_centerview", Force_CenterView_f);

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

	IN_ShutdownDIMouse();
}


/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate)
{
	int	i;

	if (s_wmv.mouseActive && !dinput)
	{
	// perform button actions
		for (i=0 ; i<mouse_buttons ; i++)
		{
			if ( (mstate & (1<<i)) &&
				!(s_wmv.oldButtonState & (1<<i)) )
			{
				Key_Event (K_MOUSE1 + i, true);
			}

			if ( !(mstate & (1<<i)) &&
				(s_wmv.oldButtonState & (1<<i)) )
			{
				Key_Event (K_MOUSE1 + i, false);
			}
		}	
			
		s_wmv.oldButtonState = mstate;
	}
}


/*
===========
IN_Move
===========
*/
void IN_Move()
{

	if (!ActiveApp || Minimized)
	{
		return;
	}

	int					mx, my;
	HDC					hdc;
	int					i;
	HRESULT				hr;

	if (!s_wmv.mouseActive)
		return;

	if (dinput)
	{
		IN_DIMouse(&mx, &my);
	}
	else
	{
		IN_Win32Mouse(&mx, &my);
	}

//if (mx ||  my)
//	Con_DPrintf("mx=%d, my=%d\n", mx, my);

	CL_MouseEvent(mx, my);
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
