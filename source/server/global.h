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

#ifndef __SERVER_GLOBAL_H__
#define __SERVER_GLOBAL_H__

#include "../common/qcommon.h"
#include "../common/file_formats/md3.h"
#include "progsvm/edict.h"
#include "quakeserverdefs.h"
#include "hexen2serverdefs.h"
#include "quake2serverdefs.h"
#include "quake3serverdefs.h"
#include "wolfserverdefs.h"
#include "tech3/Entity3.h"
#include "tech3/PlayerState3.h"

enum clientState_t
{
	CS_FREE,		// can be reused for a new connection
	CS_ZOMBIE,		// client has been disconnected, but don't reuse connection for a couple seconds
	CS_CONNECTED,	// has been assigned to a client_t, but no gamestate yet
	CS_PRIMED,		// gamestate has been sent, but client hasn't sent a usercmd
	CS_ACTIVE		// client is fully in game
};

struct client_t
{
	clientState_t state;
	char userinfo[BIGGEST_MAX_INFO_STRING];			// name, etc
	char name[MAX_NAME_LENGTH];						// extracted from userinfo, high bits masked

	netchan_t netchan;

	// Only in QuakeWorld. Hexen 2 and Quake 2
	// The datagram is written to by sound calls, prints, temp ents, etc,
	// but only cleared when it is sent out to the client.
	// It can be harmlessly overflowed.
	QMsg datagram;
	byte datagramBuffer[MAX_MSGLEN_H2];

	// Only in QuakeWorld. HexenWorld and Quake 2
	int messagelevel;					// for filtering printed messages

	// downloading
	// Not in Quake 2
	fileHandle_t download;				// file being downloaded
	int downloadSize;					// total bytes (can't use EOF because of paks)
	int downloadCount;					// bytes sent
	// Only in Quake 2
	byte* q2_downloadData;				// file being downloaded
	// Only in Tech3
	char downloadName[MAX_QPATH];		// if not empty string, we are downloading
	int downloadClientBlock;			// last block we sent to the client, awaiting ack
	int downloadCurrentBlock;			// current block number
	int downloadXmitBlock;				// last block we xmited
	unsigned char* downloadBlocks[MAX_DOWNLOAD_WINDOW];	// the buffers for the download blocks
	int downloadBlockSize[MAX_DOWNLOAD_WINDOW];
	bool downloadEOF;					// We have sent the EOF block
	int downloadSendTime;				// time we last got an ack from the client

	// Only in Quake 2 and Tech3
	int challenge;						// challenge of this user, randomly generated
	int ping;
	int rate;							// bytes / second

	qhedict_t* qh_edict;				// QH_EDICT_NUM(clientnum+1)

	// spawn parms are carried from level to level
	float qh_spawn_parms[NUM_SPAWN_PARMS];

	// client known data for deltas
	int qh_old_frags;

	//	Only in NetQuake/Hexen 2
	bool qh_dropasap;					// has been told to go to another level
	bool qh_sendsignon;					// only valid before spawned

	double qh_last_message;				// reliable messages must be sent
										// periodically

	QMsg qh_message;					// can be added to at any time,
										// copied and clear once per frame
	byte qh_messageBuffer[MAX_MSGLEN_H2];

	int qh_colors;

	float qh_ping_times[NUM_PING_TIMES];
	int qh_num_pings;					// ping_times[num_pings%NUM_PING_TIMES]

	// Only in QuakeWorld and HexenWorld
	int qh_spectator;					// non-interactive

	bool qh_sendinfo;					// at end of frame, send info to all
										// this prevents malicious multiple broadcasts

	int qh_userid;						// identifying number

	double qh_localtime;				// of last message
	int qh_oldbuttons;

	float qh_maxspeed;					// localized maxspeed
	float qh_entgravity;				// localized ent gravity

	double qh_connection_started;		// or time of disconnect for zombies
	bool qh_send_message;				// set on frames a datagram arived on

