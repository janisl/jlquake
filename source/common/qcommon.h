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

#ifndef _CORE_H
#define _CORE_H

//	C headers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cmath>

#ifdef _WIN32
#pragma warning(disable : 4018)		// signed/unsigned mismatch
#pragma warning(disable : 4032)
#pragma warning(disable : 4051)
#pragma warning(disable : 4057)		// slightly different base types
#pragma warning(disable : 4100)		// unreferenced formal parameter
#pragma warning(disable : 4115)
#pragma warning(disable : 4125)		// decimal digit terminates octal escape sequence
#pragma warning(disable : 4127)		// conditional expression is constant
#pragma warning(disable : 4136)
#pragma warning(disable : 4152)		// nonstandard extension, function/data pointer conversion in expression
//#pragma warning(disable : 4201)
//#pragma warning(disable : 4214)
#pragma warning(disable : 4244)
#pragma warning(disable : 4142)		// benign redefinition
#pragma warning(disable : 4305)		// truncation from const double to float
//#pragma warning(disable : 4310)		// cast truncates constant value
//#pragma warning(disable:  4505)   // unreferenced local function has been removed
#pragma warning(disable : 4514)
#pragma warning(disable : 4702)		// unreachable code
#pragma warning(disable : 4711)		// selected for automatic inline expansion
#pragma warning(disable : 4220)		// varargs matches remaining parameters
#pragma warning(disable : 4291)		// no matching operator delete found
#endif

#ifndef __GNUC__
#define id_attribute(whatever)
#else
#define id_attribute(params) __attribute__(params)
#endif

#ifndef _WIN32
#define __declspec(whatever)
#endif

#if (defined _M_IX86 || defined __i386__) && !defined C_ONLY && !defined __sun__
#define id386   1
#else
#define id386   0
#endif

#if (defined(powerc) || defined(powerpc) || defined(ppc) || defined(__ppc) || defined(__ppc__)) && !defined(C_ONLY)
#define idppc   1
#else
#define idppc   0
#endif

//==========================================================================
//
//	Basic types
//
//==========================================================================

#define MIN_QINT8   ((qint8) - 128)
#define MIN_QINT16  ((qint16) - 32768)
#define MIN_QINT32  ((qint32) - 2147483648)

#define MAX_QINT8   ((qint8)0x7f)
#define MAX_QINT16  ((qint16)0x7fff)
#define MAX_QINT32  ((qint32)0x7fffffff)

#define MAX_QUINT8  ((quint8)0xff)
#define MAX_QUINT16 ((quint16)0xffff)
#define MAX_QUINT32 ((quint32)0xffffffff)

typedef unsigned char byte;

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
typedef int8_t qint8;
typedef uint8_t quint8;
typedef int16_t qint16;
typedef uint16_t quint16;
typedef int32_t qint32;
typedef uint32_t quint32;
typedef intptr_t qintptr;
#else
typedef char qint8;
typedef unsigned char quint8;
typedef short qint16;
typedef unsigned short quint16;
typedef int qint32;
typedef unsigned int quint32;
#ifdef _WIN64
typedef long long qintptr;
#else
typedef int qintptr;
#endif
#endif

typedef int qboolean;

//==========================================================================
//
//	Basic templates
//
//==========================================================================

template<class T> T Min(T val1, T val2)
{
	return val1 < val2 ? val1 : val2;
}

template<class T> T Max(T val1, T val2)
{
	return val1 > val2 ? val1 : val2;
}

template<class T> T Clamp(T val, T low, T high)
{
	return val < low ? low : val > high ? high : val;
}

//==========================================================================
//
//	Interface
//
//==========================================================================

//
//  Base class for abstract classes that need virtual destructor.
//
class Interface
{
public:
	virtual ~Interface();
};

#define JLQUAKE_VERSION         1.0
#define JLQUAKE_VERSION_STRING  "1.0"

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS    1024	// max length of a string passed to Cmd_TokenizeString

// TTimo
// centralized and cleaned, that's the max string you can send to a common->Printf / common->DPrintf (above gets truncated)
#define MAXPRINTMSG 4096

#define ANGLE2SHORT(x)      ((int)((x) * 65536 / 360) & 65535)
#define SHORT2ANGLE(x)      ((x) * (360.0 / 65536))

#define random()    ((rand() & 0x7fff) / ((float)0x7fff))
#define crandom()   (2.0 * (random() - 0.5))

#define BIT(num)    (1 << (num))

//==========================================================================
//
//	Library method replacements.
//
//==========================================================================

void Com_Memset(void* dest, const int val, const size_t count);
void Com_Memcpy(void* dest, const void* src, const size_t count);

#define qassert(x)      if (x) {} else {common->FatalError("Assertion failed " #x); }

#include "Common.h"
#include "memory.h"		//	Memory allocation
#include "endian.h"		//	Endianes handling
#include "array.h"		//	Dynamic array
#include "strings.h"	//	Strings
#include "info_string.h"
#include "mathlib.h"
#include "files.h"
#include "command_buffer.h"
#include "console_variable.h"
#include "crc.h"
#include "md4.h"
#include "system.h"
#include "clip_map/public.h"
#include "content_types.h"
#include "event_queue.h"
#include "socket.h"
#include "shareddefs.h"
#include "quakedefs.h"
#include "hexen2defs.h"
#include "quake2defs.h"
#include "quake3defs.h"
#include "wolfdefs.h"
#include "message.h"
#include "huffman.h"
#include "virtual_machine/public.h"
#include "player_move.h"
#include "network_channel.h"
#include "message_utils.h"
#include "script.h"
#include "precompiler.h"
#include "game_utils.h"

#endif
