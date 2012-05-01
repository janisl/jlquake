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
// sys_null.h -- null system driver to aid porting efforts

#include <errno.h>
#include <stdio.h>
#include "../qcommon/qcommon.h"

void Sys_Error(const char* error, ...)
{
	va_list argptr;

	printf("Sys_Error: ");
	va_start(argptr,error);
	vprintf(error,argptr);
	va_end(argptr);
	printf("\n");

	exit(1);
}

void Sys_Quit(void)
{
	exit(0);
}

int     Sys_Milliseconds(void)
{
	return 0;
}

void    Sys_Init(void)
{
}


void main(int argc, char** argv)
{
	Com_Init(argc, argv);

	while (1)
	{
		Com_Frame();
	}
}
