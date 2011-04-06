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

#ifndef _UNIX_SHARED_H
#define _UNIX_SHARED_H

#include <GL/glx.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>

#define KEY_MASK	(KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK	(ButtonPressMask | ButtonReleaseMask | \
					PointerMotionMask | ButtonMotionMask)
#define X_MASK		(KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )

extern Display*		dpy;
extern int			scrnum;
extern Window		win;
extern GLXContext	ctx;

extern QCvar*		in_dgamouse; // user pref for dga mouse
extern QCvar*		in_nograb; // this is strictly for developers

extern bool			vidmode_active;
extern bool			vidmode_ext;
extern int			vidmode_MajorVersion, vidmode_MinorVersion; // major and minor of XF86VidExtensions

// gamma value of the X display before we start playing with it
extern XF86VidModeGamma	vidmode_InitialGamma;

extern QCvar *  in_joystick;

rserr_t GLimp_GLXSharedInit(int width, int height, bool fullscreen);
void GLimp_SharedShutdown();
void IN_ActivateMouse();
void IN_DeactivateMouse();
void HandleEvents();
void IN_JoyMove();

#endif
