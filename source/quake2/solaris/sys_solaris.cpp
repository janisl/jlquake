#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/file.h>

#include <dlfcn.h>

#include "../qcommon/qcommon.h"
#include "../../common/system_unix.h"

Cvar* nostdout;

void Sys_Init(void)
{
#if id386
//	Sys_SetFPCW();
#endif
}

int main(int argc, char** argv)
{
	int time, oldtime, newtime;

#if 0
	int newargc;
	char** newargv;
	int i;

	// force dedicated
	newargc = argc;
	newargv = malloc((argc + 3) * sizeof(char*));
	newargv[0] = argv[0];
	newargv[1] = "+set";
	newargv[2] = "dedicated";
	newargv[3] = "1";
	for (i = 1; i < argc; i++)
		newargv[i + 3] = argv[i];
	newargc += 3;

	Qcommon_Init(newargc, newargv);
#else
	Qcommon_Init(argc, argv);
#endif

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get("nostdout", "0", 0);

	if (!nostdout->value)
	{
		fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);
//		printf ("Linux Quake -- Version %0.3f\n", LINUX_VERSION);
	}

	Sys_ConsoleInputInit();

	oldtime = Sys_Milliseconds_();
	while (1)
	{
// find time spent rendering last frame
		do
		{
			newtime = Sys_Milliseconds_();
			time = newtime - oldtime;
		}
		while (time < 1);
		Qcommon_Frame(time);
		oldtime = newtime;
	}

}
