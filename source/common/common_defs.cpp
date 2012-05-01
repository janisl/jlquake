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

#include "qcommon.h"

#define MAX_NUM_ARGVS   50

Cvar* com_dedicated;
Cvar* com_viewlog;
Cvar* com_timescale;

Cvar* com_journal;

Cvar* com_developer;

Cvar* com_crashed = NULL;	// ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

Cvar* cl_shownet;

fileHandle_t com_journalFile;			// events are written here
fileHandle_t com_journalDataFile;		// config files are written here

int GGameType;

int com_frameTime;

int cl_connectedToPureServer;

static int com_argc;
static const char* com_argv[MAX_NUM_ARGVS + 1];

Interface::~Interface()
{
}

#if 0	//id386 && defined _MSC_VER && !defined __VECTORC
typedef enum
{
	PRE_READ,									// prefetch assuming that buffer is used for reading only
	PRE_WRITE,									// prefetch assuming that buffer is used for writing only
	PRE_READ_WRITE								// prefetch assuming that buffer is used for both reading and writing
} e_prefetch;

void Com_Prefetch(const void* s, const unsigned int bytes, e_prefetch type);

#define EMMS_INSTRUCTION    __asm emms

void _copyDWord(unsigned int* dest, const unsigned int constant, const unsigned int count)
{
	__asm
	{
		mov edx,dest
		mov eax,constant
		mov ecx,count
		and ecx,~7
		jz padding
		sub ecx,8
		jmp loopu
			align   16
loopu:
		test    [edx + ecx * 4 + 28],ebx		// fetch next block destination to L1 cache
			mov     [edx + ecx * 4 + 0],eax
			mov     [edx + ecx * 4 + 4],eax
			mov     [edx + ecx * 4 + 8],eax
			mov     [edx + ecx * 4 + 12],eax
			mov     [edx + ecx * 4 + 16],eax
			mov     [edx + ecx * 4 + 20],eax
			mov     [edx + ecx * 4 + 24],eax
			mov     [edx + ecx * 4 + 28],eax
		sub ecx,8
		jge loopu
padding:    mov ecx,count
		mov ebx,ecx
		and ecx,7
		jz outta
		and ebx,~7
		lea edx,[edx + ebx * 4]					// advance dest pointer
		test    [edx + 0],eax					// fetch destination to L1 cache
		cmp ecx,4
		jl skip4
			mov     [edx + 0],eax
			mov     [edx + 4],eax
			mov     [edx + 8],eax
			mov     [edx + 12],eax
		add edx,16
		sub ecx,4
skip4:      cmp ecx,2
		jl skip2
			mov     [edx + 0],eax
			mov     [edx + 4],eax
		add edx,8
		sub ecx,2
skip2:      cmp ecx,1
		jl outta
			mov     [edx + 0],eax
outta:
	}
}

