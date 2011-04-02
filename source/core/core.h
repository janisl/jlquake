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

#ifndef _CORE_H
#define _CORE_H

//	C headers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cmath>

#include "common.h"		//	Basic types and defines
#include "memory.h"		//	Memory allocation
#include "endian.h"		//	Endianes handling
#include "exception.h"	//	Exception handling
#include "log.h"		//	General logging interface
#include "array.h"		//	Dynamic array
#include "str.h"		//	Strings
#include "infostr.h"
#include "mathlib.h"
#include "message.h"
#include "huffman.h"
#include "files.h"
#include "cmd.h"
#include "cvar.h"
#include "crc.h"
#include "md4.h"
#include "system.h"
#include "cm_public.h"
#include "event_queue.h"

int Com_Milliseconds();

extern	QCvar	*com_journal;

extern	fileHandle_t	com_journalFile;
extern	fileHandle_t	com_journalDataFile;

#endif
