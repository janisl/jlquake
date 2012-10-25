/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

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
#include "../../common/system_unix.h"

void GetClockTicks(double* t)
{
	unsigned long lo, hi;

	__asm__ __volatile__ (
		"rdtsc ;\
		movl %%eax, %0 ;\
		movl %%edx, %1 "
		: : "m" (lo), "m" (hi));

	*t = (double)lo + (double)0xFFFFFFFF * hi;
}

float Sys_GetCPUSpeed(void)
{
	double t0, t1;

	GetClockTicks(&t0);
	sleep(1);
	GetClockTicks(&t1);

	return (float)((t1 - t0) / 1000000.0);
}

const char* Sys_GetCurrentUser(void)
{
	struct passwd* p;

	if ((p = getpwuid(getuid())) == NULL)
	{
		return "player";
	}
	return p->pw_name;
}
