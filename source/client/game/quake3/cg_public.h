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

#ifndef _QUAKE3_CG_PUBLIC_H
#define _QUAKE3_CG_PUBLIC_H

#include "../tech3/cg_shared.h"

#define MAX_ENTITIES_IN_SNAPSHOT_Q3 256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
struct q3snapshot_t
{
	int snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int ping;

	int serverTime;		// server time the message is valid for (in msec)

	byte areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	q3playerState_t ps;			// complete information about the current player at this time

	int numEntities;			// all of the entities that need to be presented
	q3entityState_t entities[MAX_ENTITIES_IN_SNAPSHOT_Q3];	// at the time of this snapshot

	int numServerCommands;		// text based server commands to execute when this
	int serverCommandSequence;	// snapshot becomes current
};

struct q3refEntity_t
{
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel;				// opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane;		// projection shadows go here, stencils go slightly lower

	vec3_t axis[3];			// rotation vectors
	qboolean nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	vec3_t origin;				// also used as MODEL_BEAM's "from"
	int frame;				// also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	vec3_t oldorigin;			// also used as MODEL_BEAM's "to"
	int oldframe;
	float backlerp;			// 0.0 = current, 1.0 = old

	// texturing
	int skinNum;			// inline skin index
	qhandle_t customSkin;			// NULL for default skin
	qhandle_t customShader;		// use one image for the entire thing

	// misc
	byte shaderRGBA[4];		// colors used by rgbgen entity shaders
	float shaderTexCoord[2];	// texture coordinates used by tcMod entity modifiers
	float shaderTime;			// subtracted from refdef time to control effect start times
	// Also used for synctime

	// extra sprite information
	float radius;
	float rotation;
};

struct q3refdef_t
{
	int x;
	int y;
	int width;
	int height;
	float fov_x;
	float fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];		// transformation matrix

	// time in milliseconds for shader effects and other time dependent rendering issues
	int time;

	int rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
};

struct q3glconfig_t
{
	char renderer_string[MAX_STRING_CHARS];
	char vendor_string[MAX_STRING_CHARS];
	char version_string[MAX_STRING_CHARS];
	char extensions_string[BIG_INFO_STRING];

	int maxTextureSize;			// queried from GL
	int maxActiveTextures;		// multitexture ability

	int colorBits, depthBits, stencilBits;

	int driverType;
	int hardwareType;

	qboolean deviceSupportsGamma;
	int textureCompression;
	qboolean textureEnvAddAvailable;

	int vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float windowAspect;

	int displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Win32 ICD that used CDS will have this set to TRUE.
	qboolean isFullscreen;
	qboolean stereoEnabled;
	qboolean smpActive;		// dual processor
};

/*
==================================================================

functions imported from the main executable

==================================================================
*/

