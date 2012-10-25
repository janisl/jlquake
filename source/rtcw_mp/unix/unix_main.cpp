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

#include "../../common/qcommon.h"
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../common/system_unix.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	int len, i;
	char* cmdline;

	InitSig();

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
			strcat(cmdline, " ");
		}
		strcat(cmdline, argv[i]);
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
