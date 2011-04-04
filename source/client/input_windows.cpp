//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "win_shared.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct joystickInfo_t
{
	bool		avail;
	int			id;			// joystick number
	JOYCAPS		jc;

	int			oldbuttonstate;
	int			oldpovstate;

	JOYINFOEX	ji;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte s_scantokey[128] =
{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6', 
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i', 
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';', 
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*', 
	K_ALT,' ',   K_CAPSLOCK  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0  , K_HOME, 
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
}; 

static int				window_center_x;
static int				window_center_y;

QCvar*					in_joystick;
QCvar*					in_debugJoystick;
QCvar*					in_joyBallScale;
QCvar*					joy_threshold;

static joystickInfo_t	joy;

static int joyDirectionKeys[16] =
{
	K_LEFTARROW, K_RIGHTARROW,
	K_UPARROW, K_DOWNARROW,
	K_JOY16, K_JOY17,
	K_JOY18, K_JOY19,
	K_JOY20, K_JOY21,
	K_JOY22, K_JOY23,

	K_JOY24, K_JOY25,
	K_JOY26, K_JOY27
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	MapKey
//
//	Map from windows to quake keynums
//
//==========================================================================

static int MapKey(int key)
{
	int modified = (key >> 16) & 255;

	if (modified > 127)
	{
		return 0;
	}

	bool is_extended = !!(key & (1 << 24));

	int result = s_scantokey[modified];

	if (!is_extended)
	{
		switch (result)
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		default:
			return result;
		}
	}
	else
	{
		switch (result)
		{
		case K_PAUSE:
			return K_KP_NUMLOCK;
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		}
		return result;
	}
}

//==========================================================================
//
//	IN_HandleInputMessage
//
//	Returns true if window proc should return 0.
//
//==========================================================================

bool IN_HandleInputMessage(UINT uMsg, WPARAM  wParam, LPARAM  lParam)
{
	switch (uMsg)
	{
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		Sys_QueEvent(sysMsgTime, SE_KEY, MapKey(lParam), true, 0, NULL);
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		Sys_QueEvent(sysMsgTime, SE_KEY, MapKey(lParam), false, 0, NULL);
		break;

	case WM_CHAR:
		Sys_QueEvent(sysMsgTime, SE_CHAR, wParam, 0, 0, NULL);
		break;
	}

	return false;
}

//**************************************************************************
//
//	WIN32 MOUSE CONTROL
//
//**************************************************************************

//==========================================================================
//
//	IN_ActivateWin32Mouse
//
//==========================================================================

void IN_ActivateWin32Mouse()
{
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	RECT window_rect;
	GetWindowRect(GMainWindow, &window_rect);
	if (window_rect.left < 0)
	{
		window_rect.left = 0;
	}
	if (window_rect.top < 0)
	{
		window_rect.top = 0;
	}
	if (window_rect.right >= width)
	{
		window_rect.right = width - 1;
	}
	if (window_rect.bottom >= height - 1)
	{
		window_rect.bottom = height - 1;
	}
	window_center_x = (window_rect.right + window_rect.left) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	SetCursorPos(window_center_x, window_center_y);

	SetCapture(GMainWindow);
	ClipCursor(&window_rect);
	while (ShowCursor(FALSE) >= 0)
		;
}

//==========================================================================
//
//	IN_DeactivateWin32Mouse
//
//==========================================================================

void IN_DeactivateWin32Mouse()
{
	ClipCursor(NULL);
	ReleaseCapture();
	while (ShowCursor(TRUE) < 0)
		;
}

//==========================================================================
//
//	IN_Win32Mouse
//
//==========================================================================

void IN_Win32Mouse(int* mx, int* my)
{
	// find mouse movement
	POINT current_pos;
	GetCursorPos(&current_pos);

	// force the mouse to the center, so there's room to move
	SetCursorPos(window_center_x, window_center_y);

	*mx = current_pos.x - window_center_x;
	*my = current_pos.y - window_center_y;
}

//**************************************************************************
//
//	JOYSTICK
//
//**************************************************************************

//==========================================================================
//
//	IN_StartupJoystick
//
//==========================================================================

void IN_StartupJoystick()
{ 
	// assume no joystick
	joy.avail = false; 

	if (!in_joystick->integer)
	{
		GLog.Write("Joystick is not active.\n");
		return;
	}

	// verify joystick driver is present
	int numdevs = joyGetNumDevs();
	if (numdevs == 0)
	{
		GLog.Write("joystick not found -- driver not present\n");
		return;
	}

	// cycle through the joystick ids for the first valid one
	MMRESULT mmr = 0;
	for (joy.id = 0; joy.id < numdevs; joy.id++)
	{
		Com_Memset(&joy.ji, 0, sizeof(joy.ji));
		joy.ji.dwSize = sizeof(joy.ji);
		joy.ji.dwFlags = JOY_RETURNCENTERED;

		mmr = joyGetPosEx(joy.id, &joy.ji);
		if (mmr == JOYERR_NOERROR)
		{
			break;
		}
	} 

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR)
	{
		GLog.Write("joystick not found -- no valid joysticks (%x)\n", mmr);
		return;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	Com_Memset(&joy.jc, 0, sizeof(joy.jc));
	mmr = joyGetDevCaps(joy.id, &joy.jc, sizeof(joy.jc));
	if (mmr != JOYERR_NOERROR)
	{
		GLog.Write("joystick not found -- invalid joystick capabilities (%x)\n", mmr); 
		return;
	}

	GLog.Write("Joystick found.\n");
	GLog.Write("Pname: %s\n", joy.jc.szPname);
	GLog.Write("OemVxD: %s\n", joy.jc.szOEMVxD);
	GLog.Write("RegKey: %s\n", joy.jc.szRegKey);

	GLog.Write("Numbuttons: %i / %i\n", joy.jc.wNumButtons, joy.jc.wMaxButtons);
	GLog.Write("Axis: %i / %i\n", joy.jc.wNumAxes, joy.jc.wMaxAxes);
	GLog.Write("Caps: 0x%x\n", joy.jc.wCaps);
	if (joy.jc.wCaps & JOYCAPS_HASPOV)
	{
		GLog.Write("HASPOV\n");
	}
	else
	{
		GLog.Write("no POV\n");
	}

	// old button and POV states default to no buttons pressed
	joy.oldbuttonstate = 0;
	joy.oldpovstate = 0;

	// mark the joystick as available
	joy.avail = true; 
}

//==========================================================================
//
//	JoyToF
//
//==========================================================================

static float JoyToF(int value)
{
	float	fValue;

	// move centerpoint to zero
	value -= 32768;

	// convert range from -32768..32767 to -1..1 
	fValue = (float)value / 32768.0;

	if (fValue < -1)
	{
		fValue = -1;
	}
	if (fValue > 1)
	{
		fValue = 1;
	}
	return fValue;
}

//==========================================================================
//
//	JoyToI
//
//==========================================================================

static int JoyToI(int value)
{
	// move centerpoint to zero
	value -= 32768;

	return value;
}

//==========================================================================
//
//	IN_JoyMove
//
//==========================================================================

void IN_JoyMove()
{
	// verify joystick is available and that the user wants to use it
	if (!joy.avail)
	{
		return; 
	}

	// collect the joystick data, if possible
	Com_Memset(&joy.ji, 0, sizeof(joy.ji));
	joy.ji.dwSize = sizeof(joy.ji);
	joy.ji.dwFlags = JOY_RETURNALL;

	if (joyGetPosEx(joy.id, &joy.ji) != JOYERR_NOERROR)
	{
		// read error occurred
		// turning off the joystick seems too harsh for 1 read error,\
		// but what should be done?
		// Com_Printf ("IN_ReadJoystick: no response\n");
		// joy.avail = false;
		return;
	}

	if (in_debugJoystick->integer)
	{
		GLog.Write("%8x %5i %5.2f %5.2f %5.2f %5.2f %6i %6i\n", 
			joy.ji.dwButtons,
			joy.ji.dwPOV,
			JoyToF(joy.ji.dwXpos), JoyToF(joy.ji.dwYpos),
			JoyToF(joy.ji.dwZpos), JoyToF(joy.ji.dwRpos),
			JoyToI(joy.ji.dwUpos), JoyToI(joy.ji.dwVpos));
	}

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	DWORD buttonstate = joy.ji.dwButtons;
	for (int i = 0; i < joy.jc.wNumButtons; i++)
	{
		if ((buttonstate & (1 << i)) && !(joy.oldbuttonstate & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, K_JOY1 + i, true, 0, NULL);
		}
		if (!(buttonstate & (1 << i)) && (joy.oldbuttonstate & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, K_JOY1 + i, false, 0, NULL);
		}
	}
	joy.oldbuttonstate = buttonstate;

	DWORD povstate = 0;

	// convert main joystick motion into 6 direction button bits
	for (int i = 0; i < joy.jc.wNumAxes && i < 4; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		float fAxisValue = JoyToF((&joy.ji.dwXpos)[i]);

		if (fAxisValue < -joy_threshold->value)
		{
			povstate |= (1 << (i * 2));
		}
		else if (fAxisValue > joy_threshold->value)
		{
			povstate |= (1 << (i * 2 + 1));
		}
	}

	// convert POV information from a direction into 4 button bits
	if (joy.jc.wCaps & JOYCAPS_HASPOV)
	{
		if (joy.ji.dwPOV != JOY_POVCENTERED)
		{
			if (joy.ji.dwPOV == JOY_POVFORWARD)
			{
				povstate |= 1 << 12;
			}
			if (joy.ji.dwPOV == JOY_POVBACKWARD)
			{
				povstate |= 1 << 13;
			}
			if (joy.ji.dwPOV == JOY_POVRIGHT)
			{
				povstate |= 1 << 14;
			}
			if (joy.ji.dwPOV == JOY_POVLEFT)
			{
				povstate |= 1 << 15;
			}
		}
	}

	// determine which bits have changed and key an auxillary event for each change
	for (int i = 0; i < 16; i++)
	{
		if ((povstate & (1 << i)) && !(joy.oldpovstate & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, joyDirectionKeys[i], true, 0, NULL);
		}

		if (!(povstate & (1 << i)) && (joy.oldpovstate & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, joyDirectionKeys[i], false, 0, NULL);
		}
	}
	joy.oldpovstate = povstate;

	// if there is a trackball like interface, simulate mouse moves
	if (joy.jc.wNumAxes >= 6)
	{
		int x = JoyToI(joy.ji.dwUpos) * in_joyBallScale->value;
		int y = JoyToI(joy.ji.dwVpos) * in_joyBallScale->value;
		if (x || y)
		{
			Sys_QueEvent(sysMsgTime, SE_MOUSE, x, y, 0, NULL);
		}
	}
}
