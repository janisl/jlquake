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

	//	Only for Quake and Hexen 2
	cshift_t qh_cshifts[NUM_CSHIFTS];		// color shifts for damage, powerups
	cshift_t qh_prev_cshifts[NUM_CSHIFTS];	// and content types

	int qh_num_entities;	// held in cl_entities array
	int qh_num_statics;		// held in cl_staticentities array

	double qh_mtime[2];		// the timestamp of last two messages	

	int qh_maxclients;

	// all player information
	q1player_info_t q1_players[BIGGEST_MAX_CLIENTS_Q1];

	h2EffectT h2_Effects[MAX_EFFECTS_H2];

	// all player information
	h2player_info_t h2_players[H2BIGGEST_MAX_CLIENTS];

	hwframe_t hw_frames[UPDATE_BACKUP_HW];

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
