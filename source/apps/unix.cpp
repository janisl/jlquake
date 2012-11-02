//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../common/qcommon.h"
#include "../common/system_unix.h"
#include "main.h"
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{
	InitSig();

	// go back to real user for config loads
	seteuid(getuid());

	Sys_ParseArgs(argc, argv);	// bk010104 - added this for support

	// merge the command line, this is kinda silly
	int len = 1;
	for (int i = 1; i < argc; i++)
	{
		len += String::Length(argv[i]) + 1;
	}
	char* cmdline = (char*)malloc(len);
	*cmdline = 0;
	for (int i = 1; i < argc; i++)
	{
		if (i > 1)
		{
			String::Cat(cmdline, len, " ");
		}
		String::Cat(cmdline, len, argv[i]);
	}

	Com_Init(argc, argv, cmdline);

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

	Sys_ConsoleInputInit();

	while (1)
	{
#ifdef __linux__
		Sys_ConfigureFPU();
#endif
		Com_Frame();

		if (com_dedicated && com_dedicated->integer && com_sv_running && !com_sv_running->integer)
		{
			usleep(25000);
		}
	}
	return 0;
}
