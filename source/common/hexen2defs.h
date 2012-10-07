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

#define MAX_CLIENT_STATES_H2    150

// Timing macros
#define HX_FRAME_TIME       0.05

// Player Classes
#define NUM_CLASSES_H2          4
#define NUM_CLASSES_H2MP        5
#define MAX_PLAYER_CLASS        6

#define CLASS_PALADIN           1
#define CLASS_CLERIC            2
#define CLASS_NECROMANCER       3
#define CLASS_THEIF             4
#define CLASS_DEMON             5
#define CLASS_DWARF             6

// edict->drawflags
#define H2MLS_MASKIN            7	// MLS: Model Light Style
#define H2MLS_MASKOUT           248
#define H2MLS_NONE              0
#define H2MLS_FULLBRIGHT        1
#define H2MLS_POWERMODE         2
#define H2MLS_TORCH             3
#define H2MLS_TOTALDARK         4
#define H2MLS_ABSLIGHT          7
#define H2SCALE_TYPE_MASKIN     24
#define H2SCALE_TYPE_MASKOUT    231
#define H2SCALE_TYPE_UNIFORM    0	// Scale X, Y, and Z
#define H2SCALE_TYPE_XYONLY     8	// Scale X and Y
#define H2SCALE_TYPE_ZONLY      16	// Scale Z
#define H2SCALE_ORIGIN_MASKIN   96
#define H2SCALE_ORIGIN_MASKOUT  159
#define H2SCALE_ORIGIN_CENTER   0	// Scaling origin at object center
#define H2SCALE_ORIGIN_BOTTOM   32	// Scaling origin at object bottom
#define H2SCALE_ORIGIN_TOP      64	// Scaling origin at object top
#define H2DRF_TRANSLUCENT       128

// h2entity_state_t is the information conveyed from the server
// in an update message
struct h2entity_state_t
{
	int number;			// edict index

	vec3_t origin;
	vec3_t angles;
	int modelindex;
	int frame;
	int colormap;
	int skinnum;
	int effects;
	int scale;			// for Alias models
	int drawflags;		// for Alias models
	int abslight;		// for Alias models
	int wpn_sound;		// for cheap playing of sounds
	int flags;			// nolerp, etc
	byte ClearCount[32];
};

// Types for various chunks
#define H2THINGTYPE_GREYSTONE   1
#define H2THINGTYPE_WOOD        2
#define H2THINGTYPE_METAL       3
#define H2THINGTYPE_FLESH       4
#define H2THINGTYPE_FIRE        5
#define H2THINGTYPE_CLAY        6
#define H2THINGTYPE_LEAVES      7
#define H2THINGTYPE_HAY         8
#define H2THINGTYPE_BROWNSTONE  9
#define H2THINGTYPE_CLOTH       10
#define H2THINGTYPE_WOOD_LEAF   11
#define H2THINGTYPE_WOOD_METAL  12
#define H2THINGTYPE_WOOD_STONE  13
#define H2THINGTYPE_METAL_STONE 14
#define H2THINGTYPE_METAL_CLOTH 15
#define H2THINGTYPE_WEBS        16
#define H2THINGTYPE_GLASS       17
#define H2THINGTYPE_ICE         18
#define H2THINGTYPE_CLEARGLASS  19
#define H2THINGTYPE_REDGLASS    20
#define H2THINGTYPE_ACID        21
#define H2THINGTYPE_METEOR      22
#define H2THINGTYPE_GREENFLESH  23
#define H2THINGTYPE_BONE        24
#define H2THINGTYPE_DIRT        25

