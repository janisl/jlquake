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

#ifndef _WOLFMP_CG_PUBLIC_H
#define _WOLFMP_CG_PUBLIC_H

#include "../tech3/cg_shared.h"

#define MAX_ENTITIES_IN_SNAPSHOT_WM    256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
struct wmsnapshot_t
{
	int snapFlags;						// SNAPFLAG_RATE_DELAYED, etc
	int ping;

	int serverTime;					// server time the message is valid for (in msec)

	byte areamask[MAX_MAP_AREA_BYTES];					// portalarea visibility bits

	wmplayerState_t ps;							// complete information about the current player at this time

	int numEntities;						// all of the entities that need to be presented
	wmentityState_t entities[MAX_ENTITIES_IN_SNAPSHOT_WM];		// at the time of this snapshot

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
	WMCG_PRINT,
	WMCG_ERROR,
	WMCG_MILLISECONDS,
	WMCG_CVAR_REGISTER,
	WMCG_CVAR_UPDATE,
	WMCG_CVAR_SET,
	WMCG_CVAR_VARIABLESTRINGBUFFER,
	WMCG_ARGC,
	WMCG_ARGV,
	WMCG_ARGS,
	WMCG_FS_FOPENFILE,
	WMCG_FS_READ,
	WMCG_FS_WRITE,
	WMCG_FS_FCLOSEFILE,
	WMCG_SENDCONSOLECOMMAND,
	WMCG_ADDCOMMAND,
	WMCG_SENDCLIENTCOMMAND,
	WMCG_UPDATESCREEN,
	WMCG_CM_LOADMAP,
	WMCG_CM_NUMINLINEMODELS,
	WMCG_CM_INLINEMODEL,
	WMCG_CM_LOADMODEL,
	WMCG_CM_TEMPBOXMODEL,
	WMCG_CM_POINTCONTENTS,
	WMCG_CM_TRANSFORMEDPOINTCONTENTS,
	WMCG_CM_BOXTRACE,
	WMCG_CM_TRANSFORMEDBOXTRACE,
// MrE:
	WMCG_CM_CAPSULETRACE,
	WMCG_CM_TRANSFORMEDCAPSULETRACE,
	WMCG_CM_TEMPCAPSULEMODEL,
// done.
	WMCG_CM_MARKFRAGMENTS,
	WMCG_S_STARTSOUND,
	WMCG_S_STARTSOUNDEX,	//----(SA)	added
	WMCG_S_STARTLOCALSOUND,
	WMCG_S_CLEARLOOPINGSOUNDS,
	WMCG_S_ADDLOOPINGSOUND,
	WMCG_S_UPDATEENTITYPOSITION,
// Ridah, talking animations
	WMCG_S_GETVOICEAMPLITUDE,
// done.
	WMCG_S_RESPATIALIZE,
	WMCG_S_REGISTERSOUND,
	WMCG_S_STARTBACKGROUNDTRACK,
	WMCG_S_STARTSTREAMINGSOUND,
	WMCG_R_LOADWORLDMAP,
	WMCG_R_REGISTERMODEL,
	WMCG_R_REGISTERSKIN,
	WMCG_R_REGISTERSHADER,

	WMCG_R_GETSKINMODEL,		// client allowed to view what the .skin loaded so they can set their model appropriately
	WMCG_R_GETMODELSHADER,	// client allowed the shader handle for given model/surface (for things like debris inheriting shader from explosive)

	WMCG_R_REGISTERFONT,
	WMCG_R_CLEARSCENE,
	WMCG_R_ADDREFENTITYTOSCENE,
	WMCG_GET_ENTITY_TOKEN,
	WMCG_R_ADDPOLYTOSCENE,
// Ridah
	WMCG_R_ADDPOLYSTOSCENE,
// done.
	WMCG_R_ADDLIGHTTOSCENE,

	WMCG_R_ADDCORONATOSCENE,
	WMCG_R_SETFOG,

	WMCG_R_RENDERSCENE,
	WMCG_R_SETCOLOR,
	WMCG_R_DRAWSTRETCHPIC,
	WMCG_R_DRAWSTRETCHPIC_GRADIENT,	//----(SA)	added
	WMCG_R_MODELBOUNDS,
	WMCG_R_LERPTAG,
	WMCG_GETGLCONFIG,
	WMCG_GETGAMESTATE,
	WMCG_GETCURRENTSNAPSHOTNUMBER,
	WMCG_GETSNAPSHOT,
	WMCG_GETSERVERCOMMAND,
	WMCG_GETCURRENTCMDNUMBER,
	WMCG_GETUSERCMD,
	WMCG_SETUSERCMDVALUE,
	WMCG_SETCLIENTLERPORIGIN,			// DHM - Nerve
	WMCG_R_REGISTERSHADERNOMIP,
	WMCG_MEMORY_REMAINING,

	WMCG_KEY_ISDOWN,
	WMCG_KEY_GETCATCHER,
	WMCG_KEY_SETCATCHER,
	WMCG_KEY_GETKEY,

	WMCG_PC_ADD_GLOBAL_DEFINE,
	WMCG_PC_LOAD_SOURCE,
	WMCG_PC_FREE_SOURCE,
	WMCG_PC_READ_TOKEN,
	WMCG_PC_SOURCE_FILE_AND_LINE,
	WMCG_S_STOPBACKGROUNDTRACK,
	WMCG_REAL_TIME,
	WMCG_SNAPVECTOR,
	WMCG_REMOVECOMMAND,
	WMCG_R_LIGHTFORPOINT,

	WMCG_SENDMOVESPEEDSTOGAME,

	WMCG_CIN_PLAYCINEMATIC,
	WMCG_CIN_STOPCINEMATIC,
	WMCG_CIN_RUNCINEMATIC,
	WMCG_CIN_DRAWCINEMATIC,
	WMCG_CIN_SETEXTENTS,
	WMCG_R_REMAP_SHADER,
	WMCG_S_ADDREALLOOPINGSOUND,
	WMCG_S_STOPLOOPINGSOUND,

	WMCG_LOADCAMERA,
	WMCG_STARTCAMERA,
	WMCG_GETCAMERAINFO,

	WMCG_MEMSET = 100,
	WMCG_MEMCPY,
	WMCG_STRNCPY,
	WMCG_SIN,
	WMCG_COS,
	WMCG_ATAN2,
	WMCG_SQRT,
	WMCG_FLOOR,
	WMCG_CEIL,

	WMCG_TESTPRINTINT,
	WMCG_TESTPRINTFLOAT,
	WMCG_ACOS,

	WMCG_INGAME_POPUP,		//----(SA)	added

	// NERVE - SMF
	WMCG_INGAME_CLOSEPOPUP,
	WMCG_LIMBOCHAT,

	WMCG_R_DRAWROTATEDPIC,

	WMCG_KEY_GETBINDINGBUF,
	WMCG_KEY_SETBINDING,
	WMCG_KEY_KEYNUMTOSTRINGBUF,

	WMCG_TRANSLATE_STRING
	// -NERVE - SMF
};

/*
==================================================================

functions exported to the main executable

==================================================================
*/

enum
{
	WMCG_GET_TAG = 9,
//	qboolean CG_GetTag( int clientNum, char *tagname, orientation_t *or );

	WMCG_CHECKCENTERVIEW,
//	qboolean CG_CheckCenterView();
};

#endif
