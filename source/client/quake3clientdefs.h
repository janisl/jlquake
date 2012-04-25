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

//!!!!!!! Used in cgame QVM, do not change !!!!!!!
#define CMD_BACKUP_Q3       64
#define CMD_MASK_Q3         (CMD_BACKUP_Q3 - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP_Q3

#define MAX_GLOBAL_SERVERS_Q3               4096
#define MAX_GLOBAL_SERVERS_WS               2048
#define MAX_GLOBAL_SERVERS_WM               2048
#define MAX_GLOBAL_SERVERS_ET               4096
#define BIGGEST_MAX_GLOBAL_SERVERS          4096
#define MAX_OTHER_SERVERS_Q3                128

#define MAX_AUTOUPDATE_SERVERS  5

// the parseEntities array must be large enough to hold PACKET_BACKUP_Q3 frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
#define MAX_PARSE_ENTITIES_Q3   2048

// snapshots are a view of the server at a given time
struct q3clSnapshot_t
{
	bool valid;				// cleared if delta parsing was invalid
	int snapFlags;			// rate delayed and dropped commands

	int serverTime;			// server time the message is valid for (in msec)

	int messageNum;			// copied from netchan->incoming_sequence
	int deltaNum;			// messageNum the delta is from
	int ping;				// time from when cmdNum-1 was sent to time packet was reeceived
	byte areamask[MAX_MAP_AREA_BYTES];	// portalarea visibility bits

	int cmdNum;				// the next cmdNum the server is expecting
	q3playerState_t ps;		// complete information about the current player at this time

	int numEntities;		// all of the entities that need to be presented
	int parseEntitiesNum;	// at the time of this snapshot

	int serverCommandNum;	// execute all commands up to this before
							// making the snapshot current
};

#define MAX_GAMESTATE_CHARS_Q3  16000

struct q3gameState_t
{
	int stringOffsets[MAX_CONFIGSTRINGS_Q3];
	char stringData[MAX_GAMESTATE_CHARS_Q3];
	int dataCount;
};

struct q3outPacket_t
{
	int p_cmdNumber;		// cl.cmdNumber when packet was sent
	int p_serverTime;		// usercmd->serverTime when packet was sent
	int p_realtime;			// cls.realtime when packet was sent
};

struct q3serverInfo_t
{
	netadr_t adr;
	char hostName[MAX_NAME_LENGTH_ET];
	char mapName[MAX_NAME_LENGTH_ET];
	char game[MAX_NAME_LENGTH_ET];
	int netType;
	int gameType;
	int clients;
	int maxClients;
	int minPing;
	int maxPing;
	int ping;
	qboolean visible;
	int punkbuster;
	int allowAnonymous;
	int friendlyFire;				// NERVE - SMF
	int maxlives;					// NERVE - SMF
	int tourney;					// NERVE - SMF
	int antilag;		// TTimo
	char gameName[MAX_NAME_LENGTH_ET];			// Arnout
	int load;
	int needpass;
	int weaprestrict;
	int balancedteams;
};

struct q3serverAddress_t
{
	byte ip[4];
	unsigned short port;
};
