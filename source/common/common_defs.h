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

#ifndef __COMMON_DEFS_H__
#define __COMMON_DEFS_H__

#include "console_variable.h"
#include "wolfdefs.h"

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

char* __CopyString(const char* in);

extern Cvar* com_dedicated;
extern Cvar* com_viewlog;			// 0 = hidden, 1 = visible, 2 = minimized
extern Cvar* com_timescale;

extern Cvar* com_developer;
#if defined(_DEBUG)
extern Cvar* com_noErrorInterrupt;
#endif

extern Cvar* com_crashed;

extern Cvar* cl_shownet;

extern Cvar* qh_registered;

extern Cvar* com_sv_running;
extern Cvar* com_cl_running;
extern Cvar* cl_paused;
extern Cvar* sv_paused;

extern Cvar* com_dropsim;			// 0.0 to 1.0, simulated packet drops

extern Cvar* com_version;
extern Cvar* comt3_timedemo;

extern Cvar* com_ignorecrash;		//bani
extern Cvar* com_pid;		//bani

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
extern int time_before_ref;
extern int time_after_ref;

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

extern bool com_fullyInitialized;

void Com_WriteConfigToFile(const char* filename);
void Com_WriteConfiguration();
void Com_WriteConfig_f();

void Com_SetRecommended(bool vid_restart);

void Com_LogToFile(const char* msg);
void Com_Shutdown();
void ComQH_HostShutdown();

void Com_Quit_f();
void COM_InitCommonCommands();

bool ComET_CheckProfile(const char* profile_path);
bool ComET_WriteProfile(const char* profile_path);

void COMQH_CheckRegistered();

#endif
