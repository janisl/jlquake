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

#include "qcommon.h"
#include "system_windows.h"
#include "../client/public.h"
#include <direct.h>
#include <io.h>
#include <sys/stat.h>

#define MAX_FOUND_FILES     0x1000

// returnbed by Sys_GetProcessorId
#define CPUID_GENERIC           0			// any unrecognized processor

#define CPUID_AXP               0x10

#define CPUID_INTEL_UNSUPPORTED 0x20			// Intel 386/486
#define CPUID_INTEL_PENTIUM     0x21			// Intel Pentium or PPro
#define CPUID_INTEL_MMX         0x22			// Intel Pentium/MMX or P2/MMX
#define CPUID_INTEL_KATMAI      0x23			// Intel Katmai

#define CPUID_AMD_3DNOW         0x30			// AMD K6 3DNOW!

HINSTANCE global_hInstance;
// when we get a windows message, we store the time off so keyboard processing
// can know the exact time of an event
unsigned sysMsgTime;
bool Minimized;

static char HomePathSuffix[MAX_OSPATH];

static double lastcurtime = 0.0;
static double curtime = 0.0;
static int lowshift;
static double pfreq;

static bool sys_timeInitialised;
static int sys_timeBase;

//	Test an file given OS path:
//	returns -1 if not found
//	returns 1 if directory
//	returns 0 otherwise
int Sys_StatFile(const char* ospath)
{
	struct _stat stat;
	if (_stat(ospath, &stat) == -1)
	{
		return -1;
	}
	if (stat.st_mode & _S_IFDIR)
	{
		return 1;
	}
	return 0;
}

void Sys_Mkdir(const char* path)
{
	_mkdir(path);
}

int Sys_Rmdir(const char* path)
{
	return _rmdir(path);
}

