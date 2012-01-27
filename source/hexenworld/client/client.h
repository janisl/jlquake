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
	char		userinfo[HWMAX_INFO_STRING];

	char		servername[MAX_OSPATH];	// name of server from original connect

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	timedemo;
	float		td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo

	float		latency;		// rolling average
};

extern client_static_t	cls;

struct clientConnection_t : clientConnectionCommon_t
{
};

extern clientConnection_t clc;

struct client_entvars_t
{
	float	movetype;
	float	health;
	float	max_health;
	float	playerclass;
	float	bluemana;
	float	greenmana;
	float	max_mana;
	float	armor_amulet;
	float	armor_bracer;
	float	armor_breastplate;
	float	armor_helmet;
	float	level;
	float	intelligence;
	float	wisdom;
	float	dexterity;
	float	strength;
	float	experience;
	float	ring_flight;
	float	ring_water;
	float	ring_turning;
	float	ring_regeneration;
	float	flags;
	float	teleport_time;
	float	rings_active;
	float	rings_low;
	float	artifact_active;
	float	artifact_low;
	float	hasted;
	float	inventory;
	float	cnt_torch;
	float	cnt_h_boost;
	float	cnt_sh_boost;
	float	cnt_mana_boost;
	float	cnt_teleport;
	float	cnt_tome;
	float	cnt_summon;
	float	cnt_invisibility;
	float	cnt_glyph;
	float	cnt_haste;
	float	cnt_blast;
	float	cnt_polymorph;
	float	cnt_flight;
	float	cnt_cubeofforce;
	float	cnt_invincibility;
	int	cameramode;
};

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

	int			inv_order[MAX_INVENTORY];
	int			inv_count, inv_startpos, inv_selected;

	client_entvars_t	v; // NOTE: not every field will be update - you must specifically add
	                   // them in functions SV_WriteClientdatatToMessage() and CL_ParseClientdata()

	char puzzle_pieces[8][10]; // puzzle piece names

// the client simulates or interpolates movement to get these values
	vec3_t		simorg;
	vec3_t		simvel;
	vec3_t		simangles;

	float		idealroll;
	float		rollvel;
	
//
// information that is static for the entire time connected to a server
//
	char		model_name[MAX_MODELS_H2][MAX_QPATH];
	char		sound_name[MAX_SOUNDS_HW][MAX_QPATH];

	int			playernum;

	char		midi_name[MAX_QPATH];     // midi file name

	h2entity_t	viewent;		// weapon model

	unsigned	PIV;			// players in view
};


// edict->flags
#define	FL_FLY					1
#define	FL_SWIM					2
//#define	FL_GLIMPSE				4
#define	FL_CONVEYOR				4
#define	FL_CLIENT				8
#define	FL_INWATER				16
#define	FL_MONSTER				32
#define	FL_GODMODE				64
#define	FL_NOTARGET				128
#define	FL_ITEM					256
#define	FL_ONGROUND				512
#define	FL_PARTIALGROUND		1024	// not all corners are valid
#define	FL_WATERJUMP			2048	// player jumping out of water
#define	FL_JUMPRELEASED			4096	// for jump debouncing
#define FL_FLASHLIGHT			8192
#define FL_ARCHIVE_OVERRIDE		1048576
#define	FL_ARTIFACTUSED			16384
#define FL_MOVECHAIN_ANGLE		32768    // when in a move chain, will update the angle
#define FL_CLASS_DEPENDENT		2097152  // model will appear different to each player
#define FL_SPECIAL_ABILITY1		4194304  // has 1st special ability
#define FL_SPECIAL_ABILITY2		8388608  // has 2nd special ability

#define	FL2_CROUCHED			4096

// edict->movetype values
#define	MOVETYPE_NONE			0		// never moves
#define	MOVETYPE_ANGLENOCLIP	1
#define	MOVETYPE_ANGLECLIP		2
#define	MOVETYPE_WALK			3		// gravity
#define	MOVETYPE_STEP			4		// gravity, special edge handling
#define	MOVETYPE_FLY			5
#define	MOVETYPE_TOSS			6		// gravity
#define	MOVETYPE_PUSH			7		// no clip to world, push and crush
#define	MOVETYPE_NOCLIP			8
#define	MOVETYPE_FLYMISSILE		9		// extra size to monsters
#define	MOVETYPE_BOUNCE			10
#define MOVETYPE_BOUNCEMISSILE	11		// bounce w/o gravity
#define MOVETYPE_FOLLOW			12		// track movement of aiment
#define MOVETYPE_PUSHPULL		13		// pushable/pullable object
#define MOVETYPE_SWIM			14		// should keep the object in water

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

extern	Cvar*	playerclass;
extern	Cvar*	spectator;

extern	Cvar	*cl_lightlevel;	// FIXME HACK

extern	Cvar*	cl_teamcolor;

extern	client_state_t	cl;

//=============================================================================

extern	qboolean	nomaster;

#define CAM_NONE	0
#define CAM_TRACK	1

extern	int		autocam;
extern int spec_track; // player# of who we are tracking
extern float	server_version;	// version of server we connected to

//
// cl_main
//
void CL_Init (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);
qboolean CL_DemoBehind(void);

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
void CL_SendMove (hwusercmd_t *cmd);

void CL_ClearState (void);

void CL_ReadPackets (void);

int  CL_ReadFromServer (void);
void CL_WriteToServer (hwusercmd_t *cmd);
void CL_BaseMove (hwusercmd_t *cmd);
void CL_MouseEvent(int mx, int my);
void CL_MouseMove(hwusercmd_t *cmd);


float CL_KeyState (kbutton_t *key);
const char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_StopPlayback (void);
qboolean CL_GetMessage (void);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_ReRecord_f (void);
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
// cl_ents.c
//
void CL_SetSolidPlayers (int playernum);

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);

qboolean CL_CheckOrDownloadFile (char *filename);
void CL_ParsePlayerinfo (void);
void CL_SavePlayer (void);
void CL_ParsePacketEntities (qboolean delta);
void CL_SetSolidEntities (void);
void CL_InitCam(void);
void CL_SetUpPlayerPrediction(qboolean dopred);
void CL_EmitEntities (void);
void CL_WriteDemoCmd (hwusercmd_t *pcmd);
void CL_PredictUsercmd (hwplayer_state_t *from, hwplayer_state_t *to, hwusercmd_t *u, qboolean spectator);
void CL_SendConnectPacket (void);
void Host_WriteConfiguration (const char *fname);
void Cam_Track(hwusercmd_t *cmd);
void Cam_FinishMove(hwusercmd_t *cmd);
void Cam_Reset(void);
