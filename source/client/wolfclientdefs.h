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

#define LIMBOCHAT_WIDTH_WA     140     // NERVE - SMF
#define LIMBOCHAT_HEIGHT_WA    7       // NERVE - SMF

// Arnout: doubleTap buttons - ETDT_NUM can be max 8
enum
{
	ETDT_NONE,
	ETDT_MOVELEFT,
	ETDT_MOVERIGHT,
	ETDT_FORWARD,
	ETDT_BACK,
	ETDT_LEANLEFT,
	ETDT_LEANRIGHT,
	ETDT_UP,
	ETDT_NUM
};

// snapshots are a view of the server at a given time
struct wsclSnapshot_t
{
	qboolean valid;                 // cleared if delta parsing was invalid
	int snapFlags;                  // rate delayed and dropped commands

	int serverTime;                 // server time the message is valid for (in msec)

	int messageNum;                 // copied from netchan->incoming_sequence
	int deltaNum;                   // messageNum the delta is from
	int ping;                       // time from when cmdNum-1 was sent to time packet was reeceived
	byte areamask[MAX_MAP_AREA_BYTES];                  // portalarea visibility bits

	int cmdNum;                     // the next cmdNum the server is expecting
	wsplayerState_t ps;                       // complete information about the current player at this time

	int numEntities;                        // all of the entities that need to be presented
	int parseEntitiesNum;                   // at the time of this snapshot

	int serverCommandNum;                   // execute all commands up to this before
											// making the snapshot current
};

// snapshots are a view of the server at a given time
struct wmclSnapshot_t
{
	qboolean valid;                 // cleared if delta parsing was invalid
	int snapFlags;                  // rate delayed and dropped commands

	int serverTime;                 // server time the message is valid for (in msec)

	int messageNum;                 // copied from netchan->incoming_sequence
	int deltaNum;                   // messageNum the delta is from
	int ping;                       // time from when cmdNum-1 was sent to time packet was reeceived
	byte areamask[MAX_MAP_AREA_BYTES];                  // portalarea visibility bits

	int cmdNum;                     // the next cmdNum the server is expecting
	wmplayerState_t ps;                       // complete information about the current player at this time

	int numEntities;                        // all of the entities that need to be presented
	int parseEntitiesNum;                   // at the time of this snapshot

	int serverCommandNum;                   // execute all commands up to this before
											// making the snapshot current
};

// snapshots are a view of the server at a given time
struct etclSnapshot_t
{
	qboolean valid;                 // cleared if delta parsing was invalid
	int snapFlags;                  // rate delayed and dropped commands

	int serverTime;                 // server time the message is valid for (in msec)

	int messageNum;                 // copied from netchan->incoming_sequence
	int deltaNum;                   // messageNum the delta is from
	int ping;                       // time from when cmdNum-1 was sent to time packet was reeceived
	byte areamask[MAX_MAP_AREA_BYTES];                  // portalarea visibility bits

	int cmdNum;                     // the next cmdNum the server is expecting
	etplayerState_t ps;                       // complete information about the current player at this time

	int numEntities;                        // all of the entities that need to be presented
	int parseEntitiesNum;                   // at the time of this snapshot

	int serverCommandNum;                   // execute all commands up to this before
											// making the snapshot current
};

struct wsgameState_t
{
	int stringOffsets[MAX_CONFIGSTRINGS_WS];
	char stringData[MAX_GAMESTATE_CHARS_Q3];
	int dataCount;
};

struct wmgameState_t
{
	int stringOffsets[MAX_CONFIGSTRINGS_WM];
	char stringData[MAX_GAMESTATE_CHARS_Q3];
	int dataCount;
};

struct etgameState_t
{
	int stringOffsets[MAX_CONFIGSTRINGS_ET];
	char stringData[MAX_GAMESTATE_CHARS_Q3];
	int dataCount;
};

// Arnout: for double tapping
struct etdoubleTap_t
{
	int pressedTime[ETDT_NUM];
	int releasedTime[ETDT_NUM];

	int lastdoubleTap;
};

// NERVE - SMF - localization
enum
{
	LANGUAGE_FRENCH = 0,
	LANGUAGE_GERMAN,
	LANGUAGE_ITALIAN,
	LANGUAGE_SPANISH,
	MAX_LANGUAGES
};