#define H2CE_NONE                       0
#define H2CE_RAIN                       1
#define H2CE_FOUNTAIN                   2
#define H2CE_QUAKE                      3
#define H2CE_WHITE_SMOKE                4	// whtsmk1.spr
#define H2CE_BLUESPARK                  5	// bspark.spr
#define H2CE_YELLOWSPARK                6	// spark.spr
#define H2CE_SM_CIRCLE_EXP              7	// fcircle.spr
#define H2CE_BG_CIRCLE_EXP              8	// fcircle.spr
#define H2CE_SM_WHITE_FLASH             9	// sm_white.spr
#define H2CE_WHITE_FLASH                10	// gryspt.spr
#define H2CE_YELLOWRED_FLASH            11	// yr_flash.spr
#define H2CE_BLUE_FLASH                 12	// bluflash.spr
#define H2CE_SM_BLUE_FLASH              13	// bluflash.spr
#define H2CE_RED_FLASH                  14	// redspt.spr
#define H2CE_SM_EXPLOSION               15	// sm_expld.spr
#define H2CE_LG_EXPLOSION               16	// bg_expld.spr
#define H2CE_FLOOR_EXPLOSION            17	// fl_expld.spr
#define H2CE_RIDER_DEATH                18
#define H2CE_BLUE_EXPLOSION             19	// xpspblue.spr
#define H2CE_GREEN_SMOKE                20	// grnsmk1.spr
#define H2CE_GREY_SMOKE                 21	// grysmk1.spr
#define H2CE_RED_SMOKE                  22	// redsmk1.spr
#define H2CE_SLOW_WHITE_SMOKE           23	// whtsmk1.spr
#define H2CE_REDSPARK                   24	// rspark.spr
#define H2CE_GREENSPARK                 25	// gspark.spr
#define H2CE_TELESMK1                   26	// telesmk1.spr
#define H2CE_TELESMK2                   27	// telesmk2.spr
#define H2CE_ICEHIT                     28	// icehit.spr
#define H2CE_MEDUSA_HIT                 29	// medhit.spr
#define H2CE_MEZZO_REFLECT              30	// mezzoref.spr
#define H2CE_FLOOR_EXPLOSION2           31	// flrexpl2.spr
#define H2CE_XBOW_EXPLOSION             32	// xbowexpl.spr
#define H2CE_NEW_EXPLOSION              33	// gen_expl.spr
#define H2CE_MAGIC_MISSILE_EXPLOSION    34	// mm_expld.spr
#define H2CE_GHOST                      35	// ghost.spr
#define H2CE_BONE_EXPLOSION             36
#define H2CE_REDCLOUD                   37
#define H2CE_TELEPORTERPUFFS            38
#define H2CE_TELEPORTERBODY             39
#define H2CE_BONESHARD                  40
#define H2CE_BONESHRAPNEL               41
#define H2CE_FLAMESTREAM                42	//Flamethrower
#define H2CE_SNOW                       43
#define H2CE_GRAVITYWELL                44
#define H2CE_BLDRN_EXPL                 45
#define H2CE_ACID_MUZZFL                46
#define H2CE_ACID_HIT                   47
#define H2CE_FIREWALL_SMALL             48
#define H2CE_FIREWALL_MEDIUM            49
#define H2CE_FIREWALL_LARGE             50
#define H2CE_LBALL_EXPL                 51
#define H2CE_ACID_SPLAT                 52
#define H2CE_ACID_EXPL                  53
#define H2CE_FBOOM                      54
#define H2CE_CHUNK                      55
#define H2CE_BOMB                       56
#define H2CE_BRN_BOUNCE                 57
#define H2CE_LSHOCK                     58
#define H2CE_FLAMEWALL                  59
#define H2CE_FLAMEWALL2                 60
#define H2CE_FLOOR_EXPLOSION3           61
#define H2CE_ONFIRE                     62

