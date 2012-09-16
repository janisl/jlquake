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

#ifndef _ET_CG_PUBLIC_H
#define _ET_CG_PUBLIC_H

#include "../tech3/cg_shared.h"

#define MAX_ENTITIES_IN_SNAPSHOT_ET    512

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
struct etsnapshot_t
{
	int snapFlags;						// SNAPFLAG_RATE_DELAYED, etc
	int ping;

	int serverTime;					// server time the message is valid for (in msec)

	byte areamask[MAX_MAP_AREA_BYTES];					// portalarea visibility bits

	etplayerState_t ps;							// complete information about the current player at this time

	int numEntities;						// all of the entities that need to be presented
	etentityState_t entities[MAX_ENTITIES_IN_SNAPSHOT_ET];	// at the time of this snapshot

	int numServerCommands;					// text based server commands to execute when this
	int serverCommandSequence;				// snapshot becomes current
};

#define ETRF_NOSHADOW           0x000010
#define ETRF_LIGHTING_ORIGIN    0x000020
#define ETRF_SHADOW_PLANE       0x000040
#define ETRF_WRAP_FRAMES        0x000080
#define ETRF_BLINK              0x000200
#define ETRF_FORCENOLOD         0x000400

struct etrefEntity_t
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
	qhandle_t frameModel;
	int torsoFrame;					// skeletal torso can have frame independant of legs frame
	qhandle_t torsoFrameModel;

	// previous data for frame interpolation
	float oldorigin[3];				// also used as MODEL_BEAM's "to"
	int oldframe;
	qhandle_t oldframeModel;
	int oldTorsoFrame;
	qhandle_t oldTorsoFrameModel;
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

struct etglfog_t
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
};

struct etrefdef_t
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
	etglfog_t glfog;
};

struct etglconfig_t
{
	char renderer_string[MAX_STRING_CHARS];
	char vendor_string[MAX_STRING_CHARS];
	char version_string[MAX_STRING_CHARS];
	char extensions_string[MAX_STRING_CHARS * 4];					// TTimo - bumping, some cards have a big extension string

	int maxTextureSize;								// queried from GL
	int maxActiveTextures;							// multitexture ability

	int colorBits, depthBits, stencilBits;

	glDriverType_t driverType;
	glHardwareType_t hardwareType;

	qboolean deviceSupportsGamma;
	textureCompression_t textureCompression;
	qboolean textureEnvAddAvailable;
	qboolean anisotropicAvailable;					//----(SA)	added
	float maxAnisotropy;							//----(SA)	added

	// vendor-specific support
	// NVidia
	qboolean NVFogAvailable;						//----(SA)	added
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
};

/*
==================================================================

functions imported from the main executable

==================================================================
*/

enum
{
	ETCG_PRINT,
	ETCG_ERROR,
	ETCG_MILLISECONDS,
	ETCG_CVAR_REGISTER,
	ETCG_CVAR_UPDATE,
	ETCG_CVAR_SET,
	ETCG_CVAR_VARIABLESTRINGBUFFER,
	ETCG_CVAR_LATCHEDVARIABLESTRINGBUFFER,
	ETCG_ARGC,
	ETCG_ARGV,
	ETCG_ARGS,
	ETCG_FS_FOPENFILE,
	ETCG_FS_READ,
	ETCG_FS_WRITE,
	ETCG_FS_FCLOSEFILE,
	ETCG_FS_GETFILELIST,
	ETCG_FS_DELETEFILE,
	ETCG_SENDCONSOLECOMMAND,
	ETCG_ADDCOMMAND,
	ETCG_SENDCLIENTCOMMAND,
	ETCG_UPDATESCREEN,
	ETCG_CM_LOADMAP,
	ETCG_CM_NUMINLINEMODELS,
	ETCG_CM_INLINEMODEL,
	ETCG_CM_LOADMODEL,
	ETCG_CM_TEMPBOXMODEL,
	ETCG_CM_POINTCONTENTS,
	ETCG_CM_TRANSFORMEDPOINTCONTENTS,
	ETCG_CM_BOXTRACE,
	ETCG_CM_TRANSFORMEDBOXTRACE,
// MrE:
	ETCG_CM_CAPSULETRACE,
	ETCG_CM_TRANSFORMEDCAPSULETRACE,
	ETCG_CM_TEMPCAPSULEMODEL,
// done.
	ETCG_CM_MARKFRAGMENTS,
	ETCG_R_PROJECTDECAL,			// ydnar: projects a decal onto brush models
	ETCG_R_CLEARDECALS,			// ydnar: clears world/entity decals
	ETCG_S_STARTSOUND,
	ETCG_S_STARTSOUNDEX,	//----(SA)	added
	ETCG_S_STARTLOCALSOUND,
	ETCG_S_CLEARLOOPINGSOUNDS,
	ETCG_S_CLEARSOUNDS,
	ETCG_S_ADDLOOPINGSOUND,
	ETCG_S_UPDATEENTITYPOSITION,
// Ridah, talking animations
	ETCG_S_GETVOICEAMPLITUDE,
// done.
	ETCG_S_RESPATIALIZE,
	ETCG_S_REGISTERSOUND,
	ETCG_S_STARTBACKGROUNDTRACK,
	ETCG_S_FADESTREAMINGSOUND,	//----(SA)	modified
	ETCG_S_FADEALLSOUNDS,			//----(SA)	added for fading out everything
	ETCG_S_STARTSTREAMINGSOUND,
	ETCG_S_GETSOUNDLENGTH,		// xkan - get the length (in milliseconds) of the sound
	ETCG_S_GETCURRENTSOUNDTIME,
	ETCG_R_LOADWORLDMAP,
	ETCG_R_REGISTERMODEL,
	ETCG_R_REGISTERSKIN,
	ETCG_R_REGISTERSHADER,

