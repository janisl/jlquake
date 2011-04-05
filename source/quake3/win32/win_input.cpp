/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
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
// win_input.c -- win32 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "../client/client.h"
#include "win_local.h"


QCvar	*in_mouse;

qboolean	in_appactive;

/*
============================================================

  MOUSE CONTROL

============================================================
*/

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse( void ) 
{
	if (!s_wmv.mouseInitialized ) {
		return;
	}
	if ( !in_mouse->integer ) 
	{
		s_wmv.mouseActive = qfalse;
		return;
	}
	if ( s_wmv.mouseActive ) 
	{
		return;
	}

	s_wmv.mouseActive = qtrue;

	if ( in_mouse->integer != -1 ) {
		IN_ActivateDIMouse();
	}
	IN_ActivateWin32Mouse();
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse( void ) {
	if (!s_wmv.mouseInitialized ) {
		return;
	}
	if (!s_wmv.mouseActive ) {
		return;
	}
	s_wmv.mouseActive = qfalse;

	IN_DeactivateDIMouse();
	IN_DeactivateWin32Mouse();
}



/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse( void ) 
{
	s_wmv.mouseInitialized = qfalse;
  s_wmv.mouseStartupDelayed = qfalse;

	if ( in_mouse->integer == 0 ) {
		Com_Printf ("Mouse control not active.\n");
		return;
	}

	// nt4.0 direct input is screwed up
	if ( ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT ) &&
		 ( g_wv.osversion.dwMajorVersion == 4 ) )
	{
		Com_Printf ("Disallowing DirectInput on NT 4.0\n");
		Cvar_Set( "in_mouse", "-1" );
	}

	if ( in_mouse->integer == -1 ) {
		Com_Printf ("Skipping check for DirectInput\n");
	} else {
    if (!GMainWindow)
    {
      Com_Printf ("No window for DirectInput mouse init, delaying\n");
      s_wmv.mouseStartupDelayed = qtrue;
      return;
    }
		if ( IN_InitDIMouse() ) {
	    s_wmv.mouseInitialized = qtrue;
			return;
		}
		Com_Printf ("Falling back to Win32 mouse support...\n");
	}
	s_wmv.mouseInitialized = qtrue;
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate)
{
	int		i;

	if ( !s_wmv.mouseInitialized )
		return;

// perform button actions
	for  (i = 0 ; i < 3 ; i++ )
	{
		if ( (mstate & (1<<i)) &&
			!(s_wmv.oldButtonState & (1<<i)) )
		{
			Sys_QueEvent( sysMsgTime, SE_KEY, K_MOUSE1 + i, qtrue, 0, NULL );
		}

		if ( !(mstate & (1<<i)) &&
			(s_wmv.oldButtonState & (1<<i)) )
		{
			Sys_QueEvent( sysMsgTime, SE_KEY, K_MOUSE1 + i, qfalse, 0, NULL );
		}
	}	

	s_wmv.oldButtonState = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove ( void ) {
	int		mx, my;

	if ( g_pMouse ) {
		IN_DIMouse( &mx, &my );
	} else {
		IN_Win32Mouse( &mx, &my );
	}

	if ( !mx && !my ) {
		return;
	}

	Sys_QueEvent( 0, SE_MOUSE, mx, my, 0, NULL );
}


/*
=========================================================================

=========================================================================
*/

/*
===========
IN_Startup
===========
*/
void IN_Startup( void ) {
	Com_Printf ("\n------- Input Initialization -------\n");
	IN_StartupMouse ();
	IN_StartupJoystick ();
	IN_StartupMIDI();
	Com_Printf ("------------------------------------\n");

	in_mouse->modified = qfalse;
	in_joystick->modified = qfalse;
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown( void ) {
	IN_DeactivateMouse();
	IN_ShutdownDIMouse();
	IN_ShutdownMIDI();
	Cmd_RemoveCommand("midiinfo" );
}


/*
===========
IN_Init
===========
*/
void IN_Init( void ) {
	// MIDI input controler variables
	in_midi					= Cvar_Get ("in_midi",					"0",		CVAR_ARCHIVE);
	in_midiport				= Cvar_Get ("in_midiport",				"1",		CVAR_ARCHIVE);
	in_midichannel			= Cvar_Get ("in_midichannel",			"1",		CVAR_ARCHIVE);
	in_mididevice			= Cvar_Get ("in_mididevice",			"0",		CVAR_ARCHIVE);

	Cmd_AddCommand( "midiinfo", MidiInfo_f );

	// mouse variables
	in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE|CVAR_LATCH);

	// joystick variables
	in_joystick				= Cvar_Get ("in_joystick",				"0",		CVAR_ARCHIVE|CVAR_LATCH);
	in_joyBallScale			= Cvar_Get ("in_joyBallScale",			"0.02",		CVAR_ARCHIVE);
	in_debugJoystick		= Cvar_Get ("in_debugjoystick",			"0",		CVAR_TEMP);

	joy_threshold			= Cvar_Get ("joy_threshold",			"0.15",		CVAR_ARCHIVE);

	IN_Startup();
}


/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active) {
	in_appactive = active;

	if ( !active )
	{
		IN_DeactivateMouse();
	}
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void) {
	// post joystick events
	IN_JoyMove();

	if ( !s_wmv.mouseInitialized ) {
    if (s_wmv.mouseStartupDelayed && GMainWindow)
		{
			Com_Printf("Proceeding with delayed mouse init\n");
      IN_StartupMouse();
			s_wmv.mouseStartupDelayed = qfalse;
		}
		return;
	}

	if ( in_keyCatchers & KEYCATCH_CONSOLE ) {
		// temporarily deactivate if not in the game and
		// running on the desktop
		if (Cvar_VariableValue("r_fullscreen") == 0)
		{
			IN_DeactivateMouse ();
			return;
		}
	}

	if ( !in_appactive ) {
		IN_DeactivateMouse ();
		return;
	}

	IN_ActivateMouse();

	// post events to the system que
	IN_MouseMove();

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
