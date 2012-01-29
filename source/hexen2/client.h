// client.h

/*
 * $Header: /H2 Mission Pack/CLIENT.H 4     3/12/98 6:31p Mgummelt $
 */

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

#define	MAX_EFRAGS		640

#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
struct client_static_t : clientStaticCommon_t
{
// personalization data sent to server	
	char		mapstring[MAX_QPATH];

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	timedemo;
	int			forcetrack;			// -1 = use normal cd track
	int			td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo


// connection information
	struct qsocket_s	*netcon;
};

extern client_static_t	cls;

struct clientConnection_t : clientConnectionCommon_t
{
};

extern clientConnection_t clc;


//
// cvars
//
extern	Cvar*	cl_playerclass;

extern	Cvar*	cl_upspeed;
extern	Cvar*	cl_forwardspeed;
extern	Cvar*	cl_backspeed;
extern	Cvar*	cl_sidespeed;

extern	Cvar*	cl_movespeedkey;

extern	Cvar*	cl_yawspeed;
extern	Cvar*	cl_pitchspeed;

extern	Cvar*	cl_anglespeedkey;

extern  Cvar*	cl_prettylights;

extern	Cvar*	cl_shownet;
extern	Cvar*	cl_nolerp;

extern	Cvar*	lookspring;
extern	Cvar*	lookstrafe;
extern	Cvar*	sensitivity;

extern	Cvar*	m_pitch;
extern	Cvar*	m_yaw;
extern	Cvar*	m_forward;
extern	Cvar*	m_side;

extern	Cvar	*cl_lightlevel;	// FIXME HACK

//=============================================================================

//
// cl_main
//
void CL_Init (void);

void CL_EstablishConnection (const char *host);
void CL_Signon1 (void);
void CL_Signon2 (void);
void CL_Signon3 (void);
void CL_Signon4 (void);
void CL_SignonReply (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);

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
void CL_SendMove (h2usercmd_t *cmd);

void CL_ClearState (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (h2usercmd_t *cmd);
void CL_BaseMove (h2usercmd_t *cmd);
void CL_MouseEvent(int mx, int my);
void CL_MouseMove(h2usercmd_t *cmd);


float CL_KeyState (kbutton_t *key);
const char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_StopPlayback (void);
int CL_GetMessage (void);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

//
// cl_parse.c
//
void CL_ParseServerMessage (void);
void CL_NewTranslation (int slot);

//
// view
//
void V_StartPitchDrift (void);
void V_StopPitchDrift (void);

void V_RenderView (void);
void V_UpdatePalette (void);
void V_Register (void);
void V_ParseDamage (void);

//
// cl_tent
//
void SV_UpdateEffects(QMsg *sb);

void CL_RemoveGIPFiles(const char *path);
void CL_CopyFiles(const char* source, const char* ext, const char* dest);