const char* Sys_Cwd()
{
	static char cwd[MAX_OSPATH];

	_getcwd(cwd, sizeof(cwd) - 1);
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

void Sys_SetHomePathSuffix(const char* Name)
{
	String::NCpyZ(HomePathSuffix, Name, sizeof(HomePathSuffix));
}

const char* Sys_DefaultHomePath()
{
	return NULL;
}

static void Sys_ListFilteredFiles(const char* basedir, const char* subdirs, const char* filter,
	char** list, int* numfiles)
{
	char search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char filename[MAX_OSPATH];
	int findhandle;
	struct _finddata_t findinfo;

	if (*numfiles >= MAX_FOUND_FILES - 1)
	{
		return;
	}

	if (String::Length(subdirs))
	{
		String::Sprintf(search, sizeof(search), "%s\\%s\\*", basedir, subdirs);
	}
	else
	{
		String::Sprintf(search, sizeof(search), "%s\\*", basedir);
	}

	findhandle = _findfirst(search, &findinfo);
	if (findhandle == -1)
	{
		return;
	}

	do
	{
		if (findinfo.attrib & _A_SUBDIR)
		{
			if (String::ICmp(findinfo.name, ".") && String::ICmp(findinfo.name, ".."))
			{
				if (String::Length(subdirs))
				{
					String::Sprintf(newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else
				{
					String::Sprintf(newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
			}
		}
		if (*numfiles >= MAX_FOUND_FILES - 1)
		{
			break;
		}
		String::Sprintf(filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name);
		if (!String::FilterPath(filter, filename, false))
		{
			continue;
		}
		list[*numfiles] = __CopyString(filename);
		(*numfiles)++;
	}
	while (_findnext(findhandle, &findinfo) != -1);

	_findclose(findhandle);
}

static bool strgtr(const char* s0, const char* s1)
{
	int l0 = String::Length(s0);
	int l1 = String::Length(s1);

	if (l1 < l0)
	{
		l0 = l1;
	}

	for (int i = 0; i < l0; i++)
	{
		if (s1[i] > s0[i])
		{
			return true;
		}
		if (s1[i] < s0[i])
		{
			return false;
		}
	}
	return false;
}

char** Sys_ListFiles(const char* directory, const char* extension, const char* filter,
	int* numfiles, bool wantsubs)
{
	char search[MAX_OSPATH];
	int nfiles;
	char** listCopy;
	char* list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	int findhandle;
	int flag;
	int i;

	if (filter)
	{
		nfiles = 0;
		Sys_ListFilteredFiles(directory, "", filter, list, &nfiles);

		list[nfiles] = 0;
		*numfiles = nfiles;

		if (!nfiles)
		{
			return NULL;
		}

		listCopy = (char**)Mem_Alloc((nfiles + 1) * sizeof(*listCopy));
		for (i = 0; i < nfiles; i++)
		{
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if (!extension)
	{
		extension = "";
	}

	// passing a slash as extension will find directories
	if (extension[0] == '/' && extension[1] == 0)
	{
		extension = "";
		flag = 0;
	}
	else
	{
		flag = _A_SUBDIR;
	}

	String::Sprintf(search, sizeof(search), "%s\\*%s", directory, extension);

	// search
	nfiles = 0;

	findhandle = _findfirst(search, &findinfo);
	if (findhandle == -1)
	{
		*numfiles = 0;
		return NULL;
	}

	do
	{
		if ((!wantsubs && flag ^ (findinfo.attrib & _A_SUBDIR)) || (wantsubs && findinfo.attrib & _A_SUBDIR))
		{
			if (nfiles == MAX_FOUND_FILES - 1)
			{
				break;
			}
			list[nfiles] = __CopyString(findinfo.name);
			nfiles++;
		}
	}
	while (_findnext(findhandle, &findinfo) != -1);

	list[nfiles] = 0;

	_findclose(findhandle);

	// return a copy of the list
	*numfiles = nfiles;

	if (!nfiles)
	{
		return NULL;
	}

	listCopy = (char**)Mem_Alloc((nfiles + 1) * sizeof(*listCopy));
	for (i = 0; i < nfiles; i++)
	{
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	do
	{
		flag = 0;
		for (i = 1; i < nfiles; i++)
		{
			if (strgtr(listCopy[i - 1], listCopy[i]))
			{
				char* temp = listCopy[i];
				listCopy[i] = listCopy[i - 1];
				listCopy[i - 1] = temp;
				flag = 1;
			}
		}
	}
	while (flag);

	return listCopy;
}

void Sys_FreeFileList(char** list)
{
	if (!list)
	{
		return;
	}

	for (int i = 0; list[i]; i++)
	{
		Mem_Free(list[i]);
	}

	Mem_Free(list);
}

int Sys_Milliseconds()
{
	if (!sys_timeInitialised)
	{
		sys_timeBase = timeGetTime();
		sys_timeInitialised = true;
	}
	return timeGetTime() - sys_timeBase;
}

double Sys_DoubleTime()
{
	DWORD now = timeGetTime();

	if (!sys_timeInitialised)
	{
		sys_timeInitialised = true;
		sys_timeBase = now;
		return 0.0;
	}

	if (now < (DWORD)sys_timeBase)// wrapped?
	{
		return (now / 1000.0) + (LONG_MAX - (DWORD)sys_timeBase / 1000.0);
	}

	if (now - (DWORD)sys_timeBase == 0)
	{
		return 0.0;
	}

	return (now - (DWORD)sys_timeBase) / 1000.0;
}

bool Sys_Quake3DllWarningConfirmation()
{
#ifdef NDEBUG
	static int lastWarning = 0;
	int timestamp = Sys_Milliseconds();
	if (((timestamp - lastWarning) > (5 * 60000)) && !Cvar_VariableIntegerValue("dedicated") &&
		!Cvar_VariableIntegerValue("com_blindlyLoadDLLs"))
	{
		if (FS_FileExists(filename))
		{
			lastWarning = timestamp;
			int ret = MessageBoxEx(NULL, "You are about to load a .DLL executable that\n"
									 "has not been verified for use with Quake III Arena.\n"
									 "This type of file can compromise the security of\n"
									 "your computer.\n\n"
									 "Select 'OK' if you choose to load it anyway.",
				"Security Warning", MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
			if (ret != IDOK)
			{
				return false;
			}
		}
	}
#endif
	return true;
}

static char* Sys_GetDllNameImpl(const char* name, const char* suffix)
{
#if defined _M_IX86
	return va("%s%sx86.dll", name, suffix);
#elif defined _M_X64
	return va("%s%sx86_64.dll", name, suffix);
#elif defined _M_ALPHA
	return va("%s%saxp.dll", name, suffix);
#else
#error "Unknown arch"
#endif
}

char* Sys_GetDllName(const char* name)
{
	return Sys_GetDllNameImpl(name, "");
}

char* Sys_GetMPDllName(const char* name)
{
	return Sys_GetDllNameImpl(name, "_mp_");
}

void* Sys_LoadDll(const char* name)
{
	void* handle = LoadLibrary(name);
	if (!handle)
	{
		common->Printf("Sys_LoadDll(%s) failed.\n", name);
	}
	return handle;
}

void* Sys_GetDllFunction(void* handle, const char* name)
{
	void* func = GetProcAddress((HMODULE)handle, name);
	if (!func)
	{
		common->Printf("Sys_GetDllFunction: %s not found\n", name);
	}
	return func;
}

void Sys_UnloadDll(void* handle)
{
	if (!handle)
	{
		return;
	}
	if (!FreeLibrary((HMODULE)handle))
	{
		common->FatalError("Sys_UnloadDll FreeLibrary failed");
	}
}

void Sys_MessageLoop()
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		sysMsgTime = msg.time;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void Sys_Quit()
{
	timeEndPeriod(1);
	Sys_DestroyConsole();
	exit(0);
}

//	Show the early console as an error dialog
void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char text[4096];

	va_start(argptr, error);
	Q_vsnprintf(text, 4096, error, argptr);
	va_end(argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

	timeEndPeriod(1);

	CL_ShutdownOnWindowsError();

	// wait for the user to quit
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Sys_DestroyConsole();
	exit(1);
}

const char* Sys_GetCurrentUser()
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

#ifndef _WIN64
#ifndef __GNUC__

//	Disable all optimizations temporarily so this code works correctly!
#pragma optimize( "", off )

static bool IsPentium()
{
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
}

static void CPUID(int func, unsigned regs[4])
{
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
}

//	Re-enable optimizations back to what they were
#pragma optimize( "", on )

#else

static bool IsPentium()
{
	return true;
}

static void CPUID(int func, unsigned regs[4])
{
	// rain - gcc style inline asm
	asm (
		"cpuid\n"
		: "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])		// outputs
		: "a" (func)	// inputs
		);
}

#endif

static bool Is3DNOW()
{
	unsigned regs[4];

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

static bool IsKNI()
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

static bool IsMMX()
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

static int Sys_GetProcessorId()
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

static void SysT3_InitCpu()
{
	//
	// figure out our CPU
	//
	Cvar_Get("sys_cpustring", "detect", 0);
	int cpuid;
	if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "detect"))
	{
		common->Printf("...detecting CPU, found ");

		cpuid = Sys_GetProcessorId();

		switch (cpuid)
		{
		case CPUID_GENERIC:
			Cvar_Set("sys_cpustring", "generic");
			break;
		case CPUID_INTEL_UNSUPPORTED:
			Cvar_Set("sys_cpustring", "x86 (pre-Pentium)");
			break;
		case CPUID_INTEL_PENTIUM:
			Cvar_Set("sys_cpustring", "x86 (P5/PPro, non-MMX)");
			break;
		case CPUID_INTEL_MMX:
			Cvar_Set("sys_cpustring", "x86 (P5/Pentium2, MMX)");
			break;
		case CPUID_INTEL_KATMAI:
			Cvar_Set("sys_cpustring", "Intel Pentium III");
			break;
		case CPUID_AMD_3DNOW:
			Cvar_Set("sys_cpustring", "AMD w/ 3DNow!");
			break;
		case CPUID_AXP:
			Cvar_Set("sys_cpustring", "Alpha AXP");
			break;
		default:
			common->FatalError("Unknown cpu type %d\n", cpuid);
			break;
		}
	}
	else
	{
		common->Printf("...forcing CPU type to ");
		if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "generic"))
		{
			cpuid = CPUID_GENERIC;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "x87"))
		{
			cpuid = CPUID_INTEL_PENTIUM;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "mmx"))
		{
			cpuid = CPUID_INTEL_MMX;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "3dnow"))
		{
			cpuid = CPUID_AMD_3DNOW;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "PentiumIII"))
		{
			cpuid = CPUID_INTEL_KATMAI;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "axp"))
		{
			cpuid = CPUID_AXP;
		}
		else
		{
			common->Printf("WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString("sys_cpustring"));
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue("sys_cpuid", cpuid);
	common->Printf("%s\n", Cvar_VariableString("sys_cpustring"));
}

//	Called after the common systems (cvars, files, etc)
// are initialized
void Sys_Init()
{
	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod(1);

	OSVERSIONINFO osversion;
	osversion.dwOSVersionInfoSize = sizeof(osversion);

	if (!GetVersionEx(&osversion))
	{
		Sys_Error("Couldn't get OS info");
	}

	if (osversion.dwMajorVersion < 5)
	{
		Sys_Error("JLQuake requires Windows version 4 or greater");
	}

	if (GGameType & GAME_Tech3)
	{
		Cvar_Set("arch", "winnt");

		SysT3_InitCpu();

		Cvar_Set("username", Sys_GetCurrentUser());

		Cmd_AddCommand("net_restart", Net_Restart_f);
	}
	Cmd_AddCommand("clearviewlog", Sys_ClearViewlog_f);
}

void Sys_Sleep(int msec)
{
	Sleep(msec);
}

int Sys_GetCurrentProcessId()
{
	return GetCurrentProcessId();
}
