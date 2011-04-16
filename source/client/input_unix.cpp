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
#ifdef __linux__
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#endif

// MACROS ------------------------------------------------------------------

//#define KBD_DBG

#define MOUSE_RESET_DELAY	50

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

QCvar*					in_dgamouse;	// user pref for dga mouse
QCvar*					in_nograb;		// this is strictly for developers

QCvar*					in_joystick;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static bool				mouse_avail;
static bool				mouse_active;
static int				mwx, mwy;
static int				mx = 0, my = 0;

static QCvar*			in_mouse;

static QCvar*			in_subframe;

static int				mouse_accel_numerator;
static int				mouse_accel_denominator;
static int				mouse_threshold;    

// Time mouse was reset, we ignore the first 50ms of the mouse to allow settling of events
static int				mouseResetTime = 0;

static QCvar*			in_joystickDebug;
static QCvar*			joy_threshold;

#ifdef __linux__
//	Our file descriptor for the joystick device.
static int				joy_fd = -1;

//	We translate axes movement into keypresses.
static int joy_keys[16] =
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
#endif

// CODE --------------------------------------------------------------------

//**************************************************************************
//
//	KEYBOARD
//
//	NOTE TTimo the keyboard handling is done with KeySyms
//	that means relying on the keyboard mapping provided by X
//	in-game it would probably be better to use KeyCode (i.e. hardware key codes)
//	you would still need the KeySyms in some cases, such as for the console and all entry textboxes
//	(cause there's nothing worse than a qwerty mapping on a french keyboard)
//
//	you can turn on some debugging and verbose of the keyboard code with #define KBD_DBG
//
//**************************************************************************

//==========================================================================
//
//	XLateKey
//
//==========================================================================

static char* XLateKey(XKeyEvent* ev, int& key)
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
//	X11_PendingInput
//
//	bk001206 - from Ryan's Fakk2
//
//	XPending() actually performs a blocking read if no events available.
// From Fakk2, by way of Heretic2, by way of SDL, original idea GGI project.
// The benefit of this approach over the quite badly behaved XAutoRepeatOn/Off
// is that you get focus handling for free, which is a major win with debug
// and windowed mode. It rests on the assumption that the X server will use
// the same timestamp on press/release event pairs for key repeats. 
//
//==========================================================================

static bool X11_PendingInput()
{
	qassert(dpy != NULL);

	// Flush the display connection
	//  and look to see if events are queued
	XFlush(dpy);
	if (XEventsQueued(dpy, QueuedAlready))
	{
		return true;
	}

	// More drastic measures are required -- see if X is ready to talk
	static struct timeval zero_time;
	fd_set fdset;

	int x11_fd = ConnectionNumber(dpy);
	FD_ZERO(&fdset);
	FD_SET(x11_fd, &fdset);
	if (select(x11_fd + 1, &fdset, NULL, NULL, &zero_time) == 1)
	{
		return XPending(dpy);
	}

	// Oh well, nothing is ready ..
	return false;
}

//==========================================================================
//
//	repeated_press
//
// bk001206 - from Ryan's Fakk2. See above.
//
//==========================================================================

static bool repeated_press(XEvent* event)
{
	qassert(dpy != NULL);

	bool repeated = false;
	if (X11_PendingInput())
	{
		XEvent peekevent;
		XPeekEvent(dpy, &peekevent);

		if ((peekevent.type == KeyPress) &&
			(peekevent.xkey.keycode == event->xkey.keycode) &&
			(peekevent.xkey.time == event->xkey.time))
		{
			repeated = true;
			XNextEvent(dpy, &peekevent);  // skip event.
		}
	}

	return repeated;
}

//**************************************************************************
//
//	MOUSE
//
//**************************************************************************

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

//**************************************************************************
//
//	PROCESSING OF X KEYBOARD AND MOUSE EVENTS
//
//**************************************************************************

//==========================================================================
//
//	Sys_XTimeToSysTime
//
//	Sub-frame timing of events returned by X
//	X uses the Time typedef - unsigned long
//	disable with in_subframe 0
//
//	sys_timeBase*1000 is the number of ms since the Epoch of our origin
//	xtime is in ms and uses the Epoch as origin
//	Time data type is an unsigned long: 0xffffffff ms - ~49 days period
//	I didn't find much info in the XWindow documentation about the wrapping
// we clamp sys_timeBase*1000 to unsigned long, that gives us the current
// origin for xtime the computation will still work if xtime wraps (at
// ~49 days period since the Epoch) after we set sys_timeBase.
//
//==========================================================================

