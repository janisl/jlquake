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

#define MAX_TOKEN_CHARS_Q2	128		// max length of an individual token

#define MAX_EDICTS_Q2		1024	// must change protocol to increase more

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

//
// muzzle flashes / player effects
//
#define Q2MZ_BLASTER			0
#define Q2MZ_MACHINEGUN			1
#define Q2MZ_SHOTGUN			2
#define Q2MZ_CHAINGUN1			3
#define Q2MZ_CHAINGUN2			4
#define Q2MZ_CHAINGUN3			5
#define Q2MZ_RAILGUN			6
#define Q2MZ_ROCKET				7
#define Q2MZ_GRENADE			8
#define Q2MZ_LOGIN				9
#define Q2MZ_LOGOUT				10
#define Q2MZ_RESPAWN			11
#define Q2MZ_BFG				12
#define Q2MZ_SSHOTGUN			13
#define Q2MZ_HYPERBLASTER		14
#define Q2MZ_ITEMRESPAWN		15
// RAFAEL
#define Q2MZ_IONRIPPER			16
#define Q2MZ_BLUEHYPERBLASTER	17
#define Q2MZ_PHALANX			18
#define Q2MZ_SILENCED			128		// bit flag ORed with one of the above numbers
//ROGUE
#define Q2MZ_ETF_RIFLE			30
#define Q2MZ_UNUSED				31
#define Q2MZ_SHOTGUN2			32
#define Q2MZ_HEATBEAM			33
#define Q2MZ_BLASTER2			34
#define Q2MZ_TRACKER			35
#define Q2MZ_NUKE1				36
#define Q2MZ_NUKE2				37
#define Q2MZ_NUKE4				38
#define Q2MZ_NUKE8				39

