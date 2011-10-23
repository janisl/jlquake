// client.h

// player_state_t is the information needed by a player entity
// to do move prediction and to generate a drawable entity
typedef struct
{
	int			messagenum;		// all player's won't be updated each frame

	double		state_time;		// not the same as the packet time,
								// because player commands come asyncronously
	usercmd_t	command;		// last command for prediction

	vec3_t		origin;
	vec3_t		viewangles;		// only for demos, not from server
	vec3_t		velocity;
	int			weaponframe;

	int			modelindex;
	int			frame;
	int			skinnum;
	int			effects;
	int			drawflags;
	int			scale;
	int			abslight;

	int			flags;			// dead, gib, etc

	float		waterjumptime;
	int			onground;		// -1 = in air, else pmove entity number
	int			oldbuttons;
} player_state_t;


#define	MAX_SCOREBOARDNAME	16
typedef struct player_info_s
{
	int			userid;
	char		userinfo[MAX_INFO_STRING];

	// scoreboard information
	char		name[MAX_SCOREBOARDNAME];
	float		entertime;
	int			frags;
	int			ping;

	// skin information
	int			topcolor;
	int			bottomcolor;
	int			playerclass;
	int			level;
	int			spectator;
	int			modelindex;
	qboolean	Translated;
	int			siege_team;
	qboolean	shownames_off;

	vec3_t		origin;
} player_info_t;


typedef struct
{
	// generated on client side
	usercmd_t	cmd;		// cmd that generated the frame
	double		senttime;	// time cmd was sent off
	int			delta_sequence;		// sequence number to delta from, -1 = full update

	// received from server
	double		receivedtime;	// time message was received, or -1
	player_state_t	playerstate[MAX_CLIENTS];	// message received that reflects performing
							// the usercmd
	packet_entities_t	packet_entities;
	qboolean	invalid;		// true if the packet_entities delta was invalid
} frame_t;


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


//
// client_state_t should hold all pieces of the client state
//
typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
} clightstyle_t;



#define	MAX_EFRAGS		512

#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef enum {
ca_disconnected, 	// full screen console with no connection
ca_demostart,		// starting up a demo
ca_connected,		// netchan_t established, waiting for svc_serverdata
ca_onserver,		// processing data lists, donwloading, etc
ca_active			// everything is in, so frames can be rendered
} cactive_t;

typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_single
} dltype_t;		// download type

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
struct client_static_t : clientStaticCommon_t
{
// connection information
	cactive_t	state;
	
// network stuff
	netchan_t	netchan;

// private userinfo for sending to masterless servers
	char		userinfo[MAX_INFO_STRING];

	char		servername[MAX_OSPATH];	// name of server from original connect

	fileHandle_t	download;		// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int			downloadnumber;
	dltype_t	downloadtype;
	int			downloadpercent;

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	demorecording;
	qboolean	demoplayback;
	qboolean	timedemo;
	fileHandle_t	demofile;
	float		td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo

	float		latency;		// rolling average
};

extern client_static_t	cls;

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
	int			servercount;	// server identification for prespawns

	char		serverinfo[MAX_SERVERINFO_STRING];

	int			parsecount;		// server message counter
	int			validsequence;	// this is the sequence number of the last good
								// packetentity_t we got.  If this is 0, we can't
								// render a frame yet
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the 
								// first frame

	int			spectator;

	double		last_ping_request;	// while showing scoreboard
	double		last_servermessage;

// sentcmds[cl.netchan.outgoing_sequence & UPDATE_MASK] = cmd
	frame_t		frames[UPDATE_BACKUP];

// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			inv_order[MAX_INVENTORY];
	int			inv_count, inv_startpos, inv_selected;
	float		item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;		// use anim frame if cl.time < this

	client_entvars_t	v; // NOTE: not every field will be update - you must specifically add
	                   // them in functions SV_WriteClientdatatToMessage() and CL_ParseClientdata()

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

	char puzzle_pieces[8][10]; // puzzle piece names

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  And only reset at level change
// and teleport times
	vec3_t		viewangles;

// the client simulates or interpolates movement to get these values
	double		serverTimeFloat;// this is the time value that the client
								// is rendering at.  allways <= realtime
	vec3_t		simorg;
	vec3_t		simvel;
	vec3_t		simangles;

	float		idealroll;
	float		rollvel;
	
