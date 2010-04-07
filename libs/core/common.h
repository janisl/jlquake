//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

#ifdef _WIN32
#pragma warning(disable : 4018)     // signed/unsigned mismatch
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
//#pragma warning(disable : 4305)		// truncation from const double to float
//#pragma warning(disable : 4310)		// cast truncates constant value
//#pragma warning(disable:  4505) 	// unreferenced local function has been removed
#pragma warning(disable : 4514)
#pragma warning(disable : 4702)		// unreachable code
#pragma warning(disable : 4711)		// selected for automatic inline expansion
#pragma warning(disable : 4220)		// varargs matches remaining parameters
#endif

#ifndef __GNUC__
#define __attribute__(whatever)
#endif

#ifndef _WIN32
#define __declspec(whatever)
#endif

#if (defined _M_IX86 || defined __i386__) && !defined C_ONLY && !defined __sun__
#define id386	1
#else
#define id386	0
#endif

//==========================================================================
//
//	Basic types
//
//==========================================================================

#define MIN_QINT8	((qint8)-128)
#define MIN_QINT16	((qint16)-32768)
#define MIN_QINT32	((qint32)-2147483648)

#define MAX_QINT8	((qint8)0x7f)
#define MAX_QINT16	((qint16)0x7fff)
#define MAX_QINT32	((qint32)0x7fffffff)

#define MAX_QUINT8	((quint8)0xff)
#define MAX_QUINT16	((quint16)0xffff)
#define MAX_QUINT32	((quint32)0xffffffff)

typedef unsigned char 		byte;

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
typedef int8_t				qint8;
typedef uint8_t				quint8;
typedef int16_t				qint16;
typedef uint16_t			quint16;
typedef int32_t				qint32;
typedef uint32_t			quint32;
#else
typedef char				qint8;
typedef unsigned char		quint8;
typedef short				qint16;
typedef unsigned short		quint16;
typedef int					qint32;
typedef unsigned int		quint32;
#endif

typedef int					qboolean;

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
//	QInterface
//
//==========================================================================

//
//  Base class for abstract classes that need virtual destructor.
//
class QInterface
{
public:
	virtual ~QInterface();
};

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		8192
#define	BIG_INFO_VALUE		8192

#define	ANGLE2SHORT(x)		((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)		((x)*(360.0/65536))
