// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

/*
 * $Header: /H2 Mission Pack/IN_WIN.C 3     3/01/98 8:20p Jmonroe $
 */

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
}


/*
===========
IN_Move
===========
*/
void IN_Move ()
{
	if (!s_wmv.mouseActive)
	{
		return;
	}
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