// pitch drifting vars
	float		idealpitch;
	float		pitchvel;
	qboolean	nodrift;
	float		driftmove;
	double		laststop;


	float		crouch;			// local amount for smoothing stepups

	qboolean	paused;			// send over by server

	float		punchangle;		// temporar yview kick from weapon firing
	
	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched ffrom time at intermission start
	
//
// information that is static for the entire time connected to a server
//
	char		model_name[MAX_MODELS][MAX_QPATH];
	char		sound_name[MAX_SOUNDS][MAX_QPATH];

	qhandle_t	model_precache[MAX_MODELS];
	sfxHandle_t	sound_precache[MAX_SOUNDS];

	char		levelname[40];	// for display on solo scoreboard
	int			playernum;

	clipHandle_t	clip_models[MAX_MODELS];

// refresh related state
	int			num_entities;	// stored bottom up in cl_entities array
	int			num_statics;	// stored top down in cl_entitiers

	int			cdtrack;		// cd audio
	char		midi_name[MAX_QPATH];     // midi file name

	entity_t	viewent;		// weapon model

	struct EffectT Effects[MAX_EFFECTS];

	unsigned	PIV;			// players in view

// all player information
	player_info_t	players[MAX_CLIENTS];
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

#define	MAX_STATIC_ENTITIES	256			// torches, etc

extern	client_state_t	cl;

// FIXME, allocate dynamically
extern	entity_state_t	cl_baselines[MAX_EDICTS];
extern	entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
extern	clightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES_Q1];
extern	int				cl_lightstylevalue[256];	// 8.8 fraction of base light value

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

void CL_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles, vec3_t angleAdd, int scale, int colorshade, int abslight, int drawflags);
void CL_AnimateLight(void);

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

void CL_ParseTEnt (void);
void CL_UpdateTEnts (void);

void CL_ClearState (void);

void CL_ReadPackets (void);

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

void V_ParseTarget(void);

extern	float	v_targAngle;
extern	float	v_targPitch;
extern	float	v_targDist;


//
// cl_tent
//
void CL_InitTEnts (void);
void CL_ClearTEnts (void);
void CL_UpdateHammer(refEntity_t *ent, int edict_num);
void CL_UpdateBug(refEntity_t *ent);
void CL_UpdateIceStorm(refEntity_t *ent, int edict_num);
void CL_UpdatePoisonGas(refEntity_t *ent, vec3_t angles, int edict_num);
void CL_UpdateAcidBlob(refEntity_t *ent, vec3_t angles, int edict_num);
void CL_UpdateOnFire(refEntity_t *ent, vec3_t angles, int edict_num);
void CL_UpdatePowerFlameBurn(refEntity_t *ent, int edict_num);

//
// cl_ents.c
//
void CL_SetSolidPlayers (int playernum);

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);

void CL_UpdateEffects(void);
qboolean CL_CheckOrDownloadFile (char *filename);
void CL_ClearProjectiles (void);
void CL_ClearMissiles (void);
void CL_ParsePlayerinfo (void);
void CL_SavePlayer (void);
void CL_ParseProjectiles (void);
void CL_ParsePackMissiles (void);
void CL_ParsePacketEntities (qboolean delta);
void CL_ParseEffect(void);
void CL_EndEffect(void);
void CL_TurnEffect(void);
void CL_ReviseEffect(void);
void CL_ParseMultiEffect(void);
void CL_SetSolidEntities (void);
void CL_ClearEffects(void);
void CL_InitEffects(void);
void CL_InitCam(void);
void CL_SetUpPlayerPrediction(qboolean dopred);
void CL_EmitEntities (void);
void CL_WriteDemoCmd (usercmd_t *pcmd);
void CL_PredictUsercmd (player_state_t *from, player_state_t *to, usercmd_t *u, qboolean spectator);
void CL_SendConnectPacket (void);
void Host_WriteConfiguration (const char *fname);
void Cam_Track(usercmd_t *cmd);
void Cam_FinishMove(usercmd_t *cmd);
void Cam_Reset(void);

void R_TranslatePlayerSkin (int playernum);
void R_HandleCustomSkin(refEntity_t* Ent, int PlayerNum);

extern	image_t*	playertextures[MAX_CLIENTS];

#define MAX_EXTRA_TEXTURES 156   // 255-100+1
extern image_t*		gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models
