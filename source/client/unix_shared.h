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

#ifndef _UNIX_SHARED_H
#define _UNIX_SHARED_H

#include "../core/system_unix.h"

#include <X11/Xlib.h>

#define KEY_MASK	(KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK	(ButtonPressMask | ButtonReleaseMask | \
					PointerMotionMask | ButtonMotionMask)
#define X_MASK		(KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )

extern Display*		dpy;
extern Window		win;
extern Atom wm_protocols;
extern Atom wm_delete_window;

extern Cvar*		in_dgamouse; // user pref for dga mouse
extern Cvar*		in_nograb; // this is strictly for developers

extern Cvar*		in_joystick;

void IN_ActivateMouse();
void IN_DeactivateMouse();

#endif
