/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qwsvdef.h"
#include "../../common/system_unix.h"
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{
	double time, oldtime, newtime;
	quakeparms_t parms;

	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(argc, argv);
	parms.argc = argc;
	parms.argv = argv;

	parms.basedir = ".";

	COM_InitServer(&parms);

	// run one frame immediately for first heartbeat
	COM_ServerFrame(0.1);

	Sys_ConsoleInputInit();

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

	//
	// main loop
	//
	oldtime = Sys_DoubleTime() - 0.1;
	while (1)
	{
		// select on the net socket and stdin
		// the only reason we have a timeout at all is so that if the last
		// connected client times out, the message would not otherwise
		// be printed until the next event.
		if (!SOCK_Sleep(ip_sockets[0], 1000))
		{
			continue;
		}

		// find time passed since last cycle
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;
		oldtime = newtime;

		COM_ServerFrame(time);

		// extrasleep is just a way to generate a fucked up connection on purpose
		if (sys_extrasleep->value)
		{
			usleep(sys_extrasleep->value);
		}
	}
	return 0;
}
