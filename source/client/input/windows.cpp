//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "public.h"
#include "keycodes.h"
#include "../public.h"
#include "../windows_shared.h"
#define DIRECTINPUT_VERSION 0x0300
#include <dinput.h>

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

#define DINPUT_BUFFERSIZE       16

#define NUM_OBJECTS             (sizeof(rgodf) / sizeof(rgodf[0]))

struct MYDATA
{
	LONG lX;				// X axis goes here
	LONG lY;				// Y axis goes here
	LONG lZ;				// Z axis goes here
	BYTE bButtonA;			// One button goes here
	BYTE bButtonB;			// Another button goes here
	BYTE bButtonC;			// Another button goes here
	BYTE bButtonD;			// Another button goes here
	BYTE bButtonE;				// Another button goes here
	BYTE bButtonF;				// Another button goes here
	BYTE bButtonG;				// Another button goes here
	BYTE bButtonH;				// Another button goes here
};

//
// MIDI definitions
//
#define MAX_MIDIIN_DEVICES  8

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern Cvar* r_fullscreen;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte s_scantokey[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9,	// 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13,    K_CTRL,'a',  's',		// 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
	'\'',    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',		// 2
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,		// 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END,	//4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,					// 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,						// 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0							// 7
};

static byte s_scantokey_german[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '?',    '\'',    K_BACKSPACE, 9,// 0
	'q',    'w',    'e',    'r',    't',    'z',    'u',    'i',
	'o',    'p',    '=',    '+',    13,    K_CTRL, 'a',  's',		// 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    '[',
	']',    '`',    K_SHIFT,'#',  'y',    'x',    'c',    'v',		// 2
	'b',    'n',    'm',    ',',    '.',    '-',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,	// 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END,	//4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             '<',              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,					// 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,						// 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0							// 7
};

static byte s_scantokey_french[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    ')',    '=',    K_BACKSPACE, 9,	// 0
	'a',    'z',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '^',    '$',    13,    K_CTRL, 'q',  's',		// 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    'm',
	'%',    '`',    K_SHIFT,'*',  'w',    'x',    'c',    'v',		// 2
	'b',    'n',    ',',    ';',    ':',    '!',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,	// 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END,	//4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             '<',              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,					// 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,						// 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0							// 7
};

static byte s_scantokey_spanish[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '\'',    '!',    K_BACKSPACE, 9,// 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13,    K_CTRL, 'a',  's',		// 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    '=',
	'{',    '`',    K_SHIFT,'}',  'z',    'x',    'c',    'v',		// 2
	'b',    'n',    'm',    ',',    '.',    '-',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,	// 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END,	//4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             '<',              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,					// 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,						// 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0							// 7
};

static byte s_scantokey_italian[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '\'',    '^',    K_BACKSPACE, 9,// 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13,    K_CTRL, 'a',  's',		// 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    '@',
	'#',    '`',    K_SHIFT,'=',  'z',    'x',    'c',    'v',		// 2
	'b',    'n',    'm',    ',',    '.',    '-',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,	// 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END,	//4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             '<',              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,					// 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,						// 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0							// 7
};

static Cvar* in_mouse;

static int window_center_x;
static int window_center_y;

static int mouse_oldButtonState;
static bool mouse_active;
static bool mouse_initialized;
static bool mouse_startupDelayed;				// delay mouse init to try DI again when we have a window

static HRESULT (WINAPI* pDirectInputCreate)(HINSTANCE hinst, DWORD dwVersion,
	LPDIRECTINPUT* lplpDirectInput, LPUNKNOWN punkOuter);

static HINSTANCE hInstDI;
static LPDIRECTINPUT g_pdi;
static LPDIRECTINPUTDEVICE g_pMouse;

