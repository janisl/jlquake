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

#define MAX_EDICTS_H2		768			// FIXME: ouch! ouch! ouch!

// Player Classes
#define NUM_CLASSES_H2				4
#define NUM_CLASSES_H2MP			5
#define MAX_PLAYER_CLASS			6

#define CLASS_PALADIN				1
#define CLASS_CLERIC 				2
#define CLASS_NECROMANCER			3
#define CLASS_THEIF   				4
#define CLASS_DEMON					5
#define CLASS_DWARF					6

// Timing macros
#define HX_FRAME_TIME		0.05

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
#define THINGTYPEH2_GREYSTONE	1
#define THINGTYPEH2_WOOD		2
#define THINGTYPEH2_METAL		3
#define THINGTYPEH2_FLESH		4
#define THINGTYPEH2_FIRE		5
#define THINGTYPEH2_CLAY		6
#define THINGTYPEH2_LEAVES		7
#define THINGTYPEH2_HAY			8
#define THINGTYPEH2_BROWNSTONE	9
#define THINGTYPEH2_CLOTH		10
#define THINGTYPEH2_WOOD_LEAF	11
#define THINGTYPEH2_WOOD_METAL	12
#define THINGTYPEH2_WOOD_STONE	13
#define THINGTYPEH2_METAL_STONE	14
#define THINGTYPEH2_METAL_CLOTH	15
#define THINGTYPEH2_WEBS		16
#define THINGTYPEH2_GLASS		17
#define THINGTYPEH2_ICE			18
#define THINGTYPEH2_CLEARGLASS	19
#define THINGTYPEH2_REDGLASS	20
#define THINGTYPEH2_ACID	 	21
#define THINGTYPEH2_METEOR	 	22
#define THINGTYPEH2_GREENFLESH	23
#define THINGTYPEH2_BONE		24
#define THINGTYPEH2_DIRT		25

#define CEH2_NONE						0
#define CEH2_RAIN						1
#define CEH2_FOUNTAIN					2
#define CEH2_QUAKE						3
#define CEH2_WHITE_SMOKE				4   // whtsmk1.spr
#define CEH2_BLUESPARK					5	// bspark.spr
#define CEH2_YELLOWSPARK				6	// spark.spr
#define CEH2_SM_CIRCLE_EXP				7	// fcircle.spr
#define CEH2_BG_CIRCLE_EXP				8	// fcircle.spr
#define CEH2_SM_WHITE_FLASH				9	// sm_white.spr
#define CEH2_WHITE_FLASH				10	// gryspt.spr
#define CEH2_YELLOWRED_FLASH			11  // yr_flash.spr
#define CEH2_BLUE_FLASH					12  // bluflash.spr
#define CEH2_SM_BLUE_FLASH				13  // bluflash.spr
#define CEH2_RED_FLASH					14  // redspt.spr
#define CEH2_SM_EXPLOSION				15  // sm_expld.spr
#define CEH2_LG_EXPLOSION				16  // bg_expld.spr
#define CEH2_FLOOR_EXPLOSION			17  // fl_expld.spr
#define CEH2_RIDER_DEATH				18
#define CEH2_BLUE_EXPLOSION				19  // xpspblue.spr
#define CEH2_GREEN_SMOKE				20  // grnsmk1.spr
#define CEH2_GREY_SMOKE					21  // grysmk1.spr
#define CEH2_RED_SMOKE					22  // redsmk1.spr
#define CEH2_SLOW_WHITE_SMOKE			23  // whtsmk1.spr
#define CEH2_REDSPARK					24  // rspark.spr
#define CEH2_GREENSPARK					25  // gspark.spr
#define CEH2_TELESMK1					26  // telesmk1.spr
#define CEH2_TELESMK2					27  // telesmk2.spr
#define CEH2_ICEHIT						28  // icehit.spr
#define CEH2_MEDUSA_HIT					29  // medhit.spr
#define CEH2_MEZZO_REFLECT				30  // mezzoref.spr
#define CEH2_FLOOR_EXPLOSION2			31  // flrexpl2.spr
#define CEH2_XBOW_EXPLOSION				32  // xbowexpl.spr
#define CEH2_NEW_EXPLOSION				33  // gen_expl.spr
#define CEH2_MAGIC_MISSILE_EXPLOSION	34  // mm_expld.spr
#define CEH2_GHOST						35  // ghost.spr
#define CEH2_BONE_EXPLOSION				36
#define CEH2_REDCLOUD					37
#define CEH2_TELEPORTERPUFFS			38
#define CEH2_TELEPORTERBODY				39
#define CEH2_BONESHARD					40
#define CEH2_BONESHRAPNEL				41
#define CEH2_FLAMESTREAM				42	//Flamethrower
#define CEH2_SNOW						43
#define CEH2_GRAVITYWELL				44
#define CEH2_BLDRN_EXPL					45
#define CEH2_ACID_MUZZFL				46
#define CEH2_ACID_HIT					47
#define CEH2_FIREWALL_SMALL				48
#define CEH2_FIREWALL_MEDIUM			49
#define CEH2_FIREWALL_LARGE				50
#define CEH2_LBALL_EXPL					51
#define CEH2_ACID_SPLAT					52
#define CEH2_ACID_EXPL					53
#define CEH2_FBOOM						54
#define CEH2_CHUNK						55
#define CEH2_BOMB						56
#define CEH2_BRN_BOUNCE					57
#define CEH2_LSHOCK						58
#define CEH2_FLAMEWALL					59
#define CEH2_FLAMEWALL2					60
#define CEH2_FLOOR_EXPLOSION3			61  
#define CEH2_ONFIRE						62

