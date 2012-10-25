/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <math.h>

/*
**
** Disable all optimizations temporarily so this code works correctly!
**
*/
#ifdef MSVC	// rain - MSVC pragma
#pragma optimize( "", off )
#endif

/*
** --------------------------------------------------------------------------------
**
** PROCESSOR STUFF
**
** --------------------------------------------------------------------------------
*/
#ifndef _WIN64
static void CPUID(int func, unsigned regs[4])
{
#ifndef __GNUC__
	unsigned regEAX, regEBX, regECX, regEDX;

	__asm mov eax, func
	__asm cpuid
	__asm mov regEAX, eax
	__asm mov regEBX, ebx
	__asm mov regECX, ecx
	__asm mov regEDX, edx

			  regs[0] = regEAX;
	regs[1] = regEBX;
	regs[2] = regECX;
	regs[3] = regEDX;
#else
	// rain - gcc style inline asm
	asm (
		"cpuid\n"
		: "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])		// outputs
		: "a" (func)	// inputs
		);
#endif

}

static int IsPentium(void)
{
#ifndef __GNUC__
	__asm
	{
		pushfd						// save eflags
		pop eax
		test eax, 0x00200000		// check ID bit
		jz set21					// bit 21 is not set, so jump to set_21
		and eax, 0xffdfffff			// clear bit 21
		push eax					// save new value in register
		popfd						// store new value in flags
		pushfd
		pop eax
		test eax, 0x00200000		// check ID bit
		jz good
		jmp err						// cpuid not supported
set21:
		or eax, 0x00200000			// set ID bit
		push eax					// store new value
		popfd						// store new value in EFLAGS
		pushfd
		pop eax
		test eax, 0x00200000		// if bit 21 is on
		jnz good
		jmp err
	}

err:
	return false;
good:
	return true;
#else
	// rain - gcc-style inline asm
	// rain - FIXME: I'm too tired to do this one right now
	return true;
#endif
}

static int Is3DNOW(void)
{
	unsigned regs[4];
	char pstring[16];
	char processorString[13];

	// get name of processor
	CPUID(0, (unsigned int*)pstring);
	processorString[0] = pstring[4];
	processorString[1] = pstring[5];
	processorString[2] = pstring[6];
	processorString[3] = pstring[7];
	processorString[4] = pstring[12];
	processorString[5] = pstring[13];
	processorString[6] = pstring[14];
	processorString[7] = pstring[15];
	processorString[8] = pstring[8];
	processorString[9] = pstring[9];
	processorString[10] = pstring[10];
	processorString[11] = pstring[11];
	processorString[12] = 0;

//  REMOVED because you can have 3DNow! on non-AMD systems
//	if ( String::Cmp( processorString, "AuthenticAMD" ) )
//		return false;

	// check AMD-specific functions
	CPUID(0x80000000, regs);
	if (regs[0] < 0x80000000)
	{
		return false;
	}

	// bit 31 of EDX denotes 3DNOW! support
	CPUID(0x80000001, regs);
	if (regs[3] & (1 << 31))
	{
		return true;
	}

	return false;
}

static int IsKNI(void)
{
	unsigned regs[4];

	// get CPU feature bits
	CPUID(1, regs);

	// bit 25 of EDX denotes KNI existence
	if (regs[3] & (1 << 25))
	{
		return true;
	}

	return false;
}

static int IsMMX(void)
{
	unsigned regs[4];

	// get CPU feature bits
	CPUID(1, regs);

	// bit 23 of EDX denotes MMX existence
	if (regs[3] & (1 << 23))
	{
		return true;
	}
	return false;
}
#endif

