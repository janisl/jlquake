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

#define H2MAX_CLIENTS			16
#define HWMAX_CLIENTS			32
#define H2BIGGEST_MAX_CLIENTS	32	//	For common arrays

#define MAX_EDICTS_H2		768			// FIXME: ouch! ouch! ouch!

// Timing macros
#define HX_FRAME_TIME		0.05

#define HWMAX_INFO_STRING	196

// Player Classes
#define NUM_CLASSES_H2			4
#define NUM_CLASSES_H2MP		5
#define MAX_PLAYER_CLASS		6

#define CLASS_PALADIN			1
#define CLASS_CLERIC 			2
#define CLASS_NECROMANCER		3
#define CLASS_THEIF   			4
#define CLASS_DEMON				5
#define CLASS_DWARF				6

// edict->drawflags
#define H2MLS_MASKIN			7	// MLS: Model Light Style
#define H2MLS_MASKOUT			248
#define H2MLS_NONE				0
#define H2MLS_FULLBRIGHT		1
#define H2MLS_POWERMODE			2
#define H2MLS_TORCH				3
#define H2MLS_TOTALDARK			4
#define H2MLS_ABSLIGHT			7
#define H2SCALE_TYPE_MASKIN		24
#define H2SCALE_TYPE_MASKOUT	231
#define H2SCALE_TYPE_UNIFORM	0	// Scale X, Y, and Z
#define H2SCALE_TYPE_XYONLY		8	// Scale X and Y
#define H2SCALE_TYPE_ZONLY		16	// Scale Z
#define H2SCALE_ORIGIN_MASKIN	96
#define H2SCALE_ORIGIN_MASKOUT	159
#define H2SCALE_ORIGIN_CENTER	0	// Scaling origin at object center
#define H2SCALE_ORIGIN_BOTTOM	32	// Scaling origin at object bottom
#define H2SCALE_ORIGIN_TOP		64	// Scaling origin at object top
#define H2DRF_TRANSLUCENT		128

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
#define H2THINGTYPE_GREYSTONE	1
#define H2THINGTYPE_WOOD		2
#define H2THINGTYPE_METAL		3
#define H2THINGTYPE_FLESH		4
#define H2THINGTYPE_FIRE		5
#define H2THINGTYPE_CLAY		6
#define H2THINGTYPE_LEAVES		7
#define H2THINGTYPE_HAY			8
#define H2THINGTYPE_BROWNSTONE	9
#define H2THINGTYPE_CLOTH		10
#define H2THINGTYPE_WOOD_LEAF	11
#define H2THINGTYPE_WOOD_METAL	12
#define H2THINGTYPE_WOOD_STONE	13
#define H2THINGTYPE_METAL_STONE	14
#define H2THINGTYPE_METAL_CLOTH	15
#define H2THINGTYPE_WEBS		16
#define H2THINGTYPE_GLASS		17
#define H2THINGTYPE_ICE			18
#define H2THINGTYPE_CLEARGLASS	19
#define H2THINGTYPE_REDGLASS	20
#define H2THINGTYPE_ACID	 	21
#define H2THINGTYPE_METEOR	 	22
#define H2THINGTYPE_GREENFLESH	23
#define H2THINGTYPE_BONE		24
#define H2THINGTYPE_DIRT		25

