//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#define UPDATE_BACKUP_Q2	16	// copies of q2entity_state_t to keep buffered
							// must be power of two
#define UPDATE_MASK_Q2		(UPDATE_BACKUP_Q2 - 1)

// q2entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define Q2EF_ROTATE				0x00000001		// rotate (bonus items)
#define Q2EF_GIB				0x00000002		// leave a trail
#define Q2EF_BLASTER			0x00000008		// redlight + trail
#define Q2EF_ROCKET				0x00000010		// redlight + trail
#define Q2EF_GRENADE			0x00000020
#define Q2EF_HYPERBLASTER		0x00000040
#define Q2EF_BFG				0x00000080
#define Q2EF_COLOR_SHELL		0x00000100
#define Q2EF_POWERSCREEN		0x00000200
#define Q2EF_ANIM01				0x00000400		// automatically cycle between frames 0 and 1 at 2 hz
#define Q2EF_ANIM23				0x00000800		// automatically cycle between frames 2 and 3 at 2 hz
#define Q2EF_ANIM_ALL			0x00001000		// automatically cycle through all frames at 2hz
#define Q2EF_ANIM_ALLFAST		0x00002000		// automatically cycle through all frames at 10hz
#define Q2EF_FLIES				0x00004000
#define Q2EF_QUAD				0x00008000
#define Q2EF_PENT				0x00010000
#define Q2EF_TELEPORTER			0x00020000		// particle fountain
#define Q2EF_FLAG1				0x00040000
#define Q2EF_FLAG2				0x00080000
// RAFAEL
#define Q2EF_IONRIPPER			0x00100000
#define Q2EF_GREENGIB			0x00200000
#define Q2EF_BLUEHYPERBLASTER	0x00400000
#define Q2EF_SPINNINGLIGHTS		0x00800000
#define Q2EF_PLASMA				0x01000000
#define Q2EF_TRAP				0x02000000
//ROGUE
#define Q2EF_TRACKER			0x04000000
#define Q2EF_DOUBLE				0x08000000
#define Q2EF_SPHERETRANS		0x10000000
#define Q2EF_TAGTRAIL			0x20000000
#define Q2EF_HALF_DAMAGE		0x40000000
#define Q2EF_TRACKERTRAIL		0x80000000

// q2entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
struct q2entity_state_t
{
	int		number;			// edict index

	vec3_t	origin;
	vec3_t	angles;
	vec3_t	old_origin;		// for lerping
	int		modelindex;
	int		modelindex2, modelindex3, modelindex4;	// weapons, CTF flags, etc
	int		frame;
	int		skinnum;
	unsigned int		effects;		// PGM - we're filling it, so it needs to be unsigned
	int		renderfx;
	int		solid;			// for client side prediction, 8*(bits 0-4) is x/y radius
							// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
							// gi.linkentity sets this properly
	int		sound;			// for looping sounds, to guarantee shutoff
	int		event;			// impulse events -- muzzle flashes, footsteps, etc
							// events only go out for a single frame, they
							// are automatically cleared each frame
};

// q2pmove_state_t is the information necessary for client side movement
// prediction
enum q2pmtype_t
{
	// can accelerate and turn
	Q2PM_NORMAL,
	Q2PM_SPECTATOR,
	// no acceleration or turning
	Q2PM_DEAD,
	Q2PM_GIB,		// different bounding box
	Q2PM_FREEZE
};

// pmove->pm_flags
#define Q2PMF_DUCKED			1
#define Q2PMF_JUMP_HELD			2
#define Q2PMF_ON_GROUND			4
#define Q2PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define Q2PMF_TIME_LAND			16	// pm_time is time before rejump
#define Q2PMF_TIME_TELEPORT		32	// pm_time is non-moving time
#define Q2PMF_NO_PREDICTION		64	// temporarily disables prediction (used for grappling hook)

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
struct q2pmove_state_t
{
	q2pmtype_t pm_type;
	short origin[3];		// 12.3
	short velocity[3];		// 12.3
	byte pm_flags;			// ducked, jump_held, etc
	byte pm_time;			// each unit = 8 ms
	short gravity;
	short delta_angles[3];	// add to command angles to get view direction
							// changed by spawns, rotating objects, and teleporters
};

// player_state->stats[] indexes
#define Q2STAT_HEALTH_ICON		0
#define Q2STAT_HEALTH			1
#define Q2STAT_AMMO_ICON		2
#define Q2STAT_AMMO				3
#define Q2STAT_ARMOR_ICON		4
#define Q2STAT_ARMOR			5
#define Q2STAT_SELECTED_ICON	6
#define Q2STAT_PICKUP_ICON		7
#define Q2STAT_PICKUP_STRING	8
#define Q2STAT_TIMER_ICON		9
#define Q2STAT_TIMER			10
#define Q2STAT_HELPICON			11
#define Q2STAT_SELECTED_ITEM	12
#define Q2STAT_LAYOUTS			13
#define Q2STAT_FRAGS			14
#define Q2STAT_FLASHES			15		// cleared each frame, 1 = health, 2 = armor
#define Q2STAT_CHASE			16
#define Q2STAT_SPECTATOR		17