	int qh_stats[MAX_CL_STATS];

	int qh_spec_track;					// entnum of player tracking

	double qh_whensaid[10];				// JACK: For floodprots
	int qh_whensaidhead;				// Head value for floodprots
	double qh_lockedtill;

	int qh_delta_sequence;				// -1 = no compression

	q1usercmd_t q1_lastUsercmd;			// movement

	float qw_lastnametime;				// time of last name change
	int qw_lastnamecount;				// time of last name change
	unsigned qw_checksum;				// checksum for calcs
	qboolean qw_drop;					// lose this guy next opportunity
	int qw_lossage;						// loss percentage

	qwusercmd_t qw_lastUsercmd;			// for filling in big drops and partial predictions

	// back buffers for client reliable data
	QMsg qw_backbuf;
	int qw_num_backbuf;
	int qw_backbuf_size[MAX_BACK_BUFFERS];
	byte qw_backbuf_data[MAX_BACK_BUFFERS][MAX_MSGLEN_QW];

	fileHandle_t qw_upload;
	char qw_uploadfn[MAX_QPATH];
	netadr_t qw_snap_from;
	bool qw_remote_snap;

	qwclient_frame_t qw_frames[UPDATE_BACKUP_QW];	// updates can be deltad from here

	int h2_playerclass;

	h2client_entvars_t h2_old_v;
	bool h2_send_all_v;

	//	Only in Hexen 2
	h2usercmd_t h2_lastUsercmd;			// movement

	byte h2_current_frame, h2_last_frame;
	byte h2_current_sequence, h2_last_sequence;

	int h2_info_mask, h2_info_mask2;

	bool hw_portals;					// They have portals mission pack installed

	hwusercmd_t hw_lastUsercmd;			// for filling in big drops and partial predictions

	int hw_siege_team;
	int hw_next_playerclass;

	hwclient_frame_t hw_frames[UPDATE_BACKUP_HW];	// updates can be deltad from here

	unsigned hw_PIV, hw_LastPIV;		// people in view

	q2edict_t* q2_edict;				// Q2_EDICT_NUM(clientnum+1)

	int q2_lastframe;					// for delta compression
	q2usercmd_t q2_lastUsercmd;			// for filling in big drops

	int q2_commandMsec;					// every seconds this is reset, if user
										// commands exhaust it, assume time cheating

	int q2_frame_latency[LATENCY_COUNTS];

	int q2_message_size[RATE_MESSAGES];	// used to rate drop packets
	int q2_surpressCount;				// number of messages rate supressed

	q2client_frame_t q2_frames[UPDATE_BACKUP_Q2];	// updates can be delta'd from here

	int q2_lastmessage;					// sv.framenum when packet was last received
	int q2_lastconnect;