#define H2CE_NONE						0
#define H2CE_RAIN						1
#define H2CE_FOUNTAIN					2
#define H2CE_QUAKE						3
#define H2CE_WHITE_SMOKE				4   // whtsmk1.spr
#define H2CE_BLUESPARK					5	// bspark.spr
#define H2CE_YELLOWSPARK				6	// spark.spr
#define H2CE_SM_CIRCLE_EXP				7	// fcircle.spr
#define H2CE_BG_CIRCLE_EXP				8	// fcircle.spr
#define H2CE_SM_WHITE_FLASH				9	// sm_white.spr
#define H2CE_WHITE_FLASH				10	// gryspt.spr
#define H2CE_YELLOWRED_FLASH			11  // yr_flash.spr
#define H2CE_BLUE_FLASH					12  // bluflash.spr
#define H2CE_SM_BLUE_FLASH				13  // bluflash.spr
#define H2CE_RED_FLASH					14  // redspt.spr
#define H2CE_SM_EXPLOSION				15  // sm_expld.spr
#define H2CE_LG_EXPLOSION				16  // bg_expld.spr
#define H2CE_FLOOR_EXPLOSION			17  // fl_expld.spr
#define H2CE_RIDER_DEATH				18
#define H2CE_BLUE_EXPLOSION				19  // xpspblue.spr
#define H2CE_GREEN_SMOKE				20  // grnsmk1.spr
#define H2CE_GREY_SMOKE					21  // grysmk1.spr
#define H2CE_RED_SMOKE					22  // redsmk1.spr
#define H2CE_SLOW_WHITE_SMOKE			23  // whtsmk1.spr
#define H2CE_REDSPARK					24  // rspark.spr
#define H2CE_GREENSPARK					25  // gspark.spr
#define H2CE_TELESMK1					26  // telesmk1.spr
#define H2CE_TELESMK2					27  // telesmk2.spr
#define H2CE_ICEHIT						28  // icehit.spr
#define H2CE_MEDUSA_HIT					29  // medhit.spr
#define H2CE_MEZZO_REFLECT				30  // mezzoref.spr
#define H2CE_FLOOR_EXPLOSION2			31  // flrexpl2.spr
#define H2CE_XBOW_EXPLOSION				32  // xbowexpl.spr
#define H2CE_NEW_EXPLOSION				33  // gen_expl.spr
#define H2CE_MAGIC_MISSILE_EXPLOSION	34  // mm_expld.spr
#define H2CE_GHOST						35  // ghost.spr
#define H2CE_BONE_EXPLOSION				36
#define H2CE_REDCLOUD					37
#define H2CE_TELEPORTERPUFFS			38
#define H2CE_TELEPORTERBODY				39
#define H2CE_BONESHARD					40
#define H2CE_BONESHRAPNEL				41
#define H2CE_FLAMESTREAM				42	//Flamethrower
#define H2CE_SNOW						43
#define H2CE_GRAVITYWELL				44
#define H2CE_BLDRN_EXPL					45
#define H2CE_ACID_MUZZFL				46
#define H2CE_ACID_HIT					47
#define H2CE_FIREWALL_SMALL				48
#define H2CE_FIREWALL_MEDIUM			49
#define H2CE_FIREWALL_LARGE				50
#define H2CE_LBALL_EXPL					51
#define H2CE_ACID_SPLAT					52
#define H2CE_ACID_EXPL					53
#define H2CE_FBOOM						54
#define H2CE_CHUNK						55
#define H2CE_BOMB						56
#define H2CE_BRN_BOUNCE					57
#define H2CE_LSHOCK						58
#define H2CE_FLAMEWALL					59
#define H2CE_FLAMEWALL2					60
#define H2CE_FLOOR_EXPLOSION3			61  
#define H2CE_ONFIRE						62

