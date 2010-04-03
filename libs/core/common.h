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
