/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
** GLimp_SetGamma
**
*/

#include <signal.h>
#include <execinfo.h>

#include "../../client/client.h"
#include "../../client/renderer/local.h"
#include "../../common/system_unix.h"
#include "linux_local.h"// bk001130

static qboolean signalcaught = false;;

static void signal_handler(int sig, siginfo_t* info, void* secret)	// bk010104 - replace this... (NOTE TTimo huh?)
{
	void* trace[64];
	char** messages = (char**)NULL;
	int i, trace_size = 0;
	if (signalcaught)
	{
		printf("DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n", sig);
		Sys_Exit(1);	// bk010104 - abstraction
	}

	signalcaught = true;
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

	GLimp_Shutdown();	// bk010104 - shouldn't this be CL_Shutdown
	Sys_Exit(0);	// bk010104 - abstraction NOTE TTimo send a 0 to avoid DOUBLE SIGNAL FAULT
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
