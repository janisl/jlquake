/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../wolfcore/system_unix.h"

//=============================================================================

#if !defined( DEDICATED )
/*
================
Sys_XTimeToSysTime
sub-frame timing of events returned by X
X uses the Time typedef - unsigned long
disable with in_subframe 0

 sys_timeBase*1000 is the number of ms since the Epoch of our origin
 xtime is in ms and uses the Epoch as origin
   Time data type is an unsigned long: 0xffffffff ms - ~49 days period
 I didn't find much info in the XWindow documentation about the wrapping
   we clamp sys_timeBase*1000 to unsigned long, that gives us the current origin for xtime
   the computation will still work if xtime wraps (at ~49 days period since the Epoch) after we set sys_timeBase

================
*/
extern Cvar *in_subframe;
int Sys_XTimeToSysTime( unsigned long xtime ) {
	int ret, time, test;

	if ( !in_subframe->value ) {
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
	test = xtime_aux - ( unsigned long )( base_aux * 1000 );
	Com_Printf( "xtime wrap test: %d\n", test );
#endif

	// show_bug.cgi?id=565
	// some X servers (like suse 8.1's) report weird event times
	// if the game is loading, resolving DNS, etc. we are also getting old events
	// so we only deal with subframe corrections that look 'normal'
	ret = xtime - ( unsigned long )( sys_timeBase * 1000 );
	time = Sys_Milliseconds();
	test = time - ret;
	//printf("delta: %d\n", test);
	if ( test < 0 || test > 30 ) { // in normal conditions I've never seen this go above
		return time;
	}

	return ret;
}
#endif

#if defined __x86_64__
void Sys_SnapVector( float *v ) { // bk001213 - see win32/win_shared.c
	// bk001213 - old linux
	v[0] = rint( v[0] );
	v[1] = rint( v[1] );
	v[2] = rint( v[2] );
}
#endif

char *strlwr( char *s ) {
	if ( s == NULL ) { // bk001204 - paranoia
		assert( 0 );
		return s;
	}
	while ( *s ) {
		*s = tolower( *s );
		s++;
	}
	return s; // bk001204 - duh
}

//============================================

void GetClockTicks( double *t ) {
	unsigned long lo, hi;

	__asm__ __volatile__ (
		"rdtsc ;\
		movl %%eax, %0 ;\
		movl %%edx, %1 "
		: : "m" ( lo ), "m" ( hi ) );

	*t = (double) lo + (double) 0xFFFFFFFF * hi ;
}

float Sys_GetCPUSpeed( void ) {
	double t0, t1;

	GetClockTicks( &t0 );
	sleep( 1 );
	GetClockTicks( &t1 );

	return (float) ( ( t1 - t0 ) / 1000000.0 );
}

void Sys_ShowConsole( int visLevel, qboolean quitOnClose ) {
}

char *Sys_GetCurrentUser( void ) {
	struct passwd *p;

	if ( ( p = getpwuid( getuid() ) ) == NULL ) {
		return "player";
	}
	return p->pw_name;
}

void *Sys_InitializeCriticalSection() {
	return (void *)-1;
}

void Sys_EnterCriticalSection( void *ptr ) {
}

void Sys_LeaveCriticalSection( void *ptr ) {
}