// optimized memory copy routine that handles all alignment
// cases and block sizes efficiently
void Com_Memcpy(void* dest, const void* src, const size_t count)
{
	Com_Prefetch(src, count, PRE_READ);
	__asm
	{
		push edi
		push esi
		mov ecx,count
		cmp ecx,0							// count = 0 check (just to be on the safe side)
		je outta
		mov edx,dest
		mov ebx,src
		cmp ecx,32							// padding only?
		jl padding

		mov edi,ecx
		and edi,~31						// edi = count&~31
		sub edi,32

		align 16
loopMisAligned:
		mov eax,[ebx + edi + 0 + 0 * 8]
		mov esi,[ebx + edi + 4 + 0 * 8]
		mov     [edx + edi + 0 + 0 * 8],eax
			mov     [edx + edi + 4 + 0 * 8],esi
		mov eax,[ebx + edi + 0 + 1 * 8]
		mov esi,[ebx + edi + 4 + 1 * 8]
		mov     [edx + edi + 0 + 1 * 8],eax
			mov     [edx + edi + 4 + 1 * 8],esi
		mov eax,[ebx + edi + 0 + 2 * 8]
		mov esi,[ebx + edi + 4 + 2 * 8]
		mov     [edx + edi + 0 + 2 * 8],eax
			mov     [edx + edi + 4 + 2 * 8],esi
		mov eax,[ebx + edi + 0 + 3 * 8]
		mov esi,[ebx + edi + 4 + 3 * 8]
		mov     [edx + edi + 0 + 3 * 8],eax
			mov     [edx + edi + 4 + 3 * 8],esi
		sub edi,32
		jge loopMisAligned

		mov edi,ecx
		and edi,~31
		add ebx,edi						// increase src pointer
		add edx,edi						// increase dst pointer
		and ecx,31						// new count
		jz outta						// if count = 0, get outta here

padding:
		cmp ecx,16
		jl skip16
		mov eax,dword ptr [ebx]
		mov dword ptr [edx],eax
		mov eax,dword ptr [ebx + 4]
		mov dword ptr [edx + 4],eax
		mov eax,dword ptr [ebx + 8]
		mov dword ptr [edx + 8],eax
		mov eax,dword ptr [ebx + 12]
		mov dword ptr [edx + 12],eax
		sub ecx,16
		add ebx,16
		add edx,16
skip16:
		cmp ecx,8
		jl skip8
		mov eax,dword ptr [ebx]
		mov dword ptr [edx],eax
		mov eax,dword ptr [ebx + 4]
		sub ecx,8
		mov dword ptr [edx + 4],eax
		add ebx,8
		add edx,8
skip8:
		cmp ecx,4
		jl skip4
		mov eax,dword ptr [ebx]		// here 4-7 bytes
		add ebx,4
		sub ecx,4
		mov dword ptr [edx],eax
		add edx,4
skip4:							// 0-3 remaining bytes
		cmp ecx,2
		jl skip2
		mov ax,word ptr [ebx]		// two bytes
		cmp ecx,3					// less than 3?
		mov word ptr [edx],ax
		jl outta
		mov al,byte ptr [ebx + 2]	// last byte
		mov byte ptr [edx + 2],al
		jmp outta
skip2:
		cmp ecx,1
		jl outta
		mov al,byte ptr [ebx]
		mov byte ptr [edx],al
outta:
		pop esi
		pop edi
	}
}

void Com_Memset(void* dest, const int val, const size_t count)
{
	unsigned int fillval;

	if (count < 8)
	{
		__asm
		{
			mov edx,dest
			mov eax, val
			mov ah,al
			mov ebx,eax
			and ebx, 0xffff
			shl eax,16
			add eax,ebx					// eax now contains pattern
			mov ecx,count
			cmp ecx,4
			jl skip4
				mov     [edx],eax		// copy first dword
			add edx,4
			sub ecx,4
skip4:  cmp ecx,2
			jl skip2
			mov word ptr [edx],ax		// copy 2 bytes
			add edx,2
			sub ecx,2
skip2:  cmp ecx,0
			je skip1
			mov byte ptr [edx],al		// copy single byte
skip1:
		}
		return;
	}

	fillval = val;

	fillval = fillval | (fillval << 8);
	fillval = fillval | (fillval << 16);		// fill dword with 8-bit pattern

	_copyDWord((unsigned int*)(dest),fillval, count / 4);

	__asm									// padding of 0-3 bytes
	{
		mov ecx,count
		mov eax,ecx
		and ecx,3
		jz skipA
		and eax,~3
		mov ebx,dest
		add ebx,eax
		mov eax,fillval
		cmp ecx,2
		jl skipB
		mov word ptr [ebx],ax
		cmp ecx,2
		je skipA
		mov byte ptr [ebx + 2],al
		jmp skipA
skipB:
		cmp ecx,0
		je skipA
		mov byte ptr [ebx],al
skipA:
	}
}