enum
{
	Q3CG_PRINT,
	Q3CG_ERROR,
	Q3CG_MILLISECONDS,
	Q3CG_CVAR_REGISTER,
	Q3CG_CVAR_UPDATE,
	Q3CG_CVAR_SET,
	Q3CG_CVAR_VARIABLESTRINGBUFFER,
	Q3CG_ARGC,
	Q3CG_ARGV,
	Q3CG_ARGS,
	Q3CG_FS_FOPENFILE,
	Q3CG_FS_READ,
	Q3CG_FS_WRITE,
	Q3CG_FS_FCLOSEFILE,
	Q3CG_SENDCONSOLECOMMAND,
	Q3CG_ADDCOMMAND,
	Q3CG_SENDCLIENTCOMMAND,
	Q3CG_UPDATESCREEN,
	Q3CG_CM_LOADMAP,
	Q3CG_CM_NUMINLINEMODELS,
	Q3CG_CM_INLINEMODEL,
	Q3CG_CM_LOADMODEL,
	Q3CG_CM_TEMPBOXMODEL,
	Q3CG_CM_POINTCONTENTS,
	Q3CG_CM_TRANSFORMEDPOINTCONTENTS,
	Q3CG_CM_BOXTRACE,
	Q3CG_CM_TRANSFORMEDBOXTRACE,
	Q3CG_CM_MARKFRAGMENTS,
	Q3CG_S_STARTSOUND,
	Q3CG_S_STARTLOCALSOUND,
	Q3CG_S_CLEARLOOPINGSOUNDS,
	Q3CG_S_ADDLOOPINGSOUND,
	Q3CG_S_UPDATEENTITYPOSITION,
	Q3CG_S_RESPATIALIZE,
	Q3CG_S_REGISTERSOUND,
	Q3CG_S_STARTBACKGROUNDTRACK,
	Q3CG_R_LOADWORLDMAP,
	Q3CG_R_REGISTERMODEL,
	Q3CG_R_REGISTERSKIN,
	Q3CG_R_REGISTERSHADER,
	Q3CG_R_CLEARSCENE,
	Q3CG_R_ADDREFENTITYTOSCENE,
	Q3CG_R_ADDPOLYTOSCENE,
	Q3CG_R_ADDLIGHTTOSCENE,
	Q3CG_R_RENDERSCENE,
	Q3CG_R_SETCOLOR,
	Q3CG_R_DRAWSTRETCHPIC,
	Q3CG_R_MODELBOUNDS,
	Q3CG_R_LERPTAG,
	Q3CG_GETGLCONFIG,
	Q3CG_GETGAMESTATE,
	Q3CG_GETCURRENTSNAPSHOTNUMBER,
	Q3CG_GETSNAPSHOT,
	Q3CG_GETSERVERCOMMAND,
	Q3CG_GETCURRENTCMDNUMBER,
	Q3CG_GETUSERCMD,
	Q3CG_SETUSERCMDVALUE,
	Q3CG_R_REGISTERSHADERNOMIP,
	Q3CG_MEMORY_REMAINING,
	Q3CG_R_REGISTERFONT,
	Q3CG_KEY_ISDOWN,
	Q3CG_KEY_GETCATCHER,
	Q3CG_KEY_SETCATCHER,
	Q3CG_KEY_GETKEY,
	Q3CG_PC_ADD_GLOBAL_DEFINE,
	Q3CG_PC_LOAD_SOURCE,
	Q3CG_PC_FREE_SOURCE,
	Q3CG_PC_READ_TOKEN,
	Q3CG_PC_SOURCE_FILE_AND_LINE,
	Q3CG_S_STOPBACKGROUNDTRACK,
	Q3CG_REAL_TIME,
	Q3CG_SNAPVECTOR,
	Q3CG_REMOVECOMMAND,
	Q3CG_R_LIGHTFORPOINT,
	Q3CG_CIN_PLAYCINEMATIC,
	Q3CG_CIN_STOPCINEMATIC,
	Q3CG_CIN_RUNCINEMATIC,
	Q3CG_CIN_DRAWCINEMATIC,
	Q3CG_CIN_SETEXTENTS,
	Q3CG_R_REMAP_SHADER,
	Q3CG_S_ADDREALLOOPINGSOUND,
	Q3CG_S_STOPLOOPINGSOUND,

	Q3CG_CM_TEMPCAPSULEMODEL,
	Q3CG_CM_CAPSULETRACE,
	Q3CG_CM_TRANSFORMEDCAPSULETRACE,
	Q3CG_R_ADDADDITIVELIGHTTOSCENE,
	Q3CG_GET_ENTITY_TOKEN,
	Q3CG_R_ADDPOLYSTOSCENE,
	Q3CG_R_INPVS,
	// 1.32
	Q3CG_FS_SEEK,

	Q3CG_MEMSET = 100,
	Q3CG_MEMCPY,
	Q3CG_STRNCPY,
	Q3CG_SIN,
	Q3CG_COS,
	Q3CG_ATAN2,
	Q3CG_SQRT,
	Q3CG_FLOOR,
	Q3CG_CEIL,
	Q3CG_TESTPRINTINT,
	Q3CG_TESTPRINTFLOAT,
	Q3CG_ACOS
};

#endif
