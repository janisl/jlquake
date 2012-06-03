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

	qhedict_t* qh_edict;				// EDICT_NUM(clientnum+1)

	// spawn parms are carried from level to level
	float qh_spawn_parms[NUM_SPAWN_PARMS];

	// client known data for deltas
	int qh_old_frags;

	//	Only in NetQuake/Hexen 2
	bool qh_dropasap;					// has been told to go to another level
	bool qh_privileged;					// can execute any host command
	bool qh_sendsignon;					// only valid before spawned

	double qh_last_message;				// reliable messages must be sent
										// periodically

	qsocket_t* qh_netconnection;		// communications handle

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

	int qh_chokecount;
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

	q2edict_t* q2_edict;				// EDICT_NUM(clientnum+1)

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
	int q3_nextReliableTime;			// svs.time when another reliable command will be allowed
	int q3_lastPacketTime;				// svs.time when packet was last received
	int q3_lastConnectTime;				// svs.time when connection started
	int q3_nextSnapshotTime;			// send another snapshot when svs.time >= nextSnapshotTime
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

	q3sharedEntity_t* q3_gentity;		// SV_GentityNum(clientnum)
	wssharedEntity_t* ws_gentity;		// SV_GentityNum(clientnum)
	wmsharedEntity_t* wm_gentity;		// SV_GentityNum(clientnum)
	etsharedEntity_t* et_gentity;		// SV_GentityNum(clientnum)

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

struct server_common_t
{
	serverState_t state;
};
