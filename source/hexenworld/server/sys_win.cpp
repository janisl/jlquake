#include <sys/timeb.h>
#include "qwsvdef.h"
#include "../../core/system_windows.h"


/*
================
Sys_Error
================
*/
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr,error);
	Q_vsnprintf(text, 1024, error, argptr);
	va_end (argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

	// wait for the user to quit
    MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
      	DispatchMessage(&msg);
	}

	Sys_DestroyConsole();
	exit (1);
}


/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	double t;
    struct _timeb tstruct;
	static int	starttime;

	_ftime( &tstruct );
 
	if (!starttime)
		starttime = tstruct.time;
	t = (tstruct.time-starttime) + tstruct.millitm*0.001;
	
	return t;
}

/*
================
Sys_Quit
================
*/
void Sys_Quit (void)
{
	Sys_DestroyConsole();
	exit (0);
}


/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init (void)
{
}

/*
==================
main

==================
*/
char	*newargv[256];

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	quakeparms_t	parms;
	double			newtime, time, oldtime;
	static	char	cwd[1024];
	int				t;

	global_hInstance = hInstance;

	Sys_CreateConsole("HexenWorld Console");

	COM_InitArgv2(__argc, __argv);
	
	parms.argc = __argc;
	parms.argv = __argv;

	parms.memsize = 16*1024*1024;

	if ((t = COM_CheckParm ("-heapsize")) != 0 &&
		t + 1 < COM_Argc())
		parms.memsize = QStr::Atoi (COM_Argv(t + 1)) * 1024;

	if ((t = COM_CheckParm ("-mem")) != 0 &&
		t + 1 < COM_Argc())
		parms.memsize = QStr::Atoi (COM_Argv(t + 1)) * 1024 * 1024;

	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error("Insufficient memory.\n");

	parms.basedir = ".";

	SV_Init (&parms);

// run one frame immediately for first heartbeat
	SV_Frame (HX_FRAME_TIME);		

//
// main loop
//
	oldtime = Sys_DoubleTime () - HX_FRAME_TIME;
	while (1)
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0))
				Sys_Quit();

      		TranslateMessage(&msg);
      		DispatchMessage(&msg);
		}

	// select on the net socket and stdin
	// the only reason we have a timeout at all is so that if the last
	// connected client times out, the message would not otherwise
	// be printed until the next event.
		//JL: Originally timeout was 0.1 ms
		if (!SOCK_Sleep(net_socket, 1))
			continue;

	// find time passed since last cycle
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;
		
		SV_Frame (time);				
	}	

	return true;
}
