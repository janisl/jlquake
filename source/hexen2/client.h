// client.h

/*
 * $Header: /H2 Mission Pack/CLIENT.H 4     3/12/98 6:31p Mgummelt $
 */

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

#define	SIGNONS		4			// signon messages to receive before connected

#define	MAX_EFRAGS		640

#define	MAX_MAPSTRING	2048
#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef enum {
ca_dedicated, 		// a dedicated server with no ability to start a client
ca_disconnected, 	// full screen console with no connection
ca_connected		// valid netcon, talking to a server
} cactive_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
struct client_static_t : clientStaticCommon_t
{
	cactive_t	state;

// personalization data sent to server	
	char		mapstring[MAX_QPATH];
	char		spawnparms[MAX_MAPSTRING];	// to restart a level

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	demorecording;
	qboolean	demoplayback;
	qboolean	timedemo;
	int			forcetrack;			// -1 = use normal cd track
	fileHandle_t	demofile;
	int			td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo


// connection information
	int			signon;			// 0 to SIGNONS
	struct qsocket_s	*netcon;
	QMsg		message;		// writing buffer to send to server
	byte		message_buf[1024];
	
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
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the 
								// first frame
	h2usercmd_t	cmd;			// last command sent to the server

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			inv_order[MAX_INVENTORY];
	int			inv_count, inv_startpos, inv_selected;
	int			items;			// inventory bit flags
	float		item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	entvars_t		v; // NOTE: not every field will be update - you must specifically add
	                   // them in functions SV_WriteClientdatatToMessage() and CL_ParseClientdata()

	char puzzle_pieces[8][10]; // puzzle piece names

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarliy offset, and an angle reset commands at the start
// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
								// between these
	vec3_t		viewangles;
	
	vec3_t		mvelocity[2];	// update by server, used for lean+bob
								// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset

	float		idealroll;
	float		rollvel;
	
// pitch drifting vars
	float		idealpitch;
	float		pitchvel;
	qboolean	nodrift;
	float		driftmove;
	double		laststop;

	float		viewheight;
	float		crouch;			// local amount for smoothing stepups

	qboolean	paused;			// send over by server
	qboolean	onground;
	qboolean	inwater;
	
	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched at intermission start
	
	double		mtime[2];		// the timestamp of last two messages	
	double		serverTimeFloat;// clients view of time, should be between
								// servertime and oldservertime to generate
								// a lerp point for other data
	double		oldtime;		// previous cl.time, time-oldtime is used
								// to decay light values and smooth step ups
	

	float		last_received_message;	// (realtime) for net trouble icon

//
// information that is static for the entire time connected to a server
//
	qhandle_t	model_precache[MAX_MODELS];
	sfxHandle_t	sound_precache[MAX_SOUNDS];

	char		levelname[40];	// for display on solo scoreboard
	int			maxclients;
	int			gametype;

// refresh related state
	int			num_entities;	// held in h2cl_entities array
	int			num_statics;	// held in cl_staticentities array
	h2entity_t	viewent;			// the gun model

	int			cdtrack, looptrack;	// cd audio
	char		midi_name[128];     // midi file name
	byte		current_frame, last_frame, reference_frame;
	byte		current_sequence, last_sequence;
	byte		need_build;

	client_frames2_t frames[3]; // 0 = base, 1 = building, 2 = 0 & 1 merged
	short RemoveList[MAX_CLIENT_STATES],NumToRemove;

	long	info_mask, info_mask2;
};


//
// cvars
//
extern	Cvar*	cl_name;
extern	Cvar*	cl_color;
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


extern	client_state_t	cl;

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
void CL_InitTEnts(void);
void CL_UpdateTEnts(void);

void SV_UpdateEffects(QMsg *sb);

void CL_RemoveGIPFiles(const char *path);
void CL_CopyFiles(const char* source, const char* ext, const char* dest);
