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

#ifndef _CLIENT_H
#define _CLIENT_H

#include "../common/qcommon.h"
#include "public.h"
#include "../common/file_formats/bsp38.h"

#include "sound/public.h"
#include "renderer/public.h"
#include "input/keycodes.h"
#include "input/public.h"
#include "cinematic/public.h"
#include "ui/ui.h"
#include "ui/console.h"
#include "quakeclientdefs.h"
#include "hexen2clientdefs.h"
#include "quake2clientdefs.h"
#include "quake3clientdefs.h"
#include "wolfclientdefs.h"
#include "game/particles.h"
#include "game/dynamic_lights.h"
#include "game/light_styles.h"
#include "game/input.h"
#include "game/camera.h"
#include "splines/public.h"
#include "translate.h"

#define CSHIFT_CONTENTS 0
#define CSHIFT_DAMAGE   1
#define CSHIFT_BONUS    2
#define CSHIFT_POWERUP  3
#define NUM_CSHIFTS     4

#define SIGNONS     4			// signon messages to receive before connected

#define MAX_MAPSTRING   2048

#define MAX_DEMOS       8
#define MAX_DEMONAME    16

struct cshift_t
{
	int destcolor[3];
	int percent;			// 0-256
};

//	The clientActive_t structure is wiped completely at every new gamestate_t,
// potentially several times during an established connection.
struct clientActive_t
{
	int serverTime;			// may be paused during play

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t viewangles;

	int mouseDx[2];	// added to by mouse events
	int mouseDy[2];
	int mouseIndex;
	int joystickAxis[MAX_JOYSTICK_AXIS];	// set by joystick events

	//	Not in Quake 3
	refdef_t refdef;
	//	Normally playernum + 1, but Hexen 2 changes this for camera views.
	int viewentity;			// cl_entitites[cl.viewentity] = player

	//
	// locally derived information from server state
	//
	qhandle_t model_draw[BIGGEST_MAX_MODELS];
	clipHandle_t model_clip[BIGGEST_MAX_MODELS];

	sfxHandle_t sound_precache[BIGGEST_MAX_SOUNDS];

	int servercount;	// server identification for prespawns

	int playernum;

	//	Only in Quake 2 and Quake 3.
	int timeoutcount;		// it requres several frames in a timeout condition
							// to disconnect, preventing debugging breaks from
							// causing immediate disconnects on continue

	int parseEntitiesNum;	// index (not anded off) into cl_parse_entities[]

	//	Only for Quake and Hexen 2
	cshift_t qh_cshifts[NUM_CSHIFTS];		// color shifts for damage, powerups
	cshift_t qh_prev_cshifts[NUM_CSHIFTS];	// and content types

	int qh_num_entities;	// held in cl_entities array
	int qh_num_statics;		// held in cl_staticentities array

	double qh_mtime[2];		// the timestamp of last two messages

	int qh_maxclients;
	int qh_parsecount;		// server message counter
	int qh_validsequence;	// this is the sequence number of the last good
							// packetentity_t we got.  If this is 0, we can't
							// render a frame yet
	int qh_movemessages;	// since connecting to this server
							// throw out the first couple, so the player
							// doesn't accidentally do something the
							// first frame

	// information for local display
	int qh_stats[MAX_CL_STATS];	// health, etc

	vec3_t qh_mviewangles[2];	// during demo playback viewangles is lerped
								// between these
	vec3_t qh_mvelocity[2];	// update by server, used for lean+bob
							// (0 is newest)
	vec3_t qh_velocity;		// lerped between mvelocity[0] and [1]

	vec3_t qh_punchangles;		// temporary offset
	float qh_punchangle;		// temporar yview kick from weapon firing

	// pitch drifting vars
	float qh_idealpitch;
	float qh_pitchvel;
	qboolean qh_nodrift;
	float qh_driftmove;
	double qh_laststop;

	float qh_viewheight;

	qboolean qh_paused;			// send over by server
	qboolean qh_onground;

	int qh_intermission;	// don't change view angle, full screen, etc
	int qh_completed_time;	// latched at intermission start

	double qh_serverTimeFloat;			// clients view of time, should be between
	// servertime and oldservertime to generate
	// a lerp point for other data
	double qh_oldtime;		// previous cl.time, time-oldtime is used
							// to decay light values and smooth step ups

	float qh_last_received_message;	// (realtime) for net trouble icon

	char qh_levelname[40];	// for display on solo scoreboard
	int qh_gametype;

