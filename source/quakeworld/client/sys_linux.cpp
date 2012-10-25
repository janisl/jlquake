/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <execinfo.h>

#include "quakedef.h"
#include "../../common/system_unix.h"

int noconinput = 0;
int nostdout = 0;

const char* basedir = ".";

// =======================================================================
// General routines
// =======================================================================

void Sys_Init(void)
{
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

	ComQH_HostShutdown();
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~FNDELAY);
	Sys_Exit(0);
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

	noconinput = COM_CheckParm("-noconinput");
	if (!noconinput)
	{
		fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | FNDELAY);
	}

	if (COM_CheckParm("-nostdout"))
	{
		nostdout = 1;
	}

	Sys_Init();

	Host_Init(&parms);

	Sys_ConsoleInputInit();

	oldtime = Sys_DoubleTime();
	while (1)
	{
// find time spent rendering last frame
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;

		Host_Frame(time);
		oldtime = newtime;
	}

}