//
// monster muzzle flashes
//
#define Q2MZ2_TANK_BLASTER_1			1
#define Q2MZ2_TANK_BLASTER_2			2
#define Q2MZ2_TANK_BLASTER_3			3
#define Q2MZ2_TANK_MACHINEGUN_1			4
#define Q2MZ2_TANK_MACHINEGUN_2			5
#define Q2MZ2_TANK_MACHINEGUN_3			6
#define Q2MZ2_TANK_MACHINEGUN_4			7
#define Q2MZ2_TANK_MACHINEGUN_5			8
#define Q2MZ2_TANK_MACHINEGUN_6			9
#define Q2MZ2_TANK_MACHINEGUN_7			10
#define Q2MZ2_TANK_MACHINEGUN_8			11
#define Q2MZ2_TANK_MACHINEGUN_9			12
#define Q2MZ2_TANK_MACHINEGUN_10		13
#define Q2MZ2_TANK_MACHINEGUN_11		14
#define Q2MZ2_TANK_MACHINEGUN_12		15
#define Q2MZ2_TANK_MACHINEGUN_13		16
#define Q2MZ2_TANK_MACHINEGUN_14		17
#define Q2MZ2_TANK_MACHINEGUN_15		18
#define Q2MZ2_TANK_MACHINEGUN_16		19
#define Q2MZ2_TANK_MACHINEGUN_17		20
#define Q2MZ2_TANK_MACHINEGUN_18		21
#define Q2MZ2_TANK_MACHINEGUN_19		22
#define Q2MZ2_TANK_ROCKET_1				23
#define Q2MZ2_TANK_ROCKET_2				24
#define Q2MZ2_TANK_ROCKET_3				25
#define Q2MZ2_INFANTRY_MACHINEGUN_1		26
#define Q2MZ2_INFANTRY_MACHINEGUN_2		27
#define Q2MZ2_INFANTRY_MACHINEGUN_3		28
#define Q2MZ2_INFANTRY_MACHINEGUN_4		29
#define Q2MZ2_INFANTRY_MACHINEGUN_5		30
#define Q2MZ2_INFANTRY_MACHINEGUN_6		31
#define Q2MZ2_INFANTRY_MACHINEGUN_7		32
#define Q2MZ2_INFANTRY_MACHINEGUN_8		33
#define Q2MZ2_INFANTRY_MACHINEGUN_9		34
#define Q2MZ2_INFANTRY_MACHINEGUN_10	35
#define Q2MZ2_INFANTRY_MACHINEGUN_11	36
#define Q2MZ2_INFANTRY_MACHINEGUN_12	37
#define Q2MZ2_INFANTRY_MACHINEGUN_13	38
#define Q2MZ2_SOLDIER_BLASTER_1			39
#define Q2MZ2_SOLDIER_BLASTER_2			40
#define Q2MZ2_SOLDIER_SHOTGUN_1			41
#define Q2MZ2_SOLDIER_SHOTGUN_2			42
#define Q2MZ2_SOLDIER_MACHINEGUN_1		43
#define Q2MZ2_SOLDIER_MACHINEGUN_2		44
#define Q2MZ2_GUNNER_MACHINEGUN_1		45
#define Q2MZ2_GUNNER_MACHINEGUN_2		46
#define Q2MZ2_GUNNER_MACHINEGUN_3		47
#define Q2MZ2_GUNNER_MACHINEGUN_4		48
#define Q2MZ2_GUNNER_MACHINEGUN_5		49
#define Q2MZ2_GUNNER_MACHINEGUN_6		50
#define Q2MZ2_GUNNER_MACHINEGUN_7		51
#define Q2MZ2_GUNNER_MACHINEGUN_8		52
#define Q2MZ2_GUNNER_GRENADE_1			53
#define Q2MZ2_GUNNER_GRENADE_2			54
#define Q2MZ2_GUNNER_GRENADE_3			55
#define Q2MZ2_GUNNER_GRENADE_4			56
#define Q2MZ2_CHICK_ROCKET_1			57
#define Q2MZ2_FLYER_BLASTER_1			58
#define Q2MZ2_FLYER_BLASTER_2			59
#define Q2MZ2_MEDIC_BLASTER_1			60
#define Q2MZ2_GLADIATOR_RAILGUN_1		61
#define Q2MZ2_HOVER_BLASTER_1			62
#define Q2MZ2_ACTOR_MACHINEGUN_1		63
#define Q2MZ2_SUPERTANK_MACHINEGUN_1	64
#define Q2MZ2_SUPERTANK_MACHINEGUN_2	65
#define Q2MZ2_SUPERTANK_MACHINEGUN_3	66
#define Q2MZ2_SUPERTANK_MACHINEGUN_4	67
#define Q2MZ2_SUPERTANK_MACHINEGUN_5	68
#define Q2MZ2_SUPERTANK_MACHINEGUN_6	69
#define Q2MZ2_SUPERTANK_ROCKET_1		70
#define Q2MZ2_SUPERTANK_ROCKET_2		71
#define Q2MZ2_SUPERTANK_ROCKET_3		72
#define Q2MZ2_BOSS2_MACHINEGUN_L1		73
#define Q2MZ2_BOSS2_MACHINEGUN_L2		74
#define Q2MZ2_BOSS2_MACHINEGUN_L3		75
#define Q2MZ2_BOSS2_MACHINEGUN_L4		76
#define Q2MZ2_BOSS2_MACHINEGUN_L5		77
#define Q2MZ2_BOSS2_ROCKET_1			78
#define Q2MZ2_BOSS2_ROCKET_2			79
#define Q2MZ2_BOSS2_ROCKET_3			80
#define Q2MZ2_BOSS2_ROCKET_4			81
#define Q2MZ2_FLOAT_BLASTER_1			82
#define Q2MZ2_SOLDIER_BLASTER_3			83
#define Q2MZ2_SOLDIER_SHOTGUN_3			84
#define Q2MZ2_SOLDIER_MACHINEGUN_3		85
#define Q2MZ2_SOLDIER_BLASTER_4			86
#define Q2MZ2_SOLDIER_SHOTGUN_4			87
#define Q2MZ2_SOLDIER_MACHINEGUN_4		88
#define Q2MZ2_SOLDIER_BLASTER_5			89
#define Q2MZ2_SOLDIER_SHOTGUN_5			90
#define Q2MZ2_SOLDIER_MACHINEGUN_5		91
#define Q2MZ2_SOLDIER_BLASTER_6			92
#define Q2MZ2_SOLDIER_SHOTGUN_6			93
#define Q2MZ2_SOLDIER_MACHINEGUN_6		94
#define Q2MZ2_SOLDIER_BLASTER_7			95
#define Q2MZ2_SOLDIER_SHOTGUN_7			96
#define Q2MZ2_SOLDIER_MACHINEGUN_7		97
#define Q2MZ2_SOLDIER_BLASTER_8			98
#define Q2MZ2_SOLDIER_SHOTGUN_8			99
#define Q2MZ2_SOLDIER_MACHINEGUN_8		100
// --- Xian shit below ---
#define Q2MZ2_MAKRON_BFG				101
#define Q2MZ2_MAKRON_BLASTER_1			102
#define Q2MZ2_MAKRON_BLASTER_2			103
#define Q2MZ2_MAKRON_BLASTER_3			104
#define Q2MZ2_MAKRON_BLASTER_4			105
#define Q2MZ2_MAKRON_BLASTER_5			106
#define Q2MZ2_MAKRON_BLASTER_6			107
#define Q2MZ2_MAKRON_BLASTER_7			108
#define Q2MZ2_MAKRON_BLASTER_8			109
#define Q2MZ2_MAKRON_BLASTER_9			110
#define Q2MZ2_MAKRON_BLASTER_10			111
#define Q2MZ2_MAKRON_BLASTER_11			112
#define Q2MZ2_MAKRON_BLASTER_12			113
#define Q2MZ2_MAKRON_BLASTER_13			114
#define Q2MZ2_MAKRON_BLASTER_14			115
#define Q2MZ2_MAKRON_BLASTER_15			116
#define Q2MZ2_MAKRON_BLASTER_16			117
#define Q2MZ2_MAKRON_BLASTER_17			118
#define Q2MZ2_MAKRON_RAILGUN_1			119
#define Q2MZ2_JORG_MACHINEGUN_L1		120
#define Q2MZ2_JORG_MACHINEGUN_L2		121
#define Q2MZ2_JORG_MACHINEGUN_L3		122
#define Q2MZ2_JORG_MACHINEGUN_L4		123
#define Q2MZ2_JORG_MACHINEGUN_L5		124
#define Q2MZ2_JORG_MACHINEGUN_L6		125
#define Q2MZ2_JORG_MACHINEGUN_R1		126
#define Q2MZ2_JORG_MACHINEGUN_R2		127
#define Q2MZ2_JORG_MACHINEGUN_R3		128
#define Q2MZ2_JORG_MACHINEGUN_R4		129
#define Q2MZ2_JORG_MACHINEGUN_R5		130
#define Q2MZ2_JORG_MACHINEGUN_R6		131
#define Q2MZ2_JORG_BFG_1				132
#define Q2MZ2_BOSS2_MACHINEGUN_R1		133
#define Q2MZ2_BOSS2_MACHINEGUN_R2		134
#define Q2MZ2_BOSS2_MACHINEGUN_R3		135
#define Q2MZ2_BOSS2_MACHINEGUN_R4		136
#define Q2MZ2_BOSS2_MACHINEGUN_R5		137
//ROGUE
#define Q2MZ2_CARRIER_MACHINEGUN_L1		138
#define Q2MZ2_CARRIER_MACHINEGUN_R1		139
#define Q2MZ2_CARRIER_GRENADE			140
#define Q2MZ2_TURRET_MACHINEGUN			141
#define Q2MZ2_TURRET_ROCKET				142
#define Q2MZ2_TURRET_BLASTER			143
#define Q2MZ2_STALKER_BLASTER			144
#define Q2MZ2_DAEDALUS_BLASTER			145
#define Q2MZ2_MEDIC_BLASTER_2			146
#define Q2MZ2_CARRIER_RAILGUN			147
#define Q2MZ2_WIDOW_DISRUPTOR			148
#define Q2MZ2_WIDOW_BLASTER				149
#define Q2MZ2_WIDOW_RAIL				150
#define Q2MZ2_WIDOW_PLASMABEAM			151		// PMM - not used
#define Q2MZ2_CARRIER_MACHINEGUN_L2		152
#define Q2MZ2_CARRIER_MACHINEGUN_R2		153
#define Q2MZ2_WIDOW_RAIL_LEFT			154
#define Q2MZ2_WIDOW_RAIL_RIGHT			155
#define Q2MZ2_WIDOW_BLASTER_SWEEP1		156
#define Q2MZ2_WIDOW_BLASTER_SWEEP2		157
#define Q2MZ2_WIDOW_BLASTER_SWEEP3		158
#define Q2MZ2_WIDOW_BLASTER_SWEEP4		159
#define Q2MZ2_WIDOW_BLASTER_SWEEP5		160
#define Q2MZ2_WIDOW_BLASTER_SWEEP6		161
#define Q2MZ2_WIDOW_BLASTER_SWEEP7		162
#define Q2MZ2_WIDOW_BLASTER_SWEEP8		163
#define Q2MZ2_WIDOW_BLASTER_SWEEP9		164
#define Q2MZ2_WIDOW_BLASTER_100			165
#define Q2MZ2_WIDOW_BLASTER_90			166
#define Q2MZ2_WIDOW_BLASTER_80			167
#define Q2MZ2_WIDOW_BLASTER_70			168
#define Q2MZ2_WIDOW_BLASTER_60			169
#define Q2MZ2_WIDOW_BLASTER_50			170
#define Q2MZ2_WIDOW_BLASTER_40			171
#define Q2MZ2_WIDOW_BLASTER_30			172
#define Q2MZ2_WIDOW_BLASTER_20			173
#define Q2MZ2_WIDOW_BLASTER_10			174
#define Q2MZ2_WIDOW_BLASTER_0			175
#define Q2MZ2_WIDOW_BLASTER_10L			176
#define Q2MZ2_WIDOW_BLASTER_20L			177
#define Q2MZ2_WIDOW_BLASTER_30L			178
#define Q2MZ2_WIDOW_BLASTER_40L			179
#define Q2MZ2_WIDOW_BLASTER_50L			180
#define Q2MZ2_WIDOW_BLASTER_60L			181
#define Q2MZ2_WIDOW_BLASTER_70L			182
#define Q2MZ2_WIDOW_RUN_1				183
#define Q2MZ2_WIDOW_RUN_2				184
#define Q2MZ2_WIDOW_RUN_3				185
#define Q2MZ2_WIDOW_RUN_4				186
#define Q2MZ2_WIDOW_RUN_5				187
#define Q2MZ2_WIDOW_RUN_6				188
#define Q2MZ2_WIDOW_RUN_7				189
#define Q2MZ2_WIDOW_RUN_8				190
#define Q2MZ2_CARRIER_ROCKET_1			191
#define Q2MZ2_CARRIER_ROCKET_2			192
#define Q2MZ2_CARRIER_ROCKET_3			193
#define Q2MZ2_CARRIER_ROCKET_4			194
#define Q2MZ2_WIDOW2_BEAMER_1			195
#define Q2MZ2_WIDOW2_BEAMER_2			196
#define Q2MZ2_WIDOW2_BEAMER_3			197
#define Q2MZ2_WIDOW2_BEAMER_4			198
#define Q2MZ2_WIDOW2_BEAMER_5			199
#define Q2MZ2_WIDOW2_BEAM_SWEEP_1		200
#define Q2MZ2_WIDOW2_BEAM_SWEEP_2		201
#define Q2MZ2_WIDOW2_BEAM_SWEEP_3		202
#define Q2MZ2_WIDOW2_BEAM_SWEEP_4		203
#define Q2MZ2_WIDOW2_BEAM_SWEEP_5		204
#define Q2MZ2_WIDOW2_BEAM_SWEEP_6		205
#define Q2MZ2_WIDOW2_BEAM_SWEEP_7		206
#define Q2MZ2_WIDOW2_BEAM_SWEEP_8		207
#define Q2MZ2_WIDOW2_BEAM_SWEEP_9		208
#define Q2MZ2_WIDOW2_BEAM_SWEEP_10		209
#define Q2MZ2_WIDOW2_BEAM_SWEEP_11		210