	char q3_reliableCommands[BIGGEST_MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
	wsreliableCommands_t ws_reliableCommands;
	int q3_reliableSequence;			// last added reliable message, not necesarily sent or acknowledged yet
	int q3_reliableAcknowledge;			// last acknowledged reliable message
	int q3_reliableSent;				// last sent reliable message, not necesarily acknowledged yet
	int q3_messageAcknowledge;

	int q3_gamestateMessageNum;			// netchan->outgoingSequence of gamestate

	q3usercmd_t q3_lastUsercmd;
	wsusercmd_t ws_lastUsercmd;
	wmusercmd_t wm_lastUsercmd;
	etusercmd_t et_lastUsercmd;
	int q3_lastMessageNum;				// for delta compression
	int q3_lastClientCommand;			// reliable client message sequence
	char q3_lastClientCommandString[MAX_STRING_CHARS];

	int q3_deltaMessage;				// frame last client usercmd message
	int q3_nextReliableTime;			// svs.q3_time when another reliable command will be allowed
	int q3_lastPacketTime;				// svs.q3_time when packet was last received
	int q3_lastConnectTime;				// svs.q3_time when connection started
	int q3_nextSnapshotTime;			// send another snapshot when svs.q3_time >= nextSnapshotTime
	bool q3_rateDelayed;				// true if nextSnapshotTime was set based on rate instead of snapshotMsec
	int q3_timeoutCount;				// must timeout a few frames in a row so debugging doesn't break
	q3clientSnapshot_t q3_frames[PACKET_BACKUP_Q3];	// updates can be delta'd from here
	int q3_snapshotMsec;				// requests a snapshot every snapshotMsec unless rate choked
	bool q3_pureAuthentic;
	bool q3_gotCP;						// additional flag to distinguish between a bad pure checksum, and no cp command at all
	// TTimo
	// queuing outgoing fragmented messages to send them properly, without udp packet bursts
	// in case large fragmented messages are stacking up
	// buffer them into this queue, and hand them out to netchan as needed
	netchan_buffer_t* q3_netchan_start_queue;
	netchan_buffer_t** q3_netchan_end_queue;
	netchan_buffer_t* et_netchan_end_queue;

	idEntity3* q3_entity;		// SVT3_ntityNum(clientnum)
	q3sharedEntity_t* q3_gentity;		// SVWS_GentityNum(clientnum)
	wssharedEntity_t* ws_gentity;		// SVWS_GentityNum(clientnum)
	wmsharedEntity_t* wm_gentity;		// SVWS_GentityNum(clientnum)
	etsharedEntity_t* et_gentity;		// SVWS_GentityNum(clientnum)

	int et_binaryMessageLength;
	char et_binaryMessage[MAX_BINARY_MESSAGE_ET];
	bool et_binaryMessageOverflowed;

	// www downloading
	bool et_bDlOK;		// passed from cl_wwwDownload CVAR_USERINFO, wether this client supports www dl
	char et_downloadURL[MAX_OSPATH];			// the URL we redirected the client to
	bool et_bWWWDl;	// we have a www download going
	bool et_bWWWing;	// the client is doing an ftp/http download
	bool et_bFallback;		// last www download attempt failed, fallback to regular download
	// note: this is one-shot, multiple downloads would cause a www download to be attempted again

	//bani
	int et_downloadnotify;
};

// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)
enum serverState_t
{
	SS_DEAD,			// no map loaded
	SS_LOADING,			// spawning level entities
	SS_GAME,			// actively running
	SS_CINEMATIC,
	SS_DEMO,
	SS_PIC
};

struct server_t
{
	serverState_t state;

	//	Only in Quake/Hexen 2 and Quake 2
	char name[MAX_QPATH];				// map name, or cinematic name

	clipHandle_t models[BIGGEST_MAX_MODELS];

	//	Only NetQuake/Hexen 2 and Quake 2
	bool loadgame;						// client begins should reuse existing entity

	// Only in QuakeWorld, HexenWorld and Quake 2
	// the multicast buffer is used to send a message to a set of clients
	// it is only used to marshall data until SV_Multicast is called
	QMsg multicast;
	byte multicastBuffer[MAX_MSGLEN];

	bool qh_paused;

	int qh_time;

	int qh_lastcheck;					// used by PF_checkclient
	int qh_lastchecktime;

	char qh_modelname[MAX_QPATH];		// maps/<name>.bsp, for model_precache[0]
	const char* qh_model_precache[BIGGEST_MAX_MODELS];	// NULL terminated
	const char* qh_sound_precache[BIGGEST_MAX_SOUNDS];	// NULL terminated
	const char* qh_lightstyles[MAX_LIGHTSTYLES_Q1];

	int qh_num_edicts;
	qhedict_t* qh_edicts;				// can NOT be array indexed, because
										// qhedict_t is variable sized, but can
										// be used to reference the world ent

	// added to every client's unreliable buffer each frame, then cleared
	QMsg qh_datagram;
	byte qh_datagramBuffer[MAX_MSGLEN];

