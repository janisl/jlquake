/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
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
//
#ifndef __Q_SHARED_H
#define __Q_SHARED_H

#include "../../common/qcommon.h"

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#define Q3_VERSION      "Q3 1.32b"
// 1.32 released 7-10-2002

#include <assert.h>
#include <time.h>
#include <limits.h>


#if (defined(powerc) || defined(powerpc) || defined(ppc) || defined(__ppc) || defined(__ppc__)) && !defined(C_ONLY)
#define idppc   1
#else
#define idppc   0
#endif

//======================= WIN32 DEFINES =================================

#ifdef WIN32

#define MAC_STATIC

// buildstring will be incorporated into the version string
#ifdef NDEBUG
#ifdef _M_IX86
#define CPUSTRING   "win-x86"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64"
#endif
#else
#ifdef _M_IX86
#define CPUSTRING   "win-x86-debug"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP-debug"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64-debug"
#endif
#endif

#endif

//======================= MAC OS X DEFINES =====================

#if defined(MACOS_X)

#define MAC_STATIC

#ifdef __ppc__
#define CPUSTRING   "MacOSX-ppc"
#elif defined __i386__
#define CPUSTRING   "MacOSX-i386"
#elif defined __x86_64__
#define CPUSTRING   "MaxOSX-x86_64"
#else
#define CPUSTRING   "MacOSX-other"
#endif

#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

#include <MacTypes.h>
#define MAC_STATIC

#define CPUSTRING   "MacOS-PPC"

#endif

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

#define MAC_STATIC	// bk: FIXME

#ifdef __i386__
#define CPUSTRING   "linux-i386"
#elif defined __axp__
#define CPUSTRING   "linux-alpha"
#elif defined __x86_64__
#define CPUSTRING   "linux-x86_64"
#else
#define CPUSTRING   "linux-other"
#endif

#endif

//======================= FreeBSD DEFINES =====================
#ifdef __FreeBSD__	// rb010123

#define MAC_STATIC

#ifdef __i386__
#define CPUSTRING       "freebsd-i386"
#elif defined __axp__
#define CPUSTRING       "freebsd-alpha"
#elif defined __x86_64__
#define CPUSTRING       "freebsd-x86_64"
#else
#define CPUSTRING       "freebsd-other"
#endif

#endif

//=============================================================

#ifdef ERR_FATAL
#undef ERR_FATAL			// this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
} errorParm_t;

/*
==============================================================

MATHLIB

==============================================================
*/

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

//=============================================

// this is only here so the functions in q_shared.c and bg_*.c can link
void    Com_Error(int level, const char* error, ...);
void    Com_Printf(const char* msg, ...);

// server browser sources
// TTimo: AS_MPLAYER is no longer used
#define AS_LOCAL            0
#define AS_MPLAYER      1
#define AS_GLOBAL           2
#define AS_FAVORITES    3

#define MAX_PINGREQUESTS                    32
#define MAX_SERVERSTATUSREQUESTS    16

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2


#endif	// __Q_SHARED_H