// q2entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
enum q2entity_event_t
{
	Q2EV_NONE,
	Q2EV_ITEM_RESPAWN,
	Q2EV_FOOTSTEP,
	Q2EV_FALLSHORT,
	Q2EV_FALL,
	Q2EV_FALLFAR,
	Q2EV_PLAYER_TELEPORT,
	Q2EV_OTHER_TELEPORT
};

// q2usercmd_t is sent to the server each client frame
struct q2usercmd_t
{
	byte	msec;
	byte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	byte	impulse;		// remove?
	byte	lightlevel;		// light level the player is standing on
};

// q2entity_state_t communication

// try to pack the common update flags into the first byte
#define Q2U_ORIGIN1		(1<<0)
#define Q2U_ORIGIN2		(1<<1)
#define Q2U_ANGLE2		(1<<2)
#define Q2U_ANGLE3		(1<<3)
#define Q2U_FRAME8		(1<<4)		// frame is a byte
#define Q2U_EVENT		(1<<5)
#define Q2U_REMOVE		(1<<6)		// REMOVE this entity, don't add it
#define Q2U_MOREBITS1	(1<<7)		// read one additional byte

// second byte
#define Q2U_NUMBER16	(1<<8)		// NUMBER8 is implicit if not set
#define Q2U_ORIGIN3		(1<<9)
#define Q2U_ANGLE1		(1<<10)
#define Q2U_MODEL		(1<<11)
#define Q2U_RENDERFX8	(1<<12)		// fullbright, etc
#define Q2U_EFFECTS8	(1<<14)		// autorotate, trails, etc
#define Q2U_MOREBITS2	(1<<15)		// read one additional byte