static DIOBJECTDATAFORMAT rgodf[] =
{
	{ &GUID_XAxis,    FIELD_OFFSET(MYDATA, lX),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
	{ &GUID_YAxis,    FIELD_OFFSET(MYDATA, lY),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
	{ &GUID_ZAxis,    FIELD_OFFSET(MYDATA, lZ),       0x80000000 | DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
	{ 0,              FIELD_OFFSET(MYDATA, bButtonA), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
	{ 0,              FIELD_OFFSET(MYDATA, bButtonB), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
	{ 0,              FIELD_OFFSET(MYDATA, bButtonC), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
	{ 0,              FIELD_OFFSET(MYDATA, bButtonD), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
	{ 0, FIELD_OFFSET(MYDATA, bButtonE), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
	{ 0, FIELD_OFFSET(MYDATA, bButtonF), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
	{ 0, FIELD_OFFSET(MYDATA, bButtonG), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
	{ 0, FIELD_OFFSET(MYDATA, bButtonH), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
};

// NOTE TTimo: would be easier using c_dfDIMouse or c_dfDIMouse2
static DIDATAFORMAT df =
{
	sizeof(DIDATAFORMAT),		// this structure
	sizeof(DIOBJECTDATAFORMAT),	// size of object data format
	DIDF_RELAXIS,				// absolute axis coordinates
	sizeof(MYDATA),				// device data size
	NUM_OBJECTS,				// number of objects
	rgodf,						// and here they are
};

Cvar* in_joystick;
static Cvar* in_debugJoystick;
static Cvar* in_joyBallScale;
static Cvar* joy_threshold;

static bool joy_avail;
static int joy_id;					// joystick number
static JOYCAPS joy_jc;
static int joy_oldbuttonstate;
static int joy_oldpovstate;
static JOYINFOEX joy_ji;

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

static Cvar* in_midi;
static Cvar* in_midiport;
static Cvar* in_midichannel;
static Cvar* in_mididevice;

static int midi_numDevices;
static MIDIINCAPS midi_caps[MAX_MIDIIN_DEVICES];
static HMIDIIN midi_hMidiIn;

static bool in_appactive;

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
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		if (cl_language->integer - 1 == LANGUAGE_FRENCH)
		{
			result = s_scantokey_french[modified];
		}
		else if (cl_language->integer - 1 == LANGUAGE_GERMAN)
		{
			result = s_scantokey_german[modified];
		}
		else if (cl_language->integer - 1 == LANGUAGE_ITALIAN)
		{
			result = s_scantokey_italian[modified];
		}
		else if (cl_language->integer - 1 == LANGUAGE_SPANISH)
		{
			result = s_scantokey_spanish[modified];
		}
	}

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
//	IN_MouseEvent
//
//==========================================================================

static void IN_MouseEvent(int mstate)
{
	if (!mouse_initialized)
	{
		return;
	}

	// perform button actions
	for (int i = 0; i < 5; i++)
	{
		if ((mstate & (1 << i)) && !(mouse_oldButtonState & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, K_MOUSE1 + i, true, 0, NULL);
		}

		if (!(mstate & (1 << i)) && (mouse_oldButtonState & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, K_MOUSE1 + i, false, 0, NULL);
		}
	}

	mouse_oldButtonState = mstate;
}

//==========================================================================
//
//	IN_HandleInputMessage
//
//	Returns true if window proc should return 0.
//
//==========================================================================

bool IN_HandleInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEMOVE:
	{
		int temp = 0;
		if (wParam & MK_LBUTTON)
		{
			temp |= 1;
		}
		if (wParam & MK_RBUTTON)
		{
			temp |= 2;
		}
		if (wParam & MK_MBUTTON)
		{
			temp |= 4;
		}
		if (wParam & MK_XBUTTON1)
		{
			temp |= 8;
		}
		if (wParam & MK_XBUTTON2)
		{
			temp |= 16;
		}
		IN_MouseEvent(temp);
	}
	break;

	case WM_MOUSEWHEEL:
		// only relevant for non-DI input and when console is toggled in window mode
		//   if console is toggled in window mode (KEYCATCH_CONSOLE) then mouse is released and DI doesn't see any mouse wheel
		if (in_mouse->integer != 1 || (!r_fullscreen->integer && (in_keyCatchers & KEYCATCH_CONSOLE)))
		{
			// 120 increments, might be 240 and multiples if wheel goes too fast
			int zDelta = (short)HIWORD(wParam) / 120;
			if (zDelta > 0)
			{
				for (int i = 0; i < zDelta; i++)
				{
					Sys_QueEvent(sysMsgTime, SE_KEY, K_MWHEELUP, true, 0, NULL);
					Sys_QueEvent(sysMsgTime, SE_KEY, K_MWHEELUP, false, 0, NULL);
				}
			}
			else
			{
				for (int i = 0; i < -zDelta; i++)
				{
					Sys_QueEvent(sysMsgTime, SE_KEY, K_MWHEELDOWN, true, 0, NULL);
					Sys_QueEvent(sysMsgTime, SE_KEY, K_MWHEELDOWN, false, 0, NULL);
				}
			}
			// when an application processes the WM_MOUSEWHEEL message, it must return zero
			return true;
		}
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

static void IN_ActivateWin32Mouse()
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

static void IN_DeactivateWin32Mouse()
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

static void IN_Win32Mouse(int* mx, int* my)
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
//	DIRECT INPUT MOUSE CONTROL
//
//**************************************************************************

//==========================================================================
//
//	IN_InitDIMouse
//
//==========================================================================

static bool IN_InitDIMouse()
{
	common->Printf("Initializing DirectInput...\n");

	if (!hInstDI)
	{
		hInstDI = LoadLibrary("dinput.dll");

		if (hInstDI == NULL)
		{
			common->Printf("Couldn't load dinput.dll\n");
			return false;
		}
	}

	if (!pDirectInputCreate)
	{
		pDirectInputCreate = (HRESULT (WINAPI*)(HINSTANCE, DWORD, IDirectInputA**, IUnknown*))
							 GetProcAddress(hInstDI,"DirectInputCreateA");

		if (!pDirectInputCreate)
		{
			common->Printf("Couldn't get DI proc addr\n");
			return false;
		}
	}

	// register with DirectInput and get an IDirectInput to play with.
	HRESULT hr = pDirectInputCreate(global_hInstance, DIRECTINPUT_VERSION, &g_pdi, NULL);

	if (FAILED(hr))
	{
		common->Printf("iDirectInputCreate failed\n");
		return false;
	}

	// obtain an interface to the system mouse device.
	hr = g_pdi->CreateDevice(GUID_SysMouse, &g_pMouse, NULL);

	if (FAILED(hr))
	{
		common->Printf("Couldn't open DI mouse device\n");
		return false;
	}

	// set the data format to "mouse format".
	hr = g_pMouse->SetDataFormat(&df);

	if (FAILED(hr))
	{
		common->Printf("Couldn't set DI mouse format\n");
		return false;
	}

	// set the cooperativity level.
	hr = g_pMouse->SetCooperativeLevel(GMainWindow, DISCL_EXCLUSIVE | DISCL_FOREGROUND);

	if (FAILED(hr))
	{
		common->Printf("Couldn't set DI coop level\n");
		return false;
	}

	DIPROPDWORD dipdw =
	{
		{
			sizeof(DIPROPDWORD),		// diph.dwSize
			sizeof(DIPROPHEADER),		// diph.dwHeaderSize
			0,							// diph.dwObj
			DIPH_DEVICE,				// diph.dwHow
		},
		DINPUT_BUFFERSIZE,				// dwData
	};

	// set the buffer size to DINPUT_BUFFERSIZE elements.
	// the buffer size is a DWORD property associated with the device
	hr = g_pMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

	if (FAILED(hr))
	{
		common->Printf("Couldn't set DI buffersize\n");
		return false;
	}

	// clear any pending samples
	for (;; )
	{
		DWORD dwElements = 1;
		DIDEVICEOBJECTDATA od;
		hr = g_pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
		if (FAILED(hr))
		{
			break;
		}
		if (dwElements == 0)
		{
			break;
		}
	}
	DIMOUSESTATE state;
	g_pMouse->GetDeviceState(sizeof(DIDEVICEOBJECTDATA), &state);

	common->Printf("DirectInput initialized.\n");
	return true;
}

//==========================================================================
//
//	IN_ShutdownDIMouse
//
//==========================================================================

static void IN_ShutdownDIMouse()
{
	if (g_pMouse)
	{
		g_pMouse->Release();
		g_pMouse = NULL;
	}

	if (g_pdi)
	{
		g_pdi->Release();
		g_pdi = NULL;
	}
}

//==========================================================================
//
//	IN_ActivateDIMouse
//
//==========================================================================

static void IN_ActivateDIMouse()
{
	if (!g_pMouse)
	{
		return;
	}

	// we may fail to reacquire if the window has been recreated
	HRESULT hr = g_pMouse->Acquire();
	if (FAILED(hr))
	{
		if (!IN_InitDIMouse())
		{
			common->Printf("Falling back to Win32 mouse support...\n");
			Cvar_Set("in_mouse", "-1");
		}
	}
}

//==========================================================================
//
//	IN_DeactivateDIMouse
//
//==========================================================================

static void IN_DeactivateDIMouse()
{
	if (!g_pMouse)
	{
		return;
	}
	g_pMouse->Unacquire();
}

//==========================================================================
//
//	IN_DIMouse
//
//==========================================================================

static void IN_DIMouse(int* mx, int* my)
{
	if (!g_pMouse)
	{
		return;
	}

	HRESULT hr;

	// fetch new events
	for (;; )
	{
		DWORD dwElements = 1;
		DIDEVICEOBJECTDATA od;

		hr = g_pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
		{
			g_pMouse->Acquire();
			return;
		}

		//	Unable to read data or no data available
		if (FAILED(hr))
		{
			break;
		}

		if (dwElements == 0)
		{
			break;
		}

		switch (od.dwOfs)
		{
		case DIMOFS_BUTTON0:
			if (od.dwData & 0x80)
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MOUSE1, true, 0, NULL);
			}
			else
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MOUSE1, false, 0, NULL);
			}
			break;

		case DIMOFS_BUTTON1:
			if (od.dwData & 0x80)
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MOUSE2, true, 0, NULL);
			}
			else
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MOUSE2, false, 0, NULL);
			}
			break;

		case DIMOFS_BUTTON2:
			if (od.dwData & 0x80)
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MOUSE3, true, 0, NULL);
			}
			else
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MOUSE3, false, 0, NULL);
			}
			break;

		case DIMOFS_BUTTON3:
			if (od.dwData & 0x80)
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MOUSE4, true, 0, NULL);
			}
			else
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MOUSE4, false, 0, NULL);
			}
			break;

		case DIMOFS_Z:
			if ((int)od.dwData < 0)
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MWHEELDOWN, true, 0, NULL);
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MWHEELDOWN, false, 0, NULL);
			}
			else if ((int)od.dwData > 0)
			{
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MWHEELUP, true, 0, NULL);
				Sys_QueEvent(od.dwTimeStamp, SE_KEY, K_MWHEELUP, false, 0, NULL);
			}
			break;
		}
	}

	// read the raw delta counter and ignore
	// the individual sample time / values
	DIMOUSESTATE state;
	hr = g_pMouse->GetDeviceState(sizeof(DIDEVICEOBJECTDATA), &state);
	if (FAILED(hr))
	{
		*mx = *my = 0;
		return;
	}
	*mx = state.lX;
	*my = state.lY;
}