static int Sys_XTimeToSysTime(unsigned long xtime)
{
	int ret, time, test;
	
	if (!in_subframe->value)
	{
		// if you don't want to do any event times corrections
		return Sys_Milliseconds();
	}

	// test the wrap issue
#if 0	
	// reference values for test: sys_timeBase 0x3dc7b5e9 xtime 0x541ea451 (read these from a test run)
	// xtime will wrap in 0xabe15bae ms >~ 0x2c0056 s (33 days from Nov 5 2002 -> 8 Dec)
	//   NOTE: date -d '1970-01-01 UTC 1039384002 seconds' +%c
	// use sys_timeBase 0x3dc7b5e9+0x2c0056 = 0x3df3b63f
	// after around 5s, xtime would have wrapped around
	// we get 7132, the formula handles the wrap safely
	unsigned long xtime_aux,base_aux;
	int test;
//	Com_Printf("sys_timeBase: %p\n", sys_timeBase);
//	Com_Printf("xtime: %p\n", xtime);
	xtime_aux = 500; // 500 ms after wrap
	base_aux = 0x3df3b63f; // the base a few seconds before wrap
	test = xtime_aux - (unsigned long)(base_aux*1000);
	Com_Printf("xtime wrap test: %d\n", test);
#endif

	// some X servers (like suse 8.1's) report weird event times
	// if the game is loading, resolving DNS, etc. we are also getting old events
	// so we only deal with subframe corrections that look 'normal'
	ret = xtime - (unsigned long)(sys_timeBase * 1000);
	time = Sys_Milliseconds();
	test = time - ret;
	//printf("delta: %d\n", test);
	if (test < 0 || test > 30) // in normal conditions I've never seen this go above
	{
		return time;
	}

	return ret;
}

//==========================================================================
//
//	HandleEvents
//
//==========================================================================

