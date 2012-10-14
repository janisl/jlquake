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
#include "Common.h"
#include "memory.h"		//	Memory allocation
#include "endian.h"		//	Endianes handling
#include "exception.h"	//	Exception handling
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

char* __CopyString(const char* in);

extern Cvar* com_dedicated;
extern Cvar* com_viewlog;			// 0 = hidden, 1 = visible, 2 = minimized
extern Cvar* com_timescale;

extern Cvar* com_journal;

extern Cvar* com_developer;

extern Cvar* com_crashed;

extern Cvar* cl_shownet;

extern Cvar* qh_registered;

extern Cvar* com_sv_running;
extern Cvar* com_cl_running;
extern Cvar* cl_paused;
extern Cvar* sv_paused;

extern Cvar* com_dropsim;			// 0.0 to 1.0, simulated packet drops

extern fileHandle_t com_journalFile;
extern fileHandle_t com_journalDataFile;

extern int com_frameTime;

extern int cl_connectedToPureServer;

extern etgameInfo_t comet_gameInfo;

extern bool q1_standard_quake;
extern bool q1_rogue;
extern bool q1_hipnotic;

// com_speeds times
extern Cvar* com_speeds;
extern int t3time_game;
extern int time_before_game;
extern int time_after_game;
extern int time_frontend;
extern int time_backend;			// renderer backend time

void FS_Restart(int checksumFeed);
bool CL_WWWBadChecksum(const char* pakname);
void Com_Quit_f();
void CLT3_PacketEvent(netadr_t from, QMsg* msg);

#endif
