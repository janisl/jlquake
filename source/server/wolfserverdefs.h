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

	qboolean linked;				// false if not in any good cluster
	int linkcount;

	int svFlags;					// Q3SVF_NOCLIENT, Q3SVF_BROADCAST, etc
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

	qboolean linked;				// false if not in any good cluster
	int linkcount;

	int svFlags;					// Q3SVF_NOCLIENT, Q3SVF_BROADCAST, etc
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
	qboolean linked;				// false if not in any good cluster
	int linkcount;

	int svFlags;					// Q3SVF_NOCLIENT, Q3SVF_BROADCAST, etc
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

#define WOLFSVF_VISDUMMY            0x00000004	// this ent is a "visibility dummy" and needs it's master to be sent to clients that can see it even if they can't see the master ent
#define WOLFSVF_CAPSULE             0x00000200	// use capsule for collision detection
#define WOLFSVF_VISDUMMY_MULTIPLE   0x00000400	// so that one vis dummy can add to snapshot multiple speakers
#define WOLFSVF_SINGLECLIENT        0x00000800	// only send to a single client (wsentityShared_t->singleClient)
#define WOLFSVF_NOSERVERINFO        0x00001000	// don't send Q3CS_SERVERINFO updates to this client
												// so that it can be updated for ping tools without
												// lagging clients
#define WOLFSVF_NOTSINGLECLIENT     0x00002000	// send entity to everyone but one client
												// (wsentityShared_t->singleClient)

#define WSSVF_CASTAI                0x00000010

#define WMSVF_CASTAI                0x00000010

#define ETSVF_IGNOREBMODELEXTENTS   0x00004000	// just use origin for in pvs check for snapshots, ignore the bmodel extents
#define ETSVF_SELF_PORTAL           0x00008000	// use self->origin2 as portal
#define ETSVF_SELF_PORTAL_EXCLUSIVE 0x00010000	// use self->origin2 as portal and DONT add self->origin PVS ents

// NERVE - SMF - wolf server/game states
enum gamestate_t
{
	GS_INITIALIZE = -1,
	GS_PLAYING,
	GS_WARMUP_COUNTDOWN,
	GS_WARMUP,
	GS_INTERMISSION,
	GS_WAITING_FOR_PLAYERS,
	GS_RESET
};

//bani - cl->downloadnotify
#define DLNOTIFY_REDIRECT   0x00000001	// "Redirecting client ..."
#define DLNOTIFY_BEGIN      0x00000002	// "clientDownload: 4 : beginning ..."
#define DLNOTIFY_ALL        (DLNOTIFY_REDIRECT | DLNOTIFY_BEGIN)

// RF, this is just here so different elements of the engine can be aware of this setting as it changes
#define WSMAX_SP_CLIENTS    64		// increasing this will increase memory usage significantly

#define ETGAME_INIT_FRAMES  6
