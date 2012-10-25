#include "quakedef.h"
#include "../common/system_unix.h"

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <execinfo.h>

int nostdout = 0;

const char* basedir = ".";

int main(int c, char** v)
{

	double time, oldtime, newtime;
	quakeparms_t parms;

	InitSig();	// trap evil signals

	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(c, v);
	parms.argc = c;
	parms.argv = v;

	parms.basedir = basedir;

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

	Host_Init(&parms);

	if (COM_CheckParm("-nostdout"))
	{
		nostdout = 1;
	}
	else
	{
		fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);
		printf("Linux Quake -- Version %0.3f\n", HEXEN2_VERSION);
	}

	Sys_ConsoleInputInit();

	oldtime = Sys_DoubleTime() - 0.1;
	while (1)
	{
// find time spent rendering last frame
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;

		if (com_dedicated->integer)
		{	// play vcrfiles at max speed
			if (time < sys_ticrate->value)
			{
				usleep(1);
				continue;		// not time to run a server only tic yet
			}
			time = sys_ticrate->value;
		}

		if (time > sys_ticrate->value * 2)
		{
			oldtime = newtime;
		}
		else
		{
			oldtime += time;
		}

		Host_Frame(time);
	}

}
