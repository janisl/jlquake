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
//----(SA)	modified since slime is no longer deadly
#define WOLFTFL_DEFAULT (TFL_WALK | TFL_CROUCH | TFL_BARRIERJUMP |	\
	TFL_JUMP | TFL_LADDER | \
	TFL_WALKOFFLEDGE | TFL_SWIM | TFL_WATERJUMP |	\
	TFL_TELEPORT | TFL_ELEVATOR | TFL_AIR | \
	TFL_WATER | TFL_SLIME | \
	TFL_FUNCBOB)

enum
{
	Q3SOLID_NOT,			// no interaction with other objects
	Q3SOLID_TRIGGER,		// only touch when inside, after moving
	Q3SOLID_BBOX,			// touch on edge
	Q3SOLID_BSP			// bsp clip, touch on edge
};

#define BLOCKINGFLAG_MOVER  (~0x7fffffff)

// route prediction stop events
#define RSE_NONE                0
#define RSE_NOROUTE             1	//no route to goal
#define RSE_USETRAVELTYPE       2	//stop as soon as on of the given travel types is used
#define RSE_ENTERCONTENTS       4	//stop when entering the given contents
#define RSE_ENTERAREA           8	//stop when entering the given area

// alternate route goals
#define ALTROUTEGOAL_ALL                1
#define ALTROUTEGOAL_CLUSTERPORTALS     2
#define ALTROUTEGOAL_VIEWPORTALS        4

// client movement prediction stop events, stop as soon as:
#define SE_NONE                 0
#define SE_HITGROUND            1		// the ground is hit
#define SE_LEAVEGROUND          2		// there's no ground
#define SE_ENTERWATER           4		// water is entered
#define SE_ENTERSLIME           8		// slime is entered
#define SE_ENTERLAVA            16		// lava is entered
#define SE_HITGROUNDDAMAGE      32		// the ground is hit with damage
#define SE_GAP                  64		// there's a gap
#define SE_TOUCHJUMPPAD         128		// touching a jump pad area
#define SE_TOUCHTELEPORTER      256		// touching teleporter
#define SE_ENTERAREA            512		// the given stoparea is entered
#define SE_HITGROUNDAREA        1024	// a ground face in the area is hit
#define Q3SE_HITBOUNDINGBOX     2048	// hit the specified bounding box
#define Q3SE_TOUCHCLUSTERPORTAL 4096	// touching a cluster portal
#define ETSE_HITENT             2048	// hit specified entity
#define ETSE_STUCK              4096

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

// area info
struct aas_areainfo_t
{
	int contents;
	int flags;
	int presencetype;
	int cluster;
	vec3_t mins;
	vec3_t maxs;
	vec3_t center;
};

//a trace is returned when a box is swept through the AAS world
struct aas_trace_t
{
	qboolean startsolid;	// if true, the initial point was in a solid area
	float fraction;			// time completed, 1.0 = didn't hit anything
	vec3_t endpos;			// final position
	int ent;				// entity blocking the trace
	int lastarea;			// last area the trace was in (zero if none)
	int area;				// area blocking the trace (zero if none)
	int planenum;			// number of the plane that was hit
};

//bsp_trace_t hit surface
struct bsp_surface_t
{
	char name[16];
	int flags;
	int value;
};

//remove the bsp_trace_t structure definition l8r on
//a trace is returned when a box is swept through the world
struct bsp_trace_t
{
	qboolean allsolid;			// if true, plane is not valid
	qboolean startsolid;		// if true, the initial point was in a solid area
	float fraction;				// time completed, 1.0 = didn't hit anything
	vec3_t endpos;				// final position
	cplane_t plane;				// surface normal at impact
	float exp_dist;				// expanded plane distance
	int sidenum;				// number of the brush side hit
	bsp_surface_t surface;		// the hit point surface
	int contents;				// contents on other side of surface hit
	int ent;					// number of entity hit
};