void HandleEvents()
{
	int t;
	int key;
	char* p;
	bool dowarp = false;

	if (!dpy)
	{
		return;
	}

	while (XPending(dpy))
	{
		XEvent event;
		XNextEvent(dpy, &event);
		switch (event.type)
		{
		case KeyPress:
			t = Sys_XTimeToSysTime(event.xkey.time);
			p = XLateKey(&event.xkey, key);
			if (key)
			{
				Sys_QueEvent(t, SE_KEY, key, true, 0, NULL);
			}
			if (p)
			{
				while (*p)
				{
					Sys_QueEvent(t, SE_CHAR, *p++, 0, 0, NULL);
				}
			}
			break;

		case KeyRelease:
			t = Sys_XTimeToSysTime(event.xkey.time);
			// bk001206 - handle key repeat w/o XAutRepatOn/Off
			//            also: not done if console/menu is active.
			// From Ryan's Fakk2.
			// see game/q_shared.h, KEYCATCH_* . 0 == in 3d game.  
			if (in_keyCatchers == 0)
			{
				// FIXME: KEYCATCH_NONE
				if (repeated_press(&event))
				{
					break;
				}
			}
			XLateKey(&event.xkey, key);
			Sys_QueEvent(t, SE_KEY, key, false, 0, NULL);
			break;

		case ButtonPress:
			t = Sys_XTimeToSysTime(event.xkey.time);
			if (event.xbutton.button == 4)
			{
				Sys_QueEvent(t, SE_KEY, K_MWHEELUP, true, 0, NULL);
			}
			else if (event.xbutton.button == 5)
			{
				Sys_QueEvent(t, SE_KEY, K_MWHEELDOWN, true, 0, NULL);
			}
			else
			{
				// NOTE TTimo there seems to be a weird mapping for K_MOUSE1 K_MOUSE2 K_MOUSE3 ..
				int b = -1;
				if (event.xbutton.button == 1)
				{
					b = 0; // K_MOUSE1
				}
				else if (event.xbutton.button == 2)
				{
					b = 2; // K_MOUSE3
				}
				else if (event.xbutton.button == 3)
				{
					b = 1; // K_MOUSE2
				}
				else if (event.xbutton.button == 6)
				{
					b = 3; // K_MOUSE4
				}
				else if (event.xbutton.button == 7)
				{
					b = 4; // K_MOUSE5
				}
				if (b >= 0)
				{
					Sys_QueEvent(t, SE_KEY, K_MOUSE1 + b, true, 0, NULL);
				}
			}
			break;

		case ButtonRelease:
			t = Sys_XTimeToSysTime(event.xkey.time);
			if (event.xbutton.button == 4)
			{
				Sys_QueEvent(t, SE_KEY, K_MWHEELUP, false, 0, NULL);
			}
			else if (event.xbutton.button == 5)
			{
				Sys_QueEvent(t, SE_KEY, K_MWHEELDOWN, false, 0, NULL);
			}
			else
			{
				int b = -1;
				if (event.xbutton.button == 1)
				{
					b = 0;
				}
				else if (event.xbutton.button == 2)
				{
					b = 2;
				}
				else if (event.xbutton.button == 3)
				{
					b = 1;
				}
				else if (event.xbutton.button == 6)
				{
					b = 3; // K_MOUSE4
				}
				else if (event.xbutton.button == 7)
				{
					b = 4; // K_MOUSE5
				}
				if (b >= 0)
				{
					Sys_QueEvent(t, SE_KEY, K_MOUSE1 + b, false, 0, NULL);
				}
			}
			break;

		case MotionNotify:
			t = Sys_XTimeToSysTime(event.xkey.time);
			if (mouse_active)
			{
				if (in_dgamouse->value)
				{
					if (abs(event.xmotion.x_root) > 1)
					{
						mx += event.xmotion.x_root * 2;
					}
					else
					{
						mx += event.xmotion.x_root;
					}
					if (abs(event.xmotion.y_root) > 1)
					{
						my += event.xmotion.y_root * 2;
					}
					else
					{
						my += event.xmotion.y_root;
					}
					if (t - mouseResetTime > MOUSE_RESET_DELAY)
					{
						Sys_QueEvent(t, SE_MOUSE, mx, my, 0, NULL);
					}
					mx = 0;
					my = 0;
				}
				else
				{
					// If it's a center motion, we've just returned from our warp
					if (event.xmotion.x == glConfig.vidWidth / 2 &&
						event.xmotion.y == glConfig.vidHeight / 2)
					{
						mwx = glConfig.vidWidth / 2;
						mwy = glConfig.vidHeight / 2;
						if (t - mouseResetTime > MOUSE_RESET_DELAY)
						{
							Sys_QueEvent(t, SE_MOUSE, mx, my, 0, NULL);
						}
						mx = my = 0;
						break;
					}

					int dx = ((int)event.xmotion.x - mwx);
					int dy = ((int)event.xmotion.y - mwy);
					if (abs(dx) > 1)
					{
						mx += dx * 2;
					}
					else
					{
						mx += dx;
					}
					if (abs(dy) > 1)
					{
						my += dy * 2;
					}
					else
					{
						my += dy;
					}

					mwx = event.xmotion.x;
					mwy = event.xmotion.y;
					dowarp = true;
				}
			}
			break;
		}
	}

	if (dowarp)
	{
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, glConfig.vidWidth / 2, glConfig.vidHeight / 2);
	}
}

//**************************************************************************
//
//	LINUX JOYSTICK
//
//**************************************************************************

#ifdef __linux__

//==========================================================================
//
//	IN_StartupJoystick
//
//==========================================================================

static void IN_StartupJoystick()
{
	joy_fd = -1;

	if (!in_joystick->integer)
	{
		GLog.Write("Joystick is not active.\n");
		return;
	}

	for (int i = 0; i < 4; i++ )
	{
		char filename[PATH_MAX];

		snprintf(filename, PATH_MAX, "/dev/js%d", i);

		joy_fd = open(filename, O_RDONLY | O_NONBLOCK);

		if (joy_fd != -1)
		{
			GLog.Write("Joystick %s found\n", filename);

			//	Get rid of initialization messages.
			js_event event;
			do
			{
				int n = read(joy_fd, &event, sizeof(event));

				if (n == -1)
				{
					break;
				}
			} while (event.type & JS_EVENT_INIT);

			//	Get joystick statistics.
			char axes = 0;
			ioctl(joy_fd, JSIOCGAXES, &axes);
			char buttons = 0;
			ioctl(joy_fd, JSIOCGBUTTONS, &buttons);

			char name[128];
			if (ioctl(joy_fd, JSIOCGNAME(sizeof(name)), name) < 0)
			{
				QStr::NCpy(name, "Unknown", sizeof(name));
			}

			GLog.Write( "Name:    %s\n", name );
			GLog.Write( "Axes:    %d\n", axes );
			GLog.Write( "Buttons: %d\n", buttons );

			//	Our work here is done.
			return;
		}
	}

	//	No soup for you.
	if (joy_fd == -1)
	{
		GLog.Write("No joystick found.\n");
		return;
	}
}

