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
#include <time.h>
#include "../client/public.h"

#define MAX_NUM_ARGVS   50

Cvar* com_dedicated;
Cvar* com_viewlog;
Cvar* com_timescale;

Cvar* com_developer;

Cvar* com_crashed = NULL;	// ydnar: set in case of a crash, prevents CVAR_UNSAFE variables from being set from a cfg

Cvar* com_sv_running;
Cvar* com_cl_running;
Cvar* cl_paused;
Cvar* sv_paused;

Cvar* cl_shownet;

Cvar* qh_registered;

Cvar* com_dropsim;			// 0.0 to 1.0, simulated packet drops

Cvar* comt3_version;

int GGameType;

int com_frameTime;

int cl_connectedToPureServer;

// Arnout: gameinfo, to let the engine know which gametypes are SP and if we should use profiles.
// This can't be dependant on gamecode as we sometimes need to know about it when no game-modules
// are loaded
etgameInfo_t comet_gameInfo;

static int com_argc;
static const char* com_argv[MAX_NUM_ARGVS + 1];

bool q1_standard_quake = true;
bool q1_rogue;
bool q1_hipnotic;

char* rd_buffer;
int rd_buffersize;
void (* rd_flush)(char* buffer);

bool com_errorEntered;

// com_speeds times
Cvar* com_speeds;
int t3time_game;
int time_before_game;
int time_after_game;
int time_frontend;			// renderer frontend time
int time_backend;			// renderer backend time
int time_before_ref;
int time_after_ref;

static int q2server_state;

#define MAX_CONSOLE_LINES   32
static int com_numConsoleLines;
static char* com_consoleLines[MAX_CONSOLE_LINES];

bool com_fullyInitialized;

static fileHandle_t logfile_;
static Cvar* com_logfile;			// 1 = buffer log, 2 = flush after each print

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
		common->FatalError("argc > MAX_NUM_ARGVS");
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
		common->FatalError("COM_AddParm: MAX_NUM)ARGS");
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
	com_logfile = Cvar_Get("logfile", "0", CVAR_TEMP);
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