void Com_Prefetch(const void* s, const unsigned int bytes, e_prefetch type)
{
	// write buffer prefetching is performed only if
	// the processor benefits from it. Read and read/write
	// prefetching is always performed.

	switch (type)
	{
	case PRE_WRITE: break;
	case PRE_READ:
	case PRE_READ_WRITE:

		__asm
		{
			mov ebx,s
			mov ecx,bytes
			cmp ecx,4096					// clamp to 4kB
			jle skipClamp
			mov ecx,4096
skipClamp:
			add ecx,0x1f
			shr ecx,5						// number of cache lines
			jz skip
			jmp loopie

				align 16
loopie: test byte ptr [ebx],al
			add ebx,32
			dec ecx
			jnz loopie
skip:
		}

		break;
	}
}
#elif 0	//id386 && defined __GNUC__
/**
 * GAS syntax equivalents of the MSVC asm memory calls in common.c
 *
 * The following changes have been made to the asm:
 * 1. Registers are loaded by the inline asm arguments when possible
 * 2. Labels have been changed to local label format (0,1,etc.) to allow inlining
 *
 * HISTORY:
 *	AH - Created on 08 Dec 2000
 */

typedef enum {
	PRE_READ,		// prefetch assuming that buffer is used for reading only
	PRE_WRITE,		// prefetch assuming that buffer is used for writing only
	PRE_READ_WRITE	// prefetch assuming that buffer is used for both reading and writing
} e_prefetch;

void Com_Prefetch(const void* s, const unsigned int bytes, e_prefetch type);

void _copyDWord(unsigned int* dest, const unsigned int constant, const unsigned int count)
{
	// MMX version not used on standard Pentium MMX
	// because the dword version is faster (with
	// proper destination prefetching)
	__asm__ __volatile__ (
		//mov			eax,constant		// eax = val
		//mov			edx,dest			// dest
		//mov			ecx,count
		"			movd		%%eax, %%mm0		\n"
		"			punpckldq	%%mm0, %%mm0		\n"

		// ensure that destination is qword aligned

		"			testl		$7, %%edx			\n"// qword padding?
		"			jz		0f						\n"
		"			movl		%%eax, (%%edx)		\n"
		"			decl		%%ecx				\n"
		"			addl		$4, %%edx			\n"

		"0:			movl		%%ecx, %%ebx		\n"
		"			andl		$0xfffffff0, %%ecx	\n"
		"			jz		2f						\n"
		"			jmp		1f						\n"
		"			.align      16					\n"

		// funny ordering here to avoid commands
		// that cross 32-byte boundaries (the
		// [edx+0] version has a special 3-byte opcode...
		"1:			movq		%%mm0, 8(%%edx)		\n"
		"			movq		%%mm0, 16(%%edx)	\n"
		"			movq		%%mm0, 24(%%edx)	\n"
		"			movq		%%mm0, 32(%%edx)	\n"
		"			movq		%%mm0, 40(%%edx)	\n"
		"			movq		%%mm0, 48(%%edx)	\n"
		"			movq		%%mm0, 56(%%edx)	\n"
		"			movq		%%mm0, (%%edx)		\n"
		"			addl		$64, %%edx			\n"
		"			subl		$16, %%ecx			\n"
		"			jnz		1b						\n"
		"2:											\n"
		"			movl		%%ebx, %%ecx		\n"// ebx = cnt
		"			andl		$0xfffffff0, %%ecx	\n"// ecx = cnt&~15
		"			subl		%%ecx, %%ebx		\n"
		"			jz		6f						\n"
		"			cmpl		$8, %%ebx			\n"
		"			jl		3f						\n"

		"			movq		%%mm0, (%%edx)		\n"
		"			movq		%%mm0, 8(%%edx)		\n"
		"			movq		%%mm0, 16(%%edx)	\n"
		"			movq		%%mm0, 24(%%edx)	\n"
		"			addl		$32, %%edx			\n"
		"			subl		$8, %%ebx			\n"
		"			jz		6f						\n"

		"3:			cmpl		$4, %%ebx			\n"
		"			jl		4f						\n"

		"			movq		%%mm0, (%%edx)		\n"
		"			movq		%%mm0, 8(%%edx)		\n"
		"			addl		$16, %%edx			\n"
		"			subl		$4, %%ebx			\n"

		"4:			cmpl		$2, %%ebx			\n"
		"			jl		5f		\n"
		"			movq		%%mm0, (%%edx)		\n"
		"			addl		$8, %%edx			\n"
		"			subl		$2, %%ebx			\n"

		"5:			cmpl		$1, %%ebx			\n"
		"			jl		6f						\n"
		"			movl		%%eax, (%%edx)		\n"
		"6:											\n"
		"			emms							\n"
		: : "a" (constant), "c" (count), "d" (dest)
		: "%ebx", "%edi", "%esi", "cc", "memory");
}

