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
struct wssnapshot_t
{
	int snapFlags;						// SNAPFLAG_RATE_DELAYED, etc
	int ping;

	int serverTime;					// server time the message is valid for (in msec)

	byte areamask[MAX_MAP_AREA_BYTES];					// portalarea visibility bits

	wsplayerState_t ps;							// complete information about the current player at this time

	int numEntities;						// all of the entities that need to be presented
	wsentityState_t entities[MAX_ENTITIES_IN_SNAPSHOT_WS];		// at the time of this snapshot

	int numServerCommands;					// text based server commands to execute when this
	int serverCommandSequence;				// snapshot becomes current
};

//	Overlaps with RF_WRAP_FRAMES
#define WSRF_BLINK            (1 << 9)		// eyes in 'blink' state

struct wsrefEntity_t
{
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel;				// opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin;			// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane;				// projection shadows go here, stencils go slightly lower

	vec3_t axis[3];					// rotation vectors
	vec3_t torsoAxis[3];			// rotation vectors for torso section of skeletal animation
	qboolean nonNormalizedAxes;		// axis are not normalized, i.e. they have scale
	float origin[3];				// also used as MODEL_BEAM's "from"
	int frame;						// also used as MODEL_BEAM's diameter
	int torsoFrame;					// skeletal torso can have frame independant of legs frame

	vec3_t scale;		//----(SA)	added

	// previous data for frame interpolation
	float oldorigin[3];				// also used as MODEL_BEAM's "to"
	int oldframe;
	int oldTorsoFrame;
	float backlerp;					// 0.0 = current, 1.0 = old
	float torsoBacklerp;

	// texturing
	int skinNum;					// inline skin index
	qhandle_t customSkin;			// NULL for default skin
	qhandle_t customShader;			// use one image for the entire thing

	// misc
	byte shaderRGBA[4];				// colors used by rgbgen entity shaders
	float shaderTexCoord[2];		// texture coordinates used by tcMod entity modifiers
	float shaderTime;				// subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	// Ridah
	vec3_t fireRiseDir;

	// Ridah, entity fading (gibs, debris, etc)
	int fadeStartTime, fadeEndTime;

	float hilightIntensity;			//----(SA)	added

	int reFlags;

	int entityNum;					// currentState.number, so we can attach rendering effects to specific entities (Zombie)
};

struct wsglfog_t
{
	int mode;					// GL_LINEAR, GL_EXP
	int hint;					// GL_DONT_CARE
	int startTime;				// in ms
	int finishTime;				// in ms
	float color[4];
	float start;				// near
	float end;					// far
	qboolean useEndForClip;		// use the 'far' value for the far clipping plane
	float density;				// 0.0-1.0
	qboolean registered;		// has this fog been set up?
	qboolean drawsky;			// draw skybox
	qboolean clearscreen;		// clear the GL color buffer

	int dirty;
};

//	Overlaps with RDF_UNDERWATER
#define WSRDF_DRAWSKYBOX    16

struct wsrefdef_t
{
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];				// transformation matrix

	int time;			// time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;					// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	//	added (needed to pass fog infos into the portal sky scene)
	wsglfog_t glfog;
};

struct wsglconfig_t
{
	char renderer_string[MAX_STRING_CHARS];
	char vendor_string[MAX_STRING_CHARS];
	char version_string[MAX_STRING_CHARS];
	char extensions_string[4 * MAX_STRING_CHARS];						// this is actually too short for many current cards/drivers  // (SA) doubled from 2x to 4x MAX_STRING_CHARS

	int maxTextureSize;								// queried from GL
	int maxActiveTextures;							// multitexture ability

	int colorBits, depthBits, stencilBits;

	int driverType;
	int hardwareType;

	qboolean deviceSupportsGamma;
	int textureCompression;
	qboolean textureEnvAddAvailable;
	qboolean anisotropicAvailable;					//----(SA)	added
	float maxAnisotropy;							//----(SA)	added

	// vendor-specific support
	// NVidia
	qboolean NVFogAvailable;					//----(SA)	added
	int NVFogMode;									//----(SA)	added
	// ATI
	int ATIMaxTruformTess;							// for truform support
	int ATINormalMode;							// for truform support
	int ATIPointMode;							// for truform support


	int vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float windowAspect;

	int displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	qboolean isFullscreen;
	qboolean stereoEnabled;
	qboolean smpActive;						// dual processor

	qboolean textureFilterAnisotropicAvailable;					//DAJ
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