//entity state
struct bot_entitystate_t
{
	int type;				// entity type
	int flags;				// entity flags
	vec3_t origin;			// origin of the entity
	vec3_t angles;			// angles of the model
	vec3_t old_origin;		// for lerping
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

struct aas_predictroute_t
{
	vec3_t endpos;			//position at the end of movement prediction
	int endarea;			//area at end of movement prediction
	int stopevent;			//event that made the prediction stop
	int endcontents;		//contents at the end of movement prediction
	int endtravelflags;		//end travel flags
	int numareas;			//number of areas predicted ahead
	int time;				//time predicted ahead (in hundreth of a sec)
};

struct aas_altroutegoal_t
{
	vec3_t origin;
	int areanum;
	unsigned short starttraveltime;
	unsigned short goaltraveltime;
	unsigned short extratraveltime;
};

struct aas_clientmove_q3_t
{
	vec3_t endpos;			//position at the end of movement prediction
	int endarea;			//area at end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	aas_trace_t trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	int endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
};

struct aas_clientmove_rtcw_t
{
	vec3_t endpos;			//position at the end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	aas_trace_t trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	float endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
};

struct aas_clientmove_et_t
{
	vec3_t endpos;			//position at the end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	bsp_trace_t trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	float endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
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
//returns the contents at the given point
int AAS_PointContents(const vec3_t point);

//returns the info of the given entity
void AAS_EntityInfo(int entnum, aas_entityinfo_t* info);
void AAS_SetAASBlockingEntity(const vec3_t absmin, const vec3_t absmax, int blocking);

//returns true if AAS is initialized
bool AAS_Initialized();
//returns the current time
float AAS_Time();
void AAS_SetCurrentWorld(int index);

//returns true if swimming at the given origin
bool AAS_Swimming(const vec3_t origin);
//movement prediction
bool AAS_PredictClientMovementQ3(aas_clientmove_q3_t* move,
	int entnum, const vec3_t origin,
	int presencetype, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, bool visualize);
bool AAS_PredictClientMovementWolf(aas_clientmove_rtcw_t* move,
	int entnum, const vec3_t origin,
	int presencetype, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, bool visualize);
bool AAS_PredictClientMovementET(aas_clientmove_et_t* move,
	int entnum, const vec3_t origin,
	int presencetype, bool onground,
	const vec3_t velocity, const vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, bool visualize);

//returns true if the are has reachabilities to other areas
int AAS_AreaReachability(int areanum);
//returns true if the area has one or more ladder faces
int AAS_AreaLadder(int areanum);

//enable or disable an area for routing
int AAS_EnableRoutingArea(int areanum, int enable);
//returns the travel time from the area to the goal area using the given travel flags
int AAS_AreaTravelTimeToGoalArea(int areanum, const vec3_t origin, int goalareanum, int travelflags);
//predict a route up to a stop event
bool AAS_PredictRoute(aas_predictroute_t* route, int areanum, const vec3_t origin,
	int goalareanum, int travelflags, int maxareas, int maxtime,
	int stopevent, int stopcontents, int stoptfl, int stopareanum);
int AAS_ListAreasInRange(const vec3_t srcpos, int srcarea, float range, int travelflags, vec3_t* outareas, int maxareas);
int AAS_AvoidDangerArea(const vec3_t srcpos, int srcarea, const vec3_t dangerpos, int dangerarea,
	float range, int travelflags);
int AAS_Retreat(const int* dangerSpots, int dangerSpotCount, const vec3_t srcpos, int srcarea,
	const vec3_t dangerpos, int dangerarea, float range, float dangerRange, int travelflags);
int AAS_NearestHideArea(int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum,
	int travelflags, float maxdist, const vec3_t distpos);
bool AAS_RT_GetHidePos(vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos);
int AAS_FindAttackSpotWithinRange(int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos);
bool AAS_GetRouteFirstVisPos(const vec3_t srcpos, const vec3_t destpos, int travelflags, vec3_t retpos);

int AAS_AlternativeRouteGoalsQ3(const vec3_t start, int startareanum,
	const vec3_t goal, int goalareanum, int travelflags,
	aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int type);
int AAS_AlternativeRouteGoalsET(const vec3_t start, const vec3_t goal, int travelflags,
	aas_altroutegoal_t* altroutegoals, int maxaltroutegoals, int color);

//returns the mins and maxs of the bounding box for the given presence type
void AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
//returns the area the point is in
int AAS_PointAreaNum(const vec3_t point);
int AAS_PointReachabilityAreaIndex(const vec3_t point);
//stores the areas the trace went through and returns the number of passed areas
int AAS_TraceAreas(const vec3_t start, const vec3_t end, int* areas, vec3_t* points, int maxareas);
//returns the areas the bounding box is in
int AAS_BBoxAreas(const vec3_t absmins, const vec3_t absmaxs, int* areas, int maxareas);
//return area information
int AAS_AreaInfo(int areanum, aas_areainfo_t* info);
void AAS_AreaCenter(int areanum, vec3_t center);
bool AAS_AreaWaypoint(int areanum, vec3_t center);
