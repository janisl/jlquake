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

// vm.c -- virtual machine

/*


intermix code and data
symbol table

a dll has one imported function: VM_SystemCall
and one exported function: Perform


*/

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../common/virtual_machine/local.h"


vm_t* lastVM    = NULL;		// bk001212

#define MAX_VM      3
vm_t vmTable[MAX_VM];


void VM_VmInfo_f(void);
void VM_VmProfile_f(void);


/*
==============
VM_Init
==============
*/
void VM_Init(void)
{
	Cvar_Get("vm_cgame", "0", CVAR_ARCHIVE);
	Cvar_Get("vm_game",  "0", CVAR_ARCHIVE);
	Cvar_Get("vm_ui",    "0", CVAR_ARCHIVE);

	Cmd_AddCommand("vmprofile", VM_VmProfile_f);
	Cmd_AddCommand("vminfo", VM_VmInfo_f);

	Com_Memset(vmTable, 0, sizeof(vmTable));
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
	const char* cdpath = Cvar_VariableString("fs_cdpath");
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
		char* fn = FS_BuildOSPath(homepath, gamedir, fname);
		libHandle = Sys_LoadDll(fn);

		if (!libHandle)
		{
			fn = FS_BuildOSPath(basepath, gamedir, fname);
			libHandle = Sys_LoadDll(fn);

			if (!libHandle && cdpath[0])
			{
				fn = FS_BuildOSPath(cdpath, gamedir, fname);
				libHandle = Sys_LoadDll(fn);
			}

			if (!libHandle && String::Length(gamedir) && String::ICmp(gamedir, BASEGAME))
			{
				if (homepath[0])
				{
					fn = FS_BuildOSPath(homepath, BASEGAME, fname);
					libHandle = Sys_LoadDll(fn);
				}

				if (!libHandle)
				{
					fn = FS_BuildOSPath(basepath, BASEGAME, fname);
					libHandle = Sys_LoadDll(fn);
				}

				if (!libHandle && cdpath[0])
				{
					fn = FS_BuildOSPath(cdpath, BASEGAME, fname);
					libHandle = Sys_LoadDll(fn);
				}
			}
		}
	}

	if (!libHandle)
	{
		return NULL;
	}

	void (* dllEntry)(qintptr (* syscallptr)(int, ...)) =
		(void (*)(qintptr (*)(int, ...)))Sys_GetDllFunction(libHandle, "dllEntry");
	*entryPoint = (qintptr (*)(int,...))Sys_GetDllFunction(libHandle, "vmMain");
	if (!*entryPoint || !dllEntry)
	{
		Sys_UnloadDll(libHandle);
		return NULL;
	}
	dllEntry(systemcalls);

	return libHandle;
}

/*
============
VM_DllSyscall

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

  As for having enough space in a signed int for your datatypes, well,
   it might be better to wait for DOOM 3 before you start porting.  :)

  The original code, while probably still inherently dangerous, seems
   to work well enough for the platforms it already works on. Rather
   than add the performance hit for those platforms, the original code
   is still in use there.

  For speed, we just grab 15 arguments, and don't worry about exactly
   how many the syscall actually needs; the extra is thrown away.

============
*/
qintptr QDECL VM_DllSyscall(int arg, ...)
{
#if !id386
	// rcg010206 - see commentary above
	qintptr args[16];
	int i;
	va_list ap;

	args[0] = arg;

	va_start(ap, arg);
	for (i = 1; i < (int)sizeof(args) / (int)sizeof(args[i]); i++)
		args[i] = va_arg(ap, qintptr);
	va_end(ap);

	return currentVM->systemCall(args);
#else	// original id code
	return currentVM->systemCall(&arg);
#endif
}