#define HWCE_NONE                       0
#define HWCE_RAIN                       1
#define HWCE_FOUNTAIN                   2
#define HWCE_QUAKE                      3
#define HWCE_WHITE_SMOKE                4	// whtsmk1.spr
#define HWCE_BLUESPARK                  5	// bspark.spr
#define HWCE_YELLOWSPARK                6	// spark.spr
#define HWCE_SM_CIRCLE_EXP              7	// fcircle.spr
#define HWCE_BG_CIRCLE_EXP              8	// fcircle.spr
#define HWCE_SM_WHITE_FLASH             9	// sm_white.spr
#define HWCE_WHITE_FLASH                10	// gryspt.spr
#define HWCE_YELLOWRED_FLASH            11	// yr_flash.spr
#define HWCE_BLUE_FLASH                 12	// bluflash.spr
#define HWCE_SM_BLUE_FLASH              13	// bluflash.spr
#define HWCE_RED_FLASH                  14	// redspt.spr
#define HWCE_SM_EXPLOSION               15	// sm_expld.spr
#define HWCE_BG_EXPLOSION               16	// bg_expld.spr
#define HWCE_FLOOR_EXPLOSION            17	// fl_expld.spr
#define HWCE_RIDER_DEATH                18
#define HWCE_BLUE_EXPLOSION             19	// xpspblue.spr
#define HWCE_GREEN_SMOKE                20	// grnsmk1.spr
#define HWCE_GREY_SMOKE                 21	// grysmk1.spr
#define HWCE_RED_SMOKE                  22	// redsmk1.spr
#define HWCE_SLOW_WHITE_SMOKE           23	// whtsmk1.spr
#define HWCE_REDSPARK                   24	// rspark.spr
#define HWCE_GREENSPARK                 25	// gspark.spr
#define HWCE_TELESMK1                   26	// telesmk1.spr
#define HWCE_TELESMK2                   27	// telesmk2.spr
#define HWCE_ICEHIT                     28	// icehit.spr
#define HWCE_MEDUSA_HIT                 29	// medhit.spr
#define HWCE_MEZZO_REFLECT              30	// mezzoref.spr
#define HWCE_FLOOR_EXPLOSION2           31	// flrexpl2.spr
#define HWCE_XBOW_EXPLOSION             32	// xbowexpl.spr
#define HWCE_NEW_EXPLOSION              33	// gen_expl.spr
#define HWCE_MAGIC_MISSILE_EXPLOSION    34	// mm_expld.spr
#define HWCE_GHOST                      35	// ghost.spr
#define HWCE_BONE_EXPLOSION             36
#define HWCE_REDCLOUD                   37
#define HWCE_TELEPORTERPUFFS            38
#define HWCE_TELEPORTERBODY             39
#define HWCE_BONESHARD                  40
#define HWCE_BONESHRAPNEL               41
#define HWCE_HWMISSILESTAR              42
#define HWCE_HWEIDOLONSTAR              43
#define HWCE_HWSHEEPINATOR              44
#define HWCE_TRIPMINE                   45
#define HWCE_HWBONEBALL                 46
#define HWCE_HWRAVENSTAFF               47
#define HWCE_TRIPMINESTILL              48
#define HWCE_SCARABCHAIN                49
#define HWCE_SM_EXPLOSION2              50
#define HWCE_HWSPLITFLASH               51
#define HWCE_HWXBOWSHOOT                52
#define HWCE_HWRAVENPOWER               53
#define HWCE_HWDRILLA                   54
#define HWCE_DEATHBUBBLES               55
//New for Mission Pack...
#define HWCE_RIPPLE                     56
#define HWCE_BLDRN_EXPL                 57
#define HWCE_ACID_MUZZFL                58
#define HWCE_ACID_HIT                   59
#define HWCE_FIREWALL_SMALL             60
#define HWCE_FIREWALL_MEDIUM            61
#define HWCE_FIREWALL_LARGE             62
#define HWCE_LBALL_EXPL                 63
#define HWCE_ACID_SPLAT                 64
#define HWCE_ACID_EXPL                  65
#define HWCE_FBOOM                      66
#define HWCE_BOMB                       67
#define HWCE_BRN_BOUNCE                 68
#define HWCE_LSHOCK                     69
#define HWCE_FLAMEWALL                  70
#define HWCE_FLAMEWALL2                 71
#define HWCE_FLOOR_EXPLOSION3           72
#define HWCE_ONFIRE                     73
#define HWCE_FLAMESTREAM                74

#define H2XBOW_IMPACT_DEFAULT           0
#define H2XBOW_IMPACT_GREENFLESH        2
#define H2XBOW_IMPACT_REDFLESH      4
#define H2XBOW_IMPACT_WOOD          6
#define H2XBOW_IMPACT_STONE         8
#define H2XBOW_IMPACT_METAL         10
#define H2XBOW_IMPACT_ICE               12
#define H2XBOW_IMPACT_MUMMY         14

#define MAX_EFFECTS_H2 256

struct h2EffectT
{
	int type;
	float expire_time;
	unsigned client_list;

	union
	{
		struct
		{
			vec3_t min_org, max_org, e_size, dir;
			float next_time,wait;
			int color, count;
		} Rain;
		struct
		{
			vec3_t min_org, max_org, dir;
			float next_time;
			int count, flags;
		} Snow;
		struct
		{
			vec3_t pos,angle,movedir;
			vec3_t vforward,vup,vright;
			int color,cnt;
		} Fountain;
		struct
		{
			vec3_t origin;
			float radius;
		} Quake;
		struct
		{
			vec3_t origin;
			vec3_t velocity;
			int entity_index;
			float time_amount,framelength,frame;
			int entity_index2;	//second is only used for telesmk1
		} Smoke;
		struct
		{
			vec3_t origin;
			vec3_t velocity;
			int ent1,owner;
			int state,material,tag;
			float time_amount,height,sound_time;
		} Chain;
		struct
		{
			vec3_t origin;
			int entity_index;
			float time_amount;
			int reverse;	// Forward animation has been run, now go backwards
		} Flash;
		struct
		{
			vec3_t origin;
			float time_amount;
			int stage;
		} RD;	// Rider Death
		struct
		{
			vec3_t origin;
			float time_amount;
			int color;
			float lifetime;
		} GravityWell;
		struct
		{
			int entity_index[16];
			vec3_t origin;
			vec3_t velocity[16];
			float time_amount,framelength;
			float skinnum;
		} Teleporter;
		struct
		{
			vec3_t angle;
			vec3_t origin;
			vec3_t avelocity;
			vec3_t velocity;
			int entity_index;
			float time_amount;
			float speed;// Only for HWCE_HWDRILLA
		} Missile;
		struct
		{
			vec3_t angle;	//as per missile
			vec3_t origin;
			vec3_t avelocity;
			vec3_t velocity;
			int entity_index;
			float time_amount;
			float scale;		//star effects on top of missile
			int scaleDir;
			int ent1, ent2;
		} Star;
		struct
		{
			vec3_t origin[6];
			vec3_t velocity;
			vec3_t angle;
			vec3_t vel[5];
			int ent[5];
			float gonetime[5];	//when a bolt isn't active, check here
								//to see where in the gone process it is?--not sure if that's
								//the best way to handle it
			int state[5];
			int activebolts,turnedbolts;
			int bolts;
			float time_amount;
			int randseed;
		} Xbow;
		struct
		{
			vec3_t offset;
			int owner;
			int count;
			float time_amount;
		} Bubble;
		struct
		{
			int entity_index[16];
			vec3_t velocity[16];
			vec3_t avel[3];
			vec3_t origin;
			unsigned char type;
			vec3_t srcVel;
			unsigned char numChunks;
			float time_amount;
			float aveScale;
		} Chunk;
	};
};

