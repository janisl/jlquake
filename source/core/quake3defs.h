//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#define MAX_TOKEN_CHARS_Q3	1024	// max length of an individual token

#define MAX_CLIENTS_Q3		64		// absolute limit
#define MAX_MODELS_Q3		256		// these are sent over the net as 8 bits so they cannot be blindly increased

#define MAX_CONFIGSTRINGS_Q3	1024

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define Q3CS_SERVERINFO		0		// an info string with all the serverinfo cvars
#define Q3CS_SYSTEMINFO		1		// an info string for server system to client system configuration (timescale, etc)
#define Q3CS_WARMUP			5		// server time when the match will be restarted

#define GENTITYNUM_BITS_Q3		10		// don't need to send any more
#define MAX_GENTITIES_Q3		(1 << GENTITYNUM_BITS_Q3)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values that are going to be communcated over the net need to
// also be in this range
#define Q3ENTITYNUM_NONE		(MAX_GENTITIES_Q3 - 1)
#define Q3ENTITYNUM_WORLD		(MAX_GENTITIES_Q3 - 2)
#define Q3ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES_Q3 - 2)

// q3usercmd_t is sent to the server each client frame
struct q3usercmd_t
{
	int serverTime;
	int angles[3];
	int buttons;
	byte weapon;           // weapon 
	signed char forwardmove, rightmove, upmove;
};

// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum
{
	q3svc_bad,
	q3svc_nop,
	q3svc_gamestate,
	q3svc_configstring,			// [short] [string] only in gamestate messages
	q3svc_baseline,				// only in gamestate messages
	q3svc_serverCommand,			// [string] to be executed by client game module
	q3svc_download,				// [short] size [size bytes]
	q3svc_snapshot,
	q3svc_EOF
};


//
// client to server
//
enum
{
	q3clc_bad,
	q3clc_nop, 		
	q3clc_move,				// [[q3usercmd_t]
	q3clc_moveNoDelta,		// [[q3usercmd_t]
	q3clc_clientCommand,		// [string] message
	q3clc_EOF
};
