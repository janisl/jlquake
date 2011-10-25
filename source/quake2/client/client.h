/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// client.h -- primary header for client

#include "../../client/client.h"

//define	PARANOID			// speed sapping error checking

#include "../qcommon/qcommon.h"

#include "screen.h"
#include "keys.h"
#include "console.h"
#include "../../core/file_formats/bsp38.h"

//=============================================================================

typedef struct
{
	qboolean		valid;			// cleared if delta parsing was invalid
	int				serverframe;
	int				servertime;		// server time the message is valid for (in msec)
	int				deltaframe;
	byte			areabits[BSP38MAX_MAP_AREAS/8];		// portalarea visibility bits
	player_state_t	playerstate;
	int				num_entities;
	int				parse_entities;	// non-masked index into cl_parse_entities array
} frame_t;

#define MAX_CLIENTWEAPONMODELS		20		// PGM -- upped from 16 to fit the chainfist vwep

typedef struct
{
	char	name[MAX_QPATH];
	char	cinfo[MAX_QPATH];
	struct image_t	*skin;
	struct image_t	*icon;
	char	iconname[MAX_QPATH];
	qhandle_t		model;
	qhandle_t		weaponmodel[MAX_CLIENTWEAPONMODELS];
} clientinfo_t;

extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
extern int num_cl_weaponmodels;

#define	CMD_BACKUP		64	// allow a lot of command backups for very fast systems

//
// the client_state_t structure is wiped completely at every
// server map change
//
struct client_state_t : clientActiveCommon_t
{
	int			timeoutcount;

	int			timedemo_frames;
	int			timedemo_start;

	qboolean	refresh_prepped;	// false if on new level or new ref dll
	qboolean	sound_prepped;		// ambient sounds can start

	int			parse_entities;		// index (not anded off) into cl_parse_entities[]

	usercmd_t	cmd;
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmd_time[CMD_BACKUP];	// time sent, for calculating pings
	short		predicted_origins[CMD_BACKUP][3];	// for debug comparing against server

	float		predicted_step;				// for stair up smoothing
	unsigned	predicted_step_time;

	vec3_t		predicted_origin;	// generated by CL_PredictMovement
	vec3_t		predicted_angles;
	vec3_t		prediction_error;

	frame_t		frame;				// received from server
	int			surpressCount;		// number of messages rate supressed
	frame_t		frames[UPDATE_BACKUP];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	float		lerpfrac;		// between oldframe and frame

	//
	// transient data from server
	//
	char		layout[1024];		// general 2D overlay
	int			inventory[MAX_ITEMS];

	//
	// server state information
	//
	qboolean	attractloop;		// running the attract loop, any key will menu
	int			servercount;	// server identification for prespawns
	char		gamedir[MAX_QPATH];
	int			playernum;

	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	//
	// locally derived information from server state
	//
	qhandle_t		model_draw[MAX_MODELS];
	clipHandle_t	model_clip[MAX_MODELS];

	sfxHandle_t		sound_precache[MAX_SOUNDS];
	struct image_t	*image_precache[MAX_IMAGES];

	clientinfo_t	clientinfo[MAX_CLIENTS];
	clientinfo_t	baseclientinfo;
};

extern	client_state_t	cl;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

typedef enum {
	ca_uninitialized,
	ca_disconnected, 	// not talking to a server
	ca_connecting,		// sending request packets to the server
	ca_connected,		// netchan_t established, waiting for svc_serverdata
	ca_active			// game views should be displayed
} connstate_t;

typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_single
} dltype_t;		// download type

struct client_static_t : clientStaticCommon_t
{
	connstate_t	state;

	int			framecount;
	int			realtime;			// always increasing, no clamping, etc
	float		frametimeFloat;		// seconds since last frame

// screen rendering information
	float		disable_screen;		// showing loading plaque between levels
									// or changing rendering dlls
									// if time gets > 30 seconds ahead, break it
	int			disable_servercount;	// when we receive a frame and cl.servercount
									// > cls.disable_servercount, clear disable_screen

// connection information
	char		servername[MAX_OSPATH];	// name of server from original connect
	float		connect_time;		// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	netchan_t	netchan;
	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

	fileHandle_t	download;			// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int			downloadnumber;
	dltype_t	downloadtype;
	int			downloadpercent;

// demo recording info must be here, so it isn't cleared on level change
	qboolean	demorecording;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	fileHandle_t	demofile;
};

extern client_static_t	cls;

//=============================================================================

//
// cvars
//
extern	Cvar	*cl_stereo_separation;

extern	Cvar	*cl_gun;
extern	Cvar	*cl_add_blend;
extern	Cvar	*cl_add_particles;
extern	Cvar	*cl_add_entities;
extern	Cvar	*cl_predict;
extern	Cvar	*cl_footsteps;
extern	Cvar	*cl_noskins;
extern	Cvar	*cl_autoskins;

