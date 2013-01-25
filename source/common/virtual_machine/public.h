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

#ifndef __VIRTUAL_MACHINE_PUBLIC_H__
#define __VIRTUAL_MACHINE_PUBLIC_H__

#include "../qcommon.h"

enum vmInterpret_t
{
	VMI_NATIVE,
	VMI_BYTECODE,
	VMI_COMPILED
};

//	Shared traps
enum
{
	TRAP_MEMSET = 100,
	TRAP_MEMCPY,
	TRAP_STRNCPY,
	TRAP_SIN,
	TRAP_COS,
	TRAP_ATAN2,
	TRAP_SQRT,
	TRAP_MATRIXMULTIPLY,
	TRAP_ANGLEVECTORS,
	TRAP_PERPENDICULARVECTOR,
	TRAP_FLOOR,
	TRAP_CEIL,

	TRAP_TESTPRINTINT,
	TRAP_TESTPRINTFLOAT
};

struct vm_t;

void VM_Debug( int level );
void* VM_ArgPtr( qintptr intValue );
void* VM_ExplicitArgPtr( vm_t* vm, qintptr intValue );
vm_t* VM_Create( const char* module, qintptr ( * systemCalls )( qintptr* ),
	vmInterpret_t interpret );
void VM_Free( vm_t* vm );
vm_t* VM_Restart( vm_t* vm );
void VM_Clear();
qintptr VM_Call( vm_t* vm, int callNum, ... );
void VM_Init();

#define VMA( x )  VM_ArgPtr( args[ x ] )
#define VMF( x )  ( *( float* )( &args[ x ] ) )

inline int FloatAsInt( float f ) {
	int temp;

	*( float* )& temp = f;

	return temp;
}

#endif
