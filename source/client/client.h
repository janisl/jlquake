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

#ifndef _CLIENT_H
#define _CLIENT_H

#include "../core/core.h"
#include "../core/file_formats/bsp38.h"

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
#include "game/particles.h"
#include "game/dynamic_lights.h"
#include "game/light_styles.h"

#define CSHIFT_CONTENTS	0
#define CSHIFT_DAMAGE	1
#define CSHIFT_BONUS	2
#define CSHIFT_POWERUP	3
#define NUM_CSHIFTS		4

#define SIGNONS		4			// signon messages to receive before connected

#define MAX_MAPSTRING	2048

struct cshift_t
{
	int		destcolor[3];
	int		percent;		// 0-256
};

struct clientActiveCommon_t
{
	int serverTime;			// may be paused during play

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t viewangles;

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
	char qh_model_name[MAX_MODELS_Q1][MAX_QPATH];
	char qh_sound_name[MAX_SOUNDS_Q1][MAX_QPATH];

	// all player information
	q1player_info_t q1_players[BIGGEST_MAX_CLIENTS_Q1];

	q1usercmd_t q1_cmd;			// last command sent to the server

	int q1_items;			// inventory bit flags
	float q1_item_gettime[32];	// cl.time of aquiring item, for blinking
	float q1_faceanimtime;	// use anim frame if cl.time < this

	q1entity_t q1_viewent;		// weapon model

	// sentcmds[cl.netchan.outgoing_sequence & UPDATE_MASK_QW] = cmd
	qwframe_t qw_frames[UPDATE_BACKUP_QW];

	h2EffectT h2_Effects[MAX_EFFECTS_H2];

	// all player information
	h2player_info_t h2_players[H2BIGGEST_MAX_CLIENTS];

	h2usercmd_t h2_cmd;			// last command sent to the server

	int h2_inv_order[MAX_INVENTORY_H2];
	int h2_inv_count;
	int h2_inv_startpos;
	int h2_inv_selected;

	h2client_entvars_t h2_v; // NOTE: not every field will be update - you must specifically add
	                   // them in functions SV_WriteClientdatatToMessage() and CL_ParseClientdata()

	char h2_puzzle_pieces[8][10]; // puzzle piece names

	float h2_idealroll;
	float h2_rollvel;

	h2entity_t h2_viewent;		// weapon model

	byte h2_current_frame;
	byte h2_reference_frame;
	byte h2_current_sequence;
	byte h2_last_sequence;
	byte h2_need_build;

	h2client_frames2_t h2_frames[3]; // 0 = base, 1 = building, 2 = 0 & 1 merged
	short h2_RemoveList[MAX_CLIENT_STATES_H2];
	short h2_NumToRemove;

	int h2_info_mask;
	int h2_info_mask2;

	hwframe_t hw_frames[UPDATE_BACKUP_HW];

	unsigned hw_PIV;			// players in view

	q2frame_t q2_frame;		// received from server
	q2frame_t q2_frames[UPDATE_BACKUP_Q2];

	float q2_lerpfrac;		// between oldframe and frame
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

struct clientConnectionCommon_t
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
	char downloadList[MAX_INFO_STRING_Q3]; // list of paks we need to download
	bool downloadRestart;	// if true, we need to do another FS_Restart because we downloaded a pak

	// demo information
	bool demorecording;
	bool demoplaying;
	fileHandle_t demofile;

	int qh_signon;			// 0 to SIGNONS
};

enum connstate_t
{
	//	!!!!!! Used by Quake 3 UI VM, do not change !!!!!!
	CA_UNINITIALIZED,
	CA_DISCONNECTED, 	// not talking to a server
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

struct clientStaticCommon_t
{
	connstate_t state;				// connection status

	int framecount;
	int frametime;			// msec since last frame

	// rendering info
	glconfig_t glconfig;
	qhandle_t charSetShader;
	qhandle_t whiteShader;
	qhandle_t consoleShader;

	int disable_screen;		// showing loading plaque between levels
							// or changing rendering dlls
							// if time gets > 30 seconds ahead, break it

	char qh_spawnparms[MAX_MAPSTRING];	// to restart a level
};

extern clientActiveCommon_t* cl_common;
extern clientConnectionCommon_t* clc_common;
extern clientStaticCommon_t* cls_common;

extern int bitcounts[32];

extern Cvar* cl_inGameVideo;

extern Cvar* clqh_name;
extern Cvar* clqh_color;

extern byte* playerTranslation;
extern int color_offsets[MAX_PLAYER_CLASS];

void CL_SharedInit();
int CL_ScaledMilliseconds();
void CL_CalcQuakeSkinTranslation(int top, int bottom, byte* translate);
void CL_CalcHexen2SkinTranslation(int top, int bottom, int playerClass, byte* translate);

char* Sys_GetClipboardData();	// note that this isn't journaled...

float frand();	// 0 to 1
float crand();	// -1 to 1

//---------------------------------------------
//	Must be provided
//	Called by Windows driver.
void Key_ClearStates();
float* CL_GetSimOrg();

#endif
