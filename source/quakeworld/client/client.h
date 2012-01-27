/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// client.h

//
// client_state_t should hold all pieces of the client state
//


#define	MAX_EFRAGS		512

#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
struct client_static_t : clientStaticCommon_t
{
// private userinfo for sending to masterless servers
	char		userinfo[MAX_INFO_STRING_QW];

	char		servername[MAX_OSPATH];	// name of server from original connect

	int			qport;

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	timedemo;
	float		td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo

	int			challenge;

	float		latency;		// rolling average
};

extern client_static_t	cls;

struct clientConnection_t : clientConnectionCommon_t
{
};

extern clientConnection_t clc;

//
// the client_state_t structure is wiped completely at every
// server signon
//
struct client_state_t : clientActiveCommon_t
{
	char		serverinfo[MAX_SERVERINFO_STRING];

	int			spectator;

	double		last_ping_request;	// while showing scoreboard
	double		last_servermessage;

// the client simulates or interpolates movement to get these values
	vec3_t		simorg;
	vec3_t		simvel;
	vec3_t		simangles;

//
// information that is static for the entire time connected to a server
//
	char		model_name[MAX_MODELS_Q1][MAX_QPATH];
	char		sound_name[MAX_SOUNDS_Q1][MAX_QPATH];

	int			playernum;
};


//
// cvars
//
extern  Cvar*	cl_warncmd;
extern	Cvar*	cl_upspeed;
extern	Cvar*	cl_forwardspeed;
extern	Cvar*	cl_backspeed;
extern	Cvar*	cl_sidespeed;

extern	Cvar*	cl_movespeedkey;

extern	Cvar*	cl_yawspeed;
extern	Cvar*	cl_pitchspeed;

extern	Cvar*	cl_anglespeedkey;

extern	Cvar*	cl_shownet;
extern	Cvar*	cl_sbar;
extern	Cvar*	cl_hudswap;

extern	Cvar*	lookspring;
extern	Cvar*	lookstrafe;
extern	Cvar*	sensitivity;

extern	Cvar*	m_pitch;
extern	Cvar*	m_yaw;
extern	Cvar*	m_forward;
extern	Cvar*	m_side;

extern	Cvar*	name;


extern	client_state_t	cl;

extern	qboolean	nomaster;
extern float	server_version;	// version of server we connected to

//=============================================================================


//
// cl_main
//
void CL_Init (void);
void Host_WriteConfiguration (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);
qboolean CL_DemoBehind(void);

void CL_BeginServerConnect(void);

extern char emodel_name[], pmodel_name[], prespawn_name[], modellist_name[], soundlist_name[];

//
// cl_input
//
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (qwusercmd_t *cmd);

void CL_ClearState (void);

void CL_ReadPackets (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (qwusercmd_t *cmd);
void CL_BaseMove (qwusercmd_t *cmd);
void CL_MouseEvent(int mx, int my);
void CL_MouseMove(qwusercmd_t *cmd);


float CL_KeyState (kbutton_t *key);
const char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_StopPlayback (void);
qboolean CL_GetMessage (void);
void CL_WriteDemoCmd (qwusercmd_t *pcmd);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_ReRecord_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

//
// cl_parse.c
//
#define NET_TIMINGS 256
#define NET_TIMINGSMASK 255
extern int	packet_latency[NET_TIMINGS];
int CL_CalcNet (void);
void CL_ParseServerMessage (void);
void CL_NewTranslation (int slot);
qboolean CL_IsUploading(void);
void CL_NextUpload(void);
void CL_StartUpload (byte *data, int size);
void CL_StopUpload(void);

//
// view.c
//
void V_StartPitchDrift (void);
void V_StopPitchDrift (void);

void V_RenderView (void);
void V_UpdatePalette (void);
void V_Register (void);
void V_ParseDamage (void);

//
// cl_ents.c
//
void CL_SetSolidPlayers (int playernum);
void CL_SetUpPlayerPrediction(qboolean dopred);
void CL_EmitEntities (void);
void CL_SetSolidEntities (void);

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);
void CL_PredictUsercmd (qwplayer_state_t *from, qwplayer_state_t *to, qwusercmd_t *u, qboolean spectator);

//
// cl_cam.c
//
#define CAM_NONE	0
#define CAM_TRACK	1

extern	int		autocam;
extern int spec_track; // player# of who we are tracking

qboolean Cam_DrawViewModel(void);
qboolean Cam_DrawPlayer(int playernum);
void Cam_Track(qwusercmd_t *cmd);
void Cam_FinishMove(qwusercmd_t *cmd);
void Cam_Reset(void);
void CL_InitCam(void);

extern	int		clq1_playerindex, cl_flagindex;