int Sys_GetProcessorId(void)
{
#if defined _M_ALPHA
	return CPUID_AXP;
#elif defined _M_X64
	return CPUID_INTEL_KATMAI;
#elif !defined _M_IX86
	return CPUID_GENERIC;
#else

	// verify we're at least a Pentium or 486 w/ CPUID support
	if (!IsPentium())
	{
		return CPUID_INTEL_UNSUPPORTED;
	}

	// check for MMX
	if (!IsMMX())
	{
		// Pentium or PPro
		return CPUID_INTEL_PENTIUM;
	}

	// see if we're an AMD 3DNOW! processor
	if (Is3DNOW())
	{
		return CPUID_AMD_3DNOW;
	}

	// see if we're an Intel Katmai
	if (IsKNI())
	{
		return CPUID_INTEL_KATMAI;
	}

	// by default we're functionally a vanilla Pentium/MMX or P2/MMX
	return CPUID_INTEL_MMX;

#endif
}

float Sys_RealGetCPUSpeed(void)
{
	LARGE_INTEGER c0, c1, freq;
	unsigned int stamp0, stamp1, cycles, ticks, tries;
	unsigned int freq1, freq2, freq3, total;
	unsigned int priorityClass;
	int threadPriority;

	freq1 = freq2 = freq3 = tries = 0;

	if (!QueryPerformanceFrequency(&freq))
	{
		return .0f;
	}

	priorityClass = GetPriorityClass(GetCurrentProcess());
	threadPriority = GetThreadPriority(GetCurrentThread());

	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	do
	{
		tries++;

		freq3 = freq2;
		freq2 = freq1;

		QueryPerformanceCounter(&c0);

		c1.LowPart = c0.LowPart;
		c1.HighPart = c0.HighPart;

		while ((int)c1.LowPart - (int)c0.LowPart < 50)
		{
			QueryPerformanceCounter(&c1);
		}

#ifdef _WIN64
		stamp0 = __rdtsc();
#elif !defined __GNUC__
		_asm
		{
			rdtsc
			mov stamp0, eax
		}
#else
		// rain - gcc-style inline asm
		asm (
			"rdtsc\n"
			: "=a" (stamp0)		// outputs
			:
			: "edx"
			);
#endif

		c0.LowPart = c1.LowPart;
		c0.HighPart = c1.HighPart;

		while ((int)c1.LowPart - (int)c0.LowPart < 1000)
		{
			QueryPerformanceCounter(&c1);
		}

#ifdef _WIN64
		stamp1 = __rdtsc();
#elif !defined __GNUC__
		_asm
		{
			rdtsc
			mov stamp1, eax
		}
#else
		// rain - gcc-style inline asm
		asm (
			"rdtsc\n"
			: "=a" (stamp1)		// outputs
			:
			: "edx"
			);
#endif

		cycles = stamp1 - stamp0;
		ticks = (int)c1.LowPart - (int)c0.LowPart;

		ticks *= 100000;
		ticks /= (freq.LowPart / 10);

		if (ticks % freq.LowPart > freq.LowPart * .5f)
		{
			ticks++;
		}

		freq1 = cycles / ticks;

		if (cycles % ticks > ticks * .5f)
		{
			freq1++;
		}

		total = freq1 + freq2 + freq3;

	}
	while ((tries < 3 || tries < 20) &&
		   ((abs((int)(3 * freq1 - total)) > 3) || (abs((int)(3 * freq2 - total)) > 3) || (abs((int)(3 * freq3 - total)) > 3)));

	if (total / 3 != (total + 1) / 3)
	{
		total++;
	}

	SetPriorityClass(GetCurrentProcess(), priorityClass);
	SetThreadPriority(GetCurrentThread(), threadPriority);

	return((float)total / 3.f);
}

/*
**
** Re-enable optimizations back to what they were
**
*/
#ifdef MSVC	// rain - MSVC pragma
#pragma optimize( "", on )
#endif

//============================================

const char* Sys_GetCurrentUser(void)
{
	static char s_userName[1024];
	unsigned long size = sizeof(s_userName);


	if (!GetUserName(s_userName, &size))
	{
		String::Cpy(s_userName, "player");
	}

	if (!s_userName[0])
	{
		String::Cpy(s_userName, "player");
	}

	return s_userName;
}
