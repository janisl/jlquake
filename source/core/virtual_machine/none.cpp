//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../core.h"
#include "local.h"

void VM_Compile(vm_t* vm, vmHeader_t* header)
{
	vm->compiled = false;
	VM_PrepareInterpreter(vm, header);
}

qintptr VM_CallCompiled(vm_t* vm, int* args)
{
	return 0;
}
