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

#include "common_defs.h"	//	Basic types and defines
#include "../core/Common.h"
#include "../core/memory.h"		//	Memory allocation
#include "../core/endian.h"		//	Endianes handling
#include "../core/exception.h"	//	Exception handling
#include "../core/log.h"		//	General logging interface
#include "../core/array.h"		//	Dynamic array
#include "../core/strings.h"	//	Strings
#if 0
#include "info_string.h"
#include "mathlib.h"
#endif
#include "files.h"
#if 0
#include "command_buffer.h"
#include "console_variable.h"
#endif
#include "../core/crc.h"
#include "../core/md4.h"
#if 0
#include "system.h"
#include "clip_map/public.h"
#include "content_types.h"
#include "event_queue.h"
#include "socket.h"
#include "shareddefs.h"
#include "quakedefs.h"
#include "hexen2defs.h"
#include "quake2defs.h"
#endif
#include "quake3defs.h"
#if 0
#include "message.h"
#include "huffman.h"
#include "virtual_machine/public.h"
#include "player_move.h"
#include "network_channel.h"

int Com_Milliseconds();

extern Cvar* com_dedicated;
extern Cvar* com_viewlog;			// 0 = hidden, 1 = visible, 2 = minimized
extern Cvar* com_timescale;

extern Cvar* com_journal;

extern Cvar* com_developer;

extern fileHandle_t com_journalFile;
extern fileHandle_t com_journalDataFile;

extern int com_frameTime;
#endif

#endif
