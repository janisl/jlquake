//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef _SYSTEM_WINDOWS_H
#define _SYSTEM_WINDOWS_H

#include <windows.h>

//	Copied from resources.h
#define IDI_ICON1                       1

// returnbed by Sys_GetProcessorId
#define CPUID_GENERIC           0			// any unrecognized processor

#define CPUID_AXP               0x10

#define CPUID_INTEL_UNSUPPORTED 0x20			// Intel 386/486
#define CPUID_INTEL_PENTIUM     0x21			// Intel Pentium or PPro
#define CPUID_INTEL_MMX         0x22			// Intel Pentium/MMX or P2/MMX
#define CPUID_INTEL_KATMAI      0x23			// Intel Katmai

#define CPUID_AMD_3DNOW         0x30			// AMD K6 3DNOW!

extern HINSTANCE global_hInstance;
extern unsigned sysMsgTime;

void Sys_CreateConsole(const char* Title);
void Sys_DestroyConsole();
void Sys_SetErrorText(const char* text);
void Sys_ClearViewlog_f();

void SysT3_InitCpu();

#endif
