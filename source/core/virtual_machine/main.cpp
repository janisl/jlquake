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

//	Insert calls to this while debugging the vm compiler
void VM_LogSyscalls(int* args)
{
	static int callnum;
	static FILE* f;

	if (!f)
	{
		f = fopen("syscalls.log", "w");
	}
	callnum++;
	fprintf(f, "%i: %i (%i) = %i %i %i %i\n", callnum, (int)(args - (int*)currentVM->dataBase),
		args[0], args[1], args[2], args[3], args[4]);
}