struct h2usercmd_t
{
	vec3_t viewangles;

	// intended velocities
	float forwardmove;
	float sidemove;
	float upmove;
	byte lightlevel;
};

struct hwusercmd_t
{
	byte msec;
	vec3_t angles;
	short forwardmove, sidemove, upmove;
	byte buttons;
	byte impulse;
	byte light_level;
};

#define HWMAX_PACKET_ENTITIES   64	// doesn't count nails

struct hwpacket_entities_t
{
	int num_entities;
	h2entity_state_t entities[HWMAX_PACKET_ENTITIES];
};

#define UPDATE_BACKUP_HW    64	// copies of entity_state_t to keep buffered
// must be power of two
#define UPDATE_MASK_HW  (UPDATE_BACKUP_HW - 1)

//
// temp entity events
//
#define H2TE_SPIKE                  0
#define H2TE_SUPERSPIKE             1
#define H2TE_GUNSHOT                2
#define H2TE_EXPLOSION              3
#define HWTE_TAREXPLOSION           4
#define H2TE_LIGHTNING1             5
#define H2TE_LIGHTNING2             6
#define H2TE_WIZSPIKE               7
#define H2TE_KNIGHTSPIKE            8
#define H2TE_LIGHTNING3             9
#define H2TE_LAVASPLASH             10
#define H2TE_TELEPORT               11
#define HWTE_BLOOD                  12
#define HWTE_LIGHTNINGBLOOD         13

// hexen 2
#define H2TE_STREAM_LIGHTNING_SMALL 24
#define H2TE_STREAM_CHAIN           25
#define H2TE_STREAM_SUNSTAFF1       26
#define H2TE_STREAM_SUNSTAFF2       27
#define H2TE_STREAM_LIGHTNING       28
#define H2TE_STREAM_COLORBEAM       29
#define H2TE_STREAM_ICECHUNKS       30
#define H2TE_STREAM_GAZE            31
#define H2TE_STREAM_FAMINE          32

#define HWTE_BIGGRENADE             33
#define HWTE_CHUNK                  34
#define HWTE_HWBONEPOWER            35
#define HWTE_HWBONEPOWER2           36
#define HWTE_METEORHIT              37
#define HWTE_HWRAVENDIE             38
#define HWTE_HWRAVENEXPLODE         39
#define HWTE_XBOWHIT                40

#define HWTE_CHUNK2                 41
#define HWTE_ICEHIT                 42
#define HWTE_ICESTORM               43
#define HWTE_HWMISSILEFLASH         44
#define HWTE_SUNSTAFF_CHEAP         45
#define HWTE_LIGHTNING_HAMMER       46
#define HWTE_DRILLA_EXPLODE         47
#define HWTE_DRILLA_DRILL           48

#define HWTE_HWTELEPORT             49
#define HWTE_SWORD_EXPLOSION        50

#define HWTE_AXE_BOUNCE             51
#define HWTE_AXE_EXPLODE            52
#define HWTE_TIME_BOMB              53
#define HWTE_FIREBALL               54
#define HWTE_SUNSTAFF_POWER         55
#define HWTE_PURIFY2_EXPLODE        56
#define HWTE_PLAYER_DEATH           57
#define HWTE_PURIFY1_EFFECT         58
#define HWTE_TELEPORT_LINGER        59
#define HWTE_LINE_EXPLOSION         60
#define HWTE_METEOR_CRUSH           61
//MISSION PACK
#define HWTE_STREAM_LIGHTNING_SMALL 62