// optimized memory copy routine that handles all alignment
// cases and block sizes efficiently
void Com_Memcpy(void* dest, const void* src, const size_t count)
{
	Com_Prefetch(src, count, PRE_READ);
	__asm__ __volatile__ (
		"		pushl		%%edi							\n"
		"		pushl		%%esi							\n"
		//mov		ecx,count
		"		cmpl		$0, %%ecx						\n"// count = 0 check (just to be on the safe side)
		"		je		6f									\n"
		//mov		edx,dest
		"		movl		%0, %%ebx						\n"
		"		cmpl		$32, %%ecx						\n"// padding only?
		"		jl		1f									\n"

		"		movl		%%ecx, %%edi					\n"
		"		andl		$0xfffffe00, %%edi				\n"// edi = count&~31
		"		subl		$32, %%edi						\n"

		"		.align 16									\n"
		"0:													\n"
		"		movl		(%%ebx, %%edi, 1), %%eax		\n"
		"		movl		4(%%ebx, %%edi, 1), %%esi		\n"
		"		movl		%%eax, (%%edx, %%edi, 1)		\n"
		"		movl		%%esi, 4(%%edx, %%edi, 1)		\n"
		"		movl		8(%%ebx, %%edi, 1), %%eax		\n"
		"		movl		12(%%ebx, %%edi, 1), %%esi		\n"
		"		movl		%%eax, 8(%%edx, %%edi, 1)		\n"
		"		movl		%%esi, 12(%%edx, %%edi, 1)		\n"
		"		movl		16(%%ebx, %%edi, 1), %%eax		\n"
		"		movl		20(%%ebx, %%edi, 1), %%esi		\n"
		"		movl		%%eax, 16(%%edx, %%edi, 1)		\n"
		"		movl		%%esi, 20(%%edx, %%edi, 1)		\n"
		"		movl		24(%%ebx, %%edi, 1), %%eax		\n"
		"		movl		28(%%ebx, %%edi, 1), %%esi		\n"
		"		movl		%%eax, 24(%%edx, %%edi, 1)		\n"
		"		movl		%%esi, 28(%%edx, %%edi, 1)		\n"
		"		subl		$32, %%edi						\n"
		"		jge		0b									\n"

		"		movl		%%ecx, %%edi		\n"
		"		andl		$0xfffffe00, %%edi	\n"
		"		addl		%%edi, %%ebx		\n"// increase src pointer
		"		addl		%%edi, %%edx		\n"// increase dst pointer
		"		andl		$31, %%ecx			\n"// new count
		"		jz		6f						\n"// if count = 0, get outta here

		"1:										\n"
		"		cmpl		$16, %%ecx			\n"
		"		jl		2f						\n"
		"		movl		(%%ebx), %%eax		\n"
		"		movl		%%eax, (%%edx)		\n"
		"		movl		4(%%ebx), %%eax		\n"
		"		movl		%%eax, 4(%%edx)		\n"
		"		movl		8(%%ebx), %%eax		\n"
		"		movl		%%eax, 8(%%edx)		\n"
		"		movl		12(%%ebx), %%eax	\n"
		"		movl		%%eax, 12(%%edx)	\n"
		"		subl		$16, %%ecx			\n"
		"		addl		$16, %%ebx			\n"
		"		addl		$16, %%edx			\n"
		"2:										\n"
		"		cmpl		$8, %%ecx			\n"
		"		jl		3f						\n"
		"		movl		(%%ebx), %%eax		\n"
		"		movl		%%eax, (%%edx)		\n"
		"		movl		4(%%ebx), %%eax		\n"
		"		subl		$8, %%ecx			\n"
		"		movl		%%eax, 4(%%edx)		\n"
		"		addl		$8, %%ebx			\n"
		"		addl		$8, %%edx			\n"
		"3:										\n"
		"		cmpl		$4, %%ecx			\n"
		"		jl		4f						\n"
		"		movl		(%%ebx), %%eax		\n"// here 4-7 bytes
		"		addl		$4, %%ebx			\n"
		"		subl		$4, %%ecx			\n"
		"		movl		%%eax, (%%edx)		\n"
		"		addl		$4, %%edx			\n"
		"4:										\n"// 0-3 remaining bytes
		"		cmpl		$2, %%ecx			\n"
		"		jl		5f						\n"
		"		movw		(%%ebx), %%ax		\n"// two bytes
		"		cmpl		$3, %%ecx			\n"// less than 3?
		"		movw		%%ax, (%%edx)		\n"
		"		jl		6f						\n"
		"		movb		2(%%ebx), %%al		\n"// last byte
		"		movb		%%al, 2(%%edx)		\n"
		"		jmp		6f						\n"
		"5:										\n"
		"		cmpl		$1, %%ecx			\n"
		"		jl		6f						\n"
		"		movb		(%%ebx), %%al		\n"
		"		movb		%%al, (%%edx)		\n"
		"6:										\n"
		"		popl		%%esi				\n"
		"		popl		%%edi				\n"
		: : "m" (src), "d" (dest), "c" (count)
		: "%eax", "%ebx", "%edi", "%esi", "cc", "memory");
}

