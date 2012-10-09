#include "quakedef.h"
#include "../common/system_unix.h"

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <execinfo.h>

qboolean isDedicated;

int nostdout = 0;

const char* basedir = ".";

// =======================================================================
// General routines
// =======================================================================

void Sys_Quit(void)
{
	Sys_ConsoleInputShutdown();
	Host_Shutdown();
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);
	fflush(stdout);
	exit(0);
}

void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char string[1024];

// change stdin to non blocking
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);

	if (ttycon_on)
	{
		tty_Hide();
	}

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	fprintf(stderr, "Error: %s\n", string);

	Sys_ConsoleInputShutdown();
	Host_Shutdown();
	exit(1);

}

static void signal_handler(int sig, siginfo_t* info, void* secret)
{
	void* trace[64];
	char** messages = (char**)NULL;
	int i, trace_size = 0;

	/* Do something useful with siginfo_t */
#if id386
	ucontext_t* uc = (ucontext_t*)secret;
	if (sig == SIGSEGV)
	{
		printf("Received signal %d, faulty address is %p, "
			   "from %p\n", sig, info->si_addr,
			uc->uc_mcontext.gregs[REG_EIP]);
	}
	else
#endif
	printf("Received signal %d, exiting...\n", sig);

	trace_size = backtrace(trace, 64);
#if id386
	/* overwrite sigaction with caller's address */
	trace[1] = (void*)uc->uc_mcontext.gregs[REG_EIP];
#endif

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");
	for (i = 1; i < trace_size; ++i)
		printf("[bt] %s\n", messages[i]);

	Sys_Quit();
	exit(0);
}

void InitSig(void)
{
	struct sigaction sa;

	sa.sa_sigaction = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
	sigaction(SIGIOT, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

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
