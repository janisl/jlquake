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

enum
{
	WMCGAME_EVENT_NONE
};

//	Overlaps with RF_WRAP_FRAMES
#define WMRF_BLINK            (1 << 9)		// eyes in 'blink' state

struct wmrefEntity_t
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

struct wmglfog_t
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

struct wmrefdef_t
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
	wmglfog_t glfog;
};

struct wmglconfig_t
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

	qboolean textureFilterAnisotropicAvailable;					//DAJ
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
	WMCG_INIT,
//	void CG_Init( int serverMessageNum, int serverCommandSequence )
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	WMCG_SHUTDOWN,
//	void (*CG_Shutdown)( void );
	// oportunity to flush and close any open files

	WMCG_CONSOLE_COMMAND,
//	qboolean (*CG_ConsoleCommand)( void );
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return false if the
	// command is not known to the game

	WMCG_DRAW_ACTIVE_FRAME,
//	void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
	// Generates and draws a game scene and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	WMCG_CROSSHAIR_PLAYER,
//	int (*CG_CrosshairPlayer)( void );

	WMCG_LAST_ATTACKER,
//	int (*CG_LastAttacker)( void );

	WMCG_KEY_EVENT,
//	void	(*CG_KeyEvent)( int key, qboolean down );

	WMCG_MOUSE_EVENT,
//	void	(*CG_MouseEvent)( int dx, int dy );
	WMCG_EVENT_HANDLING,
//	void (*CG_EventHandling)(int type);

	WMCG_GET_TAG,
//	qboolean CG_GetTag( int clientNum, char *tagname, orientation_t *or );

	WMCG_CHECKCENTERVIEW,
//	qboolean CG_CheckCenterView();
};
