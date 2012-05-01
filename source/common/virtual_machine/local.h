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

#include "../file_formats/qvm.h"

enum opcode_t
{
	OP_UNDEF,

	OP_IGNORE,

	OP_BREAK,

	OP_ENTER,
	OP_LEAVE,
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,
	OP_LOCAL,

	OP_JUMP,

	//-------------------

	OP_EQ,
	OP_NE,

	OP_LTI,
	OP_LEI,
	OP_GTI,
	OP_GEI,

	OP_LTU,
	OP_LEU,
	OP_GTU,
	OP_GEU,

	OP_EQF,
	OP_NEF,

	OP_LTF,
	OP_LEF,
	OP_GTF,
	OP_GEF,

	//-------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,				// *(stack[top-1]) = stack[top]
	OP_ARG,

	OP_BLOCK_COPY,

	//-------------------

	OP_SEX8,
	OP_SEX16,

	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,

	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,

	OP_LSH,
	OP_RSHI,
	OP_RSHU,

	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,

	OP_CVIF,
	OP_CVFI
};

struct vmSymbol_t
{
	vmSymbol_t* next;
	int symValue;
	int profileCount;
	char symName[1];		// variable sized
};

struct vm_t
{
	int programStack;		// the vm may be recursively entered
	qintptr (* systemCall)(qintptr* parms);

	//------------------------------------

	char name[MAX_QPATH];

	// for dynamic linked modules
	void* dllHandle;
	qintptr (* entryPoint)(int callNum, ...);

	// for interpreted modules
	bool currentlyInterpreting;

	bool compiled;
	byte* codeBase;
	int codeLength;

	int* instructionPointers;
	int instructionPointersLength;

	byte* dataBase;
	int dataMask;

	int stackBottom;		// if programStack < stackBottom, error

	int numSymbols;
	vmSymbol_t* symbols;

	int callLevel;			// for debug indenting
	int breakFunction;		// increment breakCount on function entry to this
	int breakCount;
};

extern int vm_debugLevel;
extern vm_t* currentVM;

const char* VM_ValueToSymbol(vm_t* vm, int value);
vmSymbol_t* VM_ValueToFunctionSymbol(vm_t* vm, int value);
void VM_LoadSymbols(vm_t* vm);
void VM_LogSyscalls(qintptr* args);

void VM_PrepareInterpreter(vm_t* vm, vmHeader_t* header);
qintptr VM_CallInterpreted(vm_t* vm, int* args);

void VM_Compile(vm_t* vm, vmHeader_t* header);
qintptr VM_CallCompiled(vm_t* vm, int* args);

void* VM_LoadDll(const char* name, qintptr(**entryPoint) (int, ...),
	qintptr (* systemcalls)(int, ...));
qintptr VM_DllSyscall(int arg, ...);