#define HWCE_NONE						0
#define HWCE_RAIN						1
#define HWCE_FOUNTAIN					2
#define HWCE_QUAKE						3
#define HWCE_WHITE_SMOKE				4   // whtsmk1.spr
#define HWCE_BLUESPARK					5	// bspark.spr
#define HWCE_YELLOWSPARK				6	// spark.spr
#define HWCE_SM_CIRCLE_EXP				7	// fcircle.spr
#define HWCE_BG_CIRCLE_EXP				8	// fcircle.spr
#define HWCE_SM_WHITE_FLASH				9	// sm_white.spr
#define HWCE_WHITE_FLASH				10	// gryspt.spr
#define HWCE_YELLOWRED_FLASH			11  // yr_flash.spr
#define HWCE_BLUE_FLASH					12  // bluflash.spr
#define HWCE_SM_BLUE_FLASH				13  // bluflash.spr
#define HWCE_RED_FLASH					14  // redspt.spr
#define HWCE_SM_EXPLOSION				15  // sm_expld.spr
#define HWCE_BG_EXPLOSION				16  // bg_expld.spr
#define HWCE_FLOOR_EXPLOSION			17  // fl_expld.spr
#define HWCE_RIDER_DEATH				18
#define HWCE_BLUE_EXPLOSION				19  // xpspblue.spr
#define HWCE_GREEN_SMOKE				20  // grnsmk1.spr
#define HWCE_GREY_SMOKE					21  // grysmk1.spr
#define HWCE_RED_SMOKE					22  // redsmk1.spr
#define HWCE_SLOW_WHITE_SMOKE			23  // whtsmk1.spr
#define HWCE_REDSPARK					24  // rspark.spr
#define HWCE_GREENSPARK					25  // gspark.spr
#define HWCE_TELESMK1					26  // telesmk1.spr
#define HWCE_TELESMK2					27  // telesmk2.spr
#define HWCE_ICEHIT						28  // icehit.spr
#define HWCE_MEDUSA_HIT					29  // medhit.spr
#define HWCE_MEZZO_REFLECT				30  // mezzoref.spr
#define HWCE_FLOOR_EXPLOSION2			31  // flrexpl2.spr
#define HWCE_XBOW_EXPLOSION				32  // xbowexpl.spr
#define HWCE_NEW_EXPLOSION				33  // gen_expl.spr
#define HWCE_MAGIC_MISSILE_EXPLOSION	34  // mm_expld.spr
#define HWCE_GHOST						35  // ghost.spr
#define HWCE_BONE_EXPLOSION				36
#define HWCE_REDCLOUD					37
#define HWCE_TELEPORTERPUFFS			38
#define HWCE_TELEPORTERBODY				39
#define HWCE_BONESHARD					40
#define HWCE_BONESHRAPNEL				41
#define HWCE_HWMISSILESTAR				42
#define HWCE_HWEIDOLONSTAR				43
#define HWCE_HWSHEEPINATOR				44
#define HWCE_TRIPMINE					45
#define HWCE_HWBONEBALL					46
#define HWCE_HWRAVENSTAFF				47
#define HWCE_TRIPMINESTILL				48
#define HWCE_SCARABCHAIN				49
#define HWCE_SM_EXPLOSION2				50
#define HWCE_HWSPLITFLASH				51
#define HWCE_HWXBOWSHOOT				52
#define HWCE_HWRAVENPOWER				53
#define HWCE_HWDRILLA					54
#define HWCE_DEATHBUBBLES				55
//New for Mission Pack...
#define HWCE_RIPPLE						56
#define HWCE_BLDRN_EXPL					57
#define HWCE_ACID_MUZZFL				58
#define HWCE_ACID_HIT					59
#define HWCE_FIREWALL_SMALL				60
#define HWCE_FIREWALL_MEDIUM			61
#define HWCE_FIREWALL_LARGE				62
#define HWCE_LBALL_EXPL					63
#define HWCE_ACID_SPLAT					64
#define HWCE_ACID_EXPL					65
#define HWCE_FBOOM						66
#define HWCE_BOMB						67
#define HWCE_BRN_BOUNCE					68
#define HWCE_LSHOCK						69
#define HWCE_FLAMEWALL					70
#define HWCE_FLAMEWALL2					71
#define HWCE_FLOOR_EXPLOSION3			72
#define HWCE_ONFIRE						73
#define HWCE_FLAMESTREAM				74

#define H2XBOW_IMPACT_DEFAULT			0
#define H2XBOW_IMPACT_GREENFLESH		2
#define H2XBOW_IMPACT_REDFLESH		4
#define H2XBOW_IMPACT_WOOD			6
#define H2XBOW_IMPACT_STONE			8
#define H2XBOW_IMPACT_METAL			10
#define H2XBOW_IMPACT_ICE				12
#define H2XBOW_IMPACT_MUMMY			14

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
			int count , flags;
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
			int entity_index2;  //second is only used for telesmk1
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
			int reverse;  // Forward animation has been run, now go backwards
		} Flash;
		struct
		{
			vec3_t origin;
			float time_amount;
			int stage;
		} RD; // Rider Death
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
			float gonetime[5];//when a bolt isn't active, check here
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
		}Chunk;
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

#define HWMAX_PACKET_ENTITIES	64	// doesn't count nails

struct hwpacket_entities_t
{
	int num_entities;
	h2entity_state_t entities[HWMAX_PACKET_ENTITIES];
};

#define UPDATE_BACKUP_HW	64	// copies of entity_state_t to keep buffered
							// must be power of two
#define UPDATE_MASK_HW	(UPDATE_BACKUP_HW - 1)

