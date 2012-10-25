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

#include "../../common/qcommon.h"
#include "../qcommon/qcommon.h"
#include "../../common/system_unix.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	int len, i;
	char* cmdline;

	InitSig();

	// go back to real user for config loads
	seteuid(getuid());

	Sys_ParseArgs(argc, argv);	// bk010104 - added this for support

	// merge the command line, this is kinda silly
	for (len = 1, i = 1; i < argc; i++)
		len += String::Length(argv[i]) + 1;
	cmdline = (char*)malloc(len);
	*cmdline = 0;
	for (i = 1; i < argc; i++)
	{
		if (i > 1)
		{
			String::Cat(cmdline, len, " ");
		}
		String::Cat(cmdline, len, argv[i]);
	}

	Com_Init(cmdline);
	NETQ23_Init();

	Sys_ConsoleInputInit();

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

	while (1)
	{
#ifdef __linux__
		Sys_ConfigureFPU();
#endif
		Com_Frame();
	}
}
