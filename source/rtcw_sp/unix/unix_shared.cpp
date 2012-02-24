/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

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

//=============================================================================

//#if 0 // bk001215 - see snapvector.nasm for replacement
#if ( defined __APPLE__ ) || defined __x86_64__ // rcg010206 - using this for PPC builds...
long fastftol( float f ) { // bk001213 - from win32/win_shared.c
	//static int tmp;
	//	__asm fld f
	//__asm fistp tmp
	//__asm mov eax, tmp
	return (long)f;
}

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

int Sys_GetProcessorId( void ) {
	// TODO TTimo add better CPU identification?
	// see Sys_GetHighQualityCPU
	return CPUID_GENERIC;
}

int Sys_GetHighQualityCPU() {
	// TODO TTimo see win_shared.c IsP3 || IsAthlon
	return 0;
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

// TTimo
// show_bug.cgi?id=448
void *Sys_InitializeCriticalSection() {
	Com_DPrintf( "WARNING: Sys_InitializeCriticalSection not implemented\n" );
	return (void *)-1;
}

void Sys_EnterCriticalSection( void *ptr ) {
}

void Sys_LeaveCriticalSection( void *ptr ) {
}

#if 0
#include <pthread.h>

void *Sys_InitializeCriticalSection() {
	pthread_mutex_t   *smpMutex;
	smpMutex = malloc( sizeof( pthread_mutex_t ) );
	pthread_mutex_init( smpMutex, NULL );
	return smpMutex;
}

void Sys_EnterCriticalSection( void *ptr ) {
	pthread_mutex_t   *crit;
	crit = ptr;
	pthread_mutex_lock( crit );
}

void Sys_LeaveCriticalSection( void *ptr ) {
	pthread_mutex_t   *crit;
	crit = ptr;
	pthread_mutex_unlock( crit );
}
#endif
