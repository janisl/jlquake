#include "quakedef.h"
#include "../core/system_unix.h"

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>

qboolean			isDedicated;

int nostdout = 0;

char *basedir = ".";

// =======================================================================
// General routines
// =======================================================================

void Sys_Quit (void)
{
	Sys_ConsoleInputShutdown();
	Host_Shutdown();
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	fflush(stdout);
	exit(0);
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

	va_start (argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	Sys_ConsoleInputShutdown();
	Host_Shutdown ();
	exit (1);

} 

void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	Sys_Quit();
	exit(0);
}

void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

int main (int c, char **v)
{

	double		time, oldtime, newtime;
	quakeparms_t parms;
	int j;

	InitSig(); // trap evil signals

	Com_Memset(&parms, 0, sizeof(parms));

	COM_InitArgv2(c, v);
	parms.argc = c;
	parms.argv = v;

	parms.memsize = 16*1024*1024;

	j = COM_CheckParm("-mem");
	if (j)
		parms.memsize = (int) (QStr::Atof(COM_Argv(j+1)) * 1024 * 1024);
	parms.membase = malloc (parms.memsize);

	parms.basedir = basedir;

	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	Host_Init(&parms);

	if (COM_CheckParm("-nostdout"))
		nostdout = 1;
	else {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
		printf ("Linux Quake -- Version %0.3f\n", HEXEN2_VERSION);
	}

	Sys_ConsoleInputInit();

	oldtime = Sys_DoubleTime () - 0.1;
	while (1)
	{
// find time spent rendering last frame
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;

		if (cls.state == ca_dedicated)
		{   // play vcrfiles at max speed
			if (time < sys_ticrate->value)
			{
				usleep(1);
				continue;       // not time to run a server only tic yet
			}
			time = sys_ticrate->value;
		}

		if (time > sys_ticrate->value*2)
			oldtime = newtime;
		else
			oldtime += time;

		Host_Frame (time);
	}

}