	ETCG_R_GETSKINMODEL,			// client allowed to view what the .skin loaded so they can set their model appropriately
	ETCG_R_GETMODELSHADER,		// client allowed the shader handle for given model/surface (for things like debris inheriting shader from explosive)

	ETCG_R_REGISTERFONT,
	ETCG_R_CLEARSCENE,
	ETCG_R_ADDREFENTITYTOSCENE,
	ETCG_GET_ENTITY_TOKEN,
	ETCG_R_ADDPOLYTOSCENE,
// Ridah
	ETCG_R_ADDPOLYSTOSCENE,
// done.
	ETCG_R_ADDPOLYBUFFERTOSCENE,
	ETCG_R_ADDLIGHTTOSCENE,

	ETCG_R_ADDCORONATOSCENE,
	ETCG_R_SETFOG,
	ETCG_R_SETGLOBALFOG,

	ETCG_R_RENDERSCENE,
	ETCG_R_SAVEVIEWPARMS,
	ETCG_R_RESTOREVIEWPARMS,
	ETCG_R_SETCOLOR,
	ETCG_R_DRAWSTRETCHPIC,
	ETCG_R_DRAWSTRETCHPIC_GRADIENT,	//----(SA)	added
	ETCG_R_MODELBOUNDS,
	ETCG_R_LERPTAG,
	ETCG_GETGLCONFIG,
	ETCG_GETGAMESTATE,
	ETCG_GETCURRENTSNAPSHOTNUMBER,
	ETCG_GETSNAPSHOT,
	ETCG_GETSERVERCOMMAND,
	ETCG_GETCURRENTCMDNUMBER,
	ETCG_GETUSERCMD,
	ETCG_SETUSERCMDVALUE,
	ETCG_SETCLIENTLERPORIGIN,			// DHM - Nerve
	ETCG_R_REGISTERSHADERNOMIP,
	ETCG_MEMORY_REMAINING,

	ETCG_KEY_ISDOWN,
	ETCG_KEY_GETCATCHER,
	ETCG_KEY_SETCATCHER,
	ETCG_KEY_GETKEY,
	ETCG_KEY_GETOVERSTRIKEMODE,
	ETCG_KEY_SETOVERSTRIKEMODE,

	ETCG_PC_ADD_GLOBAL_DEFINE,
	ETCG_PC_LOAD_SOURCE,
	ETCG_PC_FREE_SOURCE,
	ETCG_PC_READ_TOKEN,
	ETCG_PC_SOURCE_FILE_AND_LINE,
	ETCG_PC_UNREAD_TOKEN,

	ETCG_S_STOPBACKGROUNDTRACK,
	ETCG_REAL_TIME,
	ETCG_SNAPVECTOR,
	ETCG_REMOVECOMMAND,
	ETCG_R_LIGHTFORPOINT,

	ETCG_CIN_PLAYCINEMATIC,
	ETCG_CIN_STOPCINEMATIC,
	ETCG_CIN_RUNCINEMATIC,
	ETCG_CIN_DRAWCINEMATIC,
	ETCG_CIN_SETEXTENTS,
	ETCG_R_REMAP_SHADER,
	ETCG_S_ADDREALLOOPINGSOUND,
	ETCG_S_STOPSTREAMINGSOUND,	//----(SA)	added

	ETCG_LOADCAMERA,
	ETCG_STARTCAMERA,
	ETCG_STOPCAMERA,
	ETCG_GETCAMERAINFO,

	ETCG_MEMSET = 150,
	ETCG_MEMCPY,
	ETCG_STRNCPY,
	ETCG_SIN,
	ETCG_COS,
	ETCG_ATAN2,
	ETCG_SQRT,
	ETCG_FLOOR,
	ETCG_CEIL,

	ETCG_TESTPRINTINT,
	ETCG_TESTPRINTFLOAT,
	ETCG_ACOS,

	ETCG_INGAME_POPUP,		//----(SA)	added

	// NERVE - SMF
	ETCG_INGAME_CLOSEPOPUP,

	ETCG_R_DRAWROTATEDPIC,
	ETCG_R_DRAW2DPOLYS,

	ETCG_KEY_GETBINDINGBUF,
	ETCG_KEY_SETBINDING,
	ETCG_KEY_KEYNUMTOSTRINGBUF,
	ETCG_KEY_BINDINGTOKEYS,

	ETCG_TRANSLATE_STRING,
	// -NERVE - SMF

	ETCG_R_INPVS,
	ETCG_GETHUNKDATA,

	ETCG_PUMPEVENTLOOP,

	// zinx
	ETCG_SENDMESSAGE,
	ETCG_MESSAGESTATUS,
	// -zinx

	// bani
	ETCG_R_LOADDYNAMICSHADER,
	// -bani

	// fretn
	ETCG_R_RENDERTOTEXTURE,
	// -fretn
	// bani
	ETCG_R_GETTEXTUREID,
	// -bani
	// bani
	ETCG_R_FINISH,
	// -bani
};

/*
==================================================================

functions exported to the main executable

==================================================================
*/

enum
{
	ETCG_GET_TAG = 9,
//	qboolean CG_GetTag( int clientNum, char *tagname, orientation_t *or );

	ETCG_CHECKEXECKEY,

	ETCG_WANTSBINDKEYS,

	// zinx
	ETCG_MESSAGERECEIVED,
//	void (*CG_MessageReceived)( const char *buf, int buflen, int serverTime );
	// -zinx
};

#endif