	char qh_serverinfo[MAX_SERVERINFO_STRING];

	int qh_spectator;

	double qh_last_ping_request;	// while showing scoreboard

	// the client simulates or interpolates movement to get these values
	vec3_t qh_simorg;
	vec3_t qh_simvel;
	vec3_t qh_simangles;

	// information that is static for the entire time connected to a server
	char qh_model_name[BIGGEST_MAX_MODELS][MAX_QPATH];
	char qh_sound_name[BIGGEST_MAX_SOUNDS][MAX_QPATH];

	in_usercmd_t qh_cmd;			// last command sent to the server

	// all player information
	q1player_info_t q1_players[BIGGEST_MAX_CLIENTS_QH];

	int q1_items;			// inventory bit flags
	float q1_item_gettime[32];	// cl.time of aquiring item, for blinking
	float q1_faceanimtime;	// use anim frame if cl.time < this

	q1entity_t q1_viewent;		// weapon model

	// sentcmds[cl.netchan.outgoing_sequence & UPDATE_MASK_QW] = cmd
	qwframe_t qw_frames[UPDATE_BACKUP_QW];

	h2EffectT h2_Effects[MAX_EFFECTS_H2];

	// all player information
	h2player_info_t h2_players[BIGGEST_MAX_CLIENTS_QH];

	int h2_inv_order[MAX_INVENTORY_H2];
	int h2_inv_count;
	int h2_inv_startpos;
	int h2_inv_selected;

	h2client_entvars_t h2_v;// NOTE: not every field will be update - you must specifically add
	// them in functions SV_WriteClientdatatToMessage() and CL_ParseClientdata()

	char h2_puzzle_pieces[8][10];	// puzzle piece names

	float h2_idealroll;
	float h2_rollvel;

	h2entity_t h2_viewent;		// weapon model

	byte h2_current_frame;
	byte h2_reference_frame;
	byte h2_current_sequence;
	byte h2_last_sequence;
	byte h2_need_build;

	h2client_frames2_t h2_frames[3];// 0 = base, 1 = building, 2 = 0 & 1 merged
	short h2_RemoveList[MAX_CLIENT_STATES_H2];
	short h2_NumToRemove;

	int h2_info_mask;
	int h2_info_mask2;

	hwframe_t hw_frames[UPDATE_BACKUP_HW];

	unsigned hw_PIV;			// players in view

	q2frame_t q2_frame;		// received from server
	q2frame_t q2_frames[UPDATE_BACKUP_Q2];

	float q2_lerpfrac;		// between oldframe and frame

	int q2_timedemo_frames;
	int q2_timedemo_start;

	bool q2_refresh_prepped;	// false if on new level or new ref dll
	bool q2_sound_prepped;		// ambient sounds can start

	q2usercmd_t q2_cmds[CMD_BACKUP_Q2];	// each mesage will send several old cmds
	int q2_cmd_time[CMD_BACKUP_Q2];	// time sent, for calculating pings
	short q2_predicted_origins[CMD_BACKUP_Q2][3];	// for debug comparing against server

	float q2_predicted_step;				// for stair up smoothing
	unsigned q2_predicted_step_time;

	vec3_t q2_predicted_origin;	// generated by CL_PredictMovement
	vec3_t q2_predicted_angles;
	vec3_t q2_prediction_error;

	int q2_surpressCount;		// number of messages rate supressed

	//
	// transient data from server
	//
	char q2_layout[1024];		// general 2D overlay
	int q2_inventory[MAX_ITEMS_Q2];

	//
	// server state information
	//
	qboolean q2_attractloop;		// running the attract loop, any key will menu
	char q2_gamedir[MAX_QPATH];

	char q2_configstrings[MAX_CONFIGSTRINGS_Q2][MAX_QPATH];

	image_t* q2_image_precache[MAX_IMAGES_Q2];

	q2clientinfo_t q2_clientinfo[MAX_CLIENTS_Q2];
	q2clientinfo_t q2_baseclientinfo;

	q3clSnapshot_t q3_snap;			// latest received from server
	wsclSnapshot_t ws_snap;				// latest received from server
	wmclSnapshot_t wm_snap;				// latest received from server
	etclSnapshot_t et_snap;				// latest received from server

	int q3_oldServerTime;		// to prevent time from flowing bakcwards
	int q3_oldFrameServerTime;	// to check tournament restarts
	int q3_serverTimeDelta;	// cl.serverTime = cls.realtime + cl.serverTimeDelta
	// this value changes as net lag varies
	bool q3_extrapolatedSnapshot;	// set if any cgame frame has been forced to extrapolate
									// cleared when CL_AdjustTimeDelta looks at it
	bool q3_newSnapshots;		// set on parse of any valid packet

