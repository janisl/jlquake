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
			int		reverse;  // Forward animation has been run, now go backwards
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
			float speed;// Only for CE_HWDRILLA
		} Missile;
		struct
		{
			vec3_t angle;	//as per missile
			vec3_t origin;
			vec3_t avelocity;
			vec3_t velocity;
			int entity_index;
			float time_amount;	
			float	scale;		//star effects on top of missile
			int		scaleDir;
			int	ent1, ent2;
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
			int		entity_index[16];
			vec3_t	velocity[16];
			vec3_t	avel[3];
			vec3_t	origin;
			unsigned char type;
			vec3_t	srcVel;
			unsigned char numChunks;
			float	time_amount;
			float	aveScale;
		}Chunk;
	};
};