/*
=================
VM_Restart

Reload the data, but leave everything else in place
This allows a server to do a map_restart without changing memory allocation
=================
*/
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
	Com_Printf("VM_Restart()\n", filename);
	String::Sprintf(filename, sizeof(filename), "vm/%s.qvm", vm->name);
	Com_Printf("Loading vm file %s.\n", filename);
	length = FS_ReadFile(filename, (void**)&header);
	if (!header)
	{
		Com_Error(ERR_DROP, "VM_Restart failed.\n");
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
		Com_Error(ERR_FATAL, "%s has bad header", filename);
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

/*
================
VM_Create

If image ends in .qvm it will be interpreted, otherwise
it will attempt to load as a system dll
================
*/

#define STACK_SIZE  0x20000

vm_t* VM_Create(const char* module, qintptr (* systemCalls)(qintptr*),
	vmInterpret_t interpret)
{
	vm_t* vm;
	vmHeader_t* header;
	int length;
	int dataLength;
	int i, remaining;
	char filename[MAX_QPATH];

	if (!module || !module[0] || !systemCalls)
	{
		Com_Error(ERR_FATAL, "VM_Create: bad parms");
	}

	remaining = Hunk_MemoryRemaining();

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
		Com_Error(ERR_FATAL, "VM_Create: no free vm_t");
	}

	vm = &vmTable[i];

	String::NCpyZ(vm->name, module, sizeof(vm->name));
	vm->systemCall = systemCalls;

	// never allow dll loading with a demo
/*	if ( interpret == VMI_NATIVE ) {
        if ( Cvar_VariableValue( "fs_restrict" ) ) {
            interpret = VMI_COMPILED;
        }
    }
*/
	// Wolf.  /only/ allow dll loading with a demo
	if (interpret != VMI_NATIVE)
	{
		if (Cvar_VariableValue("fs_restrict"))
		{
			interpret = VMI_NATIVE;
		}
	}


	if (interpret == VMI_NATIVE)
	{
		// try to load as a system dll
//		Com_Printf( "Loading dll file %s.\n", vm->name );
		vm->dllHandle = VM_LoadDll(module, &vm->entryPoint, VM_DllSyscall);
		if (vm->dllHandle)
		{
			return vm;
		}

		Com_Printf("Failed to load dll, looking for qvm.\n");
		interpret = VMI_COMPILED;
	}

	// load the image
	String::Sprintf(filename, sizeof(filename), "vm/%s.qvm", vm->name);
	Com_Printf("Loading vm file %s.\n", filename);
	length = FS_ReadFile(filename, (void**)&header);
	if (!header)
	{
		Com_Printf("Failed.\n");
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
		Com_Error(ERR_FATAL, "%s has bad header", filename);
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header->dataLength + header->litLength + header->bssLength;
	for (i = 0; dataLength > (1 << i); i++)
	{
	}
	dataLength = 1 << i;

	// allocate zero filled space for initialized and uninitialized data
	vm->dataBase = (byte*)Hunk_Alloc(dataLength, h_high);
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
	vm->instructionPointers = (int*)Hunk_Alloc(vm->instructionPointersLength, h_high);

	// copy or compile the instructions
	vm->codeLength = header->codeLength;

	if (interpret >= VMI_COMPILED)
	{
		vm->compiled = qtrue;
		VM_Compile(vm, header);
	}
	else
	{
		vm->compiled = qfalse;
		VM_PrepareInterpreter(vm, header);
	}

	// free the original file
	FS_FreeFile(header);

	// load the map file
	VM_LoadSymbols(vm);

	// the stack is implicitly at the end of the image
	vm->programStack = vm->dataMask + 1;
	vm->stackBottom = vm->programStack - STACK_SIZE;

	Com_Printf("%s loaded in %d bytes on the hunk\n", module, remaining - Hunk_MemoryRemaining());

	return vm;
}


/*
==============
VM_Free
==============
*/
void VM_Free(vm_t* vm)
{

	if (vm->dllHandle)
	{
		Sys_UnloadDll(vm->dllHandle);
		Com_Memset(vm, 0, sizeof(*vm));
	}
#if 0	// now automatically freed by hunk
	if (vm->codeBase)
	{
		Z_Free(vm->codeBase);
	}
	if (vm->dataBase)
	{
		Z_Free(vm->dataBase);
	}
	if (vm->instructionPointers)
	{
		Z_Free(vm->instructionPointers);
	}
#endif
	Com_Memset(vm, 0, sizeof(*vm));

	currentVM = NULL;
	lastVM = NULL;
}

void VM_Clear(void)
{
	int i;
	for (i = 0; i < MAX_VM; i++)
	{
		if (vmTable[i].dllHandle)
		{
			Sys_UnloadDll(vmTable[i].dllHandle);
		}
		Com_Memset(&vmTable[i], 0, sizeof(vm_t));
	}
	currentVM = NULL;
	lastVM = NULL;
}

/*
==============
VM_Call


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
==============
*/
#define MAX_STACK   256
#define STACK_MASK  (MAX_STACK - 1)

qintptr QDECL VM_Call(vm_t* vm, int callnum, ...)
{
	vm_t* oldVM;
	qintptr r;
	//rcg010207 see dissertation at top of VM_DllSyscall() in this file.
#if !id386
	int i;
	qintptr args[16];
	va_list ap;
#endif


	if (!vm)
	{
		Com_Error(ERR_FATAL, "VM_Call with NULL vm");
	}

	oldVM = currentVM;
	currentVM = vm;
	lastVM = vm;

	if (vm_debugLevel)
	{
		Com_Printf("VM_Call( %i )\n", callnum);
	}

	// if we have a dll loaded, call it directly
	if (vm->entryPoint)
	{
		//rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
#if !id386
		va_start(ap, callnum);
		for (i = 0; i < (int)sizeof(args) / (int)sizeof(args[i]); i++)
			args[i] = va_arg(ap, qintptr);
		va_end(ap);

		r = vm->entryPoint(callnum,  args[0],  args[1],  args[2], args[3],
			args[4],  args[5],  args[6], args[7],
			args[8],  args[9], args[10], args[11],
			args[12], args[13], args[14], args[15]);
#else	// PPC above, original id code below
		r = vm->entryPoint((&callnum)[0], (&callnum)[1], (&callnum)[2], (&callnum)[3],
			(&callnum)[4], (&callnum)[5], (&callnum)[6], (&callnum)[7],
			(&callnum)[8],  (&callnum)[9],  (&callnum)[10],  (&callnum)[11],  (&callnum)[12]);
#endif
	}
	else if (vm->compiled)
	{
		r = VM_CallCompiled(vm, &callnum);
	}
	else
	{
		r = VM_CallInterpreted(vm, &callnum);
	}

	if (oldVM != NULL)		// bk001220 - assert(currentVM!=NULL) for oldVM==NULL
	{
		currentVM = oldVM;
	}
	return r;
}

//=================================================================

static int QDECL VM_ProfileSort(const void* a, const void* b)
{
	vmSymbol_t* sa, * sb;

	sa = *(vmSymbol_t**)a;
	sb = *(vmSymbol_t**)b;

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

/*
==============
VM_VmProfile_f

==============
*/
void VM_VmProfile_f(void)
{
	vm_t* vm;
	vmSymbol_t** sorted, * sym;
	int i;
	double total;

	if (!lastVM)
	{
		return;
	}

	vm = lastVM;

	if (!vm->numSymbols)
	{
		return;
	}

	sorted = (vmSymbol_t**)Z_Malloc(vm->numSymbols * sizeof(*sorted));
	sorted[0] = vm->symbols;
	total = sorted[0]->profileCount;
	for (i = 1; i < vm->numSymbols; i++)
	{
		sorted[i] = sorted[i - 1]->next;
		total += sorted[i]->profileCount;
	}

	qsort(sorted, vm->numSymbols, sizeof(*sorted), VM_ProfileSort);

	for (i = 0; i < vm->numSymbols; i++)
	{
		int perc;

		sym = sorted[i];

		perc = 100 * (float)sym->profileCount / total;
		Com_Printf("%2i%% %9i %s\n", perc, sym->profileCount, sym->symName);
		sym->profileCount = 0;
	}

	Com_Printf("    %9.0f total\n", total);

	Z_Free(sorted);
}

/*
==============
VM_VmInfo_f

==============
*/
void VM_VmInfo_f(void)
{
	vm_t* vm;
	int i;

	Com_Printf("Registered virtual machines:\n");
	for (i = 0; i < MAX_VM; i++)
	{
		vm = &vmTable[i];
		if (!vm->name[0])
		{
			break;
		}
		Com_Printf("%s : ", vm->name);
		if (vm->dllHandle)
		{
			Com_Printf("native\n");
			continue;
		}
		if (vm->compiled)
		{
			Com_Printf("compiled on load\n");
		}
		else
		{
			Com_Printf("interpreted\n");
		}
		Com_Printf("    code length : %7i\n", vm->codeLength);
		Com_Printf("    table length: %7i\n", vm->instructionPointersLength);
		Com_Printf("    data length : %7i\n", vm->dataMask + 1);
	}
}
