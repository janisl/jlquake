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

static int ParseHex(const char* text)
{
	int value = 0;
	int c;
	while (( c = *text++ ) != 0)
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
	int len = FS_ReadFile(symbols, &mapfile);
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
		char* token = String::Parse3( &text_p );
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