	q3gameState_t q3_gameState;			// configstrings
	wsgameState_t ws_gameState;			// configstrings
	wmgameState_t wm_gameState;			// configstrings
	etgameState_t et_gameState;			// configstrings
	char q3_mapname[MAX_QPATH];	// extracted from Q3CS_SERVERINFO

	// cgame communicates a few values to the client system
	int q3_cgameUserCmdValue;	// current weapon to add to q3usercmd_t
	float q3_cgameSensitivity;

	// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	// properly generated command
	q3usercmd_t q3_cmds[CMD_BACKUP_Q3];	// each mesage will send several old cmds
	wsusercmd_t ws_cmds[CMD_BACKUP_Q3];		// each mesage will send several old cmds
	wmusercmd_t wm_cmds[CMD_BACKUP_Q3];		// each mesage will send several old cmds
	etusercmd_t et_cmds[CMD_BACKUP_Q3];		// each mesage will send several old cmds
	int q3_cmdNumber;			// incremented each frame, because multiple
	// frames may need to be packed into a single packet

	q3outPacket_t q3_outPackets[PACKET_BACKUP_Q3];	// information about each packet we have sent out

	int q3_serverId;			// included in each client message so the server
	// can tell if it is for a prior map_restart
	// big stuff at end of structure so most offsets are 15 bits or less
	q3clSnapshot_t q3_snapshots[PACKET_BACKUP_Q3];
	wsclSnapshot_t ws_snapshots[PACKET_BACKUP_Q3];
	wmclSnapshot_t wm_snapshots[PACKET_BACKUP_Q3];
	etclSnapshot_t et_snapshots[PACKET_BACKUP_Q3];

	q3entityState_t q3_entityBaselines[MAX_GENTITIES_Q3];	// for delta compression when not in previous frame
	wsentityState_t ws_entityBaselines[MAX_GENTITIES_Q3];	// for delta compression when not in previous frame
	wmentityState_t wm_entityBaselines[MAX_GENTITIES_Q3];	// for delta compression when not in previous frame
	etentityState_t et_entityBaselines[MAX_GENTITIES_Q3];	// for delta compression when not in previous frame

	q3entityState_t q3_parseEntities[MAX_PARSE_ENTITIES_Q3];
	wsentityState_t ws_parseEntities[MAX_PARSE_ENTITIES_Q3];
	wmentityState_t wm_parseEntities[MAX_PARSE_ENTITIES_Q3];
	etentityState_t et_parseEntities[MAX_PARSE_ENTITIES_Q3];

	// NOTE TTimo - UI uses LIMBOCHAT_WIDTH_WA strings (140),
	// but for the processing in CL_AddToLimboChat we need some safe room
	char wa_limboChatMsgs[LIMBOCHAT_HEIGHT_WA][LIMBOCHAT_WIDTH_WA * 3 + 1];
	int wa_limboChatPos;

	bool wa_cameraMode;		//----(SA)	added for control of input while watching cinematics

	int wb_cgameUserHoldableValue;			// current holdable item to add to wsusercmd_t	//----(SA)	added

	int ws_cgameCld;						// NERVE - SMF

	int wm_cgameMpSetup;					// NERVE - SMF
	int wm_cgameMpIdentClient;				// NERVE - SMF
	vec3_t wm_cgameClientLerpOrigin;		// DHM - Nerve

	bool wm_corruptedTranslationFile;
	char wm_translationVersion[MAX_TOKEN_CHARS_Q3];

	int et_cgameFlags;						// flags that can be set by the gamecode

	// Arnout: double tapping
	etdoubleTap_t et_doubleTap;
};

// download type
enum dltype_t
{
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_single
};

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.

=============================================================================
*/

struct clientConnection_t
{
	// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t netchan;

	// file transfer from server
	fileHandle_t download;
	char downloadTempName[MAX_OSPATH];
	char downloadName[MAX_OSPATH];
	int downloadNumber;
	//	Only in QuakeWorld and HexenWorld
	dltype_t downloadType;
	//	Not in Quake 3
	int downloadPercent;
	//	Only in Quake 3
	int downloadBlock;	// block we are waiting for
	int downloadCount;	// how many bytes we got
	int downloadSize;	// how many bytes we got
	char downloadList[MAX_INFO_STRING_Q3];	// list of paks we need to download
	bool downloadRestart;	// if true, we need to do another FS_Restart because we downloaded a pak
	//	Only in Enemy Territory
	int downloadFlags;			// misc download behaviour flags sent by the server