// third byte
#define Q2U_SKIN8		(1<<16)
#define Q2U_FRAME16		(1<<17)		// frame is a short
#define Q2U_RENDERFX16	(1<<18)	// 8 + 16 = 32
#define Q2U_EFFECTS16	(1<<19)		// 8 + 16 = 32
#define Q2U_MODEL2		(1<<20)		// weapons, flags, etc
#define Q2U_MODEL3		(1<<21)
#define Q2U_MODEL4		(1<<22)
#define Q2U_MOREBITS3	(1<<23)		// read one additional byte

// fourth byte
#define Q2U_OLDORIGIN	(1<<24)		// FIXME: get rid of this
#define Q2U_SKIN16		(1<<25)
#define Q2U_SOUND		(1<<26)
#define Q2U_SOLID		(1<<27)

//=========================================

//==================
// the svc_strings[] array in cl_parse.c should mirror this
//==================

//
// server to client
//
enum
{
	q2svc_bad,

	// these ops are known to the game dll
	q2svc_muzzleflash,
	q2svc_muzzleflash2,
	q2svc_temp_entity,
	q2svc_layout,
	q2svc_inventory,

	// the rest are private to the client and server
	q2svc_nop,
	q2svc_disconnect,
	q2svc_reconnect,
	q2svc_sound,				// <see code>
	q2svc_print,				// [byte] id [string] null terminated string
	q2svc_stufftext,			// [string] stuffed into client's console buffer, should be \n terminated
	q2svc_serverdata,			// [long] protocol ...
	q2svc_configstring,			// [short] [string]
	q2svc_spawnbaseline,		
	q2svc_centerprint,			// [string] to put in center of the screen
	q2svc_download,				// [short] size [size bytes]
	q2svc_playerinfo,			// variable
	q2svc_packetentities,		// [...]
	q2svc_deltapacketentities,	// [...]
	q2svc_frame
};

