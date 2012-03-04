/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
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
// vm.c -- virtual machine

/*


intermix code and data
symbol table

a dll has one imported function: VM_SystemCall
and one exported function: Perform


*/

#include "../../core/core.h"
#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../core/virtual_machine/local.h"


vm_t	*lastVM    = NULL; // bk001212

#define	MAX_VM		3
vm_t	vmTable[MAX_VM];


void VM_VmInfo_f( void );
void VM_VmProfile_f( void );


/*
==============
VM_Init
==============
*/
void VM_Init( void ) {
	Cvar_Get( "vm_cgame", "2", CVAR_ARCHIVE );	// !@# SHIP WITH SET TO 2
	Cvar_Get( "vm_game", "2", CVAR_ARCHIVE );	// !@# SHIP WITH SET TO 2
	Cvar_Get( "vm_ui", "2", CVAR_ARCHIVE );		// !@# SHIP WITH SET TO 2

	Cmd_AddCommand ("vmprofile", VM_VmProfile_f );
	Cmd_AddCommand ("vminfo", VM_VmInfo_f );

	Com_Memset( vmTable, 0, sizeof( vmTable ) );
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

  JL: On 64 bit stack contains 64 bit values.
============
*/
int VM_DllSyscall(qintptr arg, ...)
{
#if id386
	return currentVM->systemCall(&arg);
#else
	// rcg010206 - see commentary above
	qintptr args[16];
	args[0] = arg;

	va_list ap;
	va_start(ap, arg);
	for (int i = 1; i < (int)(sizeof (args) / sizeof (args[i])); i++)
	{
		args[i] = va_arg(ap, qintptr);
	}
	va_end(ap);

	return currentVM->systemCall(args);
#endif
}

/*
=================
VM_Restart

Reload the data, but leave everything else in place
This allows a server to do a map_restart without changing memory allocation
=================
*/
vm_t *VM_Restart( vm_t *vm ) {
	vmHeader_t	*header;
	int			length;
	int			dataLength;
	int			i;
	char		filename[MAX_QPATH];

	// DLL's can't be restarted in place
	if ( vm->dllHandle ) {
		char	name[MAX_QPATH];
	    qintptr			(*systemCall)( qintptr *parms );
		
		systemCall = vm->systemCall;	
		String::NCpyZ( name, vm->name, sizeof( name ) );

		VM_Free( vm );

		vm = VM_Create( name, systemCall, VMI_NATIVE );
		return vm;
	}

	// load the image
	Log::write( "VM_Restart()\n", filename );
	String::Sprintf( filename, sizeof(filename), "vm/%s.qvm", vm->name );
	Log::write( "Loading vm file %s.\n", filename );
	length = FS_ReadFile( filename, (void **)&header );
	if (!header)
	{
		throw DropException("VM_Restart failed.\n");
	}

	// byte swap the header
	for ( i = 0 ; i < (int)sizeof( *header ) / 4 ; i++ ) {
		((int *)header)[i] = LittleLong( ((int *)header)[i] );
	}

	// validate
	if ( header->vmMagic != VM_MAGIC
		|| header->bssLength < 0 
		|| header->dataLength < 0 
		|| header->litLength < 0 
		|| header->codeLength <= 0 ) {
		VM_Free( vm );
		throw Exception(va("%s has bad header", filename));
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header->dataLength + header->litLength + header->bssLength;
	for ( i = 0 ; dataLength > ( 1 << i ) ; i++ ) {
	}
	dataLength = 1 << i;

	// clear the data
	Com_Memset( vm->dataBase, 0, dataLength );

	// copy the intialized data
	Com_Memcpy( vm->dataBase, (byte *)header + header->dataOffset, header->dataLength + header->litLength );

	// byte swap the longs
	for ( i = 0 ; i < header->dataLength ; i += 4 ) {
		*(int *)(vm->dataBase + i) = LittleLong( *(int *)(vm->dataBase + i ) );
	}

	// free the original file
	FS_FreeFile( header );

	return vm;
}

/*
================
VM_Create

If image ends in .qvm it will be interpreted, otherwise
it will attempt to load as a system dll
================
*/

#define	STACK_SIZE	0x20000

vm_t *VM_Create( const char *module, qintptr (*systemCalls)(qintptr*), 
				vmInterpret_t interpret ) {
	vm_t		*vm;
	vmHeader_t	*header;
	int			length;
	int			dataLength;
	int			i, remaining;
	char		filename[MAX_QPATH];

	if (!module || !module[0] || !systemCalls)
	{
		throw Exception("VM_Create: bad parms");
	}

	remaining = Hunk_MemoryRemaining();

	// see if we already have the VM
	for ( i = 0 ; i < MAX_VM ; i++ ) {
		if (!String::ICmp(vmTable[i].name, module)) {
			vm = &vmTable[i];
			return vm;
		}
	}

	// find a free vm
	for ( i = 0 ; i < MAX_VM ; i++ ) {
		if ( !vmTable[i].name[0] ) {
			break;
		}
	}

	if (i == MAX_VM)
	{
		throw Exception("VM_Create: no free vm_t");
	}

	vm = &vmTable[i];

	String::NCpyZ( vm->name, module, sizeof( vm->name ) );
	vm->systemCall = systemCalls;

	// never allow dll loading with a demo
	if ( interpret == VMI_NATIVE ) {
		if ( Cvar_VariableValue( "fs_restrict" ) ) {
			interpret = VMI_COMPILED;
		}
	}

	if ( interpret == VMI_NATIVE ) {
		// try to load as a system dll
		Log::write( "Loading dll file %s.\n", vm->name );
		vm->dllHandle = Sys_LoadDll( module, vm->fqpath , &vm->entryPoint, VM_DllSyscall );
		if ( vm->dllHandle ) {
			return vm;
		}

		Log::write( "Failed to load dll, looking for qvm.\n" );
		interpret = VMI_COMPILED;
	}

	// load the image
	String::Sprintf( filename, sizeof(filename), "vm/%s.qvm", vm->name );
	Log::write( "Loading vm file %s.\n", filename );
	length = FS_ReadFile( filename, (void **)&header );
	if ( !header ) {
		Log::write( "Failed.\n" );
		VM_Free( vm );
		return NULL;
	}

	// byte swap the header
	for ( i = 0 ; i < (int)sizeof( *header ) / 4 ; i++ ) {
		((int *)header)[i] = LittleLong( ((int *)header)[i] );
	}

	// validate
	if ( header->vmMagic != VM_MAGIC
		|| header->bssLength < 0 
		|| header->dataLength < 0 
		|| header->litLength < 0 
		|| header->codeLength <= 0 )
	{
		VM_Free(vm);
		throw Exception(va("%s has bad header", filename));
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header->dataLength + header->litLength + header->bssLength;
	for ( i = 0 ; dataLength > ( 1 << i ) ; i++ ) {
	}
	dataLength = 1 << i;

	// allocate zero filled space for initialized and uninitialized data
	vm->dataBase = new byte[dataLength];
	Com_Memset(vm->dataBase, 0, dataLength);
	vm->dataMask = dataLength - 1;

	// copy the intialized data
	Com_Memcpy( vm->dataBase, (byte *)header + header->dataOffset, header->dataLength + header->litLength );

	// byte swap the longs
	for ( i = 0 ; i < header->dataLength ; i += 4 ) {
		*(int *)(vm->dataBase + i) = LittleLong( *(int *)(vm->dataBase + i ) );
	}

	// allocate space for the jump targets, which will be filled in by the compile/prep functions
	vm->instructionPointersLength = header->instructionCount * 4;
	vm->instructionPointers = new int[header->instructionCount];
	Com_Memset(vm->instructionPointers, 0, vm->instructionPointersLength);

	// copy or compile the instructions
	vm->codeLength = header->codeLength;

	if ( interpret >= VMI_COMPILED ) {
		vm->compiled = true;
		VM_Compile( vm, header );
	} else {
		vm->compiled = false;
		VM_PrepareInterpreter( vm, header );
	}

	// free the original file
	FS_FreeFile( header );

	// load the map file
	VM_LoadSymbols( vm );

	// the stack is implicitly at the end of the image
	vm->programStack = vm->dataMask + 1;
	vm->stackBottom = vm->programStack - STACK_SIZE;

	Log::write("%s loaded in %d bytes on the hunk\n", module, remaining - Hunk_MemoryRemaining());

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
	for (vmSymbol_t* sym = vm->symbols; sym;)
	{
		vmSymbol_t* next = sym->next;
		Mem_Free(sym);
		sym = next;
	}
	Com_Memset(vm, 0, sizeof(*vm));

	currentVM = NULL;
	lastVM = NULL;
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
#define	MAX_STACK	256
#define	STACK_MASK	(MAX_STACK-1)

qintptr VM_Call(vm_t* vm, int callnum, ...)
{
	if (!vm)
	{
		throw Exception("VM_Call with NULL vm");
	}

	vm_t* oldVM = currentVM;
	currentVM = vm;
	lastVM = vm;

	if (vm_debugLevel)
	{
		Log::write("VM_Call( %i )\n", callnum);
	}

#if id386
	int* args = &callnum;
#else
	//rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
	int args[17];	//	Index 0 is for callnum
	args[0] = callnum;
	va_list ap;
	va_start(ap, callnum);
	for (int i = 1; i < (int)(sizeof (args) / sizeof (args[i])); i++)
	{
		args[i] = va_arg(ap, int);
	}
	va_end(ap);
#endif

	// if we have a dll loaded, call it directly
	qintptr r;
	if (vm->entryPoint)
	{
		r = vm->entryPoint(callnum,  args[1],  args[2],  args[3], args[4],
			args[5],  args[6],  args[7], args[8],
			args[9],  args[10], args[11], args[12],
			args[13], args[14], args[15], args[16]);
	}
	else if (vm->compiled)
	{
		r = VM_CallCompiled(vm, args);
	}
	else
	{
		r = VM_CallInterpreted(vm, args);
	}

	if (oldVM != NULL) // bk001220 - assert(currentVM!=NULL) for oldVM==NULL
	{
		currentVM = oldVM;
	}
	return r;
}

//=================================================================

static int VM_ProfileSort( const void *a, const void *b ) {
	vmSymbol_t	*sa, *sb;

	sa = *(vmSymbol_t **)a;
	sb = *(vmSymbol_t **)b;

	if ( sa->profileCount < sb->profileCount ) {
		return -1;
	}
	if ( sa->profileCount > sb->profileCount ) {
		return 1;
	}
	return 0;
}

/*
==============
VM_VmProfile_f

==============
*/
void VM_VmProfile_f( void ) {
	vm_t		*vm;
	vmSymbol_t	**sorted, *sym;
	int			i;
	double		total;

	if ( !lastVM ) {
		return;
	}

	vm = lastVM;

	if ( !vm->numSymbols ) {
		return;
	}

	sorted = (vmSymbol_t**)Z_Malloc( vm->numSymbols * sizeof( *sorted ) );
	sorted[0] = vm->symbols;
	total = sorted[0]->profileCount;
	for ( i = 1 ; i < vm->numSymbols ; i++ ) {
		sorted[i] = sorted[i-1]->next;
		total += sorted[i]->profileCount;
	}

	qsort( sorted, vm->numSymbols, sizeof( *sorted ), VM_ProfileSort );

	for ( i = 0 ; i < vm->numSymbols ; i++ ) {
		int		perc;

		sym = sorted[i];

		perc = 100 * (float) sym->profileCount / total;
		Log::write( "%2i%% %9i %s\n", perc, sym->profileCount, sym->symName );
		sym->profileCount = 0;
	}

	Log::write("    %9.0f total\n", total );

	Z_Free( sorted );
}

/*
==============
VM_VmInfo_f

==============
*/
void VM_VmInfo_f( void ) {
	vm_t	*vm;
	int		i;

	Log::write( "Registered virtual machines:\n" );
	for ( i = 0 ; i < MAX_VM ; i++ ) {
		vm = &vmTable[i];
		if ( !vm->name[0] ) {
			break;
		}
		Log::write( "%s : ", vm->name );
		if ( vm->dllHandle ) {
			Log::write( "native\n" );
			continue;
		}
		if ( vm->compiled ) {
			Log::write( "compiled on load\n" );
		} else {
			Log::write( "interpreted\n" );
		}
		Log::write( "    code length : %7i\n", vm->codeLength );
		Log::write( "    table length: %7i\n", vm->instructionPointersLength );
		Log::write( "    data length : %7i\n", vm->dataMask + 1 );
	}
}
