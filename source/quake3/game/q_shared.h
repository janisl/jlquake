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

#define MAX_TEAMNAME 32

#include <assert.h>
#include <time.h>
#include <limits.h>


#if (defined(powerc) || defined(powerpc) || defined(ppc) || defined(__ppc) || defined(__ppc__)) && !defined(C_ONLY)
#define idppc   1
#if defined(__VEC__)
#define idppc_altivec 1
#else
#define idppc_altivec 0
#endif
#else
#define idppc   0
#define idppc_altivec 0
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
#define __cdecl
#define __declspec(x)

#ifdef __ppc__
#define CPUSTRING   "MacOSX-ppc"
#elif defined __i386__
#define CPUSTRING   "MacOSX-i386"
#elif defined __x86_64__
#define CPUSTRING   "MaxOSX-x86_64"
#else
#define CPUSTRING   "MacOSX-other"
#endif

#define __rlwimi(out, in, shift, maskBegin, maskEnd) asm ("rlwimi %0,%1,%2,%3,%4" : "=r" (out) : "r" (in), "i" (shift), "i" (maskBegin), "i" (maskEnd))
#define __dcbt(addr, offset) asm ("dcbt %0,%1" : : "b" (addr), "r" (offset))

static inline unsigned int __lwbrx(register void* addr, register int offset)
{
	register unsigned int word;

	asm ("lwbrx %0,%2,%1" : "=r" (word) : "r" (addr), "b" (offset));
	return word;
}

static inline unsigned short __lhbrx(register void* addr, register int offset)
{
	register unsigned short halfword;

	asm ("lhbrx %0,%2,%1" : "=r" (halfword) : "r" (addr), "b" (offset));
	return halfword;
}

static inline float __fctiw(register float f)
{
	register float fi;

	asm ("fctiw %0,%1" : "=f" (fi) : "f" (f));

	return fi;
}

#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

#include <MacTypes.h>
#define MAC_STATIC

#define CPUSTRING   "MacOS-PPC"

void Sys_PumpEvents(void);

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

enum {qfalse, qtrue};


#define MAX_SAY_TEXT    150

#ifdef ERR_FATAL
#undef ERR_FATAL			// this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
	ERR_NEED_CD					// pop up the need-cd dialog
} errorParm_t;

#if defined(_DEBUG)
	#define HUNK_DEBUG
#endif

typedef enum {
	h_high,
	h_low,
	h_dontcare
} ha_pref;

#ifdef HUNK_DEBUG
#define Hunk_Alloc(size, preference)              Hunk_AllocDebug(size, preference, # size, __FILE__, __LINE__)
void* Hunk_AllocDebug(int size, ha_pref preference, char* label, char* file, int line);
#else
void* Hunk_Alloc(int size, ha_pref preference);
#endif

/*
==============================================================

MATHLIB

==============================================================
*/

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define MAKERGB(v, r, g, b) v[0] = r; v[1] = g; v[2] = b
#define MAKERGBA(v, r, g, b, a) v[0] = r; v[1] = g; v[2] = b; v[3] = a


int     Q_rand(int* seed);
float   Q_random(int* seed);

//=============================================

// removes color sequences from string
char* Q_CleanStr(char* string);

//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct
{
	byte b0;
	byte b1;
	byte b2;
	byte b3;
	byte b4;
	byte b5;
	byte b6;
	byte b7;
} qint64;

//=============================================

// this is only here so the functions in q_shared.c and bg_*.c can link
void    Com_Error(int level, const char* error, ...);
void    Com_Printf(const char* msg, ...);


/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2	// snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT    4	// toggled every map_restart so transitions can be detected

// real time
//=============================================


typedef struct qtime_s
{
	int tm_sec;		/* seconds after the minute - [0,59] */
	int tm_min;		/* minutes after the hour - [0,59] */
	int tm_hour;	/* hours since midnight - [0,23] */
	int tm_mday;	/* day of the month - [1,31] */
	int tm_mon;		/* months since January - [0,11] */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* days since Sunday - [0,6] */
	int tm_yday;	/* days since January 1 - [0,365] */
	int tm_isdst;	/* daylight savings time flag */
} qtime_t;


// server browser sources
// TTimo: AS_MPLAYER is no longer used
#define AS_LOCAL            0
#define AS_MPLAYER      1
#define AS_GLOBAL           2
#define AS_FAVORITES    3


typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,			// CTF
	FLAG_TAKEN_RED,		// One Flag CTF
	FLAG_TAKEN_BLUE,	// One Flag CTF
	FLAG_DROPPED
} flagStatus_t;



#define MAX_PINGREQUESTS                    32
#define MAX_SERVERSTATUSREQUESTS    16

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_TELL    2

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2


#endif	// __Q_SHARED_H
