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
#include "render_local.h"
#include "unix_shared.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

bool					mouse_avail;
bool					mouse_active;
int						mwx, mwy;
int						mx = 0, my = 0;

QCvar*					in_mouse;
QCvar*					in_dgamouse; // user pref for dga mouse
QCvar*					in_nograb; // this is strictly for developers

static int mouse_accel_numerator;
static int mouse_accel_denominator;
static int mouse_threshold;    

// Time mouse was reset, we ignore the first 50ms of the mouse to allow settling of events
int mouseResetTime = 0;

// CODE --------------------------------------------------------------------

/*****************************************************************************
** KEYBOARD
** NOTE TTimo the keyboard handling is done with KeySyms
**   that means relying on the keyboard mapping provided by X
**   in-game it would probably be better to use KeyCode (i.e. hardware key codes)
**   you would still need the KeySyms in some cases, such as for the console and all entry textboxes
**     (cause there's nothing worse than a qwerty mapping on a french keyboard)
**
** you can turn on some debugging and verbose of the keyboard code with #define KBD_DBG
******************************************************************************/

//#define KBD_DBG

//==========================================================================
//
//	XLateKey
//
//==========================================================================

char* XLateKey(XKeyEvent* ev, int& key)
{
	key = 0;

	static char buf[64];
	KeySym keysym;
	int XLookupRet = XLookupString(ev, buf, sizeof(buf), &keysym, 0);
#ifdef KBD_DBG
	GLog.Write("XLookupString ret: %d buf: %s keysym: %x\n", XLookupRet, buf, keysym);
#endif
  
	switch (keysym)
	{
	case XK_KP_Page_Up: 
	case XK_KP_9:
		key = K_KP_PGUP;
		break;
	case XK_Page_Up:
		key = K_PGUP;
		break;

	case XK_KP_Page_Down: 
	case XK_KP_3:
		key = K_KP_PGDN;
		break;
	case XK_Page_Down:
		key = K_PGDN;
		break;

	case XK_KP_Home:
	case XK_KP_7:
		key = K_KP_HOME;
		break;
	case XK_Home:
		key = K_HOME;
		break;

	case XK_KP_End:
	case XK_KP_1:
		key = K_KP_END;
		break;
	case XK_End:
		key = K_END;
		break;

	case XK_KP_Left:
	case XK_KP_4:
		key = K_KP_LEFTARROW;
		break;
	case XK_Left:
		key = K_LEFTARROW;
		break;

	case XK_KP_Right:
	case XK_KP_6:
		key = K_KP_RIGHTARROW;
		break;
	case XK_Right:
		key = K_RIGHTARROW;
		break;

	case XK_KP_Down:
	case XK_KP_2:
		key = K_KP_DOWNARROW;
		break;
	case XK_Down:
		key = K_DOWNARROW;
		break;

	case XK_KP_Up:   
	case XK_KP_8:
		key = K_KP_UPARROW;
		break;
	case XK_Up:
		key = K_UPARROW;
		break;

	case XK_Escape:
		key = K_ESCAPE;
		break;

	case XK_KP_Enter:
		key = K_KP_ENTER;
		break;
	case XK_Return:
		key = K_ENTER;
		break;

	case XK_Tab:
		key = K_TAB;
		break;

	case XK_F1:
		key = K_F1;
		break;

	case XK_F2:
		key = K_F2;
		break;

	case XK_F3:
		key = K_F3;
		break;

	case XK_F4:
		key = K_F4;
		break;

	case XK_F5:
		key = K_F5;
		break;

	case XK_F6:
		key = K_F6;
		break;

	case XK_F7:
		key = K_F7;
		break;

	case XK_F8:
		key = K_F8;
		break;

	case XK_F9:
		key = K_F9;
		break;

	case XK_F10:
		key = K_F10;
		break;

	case XK_F11:
		key = K_F11;
		break;

	case XK_F12:
		key = K_F12;
		break;

	case XK_BackSpace:
		key = K_BACKSPACE;
		break;

	case XK_KP_Delete:
	case XK_KP_Decimal:
		key = K_KP_DEL;
		break;
	case XK_Delete:
		key = K_DEL;
		break;

	case XK_Pause:
		key = K_PAUSE;
		break;

	case XK_Shift_L:
	case XK_Shift_R:
		key = K_SHIFT;
		break;

	case XK_Execute: 
	case XK_Control_L: 
	case XK_Control_R:
		key = K_CTRL;
		break;

	case XK_Alt_L:  
	case XK_Meta_L: 
	case XK_Alt_R:  
	case XK_Meta_R:
		key = K_ALT;
		break;

	case XK_KP_Begin:
		key = K_KP_5;
		break;

	case XK_Insert:
		key = K_INS;
		break;
	case XK_KP_Insert:
	case XK_KP_0:
		key = K_KP_INS;
		break;

	case XK_KP_Multiply:
		key = K_KP_STAR;
		break;
	case XK_KP_Add:
		key = K_KP_PLUS;
		break;
	case XK_KP_Subtract:
		key = K_KP_MINUS;
		break;
	case XK_KP_Divide:
		key = K_KP_SLASH;
		break;

	// bk001130 - from cvs1.17 (mkv)
	case XK_exclam:
		key = '1';
		break;
	case XK_at:
		key = '2';
		break;
	case XK_numbersign:
		key = '3';
		break;
	case XK_dollar:
		key = '4';
		break;
	case XK_percent:
		key = '5';
		break;
	case XK_asciicircum:
		key = '6';
		break;
	case XK_ampersand:
		key = '7';
		break;
	case XK_asterisk:
		key = '8';
		break;
	case XK_parenleft:
		key = '9';
		break;
	case XK_parenright:
		key = '0';
		break;

	// weird french keyboards ..
	// NOTE: console toggle is hardcoded in cl_keys.c, can't be unbound
	//   cleaner would be .. using hardware key codes instead of the key syms
	//   could also add a new K_KP_CONSOLE
	case XK_twosuperior:
		key = '~';
		break;
		
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=472
	case XK_space:
	case XK_KP_Space:
		key = K_SPACE;
		break;

	default:
		if (XLookupRet == 0)
		{
			GLog.DWrite("Warning: XLookupString failed on KeySym %d\n", keysym);
			return NULL;
		}
		// XK_* tests failed, but XLookupString got a buffer, so let's try it
		key = *(unsigned char*)buf;
		if (key >= 'A' && key <= 'Z')
		{
			key = key - 'A' + 'a';
		}
		// if ctrl is pressed, the keys are not between 'A' and 'Z', for instance ctrl-z == 26 ^Z ^C etc.
		// see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=19
		else if (key >= 1 && key <= 26)
		{
			key = key + 'a' - 1;
		}
		break;
	}

	return buf;
}

