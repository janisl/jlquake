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

#include "../core/common_defs.h"	//	Basic types and defines
#include "../core/Common.h"
#include "../core/memory.h"		//	Memory allocation
#include "../core/endian.h"		//	Endianes handling
#include "../core/exception.h"	//	Exception handling
#include "../core/log.h"		//	General logging interface
#include "../core/array.h"		//	Dynamic array
#include "../core/strings.h"	//	Strings
#include "../core/info_string.h"
#include "../core/mathlib.h"
#include "../core/files.h"
#include "../core/command_buffer.h"
#include "../core/console_variable.h"
#include "../core/crc.h"
#include "../core/md4.h"
#include "../core/system.h"
#include "../core/clip_map/public.h"
#include "../core/content_types.h"
#include "../core/event_queue.h"
#include "socket.h"
#include "../core/shareddefs.h"
#include "../core/quakedefs.h"
#include "../core/hexen2defs.h"
#include "../core/quake2defs.h"
#include "quake3defs.h"
#include "../core/message.h"
#include "../core/huffman.h"
#if 0
#include "virtual_machine/public.h"
#include "player_move.h"
#include "network_channel.h"

int Com_Milliseconds();
#endif

extern Cvar* com_dedicated;
extern Cvar* com_viewlog;			// 0 = hidden, 1 = visible, 2 = minimized
extern Cvar* com_timescale;

extern Cvar* com_journal;

extern Cvar* com_developer;

extern Cvar* com_crashed;

extern fileHandle_t com_journalFile;
extern fileHandle_t com_journalDataFile;

extern int com_frameTime;

#endif
