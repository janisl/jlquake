/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include <signal.h>
#include <execinfo.h>

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#ifndef DEDICATED
#include "../../client/client.h"
#include "../../client/renderer/local.h"
#endif

static qboolean signalcaught = qfalse;;

void Sys_Exit(int); // bk010104 - abstraction

static void signal_handler(int sig, siginfo_t *info, void *secret) // bk010104 - replace this... (NOTE TTimo huh?)
{
	void *trace[64];
	char **messages = (char **)NULL;
	int i, trace_size = 0;

#if id386
	/* Do something useful with siginfo_t */
	ucontext_t* uc = (ucontext_t*)secret;
	if (sig == SIGSEGV)
		printf("Received signal %d, faulty address is %p, "
			"from %p\n", sig, info->si_addr, 
			uc->uc_mcontext.gregs[REG_EIP]);
	else
#endif
		printf("Received signal %d, exiting...\n", sig);
		
	trace_size = backtrace(trace, 64);
#if id386
	/* overwrite sigaction with caller's address */
	trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
#endif

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");
	for (i=1; i<trace_size; ++i)
		printf("[bt] %s\n", messages[i]);

  if (signalcaught)
  {
    printf("DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n", sig);
    Sys_Exit(1); // bk010104 - abstraction
  }

  signalcaught = qtrue;
  printf("Received signal %d, exiting...\n", sig);
#ifndef DEDICATED
  GLimp_Shutdown(); // bk010104 - shouldn't this be CL_Shutdown
#endif
  Sys_Exit(0); // bk010104 - abstraction NOTE TTimo send a 0 to avoid DOUBLE SIGNAL FAULT
}

void InitSig(void)
{
	struct sigaction sa;

	sa.sa_sigaction = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
	sigaction(SIGIOT, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}
