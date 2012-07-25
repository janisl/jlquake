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

#define MAX_VM      3

#define STACK_SIZE  0x20000

vm_t* currentVM = NULL;
static vm_t* lastVM = NULL;
int vm_debugLevel;

static vm_t vmTable[MAX_VM];

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

static void VM_LoadSymbols(vm_t* vm)
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
		common->Printf("Couldn't load symbol file: %s\n", symbols);
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
static void* VM_LoadDll(const char* name, qintptr(**entryPoint) (int, ...),
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
		libHandle = Sys_LoadDll(fname);
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

/*
Dlls will call this directly

 rcg010206 The horror; the horror.

  The syscall mechanism relies on stack manipulation to get it's args.
   This is likely due to C's inability to pass "..." parameters to
   a function in one clean chunk. On PowerPC Linux, these parameters
   are not necessarily passed on the stack, so while (&arg[0] == arg)
   is true, (&arg[1] == 2nd function parameter) is not necessarily
   accurate, as arg's value might have been stored to the stack or
   other piece of scratch memory to give it a valid address, but the
   next parameter might still be sitting in a register.

  Quake's syscall system also assumes that the stack grows downward,
   and that any needed types can be squeezed, safely, into a signed int.

  This hack below copies all needed values for an argument to a
   array in memory, so that Quake can get the correct values. This can
   also be used on systems where the stack grows upwards, as the
   presumably standard and safe stdargs.h macros are used.

  The original code, while probably still inherently dangerous, seems
   to work well enough for the platforms it already works on. Rather
   than add the performance hit for those platforms, the original code
   is still in use there.

  For speed, we just grab 15 arguments, and don't worry about exactly
   how many the syscall actually needs; the extra is thrown away.
*/
static qintptr VM_DllSyscall(int arg, ...)
{
#if id386
	return currentVM->systemCall(&arg);
#else
	qintptr args[16];
	args[0] = arg;

	va_list ap;
	va_start(ap, arg);
	for (int i = 1; i < 16; i++)
	{
		args[i] = va_arg(ap, qintptr);
	}
	va_end(ap);

	return currentVM->systemCall(args);
#endif
}

//	If image ends in .qvm it will be interpreted, otherwise
// it will attempt to load as a system dll
vm_t* VM_Create(const char* module, qintptr (* systemCalls)(qintptr*),
	vmInterpret_t interpret)
{
	vm_t* vm;
	vmHeader_t* header;
	int length;
	int dataLength;
	int i;
	char filename[MAX_QPATH];

	if (!module || !module[0] || !systemCalls)
	{
		common->FatalError("VM_Create: bad parms");
	}

	// see if we already have the VM
	for (i = 0; i < MAX_VM; i++)
	{
		if (!String::ICmp(vmTable[i].name, module))
		{
			vm = &vmTable[i];
			return vm;
		}
	}

	// find a free vm
	for (i = 0; i < MAX_VM; i++)
	{
		if (!vmTable[i].name[0])
		{
			break;
		}
	}

	if (i == MAX_VM)
	{
		common->FatalError("VM_Create: no free vm_t");
	}

	vm = &vmTable[i];

	String::NCpyZ(vm->name, module, sizeof(vm->name));
	vm->systemCall = systemCalls;

	if (interpret == VMI_NATIVE)
	{
		// try to load as a system dll
		common->Printf("Loading dll file %s.\n", vm->name);
		vm->dllHandle = VM_LoadDll(module, &vm->entryPoint, VM_DllSyscall);
		if (vm->dllHandle)
		{
			return vm;
		}
		if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
		{
			//	Wolf games don't have QVMs.
			return NULL;
		}

		common->Printf("Failed to load dll, looking for qvm.\n");
		interpret = VMI_COMPILED;
	}

	// load the image
	String::Sprintf(filename, sizeof(filename), "vm/%s.qvm", vm->name);
	common->Printf("Loading vm file %s.\n", filename);
	length = FS_ReadFile(filename, (void**)&header);
	if (!header)
	{
		common->Printf("Failed.\n");
		VM_Free(vm);
		return NULL;
	}

	// byte swap the header
	for (i = 0; i < (int)sizeof(*header) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	// validate
	if (header->vmMagic != VM_MAGIC ||
		header->bssLength < 0 ||
		header->dataLength < 0 ||
		header->litLength < 0 ||
		header->codeLength <= 0)
	{
		VM_Free(vm);
		common->FatalError("%s has bad header", filename);
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header->dataLength + header->litLength + header->bssLength;
	for (i = 0; dataLength > (1 << i); i++)
	{
	}
	dataLength = 1 << i;

	// allocate zero filled space for initialized and uninitialized data
	vm->dataBase = new byte[dataLength];
	Com_Memset(vm->dataBase, 0, dataLength);
	vm->dataMask = dataLength - 1;

	// copy the intialized data
	Com_Memcpy(vm->dataBase, (byte*)header + header->dataOffset, header->dataLength + header->litLength);

	// byte swap the longs
	for (i = 0; i < header->dataLength; i += 4)
	{
		*(int*)(vm->dataBase + i) = LittleLong(*(int*)(vm->dataBase + i));
	}

	// allocate space for the jump targets, which will be filled in by the compile/prep functions
	vm->instructionPointersLength = header->instructionCount * 4;
	vm->instructionPointers = new int[header->instructionCount];
	Com_Memset(vm->instructionPointers, 0, vm->instructionPointersLength);

	// copy or compile the instructions
	vm->codeLength = header->codeLength;

	if (interpret >= VMI_COMPILED)
	{
		vm->compiled = true;
		VM_Compile(vm, header);
	}
	else
	{
		vm->compiled = false;
		VM_PrepareInterpreter(vm, header);
	}

	// free the original file
	FS_FreeFile(header);

	// load the map file
	VM_LoadSymbols(vm);

	// the stack is implicitly at the end of the image
	vm->programStack = vm->dataMask + 1;
	vm->stackBottom = vm->programStack - STACK_SIZE;

	common->Printf("%s loaded\n", module);

	return vm;
}

void VM_Free(vm_t* vm)
{
	if (vm->dllHandle)
	{
		Sys_UnloadDll(vm->dllHandle);
	}
	if (vm->codeBase)
	{
		delete[] vm->codeBase;
	}
	if (vm->dataBase)
	{
		delete[] vm->dataBase;
	}
	if (vm->instructionPointers)
	{
		delete[] vm->instructionPointers;
	}
	for (vmSymbol_t* sym = vm->symbols; sym; )
	{
		vmSymbol_t* next = sym->next;
		Mem_Free(sym);
		sym = next;
	}
	Com_Memset(vm, 0, sizeof(*vm));

	currentVM = NULL;
	lastVM = NULL;
}

//	Reload the data, but leave everything else in place
// This allows a server to do a map_restart without changing memory allocation
vm_t* VM_Restart(vm_t* vm)
{
	vmHeader_t* header;
	int length;
	int dataLength;
	int i;
	char filename[MAX_QPATH];

	// DLL's can't be restarted in place
	if (vm->dllHandle)
	{
		char name[MAX_QPATH];
		qintptr (* systemCall)(qintptr* parms);

		systemCall = vm->systemCall;
		String::NCpyZ(name, vm->name, sizeof(name));

		VM_Free(vm);

		vm = VM_Create(name, systemCall, VMI_NATIVE);
		return vm;
	}

	// load the image
	common->Printf("VM_Restart()\n");
	String::Sprintf(filename, sizeof(filename), "vm/%s.qvm", vm->name);
	common->Printf("Loading vm file %s.\n", filename);
	length = FS_ReadFile(filename, (void**)&header);
	if (!header)
	{
		common->Error("VM_Restart failed.\n");
	}

	// byte swap the header
	for (i = 0; i < (int)sizeof(*header) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	// validate
	if (header->vmMagic != VM_MAGIC ||
		header->bssLength < 0 ||
		header->dataLength < 0 ||
		header->litLength < 0 ||
		header->codeLength <= 0)
	{
		VM_Free(vm);
		common->FatalError("%s has bad header", filename);
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header->dataLength + header->litLength + header->bssLength;
	for (i = 0; dataLength > (1 << i); i++)
	{
	}
	dataLength = 1 << i;

	// clear the data
	Com_Memset(vm->dataBase, 0, dataLength);

	// copy the intialized data
	Com_Memcpy(vm->dataBase, (byte*)header + header->dataOffset, header->dataLength + header->litLength);

	// byte swap the longs
	for (i = 0; i < header->dataLength; i += 4)
	{
		*(int*)(vm->dataBase + i) = LittleLong(*(int*)(vm->dataBase + i));
	}

	// free the original file
	FS_FreeFile(header);

	return vm;
}

void VM_Clear()
{
	for (int i = 0; i < MAX_VM; i++)
	{
		VM_Free(&vmTable[i]);
	}
	currentVM = NULL;
	lastVM = NULL;
}

/*
Upon a system call, the stack will look like:

sp+32	parm1
sp+28	parm0
sp+24	return value
sp+20	return address
sp+16	local1
sp+14	local0
sp+12	arg1
sp+8	arg0
sp+4	return stack
sp		return address

An interpreted function will immediately execute
an OP_ENTER instruction, which will subtract space for
locals from sp
*/
qintptr VM_Call(vm_t* vm, int callnum, ...)
{
	if (!vm)
	{
		common->FatalError("VM_Call with NULL vm");
	}

	vm_t* oldVM = currentVM;
	currentVM = vm;
	lastVM = vm;

	if (vm_debugLevel)
	{
		common->Printf("VM_Call( %i )\n", callnum);
	}

	// if we have a dll loaded, call it directly
	qintptr r;
	if (vm->entryPoint)
	{
#if id386
		int* args = &callnum;
#else
		//rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
		qintptr args[16];
		va_list ap;
		va_start(ap, callnum);
		for (int i = 0; i < (int)(sizeof(args) / sizeof(args[i])); i++)
		{
			args[i] = va_arg(ap, qintptr);
		}
		va_end(ap);
#endif

		r = vm->entryPoint(callnum,  args[0],  args[1],  args[2], args[3],
			args[4],  args[5], args[6], args[7],
			args[8],  args[9], args[10], args[11],
			args[12], args[13], args[14], args[15]);
	}
	else
	{
#if id386
		int* args = &callnum;
#else
		//rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
		int args[17];	//	Index 0 is for callnum
		args[0] = callnum;
		va_list ap;
		va_start(ap, callnum);
		for (int i = 1; i < (int)(sizeof(args) / sizeof(args[i])); i++)
		{
			args[i] = va_arg(ap, int);
		}
		va_end(ap);
#endif

		if (vm->compiled)
		{
			r = VM_CallCompiled(vm, args);
		}
		else
		{
			r = VM_CallInterpreted(vm, args);
		}
	}

	if (oldVM != NULL)
	{
		currentVM = oldVM;
	}
	return r;
}

static int VM_ProfileSort(const void* a, const void* b)
{
	vmSymbol_t* sa = *(vmSymbol_t**)a;
	vmSymbol_t* sb = *(vmSymbol_t**)b;

	if (sa->profileCount < sb->profileCount)
	{
		return -1;
	}
	if (sa->profileCount > sb->profileCount)
	{
		return 1;
	}
	return 0;
}

static void VM_VmProfile_f()
{
	if (!lastVM)
	{
		return;
	}

	vm_t* vm = lastVM;

	if (!vm->numSymbols)
	{
		return;
	}

	vmSymbol_t** sorted = new vmSymbol_t*[vm->numSymbols];
	sorted[0] = vm->symbols;
	double total = sorted[0]->profileCount;
	for (int i = 1; i < vm->numSymbols; i++)
	{
		sorted[i] = sorted[i - 1]->next;
		total += sorted[i]->profileCount;
	}

	qsort(sorted, vm->numSymbols, sizeof(*sorted), VM_ProfileSort);

	for (int i = 0; i < vm->numSymbols; i++)
	{
		vmSymbol_t* sym = sorted[i];

		int perc = 100 * (float)sym->profileCount / total;
		common->Printf("%2i%% %9i %s\n", perc, sym->profileCount, sym->symName);
		sym->profileCount = 0;
	}

	common->Printf("    %9.0f total\n", total);

	delete[] sorted;
}

static void VM_VmInfo_f()
{
	common->Printf("Registered virtual machines:\n");
	for (int i = 0; i < MAX_VM; i++)
	{
		vm_t* vm = &vmTable[i];
		if (!vm->name[0])
		{
			break;
		}
		common->Printf("%s : ", vm->name);
		if (vm->dllHandle)
		{
			common->Printf("native\n");
			continue;
		}
		if (vm->compiled)
		{
			common->Printf("compiled on load\n");
		}
		else
		{
			common->Printf("interpreted\n");
		}
		common->Printf("    code length : %7i\n", vm->codeLength);
		common->Printf("    table length: %7i\n", vm->instructionPointersLength);
		common->Printf("    data length : %7i\n", vm->dataMask + 1);
	}
}

void VM_Init()
{
	if (!(GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET)))
	{
		Cvar_Get("vm_cgame", "2", CVAR_ARCHIVE);	// !@# SHIP WITH SET TO 2
		Cvar_Get("vm_game", "2", CVAR_ARCHIVE);		// !@# SHIP WITH SET TO 2
		Cvar_Get("vm_ui", "2", CVAR_ARCHIVE);		// !@# SHIP WITH SET TO 2
	}

	Cmd_AddCommand("vmprofile", VM_VmProfile_f);
	Cmd_AddCommand("vminfo", VM_VmInfo_f);

	Com_Memset(vmTable, 0, sizeof(vmTable));
}