	// added to every client's reliable buffer each frame, then cleared
	QMsg qh_reliable_datagram;
	byte qh_reliable_datagramBuffer[MAX_MSGLEN];

	// the signon buffer will be sent to each client as they connect
	// includes the entity baselines, the static entities, etc
	// large levels will have >MAX_DATAGRAM_QW sized signons, so
	// multiple signon messages are kept
	QMsg qh_signon;
	byte qh_signonBuffer[MAX_MSGLEN];
	//	Only QuakeWorld and hexenWorld
	int qh_num_signon_buffers;
	int qh_signon_buffer_size[MAX_SIGNON_BUFFERS];
	byte qh_signon_buffers[MAX_SIGNON_BUFFERS][MAX_DATAGRAM];

	//check player/eyes models for hacks
	unsigned qw_model_player_checksum;
	unsigned qw_eyes_player_checksum;

	char h2_midi_name[MAX_QPATH];		// midi file name
	byte h2_cd_track;					// cd track number

	char h2_startspot[64];

	h2EffectT h2_Effects[MAX_EFFECTS_H2];

	h2client_state2_t* h2_states;

	double hw_next_PIV_time;			// Last Player In View time

	int hw_current_skill;

	bool q2_attractloop;				// running cinematics and demos for the local system only

	unsigned q2_time;					// always sv.framenum * 100 msec
	int q2_framenum;

	char q2_configstrings[MAX_CONFIGSTRINGS_Q2][MAX_QPATH];
	q2entity_state_t q2_baselines[MAX_EDICTS_Q2];

	// demo server information
	fileHandle_t q2_demofile;

	bool q3_restarting;					// if true, send configstring changes during SS_LOADING
	int q3_serverId;					// changes each server start
	int q3_restartedServerId;			// serverId before a map_restart
	int q3_checksumFeed;				// the feed key that we use to compute the pure checksum strings
	// the serverId associated with the current checksumFeed (always <= serverId)
	int q3_checksumFeedServerId;
	int q3_snapshotCounter;				// incremented for each snapshot built
	int q3_timeResidual;				// <= 1000 / sv_frame->value

	char* q3_configstrings[BIGGEST_MAX_CONFIGSTRINGS_T3];
	q3svEntity_t q3_svEntities[MAX_GENTITIES_Q3];
	idEntity3* q3_entities[MAX_GENTITIES_Q3];

	const char* q3_entityParsePoint;	// used during game VM init

	// the game virtual machine will update these on init and changes
	q3sharedEntity_t* q3_gentities;
	wssharedEntity_t* ws_gentities;
	wmsharedEntity_t* wm_gentities;
	etsharedEntity_t* et_gentities;
	int q3_gentitySize;
	int q3_num_entities;				// current number, <= MAX_GENTITIES_Q3

	idPlayerState3** q3_gamePlayerStates;
	q3playerState_t* q3_gameClients;
	wsplayerState_t* ws_gameClients;
	wmplayerState_t* wm_gameClients;
	etplayerState_t* et_gameClients;
	int q3_gameClientSize;				// will be > sizeof(q3playerState_t) due to game private data

	int q3_restartTime;

	bool et_configstringsmodified[MAX_CONFIGSTRINGS_ET];

	// NERVE - SMF - net debugging
	int wm_bpsWindow[MAX_BPS_WINDOW];
	int wm_bpsWindowSteps;
	int wm_bpsTotalBytes;
	int wm_bpsMaxBytes;

	int wm_ubpsWindow[MAX_BPS_WINDOW];
	int wm_ubpsTotalBytes;
	int wm_ubpsMaxBytes;

	float wm_ucompAve;
	int wm_ucompNum;
	// -NERVE - SMF

	md3Tag_t et_tags[MAX_SERVER_TAGS_ET];
	ettagHeaderExt_t et_tagHeadersExt[MAX_TAG_FILES_ET];

