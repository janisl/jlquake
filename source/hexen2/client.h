// client.h

/*
 * $Header: /H2 Mission Pack/CLIENT.H 4     3/12/98 6:31p Mgummelt $
 */

typedef struct
{
	vec3_t	viewangles;

// intended velocities
	float	forwardmove;
	float	sidemove;
	float	upmove;
	byte	lightlevel;
} usercmd_t;

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
} clightstyle_t;

typedef struct
{
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	byte	translations[VID_GRADES*256];
	float	playerclass;
} scoreboard_t;

typedef struct
{
	int		destcolor[3];
	int		percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS		0
#define	CSHIFT_DAMAGE		1
#define	CSHIFT_BONUS		2
#define	CSHIFT_POWERUP		3
#define	CSHIFT_INTERVENTION 4
#define	NUM_CSHIFTS			5

#define	NAME_LENGTH	64


//
// client_state_t should hold all pieces of the client state
//

#define	SIGNONS		4			// signon messages to receive before connected

#define	MAX_DLIGHTS		32
typedef struct
{
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		key;
	qboolean	dark;			// subtracts light instead of adding
} cdlight_t;

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
typedef struct
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
	
} client_static_t;

extern client_static_t	cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the 
								// first frame
	usercmd_t	cmd;			// last command sent to the server

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			inv_order[MAX_INVENTORY];
	int			inv_count, inv_startpos, inv_selected;
	int			items;			// inventory bit flags
	float		item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	entvars_t		v; // NOTE: not every field will be update - you must specifically add
	                   // them in functions SV_WriteClientdatatToMessage() and CL_ParseClientdata()

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

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
	double		time;			// clients view of time, should be between
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
	int			viewentity;		// cl_entitites[cl.viewentity] = player
	int			maxclients;
	int			gametype;

// refresh related state
	int			num_entities;	// held in cl_entities array
	int			num_statics;	// held in cl_staticentities array
	entity_t	viewent;			// the gun model
	struct EffectT Effects[MAX_EFFECTS];

	int			cdtrack, looptrack;	// cd audio
	char		midi_name[128];     // midi file name
	byte		current_frame, last_frame, reference_frame;
	byte		current_sequence, last_sequence;
	byte		need_build;

// frag scoreboard
	scoreboard_t	*scores;		// [cl.maxclients]

// light level at player's position including dlights
// this is sent back to the server each frame
// architectually ugly but it works
	int			light_level;

	client_frames2_t frames[3]; // 0 = base, 1 = building, 2 = 0 & 1 merged
	short RemoveList[MAX_CLIENT_STATES],NumToRemove;

	long	info_mask, info_mask2;
} client_state_t;


//
// cvars
//
extern	QCvar*	cl_name;
extern	QCvar*	cl_color;
extern	QCvar*	cl_playerclass;

extern	QCvar*	cl_upspeed;
extern	QCvar*	cl_forwardspeed;
extern	QCvar*	cl_backspeed;
extern	QCvar*	cl_sidespeed;

extern	QCvar*	cl_movespeedkey;

extern	QCvar*	cl_yawspeed;
extern	QCvar*	cl_pitchspeed;

extern	QCvar*	cl_anglespeedkey;

extern  QCvar*	cl_prettylights;

extern	QCvar*	cl_shownet;
extern	QCvar*	cl_nolerp;

extern	QCvar*	lookspring;
extern	QCvar*	lookstrafe;
extern	QCvar*	sensitivity;

extern	QCvar*	m_pitch;
extern	QCvar*	m_yaw;
extern	QCvar*	m_forward;
extern	QCvar*	m_side;


#define	MAX_STATIC_ENTITIES	256			// torches, etc

extern	client_state_t	cl;

// FIXME, allocate dynamically
extern	entity_t		cl_entities[MAX_EDICTS];
extern	entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
extern	clightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	cdlight_t		cl_dlights[MAX_DLIGHTS];

//=============================================================================

//
// cl_main
//
cdlight_t *CL_AllocDlight (int key);
void	CL_DecayLights (void);

void CL_Init (void);

void CL_EstablishConnection (char *host);
void CL_Signon1 (void);
void CL_Signon2 (void);
void CL_Signon3 (void);
void CL_Signon4 (void);
void CL_SignonReply (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);

void CL_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles, int scale, int colorshade, int abslight, int drawflags);

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
void CL_SendMove (usercmd_t *cmd);

void CL_ClearState (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);
void CL_MouseEvent(int mx, int my);
void CL_MouseMove(usercmd_t *cmd);


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
void V_SetContentsColor (int contents);

//
// cl_tent
//
void CL_InitTEnts(void);
void CL_ClearTEnts(void);
void CL_ParseTEnt(void);
void CL_UpdateTEnts(void);

void CL_ClearEffects(void);
void CL_ParseEffect(void);
void CL_EndEffect(void);
void SV_UpdateEffects(QMsg *sb);

void CL_RemoveGIPFiles(const char *path);
void CL_CopyFiles(const char* source, const char* ext, const char* dest);