//==============================================

//
// client to server
//
enum
{
	q2clc_bad,
	q2clc_nop, 		
	q2clc_move,				// [usercmd_t]
	q2clc_userinfo,			// [userinfo string]
	q2clc_stringcmd			// [string] message
};

//
// per-level limits
//
#define MAX_CLIENTS_Q2			256		// absolute limit
#define MAX_LIGHTSTYLES_Q2		256
#define MAX_MODELS_Q2			256		// these are sent over the net as bytes
#define MAX_SOUNDS_Q2			256		// so they cannot be blindly increased
#define MAX_IMAGES_Q2			256
#define MAX_ITEMS_Q2			256
#define MAX_GENERAL_Q2			(MAX_CLIENTS_Q2 * 2)	// general config strings

// q2entity_state_t->renderfx flags
#define Q2RF_MINLIGHT		1		// allways have some light (viewmodel)
#define Q2RF_VIEWERMODEL	2		// don't draw through eyes, only mirrors
#define Q2RF_WEAPONMODEL	4		// only draw through eyes
#define Q2RF_FULLBRIGHT		8		// allways draw full intensity
#define Q2RF_DEPTHHACK		16		// for view weapon Z crunching
#define Q2RF_TRANSLUCENT	32
#define Q2RF_FRAMELERP		64
#define Q2RF_BEAM			128
#define Q2RF_CUSTOMSKIN		256		// skin is an index in image_precache
#define Q2RF_GLOW			512		// pulse lighting for bonus items
#define Q2RF_SHELL_RED		1024
#define Q2RF_SHELL_GREEN	2048
#define Q2RF_SHELL_BLUE		4096