#define CEHW_NONE						0
#define CEHW_RAIN						1
#define CEHW_FOUNTAIN					2
#define CEHW_QUAKE						3
#define CEHW_WHITE_SMOKE				4   // whtsmk1.spr
#define CEHW_BLUESPARK					5	// bspark.spr
#define CEHW_YELLOWSPARK				6	// spark.spr
#define CEHW_SM_CIRCLE_EXP				7	// fcircle.spr
#define CEHW_BG_CIRCLE_EXP				8	// fcircle.spr
#define CEHW_SM_WHITE_FLASH				9	// sm_white.spr
#define CEHW_WHITE_FLASH				10	// gryspt.spr
#define CEHW_YELLOWRED_FLASH			11  // yr_flash.spr
#define CEHW_BLUE_FLASH					12  // bluflash.spr
#define CEHW_SM_BLUE_FLASH				13  // bluflash.spr
#define CEHW_RED_FLASH					14  // redspt.spr
#define CEHW_SM_EXPLOSION				15  // sm_expld.spr
#define CEHW_BG_EXPLOSION				16  // bg_expld.spr
#define CEHW_FLOOR_EXPLOSION			17  // fl_expld.spr
#define CEHW_RIDER_DEATH				18
#define CEHW_BLUE_EXPLOSION				19  // xpspblue.spr
#define CEHW_GREEN_SMOKE				20  // grnsmk1.spr
#define CEHW_GREY_SMOKE					21  // grysmk1.spr
#define CEHW_RED_SMOKE					22  // redsmk1.spr
#define CEHW_SLOW_WHITE_SMOKE			23  // whtsmk1.spr
#define CEHW_REDSPARK					24  // rspark.spr
#define CEHW_GREENSPARK					25  // gspark.spr
#define CEHW_TELESMK1					26  // telesmk1.spr
#define CEHW_TELESMK2					27  // telesmk2.spr
#define CEHW_ICEHIT						28  // icehit.spr
#define CEHW_MEDUSA_HIT					29  // medhit.spr
#define CEHW_MEZZO_REFLECT				30  // mezzoref.spr
#define CEHW_FLOOR_EXPLOSION2			31  // flrexpl2.spr
#define CEHW_XBOW_EXPLOSION				32  // xbowexpl.spr
#define CEHW_NEW_EXPLOSION				33  // gen_expl.spr
#define CEHW_MAGIC_MISSILE_EXPLOSION	34  // mm_expld.spr
#define CEHW_GHOST						35  // ghost.spr
#define CEHW_BONE_EXPLOSION				36
#define CEHW_REDCLOUD					37
#define CEHW_TELEPORTERPUFFS			38
#define CEHW_TELEPORTERBODY				39
#define CEHW_BONESHARD					40
#define CEHW_BONESHRAPNEL				41
#define CEHW_HWMISSILESTAR				42
#define CEHW_HWEIDOLONSTAR				43
#define CEHW_HWSHEEPINATOR				44
#define CEHW_TRIPMINE					45
#define CEHW_HWBONEBALL					46
#define CEHW_HWRAVENSTAFF				47
#define CEHW_TRIPMINESTILL				48
#define CEHW_SCARABCHAIN				49
#define CEHW_SM_EXPLOSION2				50
#define CEHW_HWSPLITFLASH				51
#define CEHW_HWXBOWSHOOT				52
#define CEHW_HWRAVENPOWER				53
#define CEHW_HWDRILLA					54
#define CEHW_DEATHBUBBLES				55
//New for Mission Pack...
#define CEHW_RIPPLE						56
#define CEHW_BLDRN_EXPL					57
#define CEHW_ACID_MUZZFL				58
#define CEHW_ACID_HIT					59
#define CEHW_FIREWALL_SMALL				60
#define CEHW_FIREWALL_MEDIUM			61
#define CEHW_FIREWALL_LARGE				62
#define CEHW_LBALL_EXPL					63
#define CEHW_ACID_SPLAT					64
#define CEHW_ACID_EXPL					65
#define CEHW_FBOOM						66
#define CEHW_BOMB						67
#define CEHW_BRN_BOUNCE					68
#define CEHW_LSHOCK						69
#define CEHW_FLAMEWALL					70
#define CEHW_FLAMEWALL2					71
#define CEHW_FLOOR_EXPLOSION3			72
#define CEHW_ONFIRE						73
#define CEHW_FLAMESTREAM				74

#define XBOW_IMPACT_DEFAULT			0
#define XBOW_IMPACT_GREENFLESH		2
#define XBOW_IMPACT_REDFLESH		4
#define XBOW_IMPACT_WOOD			6
#define XBOW_IMPACT_STONE			8
#define XBOW_IMPACT_METAL			10
#define XBOW_IMPACT_ICE				12
#define XBOW_IMPACT_MUMMY			14

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
			float speed;// Only for CEHW_HWDRILLA
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
