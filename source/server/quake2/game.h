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

// game.h -- game dll information visible to server

#define Q2GAME_API_VERSION  3

//===============================================================

//
// functions provided by the main engine
//
struct q2game_import_t
{
	// special messages
	void (* bprintf)(int printlevel, const char* fmt, ...);
	void (* dprintf)(const char* fmt, ...);
	void (* cprintf)(q2edict_t* ent, int printlevel, const char* fmt, ...);
	void (* centerprintf)(q2edict_t* ent, const char* fmt, ...);
	void (* sound)(q2edict_t* ent, int channel, int soundindex, float volume, float attenuation, float timeofs);
	void (* positioned_sound)(const vec3_t origin, q2edict_t* ent, int channel, int soundinedex, float volume, float attenuation, float timeofs);

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void (* configstring)(int num, const char* string);

	void (* error)(const char* fmt, ...);

	// the *index functions create configstrings and some internal server state
	int (* modelindex)(const char* name);
	int (* soundindex)(const char* name);
	int (* imageindex)(const char* name);

	void (* setmodel)(q2edict_t* ent, const char* name);

	// collision detection
	q2trace_t (* trace)(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, q2edict_t* passent, int contentmask);
	int (* pointcontents)(vec3_t point);
	qboolean (* inPVS)(const vec3_t p1, const vec3_t p2);
	qboolean (* inPHS)(const vec3_t p1, const vec3_t p2);
	void (* SetAreaPortalState)(int portalnum, qboolean open);
	qboolean (* AreasConnected)(int area1, int area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void (* linkentity)(q2edict_t* ent);
	void (* unlinkentity)(q2edict_t* ent);		// call before removing an interactive edict
	int (* BoxEdicts)(vec3_t mins, vec3_t maxs, q2edict_t** list, int maxcount, int areatype);
	void (* Pmove)(q2pmove_t* pmove);			// player movement code common with client prediction

	// network messaging
	void (* multicast)(const vec3_t origin, q2multicast_t to);
	void (* unicast)(q2edict_t* ent, qboolean reliable);
	void (* WriteChar)(int c);
	void (* WriteByte)(int c);
	void (* WriteShort)(int c);
	void (* WriteLong)(int c);
	void (* WriteFloat)(float f);
	void (* WriteString)(const char* s);
	void (* WritePosition)(const vec3_t pos);		// some fractional bits
	void (* WriteDir)(const vec3_t pos);			// single byte encoded, very coarse
	void (* WriteAngle)(float f);

	// managed memory allocation
	void*(*TagMalloc)(int size, int tag);
	void (* TagFree)(void* block);
	void (* FreeTags)(int tag);

	// console variable interaction
	Cvar*(*cvar)(const char* var_name, const char* value, int flags);
	Cvar*(*cvar_set)(const char* var_name, const char* value);
	Cvar*(*cvar_forceset)(const char* var_name, const char* value);

	// ClientCommand and ServerCommand parameter access
	int (* argc)(void);
	char*(*argv)(int n);
	char*(*args)(void);			// concatenation of all argv >= 1

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void (* AddCommandString)(const char* text);

	void (* DebugGraph)(float value, int color);
};

//
// functions exported by the game subsystem
//
struct q2game_export_t
{
	int apiversion;

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void (* Init)(void);
	void (* Shutdown)(void);

	// each new level entered will cause a call to SpawnEntities
	void (* SpawnEntities)(char* mapname, char* entstring, char* spawnpoint);

	// Read/Write Game is for storing persistant cross level information
	// about the world state and the clients.
	// WriteGame is called every time a level is exited.
	// ReadGame is called on a loadgame.
	void (* WriteGame)(char* filename, qboolean autosave);
	void (* ReadGame)(char* filename);

	// ReadLevel is called after the default map information has been
	// loaded with SpawnEntities
	void (* WriteLevel)(char* filename);
	void (* ReadLevel)(char* filename);

	qboolean (* ClientConnect)(q2edict_t* ent, char* userinfo);
	void (* ClientBegin)(q2edict_t* ent);
	void (* ClientUserinfoChanged)(q2edict_t* ent, char* userinfo);
	void (* ClientDisconnect)(q2edict_t* ent);
	void (* ClientCommand)(q2edict_t* ent);
	void (* ClientThink)(q2edict_t* ent, q2usercmd_t* cmd);

	void (* RunFrame)(void);

	// ServerCommand will be called when an "sv <command>" command is issued on the
	// server console.
	// The game can issue gi.argc() / gi.argv() commands to get the rest
	// of the parameters
	void (* ServerCommand)(void);

	//
	// global variables shared between game and server
	//

	// The edict array is allocated in the game dll so it
	// can vary in size from one game to another.
	//
	// The size will be fixed when ge->Init() is called
	q2edict_t* edicts;
	int edict_size;
	int num_edicts;				// current number, <= max_edicts
	int max_edicts;
};
