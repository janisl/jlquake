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

//==========================================================================
//
//	Command line arguments
//
//==========================================================================

int COM_Argc();
const char* COM_Argv(int arg);	// range and null checked
void COM_InitArgv(int argc, const char** argv);
void COM_AddParm(const char* parm);
void COM_ClearArgv(int arg);
int COM_CheckParm(const char* parm);

//==========================================================================
//
//	Which game are we playing
//
//==========================================================================

enum
{
	GAME_Quake          = 0x01,
	GAME_Hexen2         = 0x02,
	GAME_Quake2         = 0x04,
	GAME_Quake3         = 0x08,
	GAME_WolfSP         = 0x10,
	GAME_WolfMP         = 0x20,
	GAME_ET             = 0x40,
	//	Aditional flags
	GAME_QuakeWorld     = 0x80,
	GAME_HexenWorld     = 0x100,
	GAME_H2Portals      = 0x200,

	//	Combinations
	GAME_QuakeHexen     = GAME_Quake | GAME_Hexen2,
	GAME_Tech3          = GAME_Quake3 | GAME_WolfSP | GAME_WolfMP | GAME_ET,
};

extern int GGameType;

//==========================================================================
//
//	Common cvars.
//
//==========================================================================

void COM_InitCommonCvars();

int Com_HashKey(const char* string, int maxlen);

// real time
struct qtime_t
{
	int tm_sec;		/* seconds after the minute - [0,59] */
	int tm_min;		/* minutes after the hour - [0,59] */
	int tm_hour;	/* hours since midnight - [0,23] */
	int tm_mday;	/* day of the month - [1,31] */
	int tm_mon;		/* months since January - [0,11] */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* days since Sunday - [0,6] */
	int tm_yday;	/* days since January 1 - [0,365] */
	int tm_isdst;	/* daylight savings time flag */
};

int Com_RealTime(qtime_t* qtime);

byte COMQW_BlockSequenceCRCByte(byte* base, int length, int sequence);
byte COMQ2_BlockSequenceCRCByte(byte* base, int length, int sequence);

extern char* rd_buffer;
extern int rd_buffersize;
extern void (* rd_flush)(char* buffer);

void Com_BeginRedirect(char* buffer, int buffersize, void (*flush)(char*));
void Com_EndRedirect();

extern bool com_errorEntered;

int ComQ2_ServerState();		// this should have just been a cvar...
void ComQ2_SetServerState(int state);

// centralizing the declarations for comt3_cdkey
extern char comt3_cdkey[34];
