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

#define MAX_TOKEN_CHARS_Q3  1024	// max length of an individual token

#define MAX_CLIENTS_Q3      64		// absolute limit
#define MAX_MODELS_Q3       256		// these are sent over the net as 8 bits so they cannot be blindly increased

#define MAX_CONFIGSTRINGS_Q3    1024

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define Q3CS_SERVERINFO     0		// an info string with all the serverinfo cvars
#define Q3CS_SYSTEMINFO     1		// an info string for server system to client system configuration (timescale, etc)
#define Q3CS_WARMUP         5		// server time when the match will be restarted

#define GENTITYNUM_BITS_Q3      10		// don't need to send any more
#define MAX_GENTITIES_Q3        (1 << GENTITYNUM_BITS_Q3)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values that are going to be communcated over the net need to
// also be in this range
#define Q3ENTITYNUM_NONE        (MAX_GENTITIES_Q3 - 1)
#define Q3ENTITYNUM_WORLD       (MAX_GENTITIES_Q3 - 2)
#define Q3ENTITYNUM_MAX_NORMAL  (MAX_GENTITIES_Q3 - 2)

// q3usercmd_t is sent to the server each client frame
struct q3usercmd_t
{
	int serverTime;
	int angles[3];
	int buttons;
	byte weapon;			// weapon
	signed char forwardmove, rightmove, upmove;
};

// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum
{
	q3svc_bad,
	q3svc_nop,
	q3svc_gamestate,
	q3svc_configstring,			// [short] [string] only in gamestate messages
	q3svc_baseline,				// only in gamestate messages
	q3svc_serverCommand,			// [string] to be executed by client game module
	q3svc_download,				// [short] size [size bytes]
	q3svc_snapshot,
	q3svc_EOF
};


//
// client to server
//
enum
{
	q3clc_bad,
	q3clc_nop,
	q3clc_move,				// [[q3usercmd_t]
	q3clc_moveNoDelta,		// [[q3usercmd_t]
	q3clc_clientCommand,		// [string] message
	q3clc_EOF
};

#define MAX_INFO_STRING_Q3  1024
#define MAX_INFO_KEY_Q3     1024
#define MAX_INFO_VALUE_Q3   1024

// bit field limits
#define MAX_STATS_Q3            16
#define MAX_PERSISTANT_Q3       16
#define MAX_POWERUPS_Q3         16
#define MAX_WEAPONS_Q3          16

#define MAX_PS_EVENTS_Q3        2

// q3playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// q3playerState_t is a full superset of q3entityState_t as it is used by players,
// so if a q3playerState_t is transmitted, the q3entityState_t can be fully derived
// from it.
struct q3playerState_t
{
	int commandTime;	// cmd->serverTime of last executed command
	int pm_type;
	int bobCycle;		// for view bobbing and footstep generation
	int pm_flags;		// ducked, jump_held, etc
	int pm_time;

	vec3_t origin;
	vec3_t velocity;
	int weaponTime;
	int gravity;
	int speed;
	int delta_angles[3];	// add to command angles to get view direction
							// changed by spawns, rotating objects, and teleporters

	int groundEntityNum;// Q3ENTITYNUM_NONE = in air

	int legsTimer;		// don't change low priority animations until this runs out
	int legsAnim;		// mask off ANIM_TOGGLEBIT

	int torsoTimer;		// don't change low priority animations until this runs out
	int torsoAnim;		// mask off ANIM_TOGGLEBIT

	int movementDir;	// a number 0 to 7 that represents the reletive angle
						// of movement to the view angle (axial and diagonals)
						// when at rest, the value will remain unchanged
						// used to twist the legs during strafing

	vec3_t grapplePoint;	// location of grapple to pull towards if PMF_GRAPPLE_PULL

	int eFlags;			// copied to q3entityState_t->eFlags

	int eventSequence;	// pmove generated events
	int events[MAX_PS_EVENTS_Q3];
	int eventParms[MAX_PS_EVENTS_Q3];

	int externalEvent;	// events set on player from another source
	int externalEventParm;
	int externalEventTime;

	int clientNum;		// ranges from 0 to MAX_CLIENTS_Q3-1
	int weapon;			// copied to q3entityState_t->weapon
	int weaponstate;

	vec3_t viewangles;		// for fixed views
	int viewheight;

