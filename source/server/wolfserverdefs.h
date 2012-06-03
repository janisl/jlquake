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

enum
{
	WSGT_FFA,				// free for all
	WSGT_SINGLE_PLAYER = 2,	// single player tournament
	//-- team games go after this --
	WSGT_TEAM,				// team deathmatch
};

enum
{
	WMGT_FFA,				// free for all
	WMGT_SINGLE_PLAYER = 2,	// single player tournament
	//-- team games go after this --
	WMGT_TEAM,				// team deathmatch
	WMGT_WOLF = 5,			// DHM - Nerve :: Wolfenstein Multiplayer
	WMGT_WOLF_STOPWATCH,	// NERVE - SMF - stopwatch gametype
	WMGT_WOLF_CP,			// NERVE - SMF - checkpoint gametype
	WMGT_WOLF_CPH,			// JPW NERVE - Capture & Hold gametype
};

struct wsreliableCommands_t
{
	int bufSize;
	char* buf;					// actual strings
	char** commands;			// pointers to actual strings
	int* commandLengths;		// lengths of actual strings
	char* rover;
};

struct wsentityShared_t
{
	wsentityState_t s;					// communicated by server to clients

	qboolean linked;				// qfalse if not in any good cluster
	int linkcount;

	int svFlags;					// SVF_NOCLIENT, SVF_BROADCAST, etc
	int singleClient;				// only send to this client when SVF_SINGLECLIENT is set

	qboolean bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
	vec3_t mins, maxs;
	int contents;					// BSP46CONTENTS_TRIGGER, BSP46CONTENTS_SOLID, BSP46CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t absmin, absmax;			// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and passEntityNum != Q3ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int ownerNum;
	int eventTime;
};

// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
struct wssharedEntity_t
{
	wsentityState_t s;					// communicated by server to clients
	wsentityShared_t r;				// shared by both the server system and game
};

struct wmentityShared_t
{
	wmentityState_t s;					// communicated by server to clients

	qboolean linked;				// qfalse if not in any good cluster
	int linkcount;

	int svFlags;					// SVF_NOCLIENT, SVF_BROADCAST, etc
	int singleClient;				// only send to this client when SVF_SINGLECLIENT is set

	qboolean bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
	vec3_t mins, maxs;
	int contents;					// BSP46CONTENTS_TRIGGER, BSP46CONTENTS_SOLID, BSP46CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t absmin, absmax;			// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and passEntityNum != Q3ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int ownerNum;
	int eventTime;

	int worldflags;				// DHM - Nerve
};

// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
struct wmsharedEntity_t
{
	wmentityState_t s;					// communicated by server to clients
	wmentityShared_t r;				// shared by both the server system and game
};

struct etentityShared_t
{
	qboolean linked;				// qfalse if not in any good cluster
	int linkcount;

	int svFlags;					// SVF_NOCLIENT, SVF_BROADCAST, etc
	int singleClient;				// only send to this client when SVF_SINGLECLIENT is set

	qboolean bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
	vec3_t mins, maxs;
	int contents;					// BSP46CONTENTS_TRIGGER, BSP46CONTENTS_SOLID, BSP46CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t absmin, absmax;			// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and passEntityNum != Q3ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int ownerNum;
	int eventTime;

	int worldflags;				// DHM - Nerve

	qboolean snapshotCallback;
};

// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
struct etsharedEntity_t
{
	etentityState_t s;					// communicated by server to clients
	etentityShared_t r;				// shared by both the server system and game
};

#define MAX_BPS_WINDOW      20			// NERVE - SMF - net debugging

#define MAX_SERVER_TAGS_ET  256
#define MAX_TAG_FILES_ET    64

struct ettagHeaderExt_t
{
	char filename[MAX_QPATH];
	int start;
	int count;
};

#define MAX_TEMPBAN_ADDRESSES               MAX_CLIENTS_ET

struct tempBan_t
{
	netadr_t adr;
	int endtime;
};

#define SERVER_PERFORMANCECOUNTER_SAMPLES   6
