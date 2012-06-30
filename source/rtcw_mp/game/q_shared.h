/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#include "../../common/qcommon.h"

#define Q3_VERSION      "Wolf 1.41b-MP"
// 1.41b-MP: fix autodl sploit
// 1.4-MP : (== 1.34)
// 1.3-MP : final for release
// 1.1b - TTimo SP linux release (+ MP updates)
// 1.1b5 - Mac update merge in

#if defined(ppc) || defined(__ppc) || defined(__ppc__) || defined(__POWERPC__)
#define idppc 1
#endif

#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

// for windows fastcall option

#define QDECL

//======================= WIN32 DEFINES =================================

#ifdef WIN32

#define MAC_STATIC

#undef QDECL
#define QDECL   __cdecl

// buildstring will be incorporated into the version string
#ifdef NDEBUG
#ifdef _M_IX86
#define CPUSTRING   "win-x86"
#elif defined _M_X86_64
#define CPUSTRING   "win-x86_64"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP"
#endif
#else
#ifdef _M_IX86
#define CPUSTRING   "win-x86-debug"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64-debug"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP-debug"
#endif
#endif


#endif

//======================= MAC OS X SERVER DEFINES =====================

#if defined(MACOS_X)

#define MAC_STATIC

#define CPUSTRING   "MacOS_X"

// Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
// runs *much* faster than calling sqrt(). We'll use two Newton-Raphson
// refinement steps to get bunch more precision in the 1/sqrt() value for very little cost.
// We'll then multiply 1/sqrt times the original value to get the sqrt.
// This is about 12.4 times faster than sqrt() and according to my testing (not exhaustive)
// it returns fairly accurate results (error below 1.0e-5 up to 100000.0 in 0.1 increments).

static inline float idSqrt(float x)
{
	const float half = 0.5;
	const float one = 1.0;
	float B, y0, y1;

	// This'll NaN if it hits frsqrte. Handle both +0.0 and -0.0
	if (fabs(x) == 0.0)
	{
		return x;
	}
	B = x;

#ifdef __GNUC__
	asm ("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
	y0 = __frsqrte(B);
#endif
	/* First refinement step */

	y1 = y0 + half * y0 * (one - B * y0 * y0);

	/* Second refinement step -- copy the output of the last step to the input of this step */

	y0 = y1;
	y1 = y0 + half * y0 * (one - B * y0 * y0);

	/* Get sqrt(x) from x * 1/sqrt(x) */
	return x * y1;
}
#define sqrt idSqrt


#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

#include <MacTypes.h>
#define MAC_STATIC

#define CPUSTRING   "MacOS-PPC"

static inline float idSqrt(float x)
{
	const float half = 0.5;
	const float one = 1.0;
	float B, y0, y1;

	// This'll NaN if it hits frsqrte. Handle both +0.0 and -0.0
	if (fabs(x) == 0.0)
	{
		return x;
	}
	B = x;

#ifdef __GNUC__
	asm ("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
	y0 = __frsqrte(B);
#endif
	/* First refinement step */

	y1 = y0 + half * y0 * (one - B * y0 * y0);

	/* Second refinement step -- copy the output of the last step to the input of this step */

	y0 = y1;
	y1 = y0 + half * y0 * (one - B * y0 * y0);

	/* Get sqrt(x) from x * 1/sqrt(x) */
	return x * y1;
}
#define sqrt idSqrt

#endif

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

#define MAC_STATIC

#ifdef __i386__
#define CPUSTRING   "linux-i386"
#elif defined __axp__
#define CPUSTRING   "linux-alpha"
#else
#define CPUSTRING   "linux-other"
#endif

#endif

//=============================================================


enum {qfalse, qtrue};

// TTimo gcc: was missing, added from Q3 source
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifdef  ERR_FATAL
#undef  ERR_FATAL				// this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
} errorParm_t;

#if defined(_DEBUG)
	#define HUNK_DEBUG
#endif

typedef enum {
	h_high,
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

//=============================================

#ifdef _WIN32
#define Q_putenv _putenv
#else
#define Q_putenv putenv
#endif

//=============================================

// this is only here so the functions in q_shared.c and bg_*.c can link
void QDECL Com_Error(int level, const char* error, ...);
void QDECL Com_Printf(const char* msg, ...);

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2	// snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT    4	// toggled every map_restart so transitions can be detected

// server browser sources
#define AS_LOCAL        0
#define AS_GLOBAL       1			// NERVE - SMF - modified
#define AS_FAVORITES    2
#define AS_MPLAYER      3

#define MAX_PINGREQUESTS            16
#define MAX_SERVERSTATUSREQUESTS    16

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2

// NERVE - SMF - wolf server/game states
typedef enum {
	GS_INITIALIZE = -1,
	GS_PLAYING,
	GS_WARMUP_COUNTDOWN,
	GS_WARMUP,
	GS_INTERMISSION,
	GS_WAITING_FOR_PLAYERS,
	GS_RESET
} gamestate_t;

#endif	// __Q_SHARED_H
