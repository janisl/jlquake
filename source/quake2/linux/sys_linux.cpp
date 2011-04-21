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
#include <mntent.h>

#include <dlfcn.h>

#include "../qcommon/qcommon.h"

#include "../../core/system_unix.h"

QCvar *nostdout;

unsigned	sys_frame_time;

void Sys_Quit (void)
{
	Sys_ConsoleInputShutdown();
	CL_Shutdown ();
	Qcommon_Shutdown ();
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	_exit(0);
}

void Sys_Init(void)
{
#if id386
//	Sys_SetFPCW();
#endif
}

void Sys_Error (char *error, ...)
{ 
    va_list     argptr;
    char        string[1024];

// change stdin to non blocking
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	if (ttycon_on)
	{
		tty_Hide();
	}

	CL_Shutdown ();
	Qcommon_Shutdown ();
    
    va_start (argptr,error);
    Q_vsnprintf(string, 1024, error, argptr);
    va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	Sys_ConsoleInputShutdown();
	_exit (1);

} 

/*****************************************************************************/

static void *game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (game_library) 
		dlclose (game_library);
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);

	char	name[MAX_OSPATH];
	char	curpath[MAX_OSPATH];
	char	*path;
#ifdef __i386__
	const char *gamename = "gamei386.so";
#elif defined __x86_64__
	const char *gamename = "gamex86_64.so";
#elif defined __alpha__
	const char *gamename = "gameaxp.so";
#else
#error Unknown arch
#endif

	setreuid(getuid(), getuid());
	setegid(getgid());

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	getcwd(curpath, sizeof(curpath));

	Com_Printf("------- Loading %s -------", gamename);

	// now run through the search paths
	path = NULL;
	while (1)
	{
		path = FS_NextPath (path);
		if (!path)
		{
			return NULL;		// couldn't find one anywhere
		}
		sprintf (name, "%s/%s", path, gamename);
		game_library = dlopen (name, RTLD_NOW );
		if (game_library)
		{
			Com_DPrintf ("LoadLibrary (%s)\n",name);
			break;
		}
		GLog.DWriteLine("failed:\n\"%s\"\n", dlerror());
	}

	GetGameAPI = (void* (*)(void*))dlsym (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame();
		return NULL;
	}

	return GetGameAPI(parms);
}

/*****************************************************************************/

void Sys_AppActivate (void)
{
}

/*****************************************************************************/

char *Sys_GetClipboardData(void)
{
	return NULL;
}

int main (int argc, char **argv)
{
	int 	time, oldtime, newtime;

	// go back to real user for config loads
	seteuid(getuid());

	Qcommon_Init(argc, argv);

	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
//		printf ("Linux Quake -- Version %0.3f\n", LINUX_VERSION);
	}

	Sys_ConsoleInputInit();

    oldtime = Sys_Milliseconds_ ();
    while (1)
    {
// find time spent rendering last frame
		do {
			newtime = Sys_Milliseconds_ ();
			time = newtime - oldtime;
		} while (time < 1);
        Qcommon_Frame (time);
		oldtime = newtime;
    }

}
