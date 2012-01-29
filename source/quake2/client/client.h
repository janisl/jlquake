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
#include "../../client/game/quake2/local.h"

//define	PARANOID			// speed sapping error checking

#include "../qcommon/qcommon.h"

#include "screen.h"
#include "keys.h"
#include "console.h"

//=============================================================================

extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS_Q2][MAX_QPATH];
extern int num_cl_weaponmodels;

extern	clientActive_t	cl;

struct clientConnection_t : clientConnectionCommon_t
{
};

extern clientConnection_t clc;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

struct client_static_t : clientStaticCommon_t
{
	int			framecount;
	int			realtime;			// always increasing, no clamping, etc
	float		frametimeFloat;		// seconds since last frame

// screen rendering information
	int			disable_servercount;	// when we receive a frame and cl.servercount
									// > cls.disable_servercount, clear disable_screen

// connection information
	char		servername[MAX_OSPATH];	// name of server from original connect
	float		connect_time;		// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

// demo recording info must be here, so it isn't cleared on level change
	qboolean	demowaiting;	// don't record until a non-delta message is received
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

// the cl_parse_entities must be large enough to hold UPDATE_BACKUP_Q2 frames of
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

//=================================================

#define BLASTER_PARTICLE_COLOR		0xe0
// ========

int CL_ParseEntityBits (unsigned *bits);
void CL_ParseDelta (q2entity_state_t *from, q2entity_state_t *to, int number, int bits);
void CL_ParseFrame (void);

void CL_ParseConfigString (void);

void CL_CalcViewValues();
void CL_AddPacketEntities(q2frame_t *frame);

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
void CL_SendMove (q2usercmd_t *cmd);

void CL_ClearState (void);

void CL_ReadPackets (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (q2usercmd_t *cmd);
void CL_BaseMove (q2usercmd_t *cmd);
void CL_MouseEvent(int mx, int my);
void CL_MouseMove(q2usercmd_t *cmd);

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
void CL_LoadClientinfo (q2clientinfo_t *ci, const char *s);
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
