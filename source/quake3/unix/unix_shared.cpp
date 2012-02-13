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
#ifndef __i386__ // rcg010206 - using this for PPC builds...
long fastftol( float f ) { // bk001213 - from win32/win_shared.c
  //static int tmp;
  //	__asm fld f
  //__asm fistp tmp
  //__asm mov eax, tmp
  return (long)f;
}

void Sys_SnapVector( float *v ) { // bk001213 - see win32/win_shared.c
  // bk001213 - old linux
  v[0] = rint(v[0]);
  v[1] = rint(v[1]);
  v[2] = rint(v[2]);
}
#endif


//============================================

int Sys_GetProcessorId( void )
{
	return CPUID_GENERIC;
}

const char *Sys_GetCurrentUser( void )
{
	struct passwd *p;

	if ( (p = getpwuid( getuid() )) == NULL ) {
		return "player";
	}
	return p->pw_name;
}

#if defined(__linux__)
// TTimo 
// sysconf() in libc, POSIX.1 compliant
unsigned int Sys_ProcessorCount()
{
  return sysconf(_SC_NPROCESSORS_ONLN);
}
#endif
