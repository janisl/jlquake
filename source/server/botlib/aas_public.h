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

//!!!!!!!!!!!!!!! Used by game VM !!!!!!!!!!!!!!!!!!!!!!!!!!!!

// RF, these need to be here so the botlib also knows how many bot game entities there are
#define NUM_BOTGAMEENTITIES 384

//travel flags
#define TFL_INVALID             0x00000001	//traveling temporary not possible
#define TFL_WALK                0x00000002	//walking
#define TFL_CROUCH              0x00000004	//crouching
#define TFL_BARRIERJUMP         0x00000008	//jumping onto a barrier
#define TFL_JUMP                0x00000010	//jumping
#define TFL_LADDER              0x00000020	//climbing a ladder
#define TFL_WALKOFFLEDGE        0x00000080	//walking of a ledge
#define TFL_SWIM                0x00000100	//swimming
#define TFL_WATERJUMP           0x00000200	//jumping out of the water
#define TFL_TELEPORT            0x00000400	//teleporting
#define TFL_ELEVATOR            0x00000800	//elevator
#define TFL_ROCKETJUMP          0x00001000	//rocket jumping
#define TFL_BFGJUMP             0x00002000	//bfg jumping
#define TFL_GRAPPLEHOOK         0x00004000	//grappling hook
#define TFL_DOUBLEJUMP          0x00008000	//double jump
#define TFL_RAMPJUMP            0x00010000	//ramp jump
#define TFL_STRAFEJUMP          0x00020000	//strafe jump
#define TFL_JUMPPAD             0x00040000	//jump pad
#define TFL_AIR                 0x00080000	//travel through air
#define TFL_WATER               0x00100000	//travel through water
#define TFL_SLIME               0x00200000	//travel through slime
#define TFL_LAVA                0x00400000	//travel through lava
#define TFL_DONOTENTER          0x00800000	//travel through donotenter area
#define TFL_FUNCBOB             0x01000000	//func bobbing
#define Q3TFL_BRIDGE            0x04000000	//move over a bridge
#define Q3TFL_NOTTEAM1          0x08000000	//not team 1
#define Q3TFL_NOTTEAM2          0x10000000	//not team 2
#define WOLFTFL_DONOTENTER_LARGE    0x02000000	//travel through donotenter area
#define ETTFL_TEAM_AXIS         0x04000000	//travel through axis-only areas
#define ETTFL_TEAM_ALLIES       0x08000000	//travel through allies-only areas
#define ETTFL_TEAM_AXIS_DISGUISED   0x10000000	//travel through axis+DISGUISED areas
#define ETTFL_TEAM_ALLIES_DISGUISED 0x02000000	//travel through allies+DISGUISED areas

#define ETTFL_TEAM_FLAGS    (ETTFL_TEAM_AXIS | ETTFL_TEAM_ALLIES | ETTFL_TEAM_AXIS_DISGUISED | ETTFL_TEAM_ALLIES_DISGUISED)

//default travel flags
#define Q3TFL_DEFAULT TFL_WALK | TFL_CROUCH | TFL_BARRIERJUMP | \
	TFL_JUMP | TFL_LADDER |	\
	TFL_WALKOFFLEDGE | TFL_SWIM | TFL_WATERJUMP | \
	TFL_TELEPORT | TFL_ELEVATOR | \
	TFL_AIR | TFL_WATER | TFL_JUMPPAD | TFL_FUNCBOB
//----(SA)	modified since slime is no longer deadly
#define WSTFL_DEFAULT ((TFL_WALK | TFL_CROUCH | TFL_BARRIERJUMP |	\
	TFL_JUMP | TFL_LADDER | \
	TFL_WALKOFFLEDGE | TFL_SWIM | TFL_WATERJUMP |	\
	TFL_TELEPORT | TFL_ELEVATOR | TFL_AIR | \
	TFL_WATER | TFL_SLIME | \
	TFL_JUMPPAD | TFL_FUNCBOB) \
	& ~(TFL_JUMPPAD | TFL_ROCKETJUMP | TFL_BFGJUMP | TFL_GRAPPLEHOOK | TFL_DOUBLEJUMP | TFL_RAMPJUMP | TFL_STRAFEJUMP | TFL_LAVA))
// RF, added that bottom line so it's the same as AICAST_TFL_DEFAULT
//----(SA)	modified since slime is no longer deadly
#define WMTFL_DEFAULT (TFL_WALK | TFL_CROUCH | TFL_BARRIERJUMP | \
	TFL_JUMP | TFL_LADDER | \
	TFL_WALKOFFLEDGE | TFL_SWIM | TFL_WATERJUMP | \
	TFL_TELEPORT | TFL_ELEVATOR | TFL_AIR | \
	TFL_WATER | TFL_SLIME | \
	TFL_JUMPPAD | TFL_FUNCBOB)
//----(SA)	modified since slime is no longer deadly
#define ETTFL_DEFAULT (TFL_WALK | TFL_CROUCH | TFL_BARRIERJUMP | \
	TFL_JUMP | TFL_LADDER | \
	TFL_WALKOFFLEDGE | TFL_SWIM | TFL_WATERJUMP | \
	TFL_TELEPORT | TFL_ELEVATOR | TFL_AIR | \
	TFL_WATER | TFL_SLIME | \
	TFL_JUMPPAD | TFL_FUNCBOB)

//entity info
struct aas_entityinfo_t
{
	int valid;				// true if updated this frame
	int type;				// entity type
	int flags;				// entity flags
	float ltime;			// local time
	float update_time;		// time between last and current update
	int number;				// number of the entity
	vec3_t origin;			// origin of the entity
	vec3_t angles;			// angles of the model
	vec3_t old_origin;		// for lerping
	vec3_t lastvisorigin;	// last visible origin
	vec3_t mins;			// bounding box minimums
	vec3_t maxs;			// bounding box maximums
	int groundent;			// ground entity
	int solid;				// solid type
	int modelindex;			// model used
	int modelindex2;		// weapons, CTF flags, etc
	int frame;				// model frame number
	int event;				// impulse events -- muzzle flashes, footsteps, etc
	int eventParm;			// even parameter
	int powerups;			// bit flags
	int weapon;				// determines weapon and flash model, etc
	int legsAnim;			// mask off ANIM_TOGGLEBIT
	int torsoAnim;			// mask off ANIM_TOGGLEBIT
};

//handle to the next bsp entity
int AAS_NextBSPEntity(int ent);
//return the value of the BSP epair key
bool AAS_ValueForBSPEpairKey(int ent, const char* key, char* value, int size);
//get a vector for the BSP epair key
bool AAS_VectorForBSPEpairKey(int ent, const char* key, vec3_t v);
//get a float for the BSP epair key
bool AAS_FloatForBSPEpairKey(int ent, const char* key, float* value);
//get an integer for the BSP epair key
bool AAS_IntForBSPEpairKey(int ent, const char* key, int* value);

//returns the info of the given entity
void AAS_EntityInfo(int entnum, aas_entityinfo_t* info);

//returns true if AAS is initialized
bool AAS_Initialized();
//returns the current time
float AAS_Time();
void AAS_SetCurrentWorld(int index);

//enable or disable an area for routing
int AAS_EnableRoutingArea(int areanum, int enable);