extern	Cvar	*cl_upspeed;
extern	Cvar	*cl_forwardspeed;
extern	Cvar	*cl_sidespeed;

extern	Cvar	*cl_yawspeed;
extern	Cvar	*cl_pitchspeed;

extern	Cvar	*cl_run;

extern	Cvar	*cl_anglespeedkey;

extern	Cvar	*cl_shownet;
extern	Cvar	*cl_showmiss;
extern	Cvar	*cl_showclamp;

extern	Cvar	*lookspring;
extern	Cvar	*lookstrafe;
extern	Cvar	*sensitivity;

extern	Cvar	*m_pitch;
extern	Cvar	*m_yaw;
extern	Cvar	*m_forward;
extern	Cvar	*m_side;

extern	Cvar	*freelook;

extern	Cvar	*cl_lightlevel;	// FIXME HACK

extern	Cvar	*cl_paused;
extern	Cvar	*cl_timedemo;

extern	Cvar	*cl_vwep;

extern	q2centity_t	cl_entities[MAX_EDICTS];

// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define	MAX_PARSE_ENTITIES	1024
extern	q2entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

//=============================================================================

extern	netadr_t	net_from;
extern	QMsg		net_message;

void DrawString (int x, int y, const char *s);
void DrawAltString (int x, int y, const char *s);	// toggle high bit
qboolean	CL_CheckOrDownloadFile (char *filename);

void CL_AddNetgraph (void);

#define MAX_SUSTAINS		32

//=================================================

#define BLASTER_PARTICLE_COLOR		0xe0
// ========

void CL_ClearEffects (void);
void CL_ClearTEnts (void);

int CL_ParseEntityBits (unsigned *bits);
void CL_ParseDelta (q2entity_state_t *from, q2entity_state_t *to, int number, int bits);
void CL_ParseFrame (void);

void CL_ParseTEnt (void);
void CL_ParseConfigString (void);
void CL_ParseMuzzleFlash (void);
void CL_ParseMuzzleFlash2 (void);
void SmokeAndFlash(vec3_t origin);

void CL_CalcViewValues();
void CL_AddPacketEntities(frame_t *frame);
void CL_AddTEnts (void);

//=================================================

void CL_PrepRefresh (void);
void CL_RegisterSounds (void);

void CL_Quit_f (void);

void IN_Accumulate (void);

void CL_ParseLayout (void);

void CL_InitRenderStuff();

//
// cl_main
//
void CL_Init (void);

void CL_FixUpGender(void);
void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_GetChallengePacket (void);
void CL_PingServers_f (void);
void CL_Snd_Restart_f (void);
void CL_RequestNextDownload (void);

//
// cl_input
//
typedef struct
{
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame
	int			state;
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (usercmd_t *cmd);

void CL_ClearState (void);

void CL_ReadPackets (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);
void CL_MouseEvent(int mx, int my);
void CL_MouseMove(usercmd_t *cmd);

void IN_CenterView (void);

float CL_KeyState (kbutton_t *key);
const char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_WriteDemoMessage (void);
void CL_Stop_f (void);
void CL_Record_f (void);

//
// cl_parse.c
//
extern	const char *svc_strings[256];

void CL_ParseServerMessage (void);
void CL_LoadClientinfo (clientinfo_t *ci, const char *s);
void SHOWNET(const char *s);
void CL_ParseClientinfo (int player);
void CL_Download_f (void);

//
// cl_view.c
//
extern	int			gun_frame;
extern	qhandle_t	gun_model;

extern float		v_blend[4];

void V_Init (void);
void V_RenderView( float stereo_separation );

//
// cl_tent.c
//
void CL_RegisterTEntSounds (void);
void CL_RegisterTEntModels (void);
void CL_SmokeAndFlash(vec3_t origin);

// the sound code makes callbacks to the client for entitiy position
// information, so entities can be dynamically re-spatialized
void CL_GetEntitySoundOrigin (int ent, vec3_t org);


//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);
void CL_CheckPredictionError (void);

//
// cl_fx.c
//
void CL_FlyEffect (q2centity_t *ent, vec3_t origin);
void CL_EntityEvent (q2entity_state_t *ent);
// RAFAEL
void CLQ2_TrapParticles(vec3_t origin);

//
// menus
//
void M_Init (void);
void M_Keydown (int key);
void M_Draw (void);
void M_Menu_Main_f (void);
void M_ForceMenuOff (void);
void M_AddToServerList (netadr_t adr, char *info);

//
// cl_inv.c
//
void CL_ParseInventory (void);
void CL_KeyInventory (int key);
void CL_DrawInventory (void);

//
// cl_pred.c
//
void CL_PredictMovement (void);

void CIN_SkipCinematic();

#if id386
void x86_TimerStart( void );
void x86_TimerStop( void );
void x86_TimerInit( unsigned long smallest, unsigned longest );
unsigned long *x86_TimerGetHistogram( void );
#endif

void Draw_InitLocal();
void Draw_Char(int x, int y, int c);