//ROGUE
#define Q2RF_IR_VISIBLE		0x00008000		// 32768
#define Q2RF_SHELL_DOUBLE	0x00010000		// 65536
#define Q2RF_SHELL_HALF_DAM	0x00020000
#define Q2RF_USE_DISGUISE	0x00040000
//ROGUE

// q2player_state_t->refdef flags
#define Q2RDF_UNDERWATER	1		// warp the screen as apropriate (UNUSED)
#define Q2RDF_NOWORLDMODEL	2		// used for player configuration screen

//ROGUE
#define Q2RDF_IRGOGGLES		4
#define Q2RDF_UVGOGGLES		8//UNUSED
//ROGUE

//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//
#define	Q2CS_NAME				0
#define	Q2CS_CDTRACK			1
#define	Q2CS_SKY				2
#define	Q2CS_SKYAXIS			3		// %f %f %f format
#define	Q2CS_SKYROTATE		4
#define	Q2CS_STATUSBAR		5		// display program string

#define Q2CS_AIRACCEL			29		// air acceleration control
#define	Q2CS_MAXCLIENTS		30
#define	Q2CS_MAPCHECKSUM		31		// for catching cheater maps

#define	Q2CS_MODELS			32
#define	Q2CS_SOUNDS			(Q2CS_MODELS+MAX_MODELS_Q2)
#define	Q2CS_IMAGES			(Q2CS_SOUNDS+MAX_SOUNDS_Q2)
#define	Q2CS_LIGHTS			(Q2CS_IMAGES+MAX_IMAGES_Q2)
#define	Q2CS_ITEMS			(Q2CS_LIGHTS+MAX_LIGHTSTYLES_Q2)
#define	Q2CS_PLAYERSKINS		(Q2CS_ITEMS+MAX_ITEMS_Q2)
#define Q2CS_GENERAL			(Q2CS_PLAYERSKINS+MAX_CLIENTS_Q2)
#define	MAX_CONFIGSTRINGS_Q2	(Q2CS_GENERAL+MAX_GENERAL_Q2)