//==========================================================================
//
//	CreateNullCursor
//
//	Makes a null cursor.
//
//==========================================================================

static Cursor CreateNullCursor(Display *display, Window root)
{
	Pixmap cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
	XGCValues xgc;
	xgc.function = GXclear;
	GC gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
	XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
	XColor dummycolour;
	dummycolour.pixel = 0;
	dummycolour.red = 0;
	dummycolour.flags = 04;
	Cursor cursor = XCreatePixmapCursor(display, cursormask, cursormask,
		&dummycolour, &dummycolour, 0,0);
	XFreePixmap(display, cursormask);
	XFreeGC(display, gc);
	return cursor;
}

//==========================================================================
//
//	install_grabs
//
//==========================================================================

static void install_grabs()
{
	// inviso cursor
	XWarpPointer(dpy, None, win, 0, 0, 0, 0, glConfig.vidWidth / 2, glConfig.vidHeight / 2);
	XSync(dpy, False);

	XDefineCursor(dpy, win, CreateNullCursor(dpy, win));

	XGrabPointer(dpy, win, // bk010108 - do this earlier?
		False,
		MOUSE_MASK,
		GrabModeAsync, GrabModeAsync,
		win,
		None,
		CurrentTime);

	XGetPointerControl(dpy, &mouse_accel_numerator, &mouse_accel_denominator,
		&mouse_threshold);

	XChangePointerControl(dpy, True, True, 1, 1, 0);

	XSync(dpy, False);

	mouseResetTime = Sys_Milliseconds();

	if (in_dgamouse->value)
	{
		int MajorVersion, MinorVersion;

		if (!XF86DGAQueryVersion(dpy, &MajorVersion, &MinorVersion))
		{
			// unable to query, probalby not supported, force the setting to 0
			GLog.Write("Failed to detect XF86DGA Mouse\n");
			Cvar_Set("in_dgamouse", "0");
		}
		else
		{
			XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectMouse);
			XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		}
	}
	else
	{
		mwx = glConfig.vidWidth / 2;
		mwy = glConfig.vidHeight / 2;
		mx = my = 0;
	}


	XGrabKeyboard(dpy, win, False, GrabModeAsync, GrabModeAsync, CurrentTime);

	XSync(dpy, False);
}

//==========================================================================
//
//	uninstall_grabs
//
//==========================================================================

static void uninstall_grabs()
{
	if (in_dgamouse->value)
	{
		GLog.DWrite("DGA Mouse - Disabling DGA DirectVideo\n");
		XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
	}

	XChangePointerControl(dpy, True, True, mouse_accel_numerator, 
		mouse_accel_denominator, mouse_threshold);

	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

	XWarpPointer(dpy, None, win, 0, 0, 0, 0, glConfig.vidWidth / 2, glConfig.vidHeight / 2);

	// inviso cursor
	XUndefineCursor(dpy, win);
}

//==========================================================================
//
//	IN_ActivateMouse
//
//==========================================================================

void IN_ActivateMouse() 
{
	if (!mouse_avail || !dpy || !win)
	{
		return;
	}

	if (!mouse_active)
	{
		if (!in_nograb->value)
		{
			install_grabs();
		}
		else if (in_dgamouse->value) // force dga mouse to 0 if using nograb
		{
			Cvar_Set("in_dgamouse", "0");
		}
		mouse_active = true;
	}
}

//==========================================================================
//
//	IN_DeactivateMouse
//
//==========================================================================

void IN_DeactivateMouse() 
{
	if (!mouse_avail || !dpy || !win)
	{
		return;
	}

	if (mouse_active)
	{
		if (!in_nograb->value)
		{
			uninstall_grabs();
		}
		else if (in_dgamouse->value) // force dga mouse to 0 if using nograb
		{
			Cvar_Set("in_dgamouse", "0");
		}
		mouse_active = false;
	}
}
