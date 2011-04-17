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

#include "../linux/rw_linux.h"
#include "../../core/system_unix.h"

QCvar *nostdout;

unsigned	sys_frame_time;

// =======================================================================
// General routines
// =======================================================================

void Sys_ConsoleOutput (char *string)
{
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	unsigned char		*p;

	va_start (argptr,fmt);
	Q_vsnprintf(text, 1024, fmt, argptr);
	va_end (argptr);

	if (QStr::Length(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

    if (nostdout && nostdout->value)
        return;

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
}

void Sys_Quit (void)
{
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
    
    va_start (argptr,error);
    Q_vsnprintf(string, 1024, error, argptr);
    va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	CL_Shutdown ();
	Qcommon_Shutdown ();
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
#ifndef REF_HARD_LINKED
	void	*(*GetGameAPI) (void *);

	char	name[MAX_OSPATH];
	char	curpath[MAX_OSPATH];
	char	*path;
#ifdef __sgi
	const char *gamename = "gamemips.so";
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
			return NULL;		// couldn't find one anywhere
		sprintf (name, "%s/%s/%s", curpath, path, gamename);
		Com_Printf ("Trying to load library (%s)\n",name);
		game_library = dlopen (name, RTLD_NOW );
		if (game_library)
		{
			Com_DPrintf ("LoadLibrary (%s)\n",name);
			break;
		}
	}

	GetGameAPI = (void *)dlsym (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
#else
	return (void *)GetGameAPI (parms);
#endif
}

/*****************************************************************************/

void Sys_AppActivate (void)
{
}

void Sys_SendKeyEvents (void)
{
	KBD_Update();

	// grab frame time 
	sys_frame_time = Sys_Milliseconds_();
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

/* 	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY); */

	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) {
/* 		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY); */
//		printf ("Linux Quake -- Version %0.3f\n", LINUX_VERSION);
	}

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