//**************************************************************************
//
//	MOUSE CONTROL
//
//**************************************************************************

//==========================================================================
//
//	IN_StartupMouse
//
//==========================================================================

static void IN_StartupMouse()
{
	mouse_initialized = false;
	mouse_startupDelayed = false;

	if (in_mouse->integer == 0)
	{
		common->Printf("Mouse control not active.\n");
		return;
	}

	if (in_mouse->integer == -1)
	{
		common->Printf("Skipping check for DirectInput\n");
	}
	else
	{
		if (!GMainWindow)
		{
			common->Printf("No window for DirectInput mouse init, delaying\n");
			mouse_startupDelayed = true;
			return;
		}
		if (IN_InitDIMouse())
		{
			mouse_initialized = true;
			return;
		}
		common->Printf("Falling back to Win32 mouse support...\n");
	}
	mouse_initialized = true;
}

//==========================================================================
//
//	IN_ActivateMouse
//
//	Called when the window gains focus or changes in some way
//
//==========================================================================

static void IN_ActivateMouse()
{
	if (!mouse_initialized)
	{
		return;
	}
	if (!in_mouse->integer)
	{
		mouse_active = false;
		return;
	}
	if (mouse_active)
	{
		return;
	}

	mouse_active = true;

	if (in_mouse->integer != -1)
	{
		IN_ActivateDIMouse();
	}
	IN_ActivateWin32Mouse();
}