void Com_Memset(void* dest, const int val, const size_t count)
{
	unsigned int fillval;

	if (count < 8)
	{
		__asm__ __volatile__ (
			//mov		edx,dest
			//mov		eax, val
			"			movb		%%al, %%ah		\n"
			"			movl		%%eax, %%ebx	\n"
			"			andl		$0xffff, %%ebx	\n"
			"			shll		$16, %%eax		\n"
			"			addl		%%ebx, %%eax	\n"// eax now contains pattern
			//mov		ecx,count
			"			cmpl		$4, %%ecx		\n"
			"			jl		0f					\n"
			"			movl		%%eax, (%%edx)	\n"// copy first dword
			"			addl		$4, %%edx		\n"
			"			subl		$4, %%ecx		\n"
			"	0:		cmpl		$2, %%ecx		\n"
			"			jl		1f					\n"
			"			movw		%%ax, (%%edx)	\n"// copy 2 bytes
			"			addl		$2, %%edx		\n"
			"			subl		$2, %%ecx		\n"
			"	1:		cmpl		$0, %%ecx		\n"
			"			je		2f					\n"
			"			movb		%%al, (%%edx)	\n"// copy single byte
			"	2:									\n"
			: : "d" (dest), "a" (val), "c" (count)
			: "%ebx", "%edi", "%esi", "cc", "memory");

		return;
	}

	fillval = val;

	fillval = fillval | (fillval << 8);
	fillval = fillval | (fillval << 16);		// fill dword with 8-bit pattern

	_copyDWord((unsigned int*)(dest),fillval, count / 4);

	__asm__ __volatile__ (	// padding of 0-3 bytes
		//mov		ecx,count
		"		movl		%%ecx, %%eax		\n"
		"		andl		$3, %%ecx			\n"
		"		jz		1f		\n"
		"		andl		$0xffffff00, %%eax	\n"
		//mov		ebx,dest
		"		addl		%%eax, %%edx		\n"
		"		movl		%0, %%eax			\n"
		"		cmpl		$2, %%ecx			\n"
		"		jl		0f						\n"
		"		movw		%%ax, (%%edx)		\n"
		"		cmpl		$2, %%ecx			\n"
		"		je		1f						\n"
		"		movb		%%al, 2(%%edx)		\n"
		"		jmp		1f						\n"
		"0:										\n"
		"		cmpl		$0, %%ecx			\n"
		"		je		1f						\n"
		"		movb		%%al, (%%edx)		\n"
		"1:										\n"
		: : "m" (fillval), "c" (count), "d" (dest)
		: "%eax", "%ebx", "%edi", "%esi", "cc", "memory");
}