	// demo information
	bool demorecording;
	bool demoplaying;
	fileHandle_t demofile;

	int qh_signon;			// 0 to SIGNONS

	int q3_clientNum;
	int q3_lastPacketSentTime;			// for retransmits during connection
	int q3_lastPacketTime;				// for timeouts

	netadr_t q3_serverAddress;
	int q3_connectTime;				// for connection retransmits
	int q3_connectPacketCount;			// for display on connection dialog
	char q3_serverMessage[MAX_TOKEN_CHARS_Q3];	// for display on connection dialog

	int q3_challenge;					// from the server to use for connecting
	int q3_checksumFeed;				// from the server for checksum calculations

	// these are our reliable messages that go to the server
	int q3_reliableSequence;
	int q3_reliableAcknowledge;		// the last one the server has executed
	char q3_reliableCommands[BIGGEST_MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid

	// message sequence is used by both the network layer and the
	// delta compression layer
	int q3_serverMessageSequence;

	// reliable messages received from server
	int q3_serverCommandSequence;
	int q3_lastExecutedServerCommand;		// last server command grabbed or executed with CL_GetServerCommand
	char q3_serverCommands[BIGGEST_MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	char q3_demoName[MAX_QPATH];
	bool q3_spDemoRecording;
	bool q3_demowaiting;	// don't record until a non-delta message is received
	bool q3_firstDemoFrameSkipped;

	int q3_timeDemoFrames;		// counter of rendered frames
	int q3_timeDemoStart;		// cls.realtime before first frame
	int q3_timeDemoBaseTime;	// each frame will be at this time + frameNum * 50

	int wm_onlyVisibleClients;					// DHM - Nerve

	bool wm_waverecording;
	fileHandle_t wm_wavefile;
	int wm_wavetime;

	// unreliable binary data to send to server
	int et_binaryMessageLength;
	char et_binaryMessage[MAX_BINARY_MESSAGE_ET];
	bool et_binaryMessageOverflowed;

	// www downloading
	bool et_bWWWDl;		// we have a www download going
	bool et_bWWWDlAborting;		// disable the CL_WWWDownload until server gets us a gamestate (used for aborts)
	char et_redirectedList[MAX_INFO_STRING_Q3];			// list of files that we downloaded through a redirect since last FS_ComparePaks
	char et_badChecksumList[MAX_INFO_STRING_Q3];		// list of files for which wwwdl redirect is broken (wrong checksum)
};

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

enum connstate_t
{
	//	!!!!!! Used by Quake 3 UI VM, do not change !!!!!!
	CA_UNINITIALIZED,
	CA_DISCONNECTED,	// not talking to a server
	CA_AUTHORIZING,		// not used any more, was checking cd key
	CA_CONNECTING,		// sending request packets to the server
	CA_CHALLENGING,		// sending challenge packets to the server
	CA_CONNECTED,		// netchan_t established, getting gamestate
	CA_LOADING,			// only during cgame initialization, never during main loop
	CA_PRIMED,			// got gamestate, waiting for first frame
	CA_ACTIVE,			// game views should be displayed
	CA_CINEMATIC,		// playing a cinematic or a static pic, not connected to a server

	//	New statuses.
	//	This should be replaced with cvar check.
	CA_DEDICATED,
	//	This is stupid, should get rid of it.
	CA_DEMOSTART
};

struct clientStatic_t
{
	connstate_t state;		// connection status

	int realtime;			// ignores pause
	int framecount;
	int frametime;			// msec since last frame
	int realFrametime;		// ignoring pause, so console always works

	// rendering info
	glconfig_t glconfig;
	qhandle_t charSetShader;
	qhandle_t whiteShader;
	qhandle_t consoleShader;
	qhandle_t consoleShader2;

	char servername[MAX_OSPATH];		// name of server from original connect (used by reconnect)

	int disable_screen;		// showing loading plaque between levels
							// or changing rendering dlls
							// if time gets > 30 seconds ahead, break it

	int quakePort;			// a 16 bit value that allows quake servers
							// to work around address translating routers

	int challenge;			// from the server to use for connecting

	char qh_spawnparms[MAX_MAPSTRING];	// to restart a level

	// demo loop control
	int qh_demonum;		// -1 = don't play demos
	char qh_demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

	// demo recording info must be here, because record is started before
	// entering a map (and clearing client_state_t)
	bool qh_timedemo;
	int qh_td_lastframe;		// to meter out one message a frame
	int qh_td_startframe;		// host_framecount at start
	float qh_td_starttime;		// realtime at second frame of timedemo

	int qh_forcetrack;			// -1 = use normal cd track

	// connection information
	qsocket_t* qh_netcon;

	// private userinfo for sending to masterless servers
	char qh_userinfo[MAX_INFO_STRING_QW];

	float qh_latency;		// rolling average

	float q2_frametimeFloat;		// seconds since last frame

	// screen rendering information
	int q2_disable_servercount;	// when we receive a frame and cl.servercount
								// > cls.disable_servercount, clear disable_screen

	// connection information
	float q2_connect_time;		// for connection retransmits

	int q2_serverProtocol;		// in case we are doing some kind of version hack

	// demo recording info must be here, so it isn't cleared on level change
	bool q2_demowaiting;	// don't record until a non-delta message is received

	// when the server clears the hunk, all of these must be restarted
	bool q3_rendererStarted;
	bool q3_soundStarted;
	bool q3_soundRegistered;
	bool q3_uiStarted;
	bool q3_cgameStarted;

	int q3_numlocalservers;
	q3serverInfo_t q3_localServers[MAX_OTHER_SERVERS_Q3];

	int q3_numglobalservers;
	q3serverInfo_t q3_globalServers[BIGGEST_MAX_GLOBAL_SERVERS];
	// additional global servers
	int q3_numGlobalServerAddresses;
	netadr_t q3_globalServerAddresses[BIGGEST_MAX_GLOBAL_SERVERS];

	int q3_numfavoriteservers;
	q3serverInfo_t q3_favoriteServers[MAX_OTHER_SERVERS_Q3];

	int q3_nummplayerservers;
	q3serverInfo_t q3_mplayerServers[MAX_OTHER_SERVERS_Q3];

	int q3_pingUpdateSource;		// source currently pinging or updating

	int q3_masterNum;

	// update server info
	netadr_t q3_updateServer;
	char q3_updateChallenge[MAX_TOKEN_CHARS_Q3];
	char q3_updateInfoString[MAX_INFO_STRING_Q3];

	netadr_t q3_authorizeServer;

	bool ws_endgamemenu;			// bring up the end game credits menu next frame

	bool et_doCachePurge;			// Arnout: empty the renderer cache as soon as possible

	// www downloading
	// in the static stuff since this may have to survive server disconnects
	// if new stuff gets added, CL_ClearStaticDownload code needs to be updated for clear up
	bool et_bWWWDlDisconnected;	// keep going with the download after server disconnect
	char et_downloadName[MAX_OSPATH];
	char et_downloadTempName[MAX_OSPATH];	// in wwwdl mode, this is OS path (it's a qpath otherwise)
	char et_originalDownloadName[MAX_QPATH];	// if we get a redirect, keep a copy of the original file path
	bool et_downloadRestart;// if true, we need to do another FS_Restart because we downloaded a pak
};

extern clientActive_t cl;
extern clientConnection_t clc;
extern clientStatic_t cls;

extern int bitcounts[32];

extern Cvar* cl_inGameVideo;

extern Cvar* clqh_nolerp;

extern Cvar* clqh_name;
extern Cvar* clqh_color;

extern byte* playerTranslation;
extern int color_offsets[MAX_PLAYER_CLASS];

extern float clqh_server_version;	// version of server we connected to

extern Cvar* chase_active;

void CL_SharedInit();
int CL_ScaledMilliseconds();
void CL_CalcQuakeSkinTranslation(int top, int bottom, byte* translate);
void CL_CalcHexen2SkinTranslation(int top, int bottom, int playerClass, byte* translate);
float CLQH_LerpPoint();
void CL_AddReliableCommand(const char* cmd);

void Chase_Init();
void Chase_Update();

char* Sys_GetClipboardData();	// note that this isn't journaled...

float frand();	// 0 to 1
float crand();	// -1 to 1

//---------------------------------------------
//	Must be provided
//	Called by Windows driver.
void Key_ClearStates();
float* CL_GetSimOrg();
void CL_WriteWaveFilePacket(int endtime);
void SCR_UpdateScreen();
void CL_NextDemo();
void SCRQH_BeginLoadingPlaque();
void Con_ToggleConsole_f();
void Com_Quit_f();

#endif