//
// temp entity events
//
#define H2TE_SPIKE					0
#define H2TE_SUPERSPIKE				1
#define H2TE_GUNSHOT				2
#define H2TE_EXPLOSION				3
#define HWTE_TAREXPLOSION			4
#define H2TE_LIGHTNING1				5
#define H2TE_LIGHTNING2				6
#define H2TE_WIZSPIKE				7
#define H2TE_KNIGHTSPIKE			8
#define H2TE_LIGHTNING3				9
#define H2TE_LAVASPLASH				10
#define H2TE_TELEPORT				11
#define HWTE_BLOOD					12
#define HWTE_LIGHTNINGBLOOD			13

// hexen 2
#define H2TE_STREAM_LIGHTNING_SMALL	24
#define H2TE_STREAM_CHAIN			25
#define H2TE_STREAM_SUNSTAFF1		26
#define H2TE_STREAM_SUNSTAFF2		27
#define H2TE_STREAM_LIGHTNING		28
#define H2TE_STREAM_COLORBEAM		29
#define H2TE_STREAM_ICECHUNKS		30
#define H2TE_STREAM_GAZE			31
#define H2TE_STREAM_FAMINE			32

#define HWTE_BIGGRENADE				33
#define HWTE_CHUNK					34
#define HWTE_HWBONEPOWER			35
#define HWTE_HWBONEPOWER2			36
#define HWTE_METEORHIT				37
#define HWTE_HWRAVENDIE				38
#define HWTE_HWRAVENEXPLODE			39
#define HWTE_XBOWHIT				40

#define	HWTE_CHUNK2					41
#define HWTE_ICEHIT					42
#define HWTE_ICESTORM				43
#define HWTE_HWMISSILEFLASH			44
#define HWTE_SUNSTAFF_CHEAP			45
#define HWTE_LIGHTNING_HAMMER		46
#define HWTE_DRILLA_EXPLODE			47
#define HWTE_DRILLA_DRILL			48

#define HWTE_HWTELEPORT				49
#define HWTE_SWORD_EXPLOSION		50

#define HWTE_AXE_BOUNCE				51
#define HWTE_AXE_EXPLODE			52
#define HWTE_TIME_BOMB				53
#define HWTE_FIREBALL				54
#define HWTE_SUNSTAFF_POWER			55
#define HWTE_PURIFY2_EXPLODE		56
#define HWTE_PLAYER_DEATH			57
#define HWTE_PURIFY1_EFFECT			58
#define HWTE_TELEPORT_LINGER		59
#define HWTE_LINE_EXPLOSION			60
#define HWTE_METEOR_CRUSH			61
//MISSION PACK
#define HWTE_STREAM_LIGHTNING_SMALL	62

#define HWTE_ACIDBALL				63
#define HWTE_ACIDBLOB				64
#define HWTE_FIREWALL				65
#define HWTE_FIREWALL_IMPACT		66
#define HWTE_HWBONERIC				67
#define HWTE_POWERFLAME				68
#define HWTE_BLOODRAIN				69
#define HWTE_AXE					70
#define HWTE_PURIFY2_MISSILE		71
#define HWTE_SWORD_SHOT				72
#define HWTE_ICESHOT				73
#define HWTE_METEOR					74
#define HWTE_LIGHTNINGBALL			75
#define HWTE_MEGAMETEOR				76
#define HWTE_CUBEBEAM				77
#define HWTE_LIGHTNINGEXPLODE		78
#define HWTE_ACID_BALL_FLY			79
#define HWTE_ACID_BLOB_FLY			80
#define HWTE_CHAINLIGHTNING			81

// if the high bit of the servercmd is set, the low bits are fast update flags:
#define H2U_MOREBITS	(1<<0)
#define H2U_ORIGIN1		(1<<1)
#define H2U_ORIGIN2		(1<<2)
#define H2U_ORIGIN3		(1<<3)
#define H2U_ANGLE2		(1<<4)
#define H2U_NOLERP		(1<<5)		// don't interpolate movement
#define H2U_FRAME		(1<<6)
#define H2U_SIGNAL		(1<<7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define H2U_ANGLE1		(1<<8)
#define H2U_ANGLE3		(1<<9)
#define H2U_MODEL		(1<<10)
#define H2U_CLEAR_ENT	(1<<11)
#define H2U_ENT_OFF		(1<<13)
#define H2U_LONGENTITY	(1<<14)
#define H2U_MOREBITS2	(1<<15)

#define H2U_SKIN		(1<<16)
#define H2U_EFFECTS		(1<<17)
#define H2U_SCALE		(1<<18)
#define H2U_COLORMAP	(1<<19)