int Com_RealTime(qtime_t* qtime)
{
	time_t t = time(NULL);
	if (!qtime)
	{
		return t;
	}
	tm* tms = localtime(&t);
	if (tms)
	{
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return t;
}

static byte qw_chktbl[1024 + 4] = {
	0x78,0xd2,0x94,0xe3,0x41,0xec,0xd6,0xd5,0xcb,0xfc,0xdb,0x8a,0x4b,0xcc,0x85,0x01,
	0x23,0xd2,0xe5,0xf2,0x29,0xa7,0x45,0x94,0x4a,0x62,0xe3,0xa5,0x6f,0x3f,0xe1,0x7a,
	0x64,0xed,0x5c,0x99,0x29,0x87,0xa8,0x78,0x59,0x0d,0xaa,0x0f,0x25,0x0a,0x5c,0x58,
	0xfb,0x00,0xa7,0xa8,0x8a,0x1d,0x86,0x80,0xc5,0x1f,0xd2,0x28,0x69,0x71,0x58,0xc3,
	0x51,0x90,0xe1,0xf8,0x6a,0xf3,0x8f,0xb0,0x68,0xdf,0x95,0x40,0x5c,0xe4,0x24,0x6b,
	0x29,0x19,0x71,0x3f,0x42,0x63,0x6c,0x48,0xe7,0xad,0xa8,0x4b,0x91,0x8f,0x42,0x36,
	0x34,0xe7,0x32,0x55,0x59,0x2d,0x36,0x38,0x38,0x59,0x9b,0x08,0x16,0x4d,0x8d,0xf8,
	0x0a,0xa4,0x52,0x01,0xbb,0x52,0xa9,0xfd,0x40,0x18,0x97,0x37,0xff,0xc9,0x82,0x27,
	0xb2,0x64,0x60,0xce,0x00,0xd9,0x04,0xf0,0x9e,0x99,0xbd,0xce,0x8f,0x90,0x4a,0xdd,
	0xe1,0xec,0x19,0x14,0xb1,0xfb,0xca,0x1e,0x98,0x0f,0xd4,0xcb,0x80,0xd6,0x05,0x63,
	0xfd,0xa0,0x74,0xa6,0x86,0xf6,0x19,0x98,0x76,0x27,0x68,0xf7,0xe9,0x09,0x9a,0xf2,
	0x2e,0x42,0xe1,0xbe,0x64,0x48,0x2a,0x74,0x30,0xbb,0x07,0xcc,0x1f,0xd4,0x91,0x9d,
	0xac,0x55,0x53,0x25,0xb9,0x64,0xf7,0x58,0x4c,0x34,0x16,0xbc,0xf6,0x12,0x2b,0x65,
	0x68,0x25,0x2e,0x29,0x1f,0xbb,0xb9,0xee,0x6d,0x0c,0x8e,0xbb,0xd2,0x5f,0x1d,0x8f,
	0xc1,0x39,0xf9,0x8d,0xc0,0x39,0x75,0xcf,0x25,0x17,0xbe,0x96,0xaf,0x98,0x9f,0x5f,
	0x65,0x15,0xc4,0x62,0xf8,0x55,0xfc,0xab,0x54,0xcf,0xdc,0x14,0x06,0xc8,0xfc,0x42,
	0xd3,0xf0,0xad,0x10,0x08,0xcd,0xd4,0x11,0xbb,0xca,0x67,0xc6,0x48,0x5f,0x9d,0x59,
	0xe3,0xe8,0x53,0x67,0x27,0x2d,0x34,0x9e,0x9e,0x24,0x29,0xdb,0x69,0x99,0x86,0xf9,
	0x20,0xb5,0xbb,0x5b,0xb0,0xf9,0xc3,0x67,0xad,0x1c,0x9c,0xf7,0xcc,0xef,0xce,0x69,
	0xe0,0x26,0x8f,0x79,0xbd,0xca,0x10,0x17,0xda,0xa9,0x88,0x57,0x9b,0x15,0x24,0xba,
	0x84,0xd0,0xeb,0x4d,0x14,0xf5,0xfc,0xe6,0x51,0x6c,0x6f,0x64,0x6b,0x73,0xec,0x85,
	0xf1,0x6f,0xe1,0x67,0x25,0x10,0x77,0x32,0x9e,0x85,0x6e,0x69,0xb1,0x83,0x00,0xe4,
	0x13,0xa4,0x45,0x34,0x3b,0x40,0xff,0x41,0x82,0x89,0x79,0x57,0xfd,0xd2,0x8e,0xe8,
	0xfc,0x1d,0x19,0x21,0x12,0x00,0xd7,0x66,0xe5,0xc7,0x10,0x1d,0xcb,0x75,0xe8,0xfa,
	0xb6,0xee,0x7b,0x2f,0x1a,0x25,0x24,0xb9,0x9f,0x1d,0x78,0xfb,0x84,0xd0,0x17,0x05,
	0x71,0xb3,0xc8,0x18,0xff,0x62,0xee,0xed,0x53,0xab,0x78,0xd3,0x65,0x2d,0xbb,0xc7,
	0xc1,0xe7,0x70,0xa2,0x43,0x2c,0x7c,0xc7,0x16,0x04,0xd2,0x45,0xd5,0x6b,0x6c,0x7a,
	0x5e,0xa1,0x50,0x2e,0x31,0x5b,0xcc,0xe8,0x65,0x8b,0x16,0x85,0xbf,0x82,0x83,0xfb,
	0xde,0x9f,0x36,0x48,0x32,0x79,0xd6,0x9b,0xfb,0x52,0x45,0xbf,0x43,0xf7,0x0b,0x0b,
	0x19,0x19,0x31,0xc3,0x85,0xec,0x1d,0x8c,0x20,0xf0,0x3a,0xfa,0x80,0x4d,0x2c,0x7d,
	0xac,0x60,0x09,0xc0,0x40,0xee,0xb9,0xeb,0x13,0x5b,0xe8,0x2b,0xb1,0x20,0xf0,0xce,
	0x4c,0xbd,0xc6,0x04,0x86,0x70,0xc6,0x33,0xc3,0x15,0x0f,0x65,0x19,0xfd,0xc2,0xd3,

// map checksum goes here
	0x00,0x00,0x00,0x00
};

//	For proxy protecting
byte COMQW_BlockSequenceCRCByte(byte* base, int length, int sequence)
{
	byte* p = qw_chktbl + (sequence % (sizeof(qw_chktbl) - 8));

	if (length > 60)
	{
		length = 60;
	}
	byte chkb[60 + 4];
	Com_Memcpy(chkb, base, length);

	chkb[length] = (sequence & 0xff) ^ p[0];
	chkb[length + 1] = p[1];
	chkb[length + 2] = ((sequence >> 8) & 0xff) ^ p[2];
	chkb[length + 3] = p[3];

	length += 4;

	unsigned short crc = CRC_Block(chkb, length);

	crc &= 0xff;

	return crc;
}

static byte q2_chktbl[1024] = {
	0x84, 0x47, 0x51, 0xc1, 0x93, 0x22, 0x21, 0x24, 0x2f, 0x66, 0x60, 0x4d, 0xb0, 0x7c, 0xda,
	0x88, 0x54, 0x15, 0x2b, 0xc6, 0x6c, 0x89, 0xc5, 0x9d, 0x48, 0xee, 0xe6, 0x8a, 0xb5, 0xf4,
	0xcb, 0xfb, 0xf1, 0x0c, 0x2e, 0xa0, 0xd7, 0xc9, 0x1f, 0xd6, 0x06, 0x9a, 0x09, 0x41, 0x54,
	0x67, 0x46, 0xc7, 0x74, 0xe3, 0xc8, 0xb6, 0x5d, 0xa6, 0x36, 0xc4, 0xab, 0x2c, 0x7e, 0x85,
	0xa8, 0xa4, 0xa6, 0x4d, 0x96, 0x19, 0x19, 0x9a, 0xcc, 0xd8, 0xac, 0x39, 0x5e, 0x3c, 0xf2,
	0xf5, 0x5a, 0x72, 0xe5, 0xa9, 0xd1, 0xb3, 0x23, 0x82, 0x6f, 0x29, 0xcb, 0xd1, 0xcc, 0x71,
	0xfb, 0xea, 0x92, 0xeb, 0x1c, 0xca, 0x4c, 0x70, 0xfe, 0x4d, 0xc9, 0x67, 0x43, 0x47, 0x94,
	0xb9, 0x47, 0xbc, 0x3f, 0x01, 0xab, 0x7b, 0xa6, 0xe2, 0x76, 0xef, 0x5a, 0x7a, 0x29, 0x0b,
	0x51, 0x54, 0x67, 0xd8, 0x1c, 0x14, 0x3e, 0x29, 0xec, 0xe9, 0x2d, 0x48, 0x67, 0xff, 0xed,
	0x54, 0x4f, 0x48, 0xc0, 0xaa, 0x61, 0xf7, 0x78, 0x12, 0x03, 0x7a, 0x9e, 0x8b, 0xcf, 0x83,
	0x7b, 0xae, 0xca, 0x7b, 0xd9, 0xe9, 0x53, 0x2a, 0xeb, 0xd2, 0xd8, 0xcd, 0xa3, 0x10, 0x25,
	0x78, 0x5a, 0xb5, 0x23, 0x06, 0x93, 0xb7, 0x84, 0xd2, 0xbd, 0x96, 0x75, 0xa5, 0x5e, 0xcf,
	0x4e, 0xe9, 0x50, 0xa1, 0xe6, 0x9d, 0xb1, 0xe3, 0x85, 0x66, 0x28, 0x4e, 0x43, 0xdc, 0x6e,
	0xbb, 0x33, 0x9e, 0xf3, 0x0d, 0x00, 0xc1, 0xcf, 0x67, 0x34, 0x06, 0x7c, 0x71, 0xe3, 0x63,
	0xb7, 0xb7, 0xdf, 0x92, 0xc4, 0xc2, 0x25, 0x5c, 0xff, 0xc3, 0x6e, 0xfc, 0xaa, 0x1e, 0x2a,
	0x48, 0x11, 0x1c, 0x36, 0x68, 0x78, 0x86, 0x79, 0x30, 0xc3, 0xd6, 0xde, 0xbc, 0x3a, 0x2a,
	0x6d, 0x1e, 0x46, 0xdd, 0xe0, 0x80, 0x1e, 0x44, 0x3b, 0x6f, 0xaf, 0x31, 0xda, 0xa2, 0xbd,
	0x77, 0x06, 0x56, 0xc0, 0xb7, 0x92, 0x4b, 0x37, 0xc0, 0xfc, 0xc2, 0xd5, 0xfb, 0xa8, 0xda,
	0xf5, 0x57, 0xa8, 0x18, 0xc0, 0xdf, 0xe7, 0xaa, 0x2a, 0xe0, 0x7c, 0x6f, 0x77, 0xb1, 0x26,
	0xba, 0xf9, 0x2e, 0x1d, 0x16, 0xcb, 0xb8, 0xa2, 0x44, 0xd5, 0x2f, 0x1a, 0x79, 0x74, 0x87,
	0x4b, 0x00, 0xc9, 0x4a, 0x3a, 0x65, 0x8f, 0xe6, 0x5d, 0xe5, 0x0a, 0x77, 0xd8, 0x1a, 0x14,
	0x41, 0x75, 0xb1, 0xe2, 0x50, 0x2c, 0x93, 0x38, 0x2b, 0x6d, 0xf3, 0xf6, 0xdb, 0x1f, 0xcd,
	0xff, 0x14, 0x70, 0xe7, 0x16, 0xe8, 0x3d, 0xf0, 0xe3, 0xbc, 0x5e, 0xb6, 0x3f, 0xcc, 0x81,
	0x24, 0x67, 0xf3, 0x97, 0x3b, 0xfe, 0x3a, 0x96, 0x85, 0xdf, 0xe4, 0x6e, 0x3c, 0x85, 0x05,
	0x0e, 0xa3, 0x2b, 0x07, 0xc8, 0xbf, 0xe5, 0x13, 0x82, 0x62, 0x08, 0x61, 0x69, 0x4b, 0x47,
	0x62, 0x73, 0x44, 0x64, 0x8e, 0xe2, 0x91, 0xa6, 0x9a, 0xb7, 0xe9, 0x04, 0xb6, 0x54, 0x0c,
	0xc5, 0xa9, 0x47, 0xa6, 0xc9, 0x08, 0xfe, 0x4e, 0xa6, 0xcc, 0x8a, 0x5b, 0x90, 0x6f, 0x2b,
	0x3f, 0xb6, 0x0a, 0x96, 0xc0, 0x78, 0x58, 0x3c, 0x76, 0x6d, 0x94, 0x1a, 0xe4, 0x4e, 0xb8,
	0x38, 0xbb, 0xf5, 0xeb, 0x29, 0xd8, 0xb0, 0xf3, 0x15, 0x1e, 0x99, 0x96, 0x3c, 0x5d, 0x63,
	0xd5, 0xb1, 0xad, 0x52, 0xb8, 0x55, 0x70, 0x75, 0x3e, 0x1a, 0xd5, 0xda, 0xf6, 0x7a, 0x48,
	0x7d, 0x44, 0x41, 0xf9, 0x11, 0xce, 0xd7, 0xca, 0xa5, 0x3d, 0x7a, 0x79, 0x7e, 0x7d, 0x25,
	0x1b, 0x77, 0xbc, 0xf7, 0xc7, 0x0f, 0x84, 0x95, 0x10, 0x92, 0x67, 0x15, 0x11, 0x5a, 0x5e,
	0x41, 0x66, 0x0f, 0x38, 0x03, 0xb2, 0xf1, 0x5d, 0xf8, 0xab, 0xc0, 0x02, 0x76, 0x84, 0x28,
	0xf4, 0x9d, 0x56, 0x46, 0x60, 0x20, 0xdb, 0x68, 0xa7, 0xbb, 0xee, 0xac, 0x15, 0x01, 0x2f,
	0x20, 0x09, 0xdb, 0xc0, 0x16, 0xa1, 0x89, 0xf9, 0x94, 0x59, 0x00, 0xc1, 0x76, 0xbf, 0xc1,
	0x4d, 0x5d, 0x2d, 0xa9, 0x85, 0x2c, 0xd6, 0xd3, 0x14, 0xcc, 0x02, 0xc3, 0xc2, 0xfa, 0x6b,
	0xb7, 0xa6, 0xef, 0xdd, 0x12, 0x26, 0xa4, 0x63, 0xe3, 0x62, 0xbd, 0x56, 0x8a, 0x52, 0x2b,
	0xb9, 0xdf, 0x09, 0xbc, 0x0e, 0x97, 0xa9, 0xb0, 0x82, 0x46, 0x08, 0xd5, 0x1a, 0x8e, 0x1b,
	0xa7, 0x90, 0x98, 0xb9, 0xbb, 0x3c, 0x17, 0x9a, 0xf2, 0x82, 0xba, 0x64, 0x0a, 0x7f, 0xca,
	0x5a, 0x8c, 0x7c, 0xd3, 0x79, 0x09, 0x5b, 0x26, 0xbb, 0xbd, 0x25, 0xdf, 0x3d, 0x6f, 0x9a,
	0x8f, 0xee, 0x21, 0x66, 0xb0, 0x8d, 0x84, 0x4c, 0x91, 0x45, 0xd4, 0x77, 0x4f, 0xb3, 0x8c,
	0xbc, 0xa8, 0x99, 0xaa, 0x19, 0x53, 0x7c, 0x02, 0x87, 0xbb, 0x0b, 0x7c, 0x1a, 0x2d, 0xdf,
	0x48, 0x44, 0x06, 0xd6, 0x7d, 0x0c, 0x2d, 0x35, 0x76, 0xae, 0xc4, 0x5f, 0x71, 0x85, 0x97,
	0xc4, 0x3d, 0xef, 0x52, 0xbe, 0x00, 0xe4, 0xcd, 0x49, 0xd1, 0xd1, 0x1c, 0x3c, 0xd0, 0x1c,
	0x42, 0xaf, 0xd4, 0xbd, 0x58, 0x34, 0x07, 0x32, 0xee, 0xb9, 0xb5, 0xea, 0xff, 0xd7, 0x8c,
	0x0d, 0x2e, 0x2f, 0xaf, 0x87, 0xbb, 0xe6, 0x52, 0x71, 0x22, 0xf5, 0x25, 0x17, 0xa1, 0x82,
	0x04, 0xc2, 0x4a, 0xbd, 0x57, 0xc6, 0xab, 0xc8, 0x35, 0x0c, 0x3c, 0xd9, 0xc2, 0x43, 0xdb,
	0x27, 0x92, 0xcf, 0xb8, 0x25, 0x60, 0xfa, 0x21, 0x3b, 0x04, 0x52, 0xc8, 0x96, 0xba, 0x74,
	0xe3, 0x67, 0x3e, 0x8e, 0x8d, 0x61, 0x90, 0x92, 0x59, 0xb6, 0x1a, 0x1c, 0x5e, 0x21, 0xc1,
	0x65, 0xe5, 0xa6, 0x34, 0x05, 0x6f, 0xc5, 0x60, 0xb1, 0x83, 0xc1, 0xd5, 0xd5, 0xed, 0xd9,
	0xc7, 0x11, 0x7b, 0x49, 0x7a, 0xf9, 0xf9, 0x84, 0x47, 0x9b, 0xe2, 0xa5, 0x82, 0xe0, 0xc2,
	0x88, 0xd0, 0xb2, 0x58, 0x88, 0x7f, 0x45, 0x09, 0x67, 0x74, 0x61, 0xbf, 0xe6, 0x40, 0xe2,
	0x9d, 0xc2, 0x47, 0x05, 0x89, 0xed, 0xcb, 0xbb, 0xb7, 0x27, 0xe7, 0xdc, 0x7a, 0xfd, 0xbf,
	0xa8, 0xd0, 0xaa, 0x10, 0x39, 0x3c, 0x20, 0xf0, 0xd3, 0x6e, 0xb1, 0x72, 0xf8, 0xe6, 0x0f,
	0xef, 0x37, 0xe5, 0x09, 0x33, 0x5a, 0x83, 0x43, 0x80, 0x4f, 0x65, 0x2f, 0x7c, 0x8c, 0x6a,
	0xa0, 0x82, 0x0c, 0xd4, 0xd4, 0xfa, 0x81, 0x60, 0x3d, 0xdf, 0x06, 0xf1, 0x5f, 0x08, 0x0d,
	0x6d, 0x43, 0xf2, 0xe3, 0x11, 0x7d, 0x80, 0x32, 0xc5, 0xfb, 0xc5, 0xd9, 0x27, 0xec, 0xc6,
	0x4e, 0x65, 0x27, 0x76, 0x87, 0xa6, 0xee, 0xee, 0xd7, 0x8b, 0xd1, 0xa0, 0x5c, 0xb0, 0x42,
	0x13, 0x0e, 0x95, 0x4a, 0xf2, 0x06, 0xc6, 0x43, 0x33, 0xf4, 0xc7, 0xf8, 0xe7, 0x1f, 0xdd,
	0xe4, 0x46, 0x4a, 0x70, 0x39, 0x6c, 0xd0, 0xed, 0xca, 0xbe, 0x60, 0x3b, 0xd1, 0x7b, 0x57,
	0x48, 0xe5, 0x3a, 0x79, 0xc1, 0x69, 0x33, 0x53, 0x1b, 0x80, 0xb8, 0x91, 0x7d, 0xb4, 0xf6,
	0x17, 0x1a, 0x1d, 0x5a, 0x32, 0xd6, 0xcc, 0x71, 0x29, 0x3f, 0x28, 0xbb, 0xf3, 0x5e, 0x71,
	0xb8, 0x43, 0xaf, 0xf8, 0xb9, 0x64, 0xef, 0xc4, 0xa5, 0x6c, 0x08, 0x53, 0xc7, 0x00, 0x10,
	0x39, 0x4f, 0xdd, 0xe4, 0xb6, 0x19, 0x27, 0xfb, 0xb8, 0xf5, 0x32, 0x73, 0xe5, 0xcb, 0x32
};

//	For proxy protecting
byte COMQ2_BlockSequenceCRCByte(byte* base, int length, int sequence)
{
	if (sequence < 0)
	{
		common->FatalError("sequence < 0, this shouldn't happen\n");
	}

	byte* p = q2_chktbl + (sequence % (sizeof(q2_chktbl) - 4));

	if (length > 60)
	{
		length = 60;
	}
	byte chkb[60 + 4];
	Com_Memcpy(chkb, base, length);

	chkb[length] = p[0];
	chkb[length + 1] = p[1];
	chkb[length + 2] = p[2];
	chkb[length + 3] = p[3];

	length += 4;

	unsigned short crc = CRC_Block(chkb, length);

	int x = 0;
	for (int n = 0; n < length; n++)
	{
		x += chkb[n];
	}

	crc = (crc ^ x) & 0xff;

	return crc;
}

void Com_BeginRedirect(char* buffer, int buffersize, void (*flush)(char*))
{
	if (!buffer || !buffersize || !flush)
	{
		return;
	}
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect(void)
{
	if (rd_flush)
	{
		rd_flush(rd_buffer);
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

int ComQ2_ServerState()
{
	return q2server_state;
}

void ComQ2_SetServerState(int state)
{
	q2server_state = state;
}

/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

//	Break it up into multiple console lines
void Com_ParseCommandLine(char* commandLine)
{
	int inq = 0;
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while (*commandLine)
	{
		if (*commandLine == '"')
		{
			inq = !inq;
		}
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if ((*commandLine == '+' && !inq) || *commandLine == '\n' || *commandLine == '\r')
		{
			if (com_numConsoleLines == MAX_CONSOLE_LINES)
			{
				return;
			}
			com_consoleLines[com_numConsoleLines] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}

//	Check for "safe" on the command line, which will
// skip loading of q3config.cfg
bool Com_SafeMode()
{
	for (int i = 0; i < com_numConsoleLines; i++)
	{
		Cmd_TokenizeString(com_consoleLines[i]);
		if (!String::ICmp(Cmd_Argv(0), "safe") ||
			!String::ICmp(Cmd_Argv(0), "cvar_restart"))
		{
			com_consoleLines[i][0] = 0;
			return true;
		}
	}
	return false;
}

//	Adds command line parameters as script statements
// Commands are seperated by + signs
//	Returns true if any late commands were added, which
// will keep the demoloop from immediately starting
bool Com_AddStartupCommands()
{
	bool added = false;
	// quote every token, so args with semicolons can work
	for (int i = 0; i < com_numConsoleLines; i++)
	{
		if (!com_consoleLines[i] || !com_consoleLines[i][0])
		{
			continue;
		}

		// set commands won't override menu startup
		if (String::NICmp(com_consoleLines[i], "set", 3))
		{
			added = true;
		}
		Cbuf_AddText(com_consoleLines[i]);
		Cbuf_AddText("\n");
	}

	return added;
}

//	Searches for command line parameters that are set commands.
// If match is not NULL, only that cvar will be looked for.
// That is necessary because cddir and basedir need to be set
// before the filesystem is started, but all other sets shouls
// be after execing the config and default.
void Com_StartupVariable(const char* match)
{
	for (int i = 0; i < com_numConsoleLines; i++)
	{
		Cmd_TokenizeString(com_consoleLines[i]);
		if (String::Cmp(Cmd_Argv(0), "set"))
		{
			continue;
		}

		const char* s = Cmd_Argv(1);
		if (!match || !String::Cmp(s, match))
		{
			Cvar_Set(s, Cmd_Argv(2));
			Cvar* cv = Cvar_Get(s, "", 0);
			cv->flags |= CVAR_USER_CREATED;
//			com_consoleLines[i] = 0;
		}
	}
}

void Com_WriteConfigToFile(const char* filename)
{
	fileHandle_t f = FS_FOpenFileWrite(filename);
	if (!f)
	{
		common->Printf("Couldn't write %s.\n", filename);
		return;
	}

	FS_Printf(f, "// generated by quake, do not modify\n");
	Key_WriteBindings(f);
	Cvar_WriteVariables(f);
	FS_FCloseFile(f);
}

//	Writes key bindings and archived cvars to config file if modified
void Com_WriteConfiguration()
{
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if (!com_fullyInitialized)
	{
		return;
	}

	if (!(GGameType & GAME_Tech3) && com_dedicated->integer)
	{
		return;
	}

	if (!(cvar_modifiedFlags & CVAR_ARCHIVE))
	{
		return;
	}
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	if (GGameType & GAME_Quake3)
	{
		Com_WriteConfigToFile("q3config.cfg");
	}
	else if (GGameType & GAME_WolfSP)
	{
		Com_WriteConfigToFile("wolfconfig.cfg");
	}
	else if (GGameType & GAME_WolfMP)
	{
		Com_WriteConfigToFile("wolfconfig_mp.cfg");
	}
	else if (GGameType & GAME_ET)
	{
		const char* cl_profileStr = Cvar_VariableString("cl_profile");
		if (comet_gameInfo.usesProfiles && cl_profileStr[0])
		{
			Com_WriteConfigToFile(va("profiles/%s/%s", cl_profileStr, ETCONFIG_NAME));
		}
		else
		{
			Com_WriteConfigToFile(ETCONFIG_NAME);
		}
	}
	else
	{
		Com_WriteConfigToFile("config.cfg");
	}

	if (GGameType & GAME_Tech3)
	{
		CLT3_WriteCDKey();
	}
}

//	Write the config file to a specific name
void Com_WriteConfig_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: writeconfig <filename>\n");
		return;
	}

	char filename[MAX_QPATH];
	String::NCpyZ(filename, Cmd_Argv(1), sizeof(filename));
	String::DefaultExtension(filename, sizeof(filename), ".cfg");
	common->Printf("Writing %s.\n", filename);
	Com_WriteConfigToFile(filename);
}

void Com_SetRecommended(bool vid_restart)
{
	common->Printf("Assume high quality video and fast CPU\n");
	if (GGameType & GAME_ET)
	{
		Cvar_Get("com_recommended", "-1", CVAR_ARCHIVE);
		Cbuf_AddText("exec preset_high.cfg\n");
		Cvar_Set("com_recommended", "0");
	}
	else
	{
		Cbuf_AddText("exec highVidhighCPU.cfg\n");
	}

	if (GGameType & GAME_WolfSP)
	{
		// (SA) set the cvar so the menu will reflect this on first run
		Cvar_Set("ui_glCustom", "999");		// 'recommended'
	}

	if (vid_restart)
	{
		Cbuf_AddText("vid_restart\n");
	}
}

void Com_LogToFile(const char* msg)
{
	if (com_logfile && com_logfile->integer)
	{
		// TTimo: only open the qconsole.log if the filesystem is in an initialized state
		//   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		static bool opening_qconsole = false;
		if (!logfile_ && FS_Initialized() && !opening_qconsole)
		{
			opening_qconsole = true;

			time_t aclock;
			time(&aclock);
			tm* newtime = localtime(&aclock);

			logfile_ = FS_FOpenFileWrite("qconsole.log");
			common->Printf("logfile opened on %s\n", asctime(newtime));
			if (com_logfile->integer > 1)
			{
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush(logfile_);
			}

			opening_qconsole = false;
		}
		if (logfile_ && FS_Initialized())
		{
			FS_Write(msg, String::Length(msg), logfile_);
		}
	}
}

void Com_Shutdown()
{
	if (GGameType & GAME_ET)
	{
		// delete pid file
		const char* cl_profileStr = Cvar_VariableString("cl_profile");
		if (comet_gameInfo.usesProfiles && cl_profileStr[0])
		{
			if (FS_FileExists(va("profiles/%s/profile.pid", cl_profileStr)))
			{
				FS_Delete(va("profiles/%s/profile.pid", cl_profileStr));
			}
		}
	}

	if (logfile_)
	{
		FS_FCloseFile(logfile_);
		logfile_ = 0;
	}

	if (com_journalFile)
	{
		FS_FCloseFile(com_journalFile);
		com_journalFile = 0;
	}
}
