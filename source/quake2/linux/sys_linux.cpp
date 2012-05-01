#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <execinfo.h>

#include "../qcommon/qcommon.h"

#include "../../common/system_unix.h"

Cvar* nostdout;

void Sys_Quit(void)
{
	Sys_ConsoleInputShutdown();
	CL_Shutdown();
	Qcommon_Shutdown();
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);
	_exit(0);
}

void Sys_Init(void)
{
#if id386
//	Sys_SetFPCW();
#endif
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

	CL_Shutdown();
	Qcommon_Shutdown();

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);
	fprintf(stderr, "Error: %s\n", string);

	Sys_ConsoleInputShutdown();
	_exit(1);

}

void Sys_AppActivate(void)
{
}

/*****************************************************************************/

static void signal_handler(int sig, siginfo_t* info, void* secret)
{
	void* trace[64];
	char** messages = (char**)NULL;
	int i, trace_size = 0;

#if id386
	/* Do something useful with siginfo_t */
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

static void InitSig()
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

int main(int argc, char** argv)
{
	int time, oldtime, newtime;

	InitSig();

	// go back to real user for config loads
	seteuid(getuid());

	Qcommon_Init(argc, argv);

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