#define HWTE_ACIDBALL               63
#define HWTE_ACIDBLOB               64
#define HWTE_FIREWALL               65
#define HWTE_FIREWALL_IMPACT        66
#define HWTE_HWBONERIC              67
#define HWTE_POWERFLAME             68
#define HWTE_BLOODRAIN              69
#define HWTE_AXE                    70
#define HWTE_PURIFY2_MISSILE        71
#define HWTE_SWORD_SHOT             72
#define HWTE_ICESHOT                73
#define HWTE_METEOR                 74
#define HWTE_LIGHTNINGBALL          75
#define HWTE_MEGAMETEOR             76
#define HWTE_CUBEBEAM               77
#define HWTE_LIGHTNINGEXPLODE       78
#define HWTE_ACID_BALL_FLY          79
#define HWTE_ACID_BLOB_FLY          80
#define HWTE_CHAINLIGHTNING         81

// if the high bit of the servercmd is set, the low bits are fast update flags:
#define H2U_MOREBITS    (1 << 0)
#define H2U_ORIGIN1     (1 << 1)
#define H2U_ORIGIN2     (1 << 2)
#define H2U_ORIGIN3     (1 << 3)
#define H2U_ANGLE2      (1 << 4)
#define H2U_NOLERP      (1 << 5)		// don't interpolate movement
#define H2U_FRAME       (1 << 6)
#define H2U_SIGNAL      (1 << 7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define H2U_ANGLE1      (1 << 8)
#define H2U_ANGLE3      (1 << 9)
#define H2U_MODEL       (1 << 10)
#define H2U_CLEAR_ENT   (1 << 11)
#define H2U_ENT_OFF     (1 << 13)
#define H2U_LONGENTITY  (1 << 14)
#define H2U_MOREBITS2   (1 << 15)

#define H2U_SKIN        (1 << 16)
#define H2U_EFFECTS     (1 << 17)
#define H2U_SCALE       (1 << 18)
#define H2U_COLORMAP    (1 << 19)

// the first 16 bits of a packetentities update holds 9 bits
// of entity number and 7 bits of flags
#define HWU_ORIGIN1     (1 << 9)
#define HWU_ORIGIN2     (1 << 10)
#define HWU_ORIGIN3     (1 << 11)
#define HWU_ANGLE2      (1 << 12)
#define HWU_FRAME       (1 << 13)
#define HWU_REMOVE      (1 << 14)		// REMOVE this entity, don't add it
#define HWU_MOREBITS    (1 << 15)

// if MOREBITS is set, these additional flags are read in next
#define HWU_ANGLE1      (1 << 0)
#define HWU_ANGLE3      (1 << 1)
#define HWU_SCALE       (1 << 2)
#define HWU_COLORMAP    (1 << 3)
#define HWU_SKIN        (1 << 4)
#define HWU_EFFECTS     (1 << 5)
#define HWU_MODEL16     (1 << 6)
#define HWU_MOREBITS2   (1 << 7)

//if MOREBITS2 is set, then send the 3rd byte

#define HWU_MODEL       (1 << 16)
#define HWU_SOUND       (1 << 17)
#define HWU_DRAWFLAGS   (1 << 18)
#define HWU_ABSLIGHT    (1 << 19)

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
#define h2svc_bad                   0
#define h2svc_nop                   1
#define h2svc_disconnect            2
#define h2svc_updatestat            3	// [byte] [long]
#define h2svc_version               4	// [long] server version, not in HW
#define h2svc_setview               5	// [short] entity number
#define h2svc_sound                 6	// <see code>
#define h2svc_time                  7	// [float] server time
#define h2svc_print                 8	// [string] null terminated string
#define h2svc_stufftext             9	// [string] stuffed into client's console buffer
										// the string should be \n terminated
#define h2svc_setangle              10	// [angle3] set the view angle to this absolute value

#define h2svc_serverinfo            11	// [long] version
										// [string] signon string
										// [string]..[0]model cache
										// [string]...[0]sounds cache
#define hwsvc_serverdata            11	// [long] protocol ...
#define h2svc_lightstyle            12	// [byte] [string]
#define h2svc_updatename            13	// [byte] [string], not in HW
#define h2svc_updatefrags           14	// [byte] [short]
#define h2svc_clientdata            15	// <shortbits + data>, not in HW
#define h2svc_stopsound             16	// <see code>
#define h2svc_updatecolors          17	// [byte] [byte], not in HW
#define h2svc_particle              18	// [vec3] <variable>
#define h2svc_damage                19

#define h2svc_spawnstatic           20

#define h2svc_raineffect            21	// not in HW

#define h2svc_spawnbaseline         22

#define h2svc_temp_entity           23

#define h2svc_setpause              24	// [byte] on / off, not in HW
#define h2svc_signonnum             25	// [byte]  used for the signon sequence, not in HW

#define h2svc_centerprint           26	// [string] to put in center of the screen

#define h2svc_killedmonster         27
#define h2svc_foundsecret           28

#define h2svc_spawnstaticsound      29	// [coord3] [byte] samp [byte] vol [byte] aten