	int et_num_tagheaders;
	int et_num_tags;
};

struct svstats_t
{
	double active;
	double idle;
	int count;
	int packets;

	double latched_active;
	double latched_idle;
	int latched_packets;
};

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define MAX_CHALLENGES  1024

struct challenge_t
{
	netadr_t adr;
	int challenge;
	int time;						// time the last packet was sent to the autherize server
	int pingTime;					// time the challenge response was sent to client
	int firstTime;					// time the adr was first used, for authorize timeout checks
	int firstPing;					// Used for min and max ping checks
	bool connected;
};

// this structure will be cleared only when the game dll changes
struct serverStatic_t
{
	bool initialized;					// sv_init has completed

	client_t* clients;					// [sv_maxclients->integer];

	challenge_t challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	// Only QuakeWorld, HexenWorld, and Quake 2.
	int spawncount;						// incremented each server start
										// used to check late spawns

	int realtime;					// always increasing, no clamping, etc

	netadr_t redirectAddress;		// for rcon return messages

	int qh_serverflags;					// episode completion information

	// Only NetQuake/Hexen2
	int qh_maxclients;
	int qh_maxclientslimit;
	bool qh_changelevel_issued;			// cleared when at SV_SpawnServer

	// Only QuakeWorld and HexenWorld
	double qh_last_heartbeat;
	int qh_heartbeat_sequence;
	svstats_t qh_stats;

	char qh_info[MAX_SERVERINFO_STRING];

	// log messages are used so that fraglog processes can get stats
	int qh_logsequence;					// the message currently being filled
	double qh_logtime;					// time of last swap
	QMsg qh_log[2];
	byte qh_log_buf[2][MAX_DATAGRAM];

	char q2_mapcmd[MAX_TOKEN_CHARS_Q2];	// ie: *intro.cin+base

	int q2_num_client_entities;			// maxclients->value*UPDATE_BACKUP_Q2*MAX_PACKET_ENTITIES
	int q2_next_client_entities;		// next client_entity to use
	q2entity_state_t* q2_client_entities;	// [num_client_entities]

	int q2_last_heartbeat;

	// serverrecord values
	fileHandle_t q2_demofile;
	QMsg q2_demo_multicast;
	byte q2_demo_multicast_buf[MAX_MSGLEN_Q2];

	int q3_time;						// will be strictly increasing across level changes

	int q3_snapFlagServerBit;			// ^= Q3SNAPFLAG_SERVERCOUNT every SV_SpawnServer()

	int q3_numSnapshotEntities;			// sv_maxclients->integer*PACKET_BACKUP_Q3*MAX_PACKET_ENTITIES
	int q3_nextSnapshotEntities;		// next snapshotEntities to use
	q3entityState_t* q3_snapshotEntities;	// [numSnapshotEntities]
	wsentityState_t* ws_snapshotEntities;	// [numSnapshotEntities]
	wmentityState_t* wm_snapshotEntities;	// [numSnapshotEntities]
	etentityState_t* et_snapshotEntities;	// [numSnapshotEntities]
	int q3_nextHeartbeatTime;

	netadr_t q3_authorizeAddress;		// for rcon return messages

	tempBan_t et_tempBanAddresses[MAX_TEMPBAN_ADDRESSES];

	int et_sampleTimes[SERVER_PERFORMANCECOUNTER_SAMPLES];
	int et_currentSampleIndex;
	int et_totalFrameTime;
	int et_currentFrameIndex;
	int et_serverLoad;
};

extern serverStatic_t svs;					// persistant server info across maps
extern server_t sv;							// cleared each map

struct ucmd_t
{
	const char* name;
	void (*func)(client_t* cl);
	bool allowedpostmapchange;
};

#define MAX_MASTERS 8				// max recipients for heartbeat packets

extern Cvar* sv_maxclients;

extern netadr_t master_adr[MAX_MASTERS];		// address of the master server

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart);

#endif