//==========================================================================
//
//	IN_JoyMove
//
//==========================================================================

void IN_JoyMove()
{
	//	Store instantaneous joystick state. Hack to get around event model
	// used in Linux joystick driver.
	static int axes_state[16];
	//	Old bits for Quake-style input compares.
	static unsigned int old_axes = 0;

	if (joy_fd == -1)
	{
		return;
	}

	//	Empty the queue, dispatching button presses immediately and updating
	// the instantaneous state for the axes.
	do
	{
		js_event event;
		int n = read(joy_fd, &event, sizeof(event));

		if (n == -1)
		{
			//	No error, we're non-blocking.
			break;
		}

		if (event.type & JS_EVENT_BUTTON)
		{
			Sys_QueEvent(0, SE_KEY, K_JOY1 + event.number, event.value, 0, NULL);
		}
		else if (event.type & JS_EVENT_AXIS)
		{
			if (event.number >= 16)
			{
				continue;
			}
			axes_state[event.number] = event.value;
		}
		else
		{
			GLog.Write("Unknown joystick event type\n");
		}

	} while (1);

	//	Translate our instantaneous state to bits.
	unsigned int axes = 0;
	for (int i = 0; i < 16; i++)
	{
		float f = ((float)axes_state[i]) / 32767.0f;

		if (f < -joy_threshold->value)
		{
			axes |= (1 << (i * 2));
		}
		else if (f > joy_threshold->value)
		{
			axes |= (1 << ((i * 2) + 1));
		}
	}

	//	Time to update axes state based on old vs. new.
	for (int i = 0; i < 16; i++)
	{
		if ((axes & (1 << i)) && !(old_axes & (1 << i)))
		{
			Sys_QueEvent(0, SE_KEY, joy_keys[i], true, 0, NULL);
		}

		if (!(axes & (1 << i)) && (old_axes & (1 << i)))
		{
			Sys_QueEvent(0, SE_KEY, joy_keys[i], false, 0, NULL);
		}
	}

	//	Save for future generations.
	old_axes = axes;
}

//**************************************************************************
//
//	NULL JOYSTICK
//
//**************************************************************************

#else

//==========================================================================
//
//	IN_StartupJoystick
//
//==========================================================================

static void IN_StartupJoystick()
{
}

//==========================================================================
//
//	IN_JoyMove
//
//==========================================================================

void IN_JoyMove()
{
}

#endif

//**************************************************************************
//
//	MAIN INPUT API
//
//**************************************************************************

//==========================================================================
//
//	IN_Init
//
//==========================================================================

void IN_Init()
{
	GLog.Write("\n------- Input Initialization -------\n");

	// mouse variables
	in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE);
	in_dgamouse = Cvar_Get("in_dgamouse", "1", CVAR_ARCHIVE);

	// turn on-off sub-frame timing of X events
	in_subframe = Cvar_Get("in_subframe", "1", CVAR_ARCHIVE);

	// developer feature, allows to break without loosing mouse pointer
	in_nograb = Cvar_Get("in_nograb", "0", 0);

	// bk001130 - from cvs.17 (mkv), joystick variables
	in_joystick = Cvar_Get("in_joystick", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	// bk001130 - changed this to match win32
	in_joystickDebug = Cvar_Get("in_debugjoystick", "0", CVAR_TEMP);
	joy_threshold = Cvar_Get("joy_threshold", "0.15", CVAR_ARCHIVE); // FIXME: in_joythreshold

	if (in_mouse->value)
	{
		mouse_avail = true;
	}
	else
	{
		mouse_avail = false;
	}

	IN_StartupJoystick();

	GLog.Write("------------------------------------\n");
}

//==========================================================================
//
//	IN_Shutdown
//
//==========================================================================

void IN_Shutdown()
{
	mouse_avail = false;
}

//==========================================================================
//
//	IN_Frame
//
//==========================================================================

void IN_Frame()
{
	// bk001130 - from cvs 1.17 (mkv)
	IN_JoyMove(); // FIXME: disable if on desktop?

	// temporarily deactivate if not in the game and running on the desktop
	if (in_keyCatchers & KEYCATCH_CONSOLE && r_fullscreen->integer == 0)
	{
		IN_DeactivateMouse ();
		return;
	}

	IN_ActivateMouse();
}