#define h2svc_intermission          30		// [string] music
#define h2svc_finale                31		// [string] music [string] text

#define h2svc_cdtrack               32		// [byte] track [byte] looptrack
#define h2svc_sellscreen            33

#define h2svc_particle2             34	// [vec3] <variable>
#define h2svc_cutscene              35
#define h2svc_midi_name             36		// [string] name
#define h2svc_updateclass           37		// [byte] [byte]
#define h2svc_particle3             38
#define h2svc_particle4             39
#define h2svc_set_view_flags        40
#define h2svc_clear_view_flags      41
#define h2svc_start_effect          42
#define h2svc_end_effect            43
#define h2svc_plaque                44
#define h2svc_particle_explosion    45
#define h2svc_set_view_tint         46
#define h2svc_reference             47
#define h2svc_clear_edicts          48
#define h2svc_update_inv            49

#define h2svc_setangle_interpolate  50
#define h2svc_update_kingofhill     51
#define h2svc_toggle_statbar        52
#define h2svc_sound_update_pos      53	//[short] ent+channel [coord3] pos

#define hwsvc_smallkick             34		// set client punchangle to 2
#define hwsvc_bigkick               35		// set client punchangle to 4

#define hwsvc_updateping            36		// [byte] [short]
#define hwsvc_updateentertime       37		// [byte] [float]

#define hwsvc_updatestatlong        38		// [byte] [long]

#define hwsvc_muzzleflash           39		// [short] entity

#define hwsvc_updateuserinfo        40		// [byte] slot [long] uid
// [string] userinfo

#define hwsvc_download              41		// [short] size [size bytes]
#define hwsvc_playerinfo            42		// variable
#define hwsvc_nails                 43		// [byte] num [48 bits] xyzpy 12 12 12 4 8
#define hwsvc_chokecount            44		// [byte] packets choked
#define hwsvc_modellist             45		// [strings]
#define hwsvc_soundlist             46		// [strings]
#define hwsvc_packetentities        47		// [...]
#define hwsvc_deltapacketentities   48		// [...]
#define hwsvc_maxspeed              49		// maxspeed change, for prediction
#define hwsvc_entgravity            50		// gravity change, for prediction

// Hexen2 specifics
#define hwsvc_plaque                51
#define hwsvc_particle_explosion    52
#define hwsvc_set_view_tint         53
#define hwsvc_start_effect          54
#define hwsvc_end_effect            55
#define hwsvc_set_view_flags        56
#define hwsvc_clear_view_flags      57
#define hwsvc_update_inv            58
#define hwsvc_particle2             59
#define hwsvc_particle3             60
#define hwsvc_particle4             61
#define hwsvc_turn_effect           62
#define hwsvc_update_effect         63
#define hwsvc_multieffect           64
#define hwsvc_midi_name             65
#define hwsvc_raineffect            66

#define hwsvc_packmissile           67	//[byte] num [40 bits] xyz type 12 12 12 4

#define hwsvc_indexed_print         68	//same as h2svc_print, but sends an index in strings.txt instead of string

#define hwsvc_targetupdate          69	// [byte] angle [byte] pitch [byte] dist/4 - Hey, I got number 69!  Woo hoo!

#define hwsvc_name_print            70	// print player's name
#define hwsvc_sound_update_pos      71	//[short] ent+channel [coord3] pos
#define hwsvc_update_piv            72	// update players in view
#define hwsvc_player_sound          73	// sends weapon sound for invisible player
#define hwsvc_updatepclass          74	// [byte] [byte]
#define hwsvc_updatedminfo          75	// [byte] [short] [byte]
#define hwsvc_updatesiegeinfo       76	// [byte] [byte]
#define hwsvc_updatesiegeteam       77	// [byte] [byte]
#define hwsvc_updatesiegelosses     78	// [byte] [byte]

#define hwsvc_haskey                79	// [byte] [byte]
#define hwsvc_nonehaskey            80	// [byte] [byte]
#define hwsvc_isdoc                 81	// [byte] [byte]
#define hwsvc_nodoc                 82	// [byte] [byte]
#define hwsvc_playerskipped         83	// [byte]

//
// client to server
//
#define h2clc_bad           0
#define h2clc_nop           1
#define h2clc_disconnect    2	// Not in HW
#define h2clc_move          3	// [usercmd_t]
#define h2clc_stringcmd     4	// [string] message

#define h2clc_inv_select    5
#define h2clc_frame         6

#define hwclc_delta         5	// [byte] sequence number, requests delta compression of message
#define hwclc_tmove         6	// teleport request, spectator only
#define hwclc_inv_select    7
#define hwclc_get_effect    8	//[byte] effect id

#define MAX_MODELS_H2       512			// Sent over the net as a word
#define MAX_SOUNDS_H2       512			// Sent over the net as a byte
#define MAX_SOUNDS_HW       256			// so they cannot be blindly increased