//==========================================================================
//
//	IN_DeactivateMouse
//
//	Called when the window loses focus
//
//==========================================================================

static void IN_DeactivateMouse()
{
	if (!mouse_initialized)
	{
		return;
	}
	if (!mouse_active)
	{
		return;
	}
	mouse_active = false;

	IN_DeactivateDIMouse();
	IN_DeactivateWin32Mouse();
}

//==========================================================================
//
//	IN_MouseMove
//
//==========================================================================

static void IN_MouseMove()
{
	int mx, my;
	if (g_pMouse)
	{
		IN_DIMouse(&mx, &my);
	}
	else
	{
		IN_Win32Mouse(&mx, &my);
	}

	if (!mx && !my)
	{
		return;
	}

	Sys_QueEvent(0, SE_MOUSE, mx, my, 0, NULL);
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

static void IN_StartupJoystick()
{
	// assume no joystick
	joy_avail = false;

	if (!in_joystick->integer)
	{
		common->Printf("Joystick is not active.\n");
		return;
	}

	// verify joystick driver is present
	int numdevs = joyGetNumDevs();
	if (numdevs == 0)
	{
		common->Printf("joystick not found -- driver not present\n");
		return;
	}

	// cycle through the joystick ids for the first valid one
	MMRESULT mmr = 0;
	for (joy_id = 0; joy_id < numdevs; joy_id++)
	{
		Com_Memset(&joy_ji, 0, sizeof(joy_ji));
		joy_ji.dwSize = sizeof(joy_ji);
		joy_ji.dwFlags = JOY_RETURNCENTERED;

		mmr = joyGetPosEx(joy_id, &joy_ji);
		if (mmr == JOYERR_NOERROR)
		{
			break;
		}
	}

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR)
	{
		common->Printf("joystick not found -- no valid joysticks (%x)\n", mmr);
		return;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	Com_Memset(&joy_jc, 0, sizeof(joy_jc));
	mmr = joyGetDevCaps(joy_id, &joy_jc, sizeof(joy_jc));
	if (mmr != JOYERR_NOERROR)
	{
		common->Printf("joystick not found -- invalid joystick capabilities (%x)\n", mmr);
		return;
	}

	common->Printf("Joystick found.\n");
	common->Printf("Pname: %s\n", joy_jc.szPname);
	common->Printf("OemVxD: %s\n", joy_jc.szOEMVxD);
	common->Printf("RegKey: %s\n", joy_jc.szRegKey);

	common->Printf("Numbuttons: %i / %i\n", joy_jc.wNumButtons, joy_jc.wMaxButtons);
	common->Printf("Axis: %i / %i\n", joy_jc.wNumAxes, joy_jc.wMaxAxes);
	common->Printf("Caps: 0x%x\n", joy_jc.wCaps);
	if (joy_jc.wCaps & JOYCAPS_HASPOV)
	{
		common->Printf("HASPOV\n");
	}
	else
	{
		common->Printf("no POV\n");
	}

	// old button and POV states default to no buttons pressed
	joy_oldbuttonstate = 0;
	joy_oldpovstate = 0;

	// mark the joystick as available
	joy_avail = true;
}

//==========================================================================
//
//	JoyToF
//
//==========================================================================

static float JoyToF(int value)
{
	float fValue;

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

static void IN_JoyMove()
{
	// verify joystick is available and that the user wants to use it
	if (!joy_avail)
	{
		return;
	}

	// collect the joystick data, if possible
	Com_Memset(&joy_ji, 0, sizeof(joy_ji));
	joy_ji.dwSize = sizeof(joy_ji);
	joy_ji.dwFlags = JOY_RETURNALL;

	if (joyGetPosEx(joy_id, &joy_ji) != JOYERR_NOERROR)
	{
		// read error occurred
		// turning off the joystick seems too harsh for 1 read error,\
		// but what should be done?
		// common->Printf ("IN_ReadJoystick: no response\n");
		// joy_avail = false;
		return;
	}

	if (in_debugJoystick->integer)
	{
		common->Printf("%8x %5i %5.2f %5.2f %5.2f %5.2f %6i %6i\n",
			joy_ji.dwButtons,
			joy_ji.dwPOV,
			JoyToF(joy_ji.dwXpos), JoyToF(joy_ji.dwYpos),
			JoyToF(joy_ji.dwZpos), JoyToF(joy_ji.dwRpos),
			JoyToI(joy_ji.dwUpos), JoyToI(joy_ji.dwVpos));
	}

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	DWORD buttonstate = joy_ji.dwButtons;
	for (int i = 0; i < joy_jc.wNumButtons; i++)
	{
		if ((buttonstate & (1 << i)) && !(joy_oldbuttonstate & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, K_JOY1 + i, true, 0, NULL);
		}
		if (!(buttonstate & (1 << i)) && (joy_oldbuttonstate & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, K_JOY1 + i, false, 0, NULL);
		}
	}
	joy_oldbuttonstate = buttonstate;

	DWORD povstate = 0;

	// convert main joystick motion into 6 direction button bits
	for (int i = 0; i < joy_jc.wNumAxes && i < 4; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		float fAxisValue = JoyToF((&joy_ji.dwXpos)[i]);

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
	if (joy_jc.wCaps & JOYCAPS_HASPOV)
	{
		if (joy_ji.dwPOV != JOY_POVCENTERED)
		{
			if (joy_ji.dwPOV == JOY_POVFORWARD)
			{
				povstate |= 1 << 12;
			}
			if (joy_ji.dwPOV == JOY_POVBACKWARD)
			{
				povstate |= 1 << 13;
			}
			if (joy_ji.dwPOV == JOY_POVRIGHT)
			{
				povstate |= 1 << 14;
			}
			if (joy_ji.dwPOV == JOY_POVLEFT)
			{
				povstate |= 1 << 15;
			}
		}
	}

	// determine which bits have changed and key an auxillary event for each change
	for (int i = 0; i < 16; i++)
	{
		if ((povstate & (1 << i)) && !(joy_oldpovstate & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, joyDirectionKeys[i], true, 0, NULL);
		}

		if (!(povstate & (1 << i)) && (joy_oldpovstate & (1 << i)))
		{
			Sys_QueEvent(sysMsgTime, SE_KEY, joyDirectionKeys[i], false, 0, NULL);
		}
	}
	joy_oldpovstate = povstate;

	// if there is a trackball like interface, simulate mouse moves
	if (joy_jc.wNumAxes >= 6)
	{
		int x = JoyToI(joy_ji.dwUpos) * in_joyBallScale->value;
		int y = JoyToI(joy_ji.dwVpos) * in_joyBallScale->value;
		if (x || y)
		{
			Sys_QueEvent(sysMsgTime, SE_MOUSE, x, y, 0, NULL);
		}
	}
}

//**************************************************************************
//
//	MIDI
//
//**************************************************************************

//==========================================================================
//
//	MIDI_NoteOff
//
//==========================================================================

static void MIDI_NoteOff(int note)
{
	int qkey = note - 60 + K_AUX1;

	if (qkey > 255 || qkey < K_AUX1)
	{
		return;
	}

	Sys_QueEvent(sysMsgTime, SE_KEY, qkey, false, 0, NULL);
}

//==========================================================================
//
//	MIDI_NoteOn
//
//==========================================================================

static void MIDI_NoteOn(int note, int velocity)
{
	if (velocity == 0)
	{
		MIDI_NoteOff(note);
	}

	int qkey = note - 60 + K_AUX1;

	if (qkey > 255 || qkey < K_AUX1)
	{
		return;
	}

	Sys_QueEvent(sysMsgTime, SE_KEY, qkey, true, 0, NULL);
}

//==========================================================================
//
//	MidiInProc
//
//==========================================================================

static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT uMsg, DWORD dwInstance,
	DWORD dwParam1, DWORD dwParam2)
{
	int message;

	switch (uMsg)
	{
	case MIM_OPEN:
		break;
	case MIM_CLOSE:
		break;
	case MIM_DATA:
		message = dwParam1 & 0xff;

		// note on
		if ((message & 0xf0) == 0x90)
		{
			if (((message & 0x0f) + 1) == in_midichannel->integer)
			{
				MIDI_NoteOn((dwParam1 & 0xff00) >> 8, (dwParam1 & 0xff0000) >> 16);
			}
		}
		else if ((message & 0xf0) == 0x80)
		{
			if (((message & 0x0f) + 1) == in_midichannel->integer)
			{
				MIDI_NoteOff((dwParam1 & 0xff00) >> 8);
			}
		}
		break;
	case MIM_LONGDATA:
		break;
	case MIM_ERROR:
		break;
	case MIM_LONGERROR:
		break;
	}
}

//==========================================================================
//
//	MidiInfo_f
//
//==========================================================================

static void MidiInfo_f()
{
	const char* enableStrings[] = { "disabled", "enabled" };

	common->Printf("\nMIDI control:       %s\n", enableStrings[in_midi->integer != 0]);
	common->Printf("port:               %d\n", in_midiport->integer);
	common->Printf("channel:            %d\n", in_midichannel->integer);
	common->Printf("current device:     %d\n", in_mididevice->integer);
	common->Printf("number of devices:  %d\n", midi_numDevices);
	for (int i = 0; i < midi_numDevices; i++)
	{
		if (i == Cvar_VariableValue("in_mididevice"))
		{
			common->Printf("***");
		}
		else
		{
			common->Printf("...");
		}
		common->Printf("device %2d:       %s\n", i, midi_caps[i].szPname);
		common->Printf("...manufacturer ID: 0x%hx\n", midi_caps[i].wMid);
		common->Printf("...product ID:      0x%hx\n", midi_caps[i].wPid);

		common->Printf("\n");
	}
}

//==========================================================================
//
//	IN_StartupMIDI
//
//==========================================================================

static void IN_StartupMIDI()
{
	if (!Cvar_VariableValue("in_midi"))
	{
		return;
	}

	//
	// enumerate MIDI IN devices
	//
	midi_numDevices = midiInGetNumDevs();

	for (int i = 0; i < midi_numDevices; i++)
	{
		midiInGetDevCaps(i, &midi_caps[i], sizeof(midi_caps[i]));
	}

	//
	// open the MIDI IN port
	//
	if (midiInOpen(&midi_hMidiIn, in_mididevice->integer,
			(DWORD_PTR)MidiInProc, (DWORD_PTR)NULL, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
		common->Printf("WARNING: could not open MIDI device %d: '%s'\n", in_mididevice->integer, midi_caps[(int)in_mididevice->value]);
		return;
	}

	midiInStart(midi_hMidiIn);
}

//==========================================================================
//
//	IN_ShutdownMIDI
//
//==========================================================================

static void IN_ShutdownMIDI()
{
	if (midi_hMidiIn)
	{
		midiInClose(midi_hMidiIn);
	}
	midi_numDevices = 0;
	Com_Memset(&midi_caps, 0, sizeof(midi_caps));
	midi_hMidiIn = NULL;
}

//**************************************************************************
//
//	MAIN INPUT API
//
//**************************************************************************

//==========================================================================
//
//	IN_Startup
//
//==========================================================================

static void IN_Startup()
{
	common->Printf("\n------- Input Initialization -------\n");
	IN_StartupMouse();
	IN_StartupJoystick();
	IN_StartupMIDI();
	common->Printf("------------------------------------\n");

	in_mouse->modified = false;
	in_joystick->modified = false;
}

//==========================================================================
//
//	IN_Init
//
//==========================================================================

void IN_Init()
{
	// mouse variables
	in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE | CVAR_LATCH2);

	// joystick variables
	in_joystick = Cvar_Get("in_joystick", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	in_joyBallScale = Cvar_Get("in_joyBallScale", "0.02", CVAR_ARCHIVE);
	in_debugJoystick = Cvar_Get("in_debugjoystick", "0", CVAR_TEMP);
	joy_threshold = Cvar_Get("joy_threshold", "0.15", CVAR_ARCHIVE);

	// MIDI input controler variables
	in_midi = Cvar_Get("in_midi", "0", CVAR_ARCHIVE);
	in_midiport = Cvar_Get("in_midiport", "1", CVAR_ARCHIVE);
	in_midichannel = Cvar_Get("in_midichannel", "1", CVAR_ARCHIVE);
	in_mididevice = Cvar_Get("in_mididevice", "0", CVAR_ARCHIVE);

	Cmd_AddCommand("midiinfo", MidiInfo_f);

	IN_Startup();
}

//==========================================================================
//
//	IN_Shutdown
//
//==========================================================================

void IN_Shutdown()
{
	IN_DeactivateMouse();
	IN_ShutdownDIMouse();
	IN_ShutdownMIDI();
	Cmd_RemoveCommand("midiinfo");
}

//==========================================================================
//
//	IN_Activate
//
//	Called when the main window gains or loses focus. The window may have
// been destroyed and recreated between a deactivate and an activate.
//
//==========================================================================

void IN_Activate(bool active)
{
	in_appactive = active;

	if (!active)
	{
		IN_DeactivateMouse();
	}
}

//==========================================================================
//
//	IN_Frame
//
//	Called every frame, even if not generating commands
//
//==========================================================================

void IN_Frame()
{
	// post joystick events
	IN_JoyMove();

	if (!mouse_initialized)
	{
		if (mouse_startupDelayed && GMainWindow)
		{
			common->Printf("Proceeding with delayed mouse init\n");
			IN_StartupMouse();
			mouse_startupDelayed = false;
		}
		return;
	}

	// temporarily deactivate if not in the game and running on the desktop
	if ((in_keyCatchers & KEYCATCH_CONSOLE) && r_fullscreen->integer == 0)
	{
		IN_DeactivateMouse();
		return;
	}

	if (!in_appactive)
	{
		IN_DeactivateMouse();
		return;
	}

	IN_ActivateMouse();

	// post events to the system que
	IN_MouseMove();
}