	// damage feedback
	int damageEvent;	// when it changes, latch the other parms
	int damageYaw;
	int damagePitch;
	int damageCount;

	int stats[MAX_STATS_Q3];
	int persistant[MAX_PERSISTANT_Q3];	// stats that aren't cleared on death
	int powerups[MAX_POWERUPS_Q3];	// level.time that the powerup runs out
	int ammo[MAX_WEAPONS_Q3];

	int generic1;
	int loopSound;
	int jumppad_ent;	// jumppad entity hit this frame

	// not communicated over the net at all
	int ping;			// server to game info for scoreboard
	int pmove_framecount;	// FIXME: don't transmit over the network
	int jumppad_frame;
	int entityEventSequence;
};

// if entityState->solid == Q3SOLID_BMODEL, modelindex is an inline model number
#define Q3SOLID_BMODEL  0xffffff

struct q3trajectory_t
{
	int trType;
	int trTime;
	int trDuration;			// if non 0, trTime + trDuration = stop time
	vec3_t trBase;
	vec3_t trDelta;			// velocity, etc
};

// q3entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

struct q3entityState_t
{
	int number;			// entity index
	int eType;			// entityType_t
	int eFlags;

	q3trajectory_t pos;	// for calculating position
	q3trajectory_t apos;	// for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum;	// shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum;	// -1 = in air

	int constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
	int loopSound;		// constantly loop this sound

	int modelindex;
	int modelindex2;
	int clientNum;		// 0 to (MAX_CLIENTS_Q3 - 1), for players and corpses
	int frame;

	int solid;			// for client side prediction, trap_linkentity sets this properly

	int event;			// impulse events -- muzzle flashes, footsteps, etc
	int eventParm;

	// for players
	int powerups;		// bit flags
	int weapon;			// determines weapon and flash model, etc
	int legsAnim;		// mask off ANIM_TOGGLEBIT
	int torsoAnim;		// mask off ANIM_TOGGLEBIT

	int generic1;
};

#define PACKET_BACKUP_Q3    32	// number of old messages that must be kept on client and
// server for delta comrpession and ping estimation
#define PACKET_MASK_Q3      (PACKET_BACKUP_Q3 - 1)

#define MAX_RELIABLE_COMMANDS_Q3    64			// max string commands buffered for restransmit

//
// q3usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define Q3BUTTON_ATTACK         1
#define Q3BUTTON_TALK           2			// displays talk balloon and disables actions
#define Q3BUTTON_WALKING        16			// walking can't just be infered from MOVE_RUN
// because a key pressed late in the frame will
// only generate a small move value for that frame
// walking will use different animations and
// won't generate footsteps
#define Q3BUTTON_ANY            2048			// any key whatsoever

// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
enum
{
	Q3CHAN_AUTO,
	Q3CHAN_LOCAL,		// menu sounds, etc
	Q3CHAN_WEAPON,
	Q3CHAN_VOICE,
	Q3CHAN_ITEM,
	Q3CHAN_BODY,
	Q3CHAN_LOCAL_SOUND,	// chat messages, etc
	Q3CHAN_ANNOUNCER		// announcer voices, etc
};

struct orientation_t
{
	vec3_t origin;
	vec3_t axis[3];
};

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
enum
{
	Q3PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	// ... the rest are game specific.
};

enum
{
	Q3PM_INTERMISSION = 5	// no movement or status bar
};

#define Q3SNAPFLAG_RATE_DELAYED 1
#define Q3SNAPFLAG_NOT_ACTIVE   2	// snapshot used during connection and for zombies
#define Q3SNAPFLAG_SERVERCOUNT  4	// toggled every map_restart so transitions can be detected

#define Q3SV_ENCODE_START       4
#define Q3SV_DECODE_START       12
#define Q3CL_ENCODE_START       12
#define Q3CL_DECODE_START       4

#define Q3AUTHORIZE_SERVER_NAME   "authorize.quake3arena.com"

#define Q3PORT_AUTHORIZE        27952
#define Q3PORT_MASTER           27950

#define Q3PROTOCOL_VERSION      68
// 1.31 - 67

#define Q3MAX_PACKET_USERCMDS   32		// max number of q3usercmd_t in a packet

#define Q3MASTER_SERVER_NAME  "master.quake3arena.com"