#define MAX_STATS_Q2			32

// q2player_state_t is the information needed in addition to q2pmove_state_t
// to rendered a view.  There will only be 10 q2player_state_t sent each second,
// but the number of q2pmove_state_t changes will be reletive to client
// frame rates
struct q2player_state_t
{
	q2pmove_state_t pmove;	// for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t viewangles;		// for fixed views
	vec3_t viewoffset;		// add to pmovestate->origin
	vec3_t kick_angles;		// add to view direction to get render angles
							// set by weapon kicks, pain effects, etc

	vec3_t gunangles;
	vec3_t gunoffset;
	int gunindex;
	int gunframe;

	float blend[4];			// rgba full screen effect

	float fov;				// horizontal field of view

	int rdflags;			// refdef flags

	short stats[MAX_STATS_Q2];	// fast status bar updates
};

#define Q2SPLASH_UNKNOWN		0
#define Q2SPLASH_SPARKS			1
#define Q2SPLASH_BLUE_WATER		2
#define Q2SPLASH_BROWN_WATER	3
#define Q2SPLASH_SLIME			4
#define Q2SPLASH_LAVA			5
#define Q2SPLASH_BLOOD			6

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
#define Q2CHAN_AUTO			0
#define Q2CHAN_WEAPON		1
#define Q2CHAN_VOICE		2
#define Q2CHAN_ITEM			3
#define Q2CHAN_BODY			4
// modifier flags
#define Q2CHAN_NO_PHS_ADD	8	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
#define Q2CHAN_RELIABLE		16	// send by reliable message, not datagram

// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
enum q2temp_event_t
{
	Q2TE_GUNSHOT,
	Q2TE_BLOOD,
	Q2TE_BLASTER,
	Q2TE_RAILTRAIL,
	Q2TE_SHOTGUN,
	Q2TE_EXPLOSION1,
	Q2TE_EXPLOSION2,
	Q2TE_ROCKET_EXPLOSION,
	Q2TE_GRENADE_EXPLOSION,
	Q2TE_SPARKS,
	Q2TE_SPLASH,
	Q2TE_BUBBLETRAIL,
	Q2TE_SCREEN_SPARKS,
	Q2TE_SHIELD_SPARKS,
	Q2TE_BULLET_SPARKS,
	Q2TE_LASER_SPARKS,
	Q2TE_PARASITE_ATTACK,
	Q2TE_ROCKET_EXPLOSION_WATER,
	Q2TE_GRENADE_EXPLOSION_WATER,
	Q2TE_MEDIC_CABLE_ATTACK,
	Q2TE_BFG_EXPLOSION,
	Q2TE_BFG_BIGEXPLOSION,
	Q2TE_BOSSTPORT,			// used as '22' in a map, so DON'T RENUMBER!!!
	Q2TE_BFG_LASER,
	Q2TE_GRAPPLE_CABLE,
	Q2TE_WELDING_SPARKS,
	Q2TE_GREENBLOOD,
	Q2TE_BLUEHYPERBLASTER,
	Q2TE_PLASMA_EXPLOSION,
	Q2TE_TUNNEL_SPARKS,
//ROGUE
	Q2TE_BLASTER2,
	Q2TE_RAILTRAIL2,
	Q2TE_FLAME,
	Q2TE_LIGHTNING,
	Q2TE_DEBUGTRAIL,
	Q2TE_PLAIN_EXPLOSION,
	Q2TE_FLASHLIGHT,
	Q2TE_FORCEWALL,
	Q2TE_HEATBEAM,
	Q2TE_MONSTER_HEATBEAM,
	Q2TE_STEAM,
	Q2TE_BUBBLETRAIL2,
	Q2TE_MOREBLOOD,
	Q2TE_HEATBEAM_SPARKS,
	Q2TE_HEATBEAM_STEAM,
	Q2TE_CHAINFIST_SMOKE,
	Q2TE_ELECTRIC_SPARKS,
	Q2TE_TRACKER_EXPLOSION,
	Q2TE_TELEPORT_EFFECT,
	Q2TE_DBALL_GOAL,
	Q2TE_WIDOWBEAMOUT,
	Q2TE_NUKEBLAST,
	Q2TE_WIDOWSPLASH,
	Q2TE_EXPLOSION1_BIG,
	Q2TE_EXPLOSION1_NP,
	Q2TE_FLECHETTE
//ROGUE
};
