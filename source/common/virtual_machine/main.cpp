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

#include "../qcommon.h"
#include "local.h"

vm_t* currentVM = NULL;
int vm_debugLevel;

void VM_Debug(int level)
{
	vm_debugLevel = level;
}

//	Assumes a program counter value
const char* VM_ValueToSymbol(vm_t* vm, int value)
{
	static char text[MAX_TOKEN_CHARS_Q3];

	vmSymbol_t* sym = vm->symbols;
	if (!sym)
	{
		return "NO SYMBOLS";
	}

	// find the symbol
	while (sym->next && sym->next->symValue <= value)
	{
		sym = sym->next;
	}

	if (value == sym->symValue)
	{
		return sym->symName;
	}

	String::Sprintf(text, sizeof(text), "%s+%i", sym->symName, value - sym->symValue);

	return text;
}

//	For profiling, find the symbol behind this value
vmSymbol_t* VM_ValueToFunctionSymbol(vm_t* vm, int value)
{
	static vmSymbol_t nullSym;

	vmSymbol_t* sym = vm->symbols;
	if (!sym)
	{
		return &nullSym;
	}

	while (sym->next && sym->next->symValue <= value)
	{
		sym = sym->next;
	}

	return sym;
}

static int ParseHex(const char* text)
{
	int value = 0;
	int c;
	while ((c = *text++) != 0)
	{
		if (c >= '0' && c <= '9')
		{
			value = value * 16 + c - '0';
			continue;
		}
		if (c >= 'a' && c <= 'f')
		{
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if (c >= 'A' && c <= 'F')
		{
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}

void VM_LoadSymbols(vm_t* vm)
{
	// don't load symbols if not developer
	if (!com_developer->integer)
	{
		return;
	}

	char name[MAX_QPATH];
	String::StripExtension(vm->name, name);
	char symbols[MAX_QPATH];
	String::Sprintf(symbols, sizeof(symbols), "vm/%s.map", name);
	void* mapfile;
	FS_ReadFile(symbols, &mapfile);
	if (!mapfile)
	{
		Log::write("Couldn't load symbol file: %s\n", symbols);
		return;
	}

	int numInstructions = vm->instructionPointersLength >> 2;

	// parse the symbols
	const char* text_p = reinterpret_cast<char*>(mapfile);
	vmSymbol_t** prev = &vm->symbols;
	int count = 0;

	while (1)
	{
		char* token = String::Parse3(&text_p);
		if (!token[0])
		{
			break;
		}
		int segment = ParseHex(token);
		if (segment)
		{
			String::Parse3(&text_p);
			String::Parse3(&text_p);
			continue;		// only load code segment values
		}

		token = String::Parse3(&text_p);
		if (!token[0])
		{
			common->Printf("WARNING: incomplete line at end of file\n");
			break;
		}
		int value = ParseHex(token);

		token = String::Parse3(&text_p);
		if (!token[0])
		{
			common->Printf("WARNING: incomplete line at end of file\n");
			break;
		}
		int chars = String::Length(token);
		vmSymbol_t* sym = (vmSymbol_t*)Mem_Alloc(sizeof(*sym) + chars);
		*prev = sym;
		prev = &sym->next;
		sym->next = NULL;

		// convert value from an instruction number to a code offset
		if (value >= 0 && value < numInstructions)
		{
			value = vm->instructionPointers[value];
		}

		sym->symValue = value;
		sym->profileCount = 0;
		String::NCpyZ(sym->symName, token, chars + 1);

		count++;
	}

	vm->numSymbols = count;
	common->Printf("%i symbols parsed from %s\n", count, symbols);
	FS_FreeFile(mapfile);
}

//	Insert calls to this while debugging the vm compiler
void VM_LogSyscalls(qintptr* args)
{
	static int callnum;
	static FILE* f;

	if (!f)
	{
		f = fopen("syscalls.log", "w");
	}
	callnum++;
	fprintf(f, "%i: %i (%i) = %i %i %i %i\n", callnum, (int)((int*)args - (int*)currentVM->dataBase),
		(int)args[0], (int)args[1], (int)args[2], (int)args[3], (int)args[4]);
}

void* VM_ArgPtr(qintptr intValue)
{
	if (!intValue)
	{
		return NULL;
	}
	// bk001220 - currentVM is missing on reconnect
	if (currentVM == NULL)
	{
		return NULL;
	}

	if (currentVM->entryPoint)
	{
		return (void*)(currentVM->dataBase + intValue);
	}
	else
	{
		return (void*)(currentVM->dataBase + (intValue & currentVM->dataMask));
	}
}

void* VM_ExplicitArgPtr(vm_t* vm, qintptr intValue)
{
	if (!intValue)
	{
		return NULL;
	}

	// bk010124 - currentVM is missing on reconnect here as well?
	if (currentVM == NULL)
	{
		return NULL;
	}

	if (vm->entryPoint)
	{
		return (void*)(vm->dataBase + intValue);
	}
	else
	{
		return (void*)(vm->dataBase + (intValue & vm->dataMask));
	}
}

/*
Used to load a development dll instead of a virtual machine

NOTE: DLL checksuming / DLL pk3 and search paths

if we are connecting to an sv_pure server, the .so will be extracted
from a pk3 and placed in the homepath (i.e. the write area)

since DLLs are looked for in several places, it is important that
we are sure we are going to load the DLL that was just extracted

in release we start searching directly in fs_homepath then fs_basepath
(that makes sure we are going to load the DLL we extracted from pk3)
this should not be a problem for mod makers, since they always copy their
DLL to the main installation prior to run (sv_pure 1 would have a tendency
to kill their compiled DLL with extracted ones though :-( )
*/
void* VM_LoadDll(const char* name, qintptr(**entryPoint) (int, ...),
	qintptr (* systemcalls)(int, ...))
{
	qassert(name);

	if (GGameType & GAME_Quake3 && !Sys_Quake3DllWarningConfirmation())
	{
		return NULL;
	}

	char fname[MAX_OSPATH];
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		String::NCpyZ(fname, Sys_GetMPDllName(name), sizeof(fname));
	}
	else
	{
		String::NCpyZ(fname, Sys_GetDllName(name), sizeof(fname));
	}

	const char* homepath = Cvar_VariableString("fs_homepath");
	const char* basepath = Cvar_VariableString("fs_basepath");
	const char* gamedir = Cvar_VariableString("fs_game");

	void* libHandle = NULL;
#ifdef _WIN32
	if (GGameType & GAME_WolfSP)
	{
		// On Windows instalations DLLs are in the same directory as exe file.
		libHandle = Sys_LoadDll(filename);
	}
#endif

	if (!libHandle)
	{
		// try homepath first
		char* fn = FS_BuildOSPath(homepath, gamedir, fname);

		// NERVE - SMF - extract dlls from pak file for security
		// we have to handle the game dll a little differently
		// TTimo - passing the exact path to check against
		//   (compatibility with other OSes loading procedure)
		if (GGameType & (GAME_WolfMP | GAME_ET) && cl_connectedToPureServer && String::NCmp(name, "qagame", 6))
		{
			if (!FS_CL_ExtractFromPakFile(fn, gamedir, fname))
			{
				common->Error("Game code(%s) failed Pure Server check", fname);
			}
		}

		libHandle = Sys_LoadDll(fn);

		if (!libHandle)
		{
			fn = FS_BuildOSPath(basepath, gamedir, fname);
			libHandle = Sys_LoadDll(fn);

			if (!libHandle && String::Length(gamedir) && String::ICmp(gamedir, fs_PrimaryBaseGame))
			{
				// First try falling back to BASEGAME
				fn = FS_BuildOSPath(homepath, fs_PrimaryBaseGame, fname);
				libHandle = Sys_LoadDll(fn);

				if (!libHandle)
				{
					fn = FS_BuildOSPath(basepath, fs_PrimaryBaseGame, fname);
					libHandle = Sys_LoadDll(fn);
				}
			}

			if (!libHandle)
			{
				return NULL;
			}
		}
	}

	void (* dllEntry)(qintptr (* syscallptr)(int, ...)) =
		(void (*)(qintptr (*)(int, ...)))Sys_GetDllFunction(libHandle, "dllEntry");
	*entryPoint = (qintptr (*)(int, ...))Sys_GetDllFunction(libHandle, "vmMain");
	if (!*entryPoint || !dllEntry)
	{
		Sys_UnloadDll(libHandle);
		return NULL;
	}
	dllEntry(systemcalls);

	return libHandle;
}
