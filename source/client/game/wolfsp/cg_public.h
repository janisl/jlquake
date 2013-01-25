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

#ifndef _WOLFSP_CG_PUBLIC_H
#define _WOLFSP_CG_PUBLIC_H

#include "../tech3/cg_shared.h"

#define MAX_ENTITIES_IN_SNAPSHOT_WS    256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
struct wssnapshot_t {
	int snapFlags;						// SNAPFLAG_RATE_DELAYED, etc
	int ping;

	int serverTime;					// server time the message is valid for (in msec)

	byte areamask[ MAX_MAP_AREA_BYTES ];					// portalarea visibility bits

	wsplayerState_t ps;							// complete information about the current player at this time

	int numEntities;						// all of the entities that need to be presented
	wsentityState_t entities[ MAX_ENTITIES_IN_SNAPSHOT_WS ];		// at the time of this snapshot

	int numServerCommands;					// text based server commands to execute when this
	int serverCommandSequence;				// snapshot becomes current
};

/*
==================================================================

functions imported from the main executable

==================================================================
*/

enum
{
	WSCG_PRINT,
	WSCG_ERROR,
	WSCG_MILLISECONDS,
	WSCG_CVAR_REGISTER,
	WSCG_CVAR_UPDATE,
	WSCG_CVAR_SET,
	WSCG_CVAR_VARIABLESTRINGBUFFER,
	WSCG_ARGC,
	WSCG_ARGV,
	WSCG_ARGS,
	WSCG_FS_FOPENFILE,
	WSCG_FS_READ,
	WSCG_FS_WRITE,
	WSCG_FS_FCLOSEFILE,
	WSCG_SENDCONSOLECOMMAND,
	WSCG_ADDCOMMAND,
	WSCG_SENDCLIENTCOMMAND,
	WSCG_UPDATESCREEN,
	WSCG_CM_LOADMAP,
	WSCG_CM_NUMINLINEMODELS,
	WSCG_CM_INLINEMODEL,
	WSCG_CM_LOADMODEL,
	WSCG_CM_TEMPBOXMODEL,
	WSCG_CM_POINTCONTENTS,
	WSCG_CM_TRANSFORMEDPOINTCONTENTS,
	WSCG_CM_BOXTRACE,
	WSCG_CM_TRANSFORMEDBOXTRACE,
// MrE:
	WSCG_CM_CAPSULETRACE,
	WSCG_CM_TRANSFORMEDCAPSULETRACE,
	WSCG_CM_TEMPCAPSULEMODEL,
// done.
	WSCG_CM_MARKFRAGMENTS,
	WSCG_S_STARTSOUND,
	WSCG_S_STARTSOUNDEX,	//----(SA)	added
	WSCG_S_STARTLOCALSOUND,
	WSCG_S_CLEARLOOPINGSOUNDS,
	WSCG_S_ADDLOOPINGSOUND,
	WSCG_S_UPDATEENTITYPOSITION,
// Ridah, talking animations
	WSCG_S_GETVOICEAMPLITUDE,
// done.
	WSCG_S_RESPATIALIZE,
	WSCG_S_REGISTERSOUND,
	WSCG_S_STARTBACKGROUNDTRACK,
	WSCG_S_FADESTREAMINGSOUND,	//----(SA)	modified
	WSCG_S_FADEALLSOUNDS,			//----(SA)	added for fading out everything
	WSCG_S_STARTSTREAMINGSOUND,
	WSCG_R_LOADWORLDMAP,
	WSCG_R_REGISTERMODEL,
	WSCG_R_REGISTERSKIN,
	WSCG_R_REGISTERSHADER,

	WSCG_R_GETSKINMODEL,		// client allowed to view what the .skin loaded so they can set their model appropriately
	WSCG_R_GETMODELSHADER,	// client allowed the shader handle for given model/surface (for things like debris inheriting shader from explosive)

	WSCG_R_REGISTERFONT,
	WSCG_R_CLEARSCENE,
	WSCG_R_ADDREFENTITYTOSCENE,
	WSCG_GET_ENTITY_TOKEN,
	WSCG_R_ADDPOLYTOSCENE,
// Ridah
	WSCG_R_ADDPOLYSTOSCENE,
	WSCG_RB_ZOMBIEFXADDNEWHIT,
// done.
	WSCG_R_ADDLIGHTTOSCENE,

	WSCG_R_ADDCORONATOSCENE,
	WSCG_R_SETFOG,

	WSCG_R_RENDERSCENE,
	WSCG_R_SETCOLOR,
	WSCG_R_DRAWSTRETCHPIC,
	WSCG_R_DRAWSTRETCHPIC_GRADIENT,	//----(SA)	added
	WSCG_R_MODELBOUNDS,
	WSCG_R_LERPTAG,
	WSCG_GETGLCONFIG,
	WSCG_GETGAMESTATE,
	WSCG_GETCURRENTSNAPSHOTNUMBER,
	WSCG_GETSNAPSHOT,
	WSCG_GETSERVERCOMMAND,
	WSCG_GETCURRENTCMDNUMBER,
	WSCG_GETUSERCMD,
	WSCG_SETUSERCMDVALUE,
	WSCG_R_REGISTERSHADERNOMIP,
	WSCG_MEMORY_REMAINING,

	WSCG_KEY_ISDOWN,
	WSCG_KEY_GETCATCHER,
	WSCG_KEY_SETCATCHER,
	WSCG_KEY_GETKEY,

	WSCG_PC_ADD_GLOBAL_DEFINE,
	WSCG_PC_LOAD_SOURCE,
	WSCG_PC_FREE_SOURCE,
	WSCG_PC_READ_TOKEN,
	WSCG_PC_SOURCE_FILE_AND_LINE,
	WSCG_S_STOPBACKGROUNDTRACK,
	WSCG_REAL_TIME,
	WSCG_SNAPVECTOR,
	WSCG_REMOVECOMMAND,

	WSCG_SENDMOVESPEEDSTOGAME,

	WSCG_CIN_PLAYCINEMATIC,
	WSCG_CIN_STOPCINEMATIC,
	WSCG_CIN_RUNCINEMATIC,
	WSCG_CIN_DRAWCINEMATIC,
	WSCG_CIN_SETEXTENTS,
	WSCG_R_REMAP_SHADER,
	WSCG_S_STOPLOOPINGSOUND,
	WSCG_S_STOPSTREAMINGSOUND,	//----(SA)	added

	WSCG_LOADCAMERA,
	WSCG_STARTCAMERA,
	WSCG_STOPCAMERA,	//----(SA)	added
	WSCG_GETCAMERAINFO,

	WSCG_MEMSET = 110,
	WSCG_MEMCPY,
	WSCG_STRNCPY,
	WSCG_SIN,
	WSCG_COS,
	WSCG_ATAN2,
	WSCG_SQRT,
	WSCG_FLOOR,
	WSCG_CEIL,

	WSCG_TESTPRINTINT,
	WSCG_TESTPRINTFLOAT,
	WSCG_ACOS,

	WSCG_INGAME_POPUP,		//----(SA)	added
	WSCG_INGAME_CLOSEPOPUP,	// NERVE - SMF
	WSCG_LIMBOCHAT,			// NERVE - SMF

	WSCG_GETMODELINFO
};

/*
==================================================================

functions exported to the main executable

==================================================================
*/

enum
{
	WSCG_GET_TAG = 9,
//	qboolean CG_GetTag( int clientNum, char *tagname, orientation_t *or );
};

#endif
