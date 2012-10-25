#include "../qcommon/qcommon.h"
#include "../../common/system_unix.h"
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	int time, oldtime, newtime;

	InitSig();

	// go back to real user for config loads
	seteuid(getuid());

	Qcommon_Init(argc, argv);

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);

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