struct h2client_entvars_t
{
	float movetype;
	float health;
	float max_health;
	float bluemana;
	float greenmana;
	float max_mana;
	float armor_amulet;
	float armor_bracer;
	float armor_breastplate;
	float armor_helmet;
	float level;
	float intelligence;
	float wisdom;
	float dexterity;
	float strength;
	float experience;
	float ring_flight;
	float ring_water;
	float ring_turning;
	float ring_regeneration;
	float flags;
	float rings_active;
	float rings_low;
	float artifact_active;
	float artifact_low;
	float hasted;
	float inventory;
	float cnt_torch;
	float cnt_h_boost;
	float cnt_sh_boost;
	float cnt_mana_boost;
	float cnt_teleport;
	float cnt_tome;
	float cnt_summon;
	float cnt_invisibility;
	float cnt_glyph;
	float cnt_haste;
	float cnt_blast;
	float cnt_polymorph;
	float cnt_flight;
	float cnt_cubeofforce;
	float cnt_invincibility;
	int cameramode;

	//	Hexen 2 only
	float weapon;
	float haste_time;
	float tome_time;

	//	Server side only
	int puzzle_inv1;
	int puzzle_inv2;
	int puzzle_inv3;
	int puzzle_inv4;
	int puzzle_inv5;
	int puzzle_inv6;
	int puzzle_inv7;
	int puzzle_inv8;

	//	Hexen 2 server side only
	vec3_t velocity;
	vec3_t punchangle;
	int weaponmodel;
	float weaponframe;
	vec3_t view_ofs;
	float idealpitch;
	float idealroll;
	float armorvalue;

	//	HexenWorld client side only
	float teleport_time;

	//	HexenWorld server side only
	//FIXME Only used in stats command, may be better to get actual
	// value from entity.
	float flags2;
};

#define H2ARTFLAG_HASTE                 1
#define H2ARTFLAG_INVINCIBILITY         2
#define H2ARTFLAG_TOMEOFPOWER           4
#define H2ARTFLAG_INVISIBILITY          8
#define H2ARTFLAG_FROZEN                128
#define H2ARTFLAG_STONED                256
#define H2ARTFLAG_DIVINE_INTERVENTION   512

#define HWARTFLAG_FROZEN                16
#define HWARTFLAG_STONED                32
#define HWARTFLAG_DIVINE_INTERVENTION   64
#define HWARTFLAG_BOOTS                 128

// playerinfo flags from server
// playerinfo allways sends: playernum, flags, origin[] and framenumber

#define HWPF_MSEC         (1 << 0)
#define HWPF_COMMAND      (1 << 1)
#define HWPF_VELOCITY1    (1 << 2)
#define HWPF_VELOCITY2    (1 << 3)
#define HWPF_VELOCITY3    (1 << 4)
#define HWPF_MODEL        (1 << 5)
#define HWPF_SKINNUM      (1 << 6)
#define HWPF_EFFECTS      (1 << 7)
#define HWPF_WEAPONFRAME  (1 << 8)		// only sent for view player
#define HWPF_DEAD         (1 << 9)		// don't block movement any more
#define HWPF_CROUCH       (1 << 10)		// offset the view height differently
#define HWPF_EFFECTS2     (1 << 11)		// player has high byte of effects set...
#define HWPF_DRAWFLAGS    (1 << 12)
#define HWPF_SCALE        (1 << 13)
#define HWPF_ABSLIGHT     (1 << 14)
#define HWPF_SOUND        (1 << 15)		//play a sound in the weapon channel

#define H2MAX_FRAMES 5

//Siege teams
#define HWST_DEFENDER                 1
#define HWST_ATTACKER                 2

// if the high bit of the client to server byte is set, the low bits are
// client move cmd bits
// ms and angle2 are allways sent, the others are optional
#define HWCM_ANGLE1   (1 << 0)
#define HWCM_ANGLE3   (1 << 1)
#define HWCM_FORWARD  (1 << 2)
#define HWCM_SIDE     (1 << 3)
#define HWCM_UP       (1 << 4)
#define HWCM_BUTTONS  (1 << 5)
#define HWCM_IMPULSE  (1 << 6)
#define HWCM_MSEC     (1 << 7)

// entity effects
#define HWEF_ONFIRE               0x00000001
#define H2EF_MUZZLEFLASH          0x00000002
#define H2EF_BRIGHTLIGHT          0x00000004
#define H2EF_DIMLIGHT             0x00000008
#define H2EF_DARKLIGHT            0x00000010
#define H2EF_DARKFIELD            0x00000020
#define H2EF_LIGHT                0x00000040
#define H2EF_NODRAW               0x00000080
#define HWEF_BRIGHTFIELD          0x00000400
#define HWEF_POWERFLAMEBURN       0x00000800
#define HWEF_UPDATESOUND          0x00002000
#define HWEF_POISON_GAS           0x00200000
#define HWEF_ACIDBLOB             0x00400000
#define HWEF_ICESTORM_EFFECT      0x02000000
#define HWEF_HAMMER_EFFECTS       0x10000000
#define HWEF_BEETLE_EFFECTS       0x20000000