void Com_Prefetch(const void* s, const unsigned int bytes, e_prefetch type)
{
	// write buffer prefetching is performed only if
	// the processor benefits from it. Read and read/write
	// prefetching is always performed.

	switch (type)
	{
	case PRE_WRITE: break;
	case PRE_READ:
	case PRE_READ_WRITE:

		__asm__ __volatile__ (
			//mov		ebx,s
			//mov		ecx,bytes
			"			cmpl		$4096, %%ecx	\n"// clamp to 4kB
			"			jle		0f					\n"
			"			movl		$4096, %%ecx	\n"
			"	0:									\n"
			"			addl		$0x1f, %%ecx	\n"
			"			shrl		$5, %%ecx		\n"// number of cache lines
			"			jz		2f					\n"
			"			jmp		1f					\n"

			"			.align 16					\n"
			"	1:		testb		%%al, (%%edx)	\n"
			"			addl		$32, %%edx		\n"
			"			decl		%%ecx			\n"
			"			jnz		1b					\n"
			"	2:									\n"
			: : "d" (s), "c" (bytes)
			: "%eax", "%ebx", "%edi", "%esi", "memory", "cc");

		break;
	}
}
#else
void Com_Memcpy(void* dest, const void* src, const size_t count)
{
	memcpy(dest, src, count);
}

void Com_Memset(void* dest, const int val, const size_t count)
{
	memset(dest, val, count);
}
#endif	// bk001208 - memset/memcpy assembly, Q_acos needed (RC4)

int COM_Argc()
{
	return com_argc;
}

const char* COM_Argv(int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
	{
		return "";
	}
	return com_argv[arg];
}

void COM_InitArgv(int argc, const char** argv)
{
	if (argc > MAX_NUM_ARGVS)
	{
		throw Exception("argc > MAX_NUM_ARGVS");
	}
	com_argc = argc;
	for (int i = 0; i < argc; i++)
	{
		if (!argv[i])	// || String::Length(argv[i]) >= MAX_TOKEN_CHARS)
		{
			com_argv[i] = "";
		}
		else
		{
			com_argv[i] = argv[i];
		}
	}
}

//	Adds the given string at the end of the current argument list
void COM_AddParm(const char* parm)
{
	if (com_argc == MAX_NUM_ARGVS)
	{
		throw Exception("COM_AddParm: MAX_NUM)ARGS");
	}
	com_argv[com_argc++] = parm;
}

void COM_ClearArgv(int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
	{
		return;
	}
	com_argv[arg] = "";
}

//	Returns the position (1 to argc-1) in the program's argument list
// where the given parameter apears, or 0 if not present
int COM_CheckParm(const char* parm)
{
	for (int i = 1; i < com_argc; i++)
	{
		if (!String::Cmp(parm, com_argv[i]))
		{
			return i;
		}
	}

	return 0;
}

void COM_InitCommonCvars()
{
	com_viewlog = Cvar_Get("viewlog", "0", CVAR_CHEAT);
	com_timescale = Cvar_Get("timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO);
	com_developer = Cvar_Get("developer", "0", CVAR_TEMP);
}

int Com_HashKey(const char* string, int maxlen)
{
	int hash = 0;
	for (int i = 0; i < maxlen && string[i] != '\0'; i++)
	{
		hash += string[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}
