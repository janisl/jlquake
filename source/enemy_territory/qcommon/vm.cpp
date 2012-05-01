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
	Com_Printf("VM_Restart()\n");
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

void VM_Clear(void)
{
	int i;
	for (i = 0; i < MAX_VM; i++)
	{
		VM_Free(&vmTable[i]);
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