// Bits to help send server info about the client's edict variables
#define SC1_HEALTH              (1 << 0)		// changes stat bar
#define SC1_LEVEL               (1 << 1)		// changes stat bar
#define SC1_INTELLIGENCE        (1 << 2)		// changes stat bar
#define SC1_WISDOM              (1 << 3)		// changes stat bar
#define SC1_STRENGTH            (1 << 4)		// changes stat bar
#define SC1_DEXTERITY           (1 << 5)		// changes stat bar
#define SC1_WEAPON              (1 << 6)		// changes stat bar
#define SC1_TELEPORT_TIME       (1 << 6)		// can't airmove for 2 seconds
#define SC1_BLUEMANA            (1 << 7)		// changes stat bar
#define SC1_GREENMANA           (1 << 8)		// changes stat bar
#define SC1_EXPERIENCE          (1 << 9)		// changes stat bar
#define SC1_CNT_TORCH           (1 << 10)		// changes stat bar
#define SC1_CNT_H_BOOST         (1 << 11)		// changes stat bar
#define SC1_CNT_SH_BOOST        (1 << 12)		// changes stat bar
#define SC1_CNT_MANA_BOOST      (1 << 13)		// changes stat bar
#define SC1_CNT_TELEPORT        (1 << 14)		// changes stat bar
#define SC1_CNT_TOME            (1 << 15)		// changes stat bar
#define SC1_CNT_SUMMON          (1 << 16)		// changes stat bar
#define SC1_CNT_INVISIBILITY    (1 << 17)		// changes stat bar
#define SC1_CNT_GLYPH           (1 << 18)		// changes stat bar
#define SC1_CNT_HASTE           (1 << 19)		// changes stat bar
#define SC1_CNT_BLAST           (1 << 20)		// changes stat bar
#define SC1_CNT_POLYMORPH       (1 << 21)		// changes stat bar
#define SC1_CNT_FLIGHT          (1 << 22)		// changes stat bar
#define SC1_CNT_CUBEOFFORCE     (1 << 23)		// changes stat bar
#define SC1_CNT_INVINCIBILITY   (1 << 24)		// changes stat bar
#define SC1_ARTIFACT_ACTIVE     (1 << 25)
#define SC1_ARTIFACT_LOW        (1 << 26)
#define SC1_MOVETYPE            (1 << 27)
#define SC1_CAMERAMODE          (1 << 28)
#define SC1_HASTED              (1 << 29)
#define SC1_INVENTORY           (1 << 30)
#define SC1_RINGS_ACTIVE        (1 << 31)

#define SC2_RINGS_LOW           (1 << 0)
#define SC2_AMULET              (1 << 1)
#define SC2_BRACER              (1 << 2)
#define SC2_BREASTPLATE         (1 << 3)
#define SC2_HELMET              (1 << 4)
#define SC2_FLIGHT_T            (1 << 5)
#define SC2_WATER_T             (1 << 6)
#define SC2_TURNING_T           (1 << 7)
#define SC2_REGEN_T             (1 << 8)
#define SC2_HASTE_T             (1 << 9)
#define SC2_TOME_T              (1 << 10)
#define SC2_PUZZLE1             (1 << 11)
#define SC2_PUZZLE2             (1 << 12)
#define SC2_PUZZLE3             (1 << 13)
#define SC2_PUZZLE4             (1 << 14)
#define SC2_PUZZLE5             (1 << 15)
#define SC2_PUZZLE6             (1 << 16)
#define SC2_PUZZLE7             (1 << 17)
#define SC2_PUZZLE8             (1 << 18)
#define SC2_MAXHEALTH           (1 << 19)
#define SC2_MAXMANA             (1 << 20)
#define SC2_FLAGS               (1 << 21)
#define SC2_OBJ                 (1 << 22)
#define SC2_OBJ2                (1 << 23)

// This is to mask out those items in the inventory (for inventory changes)
#define SC1_INV 0x01fffc00
#define SC2_INV 0x00000000

#define H2PROTOCOL_VERSION      19
#define HWOLD_PROTOCOL_VERSION  24
#define HWPROTOCOL_VERSION      25

//
// item flags
//
#define H2IT_WEAPON2            1

#define HWPORT_CLIENT 26901
#define HWPORT_MASTER 26900
#define HWPORT_SERVER 26950
