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
