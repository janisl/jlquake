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

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#ifndef DEDICATED
#include "../renderer/tr_local.h"
#endif

static qboolean signalcaught = qfalse;;

void Sys_Exit(int); // bk010104 - abstraction

#define MAX_STACK_ADDR 40

// __builtin_return_address needs a constant, so this cannot be in a loop

#define handle_stack_address(X) \
	if (continue_stack_trace && ((unsigned long)__builtin_frame_address((X)) != 0L) && ((X) < MAX_STACK_ADDR)) \
	{ \
		stack_addr[(X)]= __builtin_return_address((X)); \
		printf("stack %d %8p frame %d %8p\n", \
			(X), __builtin_return_address((X)), (X), __builtin_frame_address((X))); \
	} \
	else if (continue_stack_trace) \
	{ \
		continue_stack_trace = false; \
	}

static void stack_trace()
{
	FILE			*fff;
	int				i;
	static void*	stack_addr[MAX_STACK_ADDR];
	// can we still print entries on the calling stack or have we finished?
	static bool		continue_stack_trace = true;

	// clean the stack addresses if necessary
	for (i = 0; i < MAX_STACK_ADDR; i++)
	{
		stack_addr[i] = 0;
	}

	printf("STACK TRACE:\n\n");

	handle_stack_address(0);
	handle_stack_address(1);
	handle_stack_address(2);
	handle_stack_address(3);
	handle_stack_address(4);
	handle_stack_address(5);
	handle_stack_address(6);
	handle_stack_address(7);
	handle_stack_address(8);
	handle_stack_address(9);
	handle_stack_address(10);
	handle_stack_address(11);
	handle_stack_address(12);
	handle_stack_address(13);
	handle_stack_address(14);
	handle_stack_address(15);
	handle_stack_address(16);
	handle_stack_address(17);
	handle_stack_address(18);
	handle_stack_address(19);
	handle_stack_address(20);
	handle_stack_address(21);
	handle_stack_address(22);
	handle_stack_address(23);
	handle_stack_address(24);
	handle_stack_address(25);
	handle_stack_address(26);
	handle_stack_address(27);
	handle_stack_address(28);
	handle_stack_address(29);
	handle_stack_address(30);
	handle_stack_address(31);
	handle_stack_address(32);
	handle_stack_address(33);
	handle_stack_address(34);
	handle_stack_address(35);
	handle_stack_address(36);
	handle_stack_address(37);
	handle_stack_address(38);
	handle_stack_address(39);

	// Give a warning
	//fprintf(stderr, "You suddenly see a gruesome SOFTWARE BUG leap for your throat!\n");

	// Open the non-existing file
	fff = fopen("crash.txt", "w");

	// Invalid file
	if (fff)
	{
		// dump stack frame
		for (i = (MAX_STACK_ADDR - 1); i >= 0 ; i--)
		{
			fprintf(fff,"%8p\n", stack_addr[i]);
		}
		fclose(fff);
	}
}

static void signal_handler(int sig) // bk010104 - replace this... (NOTE TTimo huh?)
{
  if (signalcaught)
  {
    printf("DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n", sig);
    Sys_Exit(1); // bk010104 - abstraction
  }

//stack_trace();
  signalcaught = qtrue;
  printf("Received signal %d, exiting...\n", sig);
#ifndef DEDICATED
  GLimp_Shutdown(); // bk010104 - shouldn't this be CL_Shutdown
#endif
  Sys_Exit(0); // bk010104 - abstraction NOTE TTimo send a 0 to avoid DOUBLE SIGNAL FAULT
}

void InitSig(void)
{
  signal(SIGHUP, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGILL, signal_handler);
  signal(SIGTRAP, signal_handler);
  signal(SIGIOT, signal_handler);
  signal(SIGBUS, signal_handler);
  signal(SIGFPE, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGTERM, signal_handler);
}
