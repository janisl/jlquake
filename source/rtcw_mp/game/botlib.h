/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//===========================================================================
//
/*****************************************************************************
 * name:		botlib.h
 *
 * desc:		bot AI library
 *
 *
 *****************************************************************************/

#define BOTLIB_API_VERSION      2

struct aas_clientmove_s;
struct bot_moveresult_t;
struct bot_initmove_q3_t;


//debug line colors
#define LINECOLOR_NONE          -1
#define LINECOLOR_RED           1	//0xf2f2f0f0L
#define LINECOLOR_GREEN         2	//0xd0d1d2d3L
#define LINECOLOR_BLUE          3	//0xf3f3f1f1L
#define LINECOLOR_YELLOW        4	//0xdcdddedfL
#define LINECOLOR_ORANGE        5	//0xe0e1e2e3L

//console message types
#define CMS_NORMAL              0
#define CMS_CHAT                1

//action flags
#define ACTION_ATTACK           1
#define ACTION_USE              2
#define ACTION_RESPAWN          4
#define ACTION_JUMP             8
#define ACTION_MOVEUP           8
#define ACTION_CROUCH           16
#define ACTION_MOVEDOWN         16
#define ACTION_MOVEFORWARD      32
#define ACTION_MOVEBACK         64
#define ACTION_MOVELEFT         128
#define ACTION_MOVERIGHT        256
#define ACTION_DELAYEDJUMP      512
#define ACTION_TALK             1024
#define ACTION_GESTURE          2048
#define ACTION_WALK             4096
#define ACTION_RELOAD           8192

//the bot input, will be converted to an wmusercmd_t
typedef struct bot_input_s
{
	float thinktime;		//time since last output (in seconds)
	vec3_t dir;				//movement direction
	float speed;			//speed in the range [0, 400]
	vec3_t viewangles;		//the view angles
	int actionflags;		//one of the ACTION_? flags
	int weapon;				//weapon to use
} bot_input_t;

#ifndef BSPTRACE

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//remove the bsp_trace_s structure definition l8r on
//a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
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
} bsp_trace_t;

#define BSPTRACE
#endif	// BSPTRACE

//entity state
typedef struct bot_entitystate_s
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
//	int		weapAnim;		// mask off ANIM_TOGGLEBIT	//----(SA)	added
//----(SA)	didn't want to comment in as I wasn't sure of any implications of changing this structure.
} bot_entitystate_t;

//bot AI library exported functions
typedef struct botlib_import_s
{
	//trace a bbox through the world
	void (* Trace)(bsp_trace_t* trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
	//trace a bbox against a specific entity
	void (* EntityTrace)(bsp_trace_t* trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask);
	//retrieve the contents at the given point
	int (* PointContents)(vec3_t point);
	//check if the point is in potential visible sight
	int (* inPVS)(vec3_t p1, vec3_t p2);
	//
	void (* BotClientCommand)(int client, const char* command);
	//memory allocation
	void*(*GetMemory)(int size);
	void (* FreeMemory)(void* ptr);
	void (* FreeZoneMemory)(void);
	void*(*HunkAlloc)(int size);
	//
	// Ridah, Cast AI stuff
	qboolean (* AICast_VisibleFromPos)(vec3_t srcpos, int srcnum,
		vec3_t destpos, int destnum, qboolean updateVisPos);
	qboolean (* AICast_CheckAttackAtPos)(int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld);
	// done.
} botlib_import_t;

typedef struct aas_export_s
{
	//-----------------------------------
	// be_aas_main.h
	//-----------------------------------
	void (* AAS_PresenceTypeBoundingBox)(int presencetype, vec3_t mins, vec3_t maxs);
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	int (* AAS_PointAreaNum)(vec3_t point);
	int (* AAS_TraceAreas)(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas);
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	int (* AAS_PointContents)(vec3_t point);
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	int (* AAS_AreaTravelTimeToGoalArea)(int areanum, vec3_t origin, int goalareanum, int travelflags);
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	int (* AAS_Swimming)(vec3_t origin);
	int (* AAS_PredictClientMovement)(struct aas_clientmove_s* move,
		int entnum, vec3_t origin,
		int presencetype, int onground,
		vec3_t velocity, vec3_t cmdmove,
		int cmdframes,
		int maxframes, float frametime,
		int stopevent, int stopareanum, int visualize);

	// Ridah, route-tables
	//--------------------------------------------
	// be_aas_routetable.c
	//--------------------------------------------
	qboolean (* AAS_RT_GetHidePos)(vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos);
	int (* AAS_FindAttackSpotWithinRange)(int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos);
	void (* AAS_SetAASBlockingEntity)(vec3_t absmin, vec3_t absmax, qboolean blocking);
	// done.
} aas_export_t;

typedef struct ea_export_s
{
	//ClientCommand elementary actions
	void (* EA_Say)(int client, char* str);
	void (* EA_SayTeam)(int client, char* str);
	void (* EA_UseItem)(int client, char* it);
	void (* EA_DropItem)(int client, char* it);
	void (* EA_UseInv)(int client, char* inv);
	void (* EA_DropInv)(int client, char* inv);
	void (* EA_Gesture)(int client);
	void (* EA_Command)(int client, const char* command);
	//regular elementary actions
	void (* EA_SelectWeapon)(int client, int weapon);
	void (* EA_Talk)(int client);
	void (* EA_Attack)(int client);
	void (* EA_Reload)(int client);
	void (* EA_Use)(int client);
	void (* EA_Respawn)(int client);
	void (* EA_Jump)(int client);
	void (* EA_DelayedJump)(int client);
	void (* EA_Crouch)(int client);
	void (* EA_MoveUp)(int client);
	void (* EA_MoveDown)(int client);
	void (* EA_MoveForward)(int client);
	void (* EA_MoveBack)(int client);
	void (* EA_MoveLeft)(int client);
	void (* EA_MoveRight)(int client);
	void (* EA_Move)(int client, vec3_t dir, float speed);
	void (* EA_View)(int client, vec3_t viewangles);
	//send regular input to the server
	void (* EA_EndRegular)(int client, float thinktime);
	void (* EA_GetInput)(int client, float thinktime, bot_input_t* input);
	void (* EA_ResetInput)(int client, bot_input_t* init);
} ea_export_t;

typedef struct ai_export_s
{
	//-----------------------------------
	// be_ai_chat.h
	//-----------------------------------
	void (* BotEnterChat)(int chatstate, int client, int sendto);
	//-----------------------------------
	// be_ai_goal.h
	//-----------------------------------
	void (* BotRemoveFromAvoidGoals)(int goalstate, int number);
	void (* BotDumpAvoidGoals)(int goalstate);
	int (* BotChooseLTGItem)(int goalstate, vec3_t origin, int* inventory, int travelflags);
	int (* BotChooseNBGItem)(int goalstate, vec3_t origin, int* inventory, int travelflags,
		struct bot_goal_q3_t* ltg, float maxtime);
	int (* BotTouchingGoal)(vec3_t origin, struct bot_goal_q3_t* goal);
	int (* BotItemGoalInVisButNotVisible)(int viewer, vec3_t eye, vec3_t viewangles, struct bot_goal_q3_t* goal);
	float (* BotAvoidGoalTime)(int goalstate, int number);
	void (* BotInitLevelItems)(void);
	void (* BotUpdateEntityItems)(void);
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	void (* BotMoveToGoal)(struct bot_moveresult_t* result, int movestate, struct bot_goal_q3_t* goal, int travelflags);
	int (* BotMoveInDirection)(int movestate, vec3_t dir, float speed, int type);
	int (* BotReachabilityArea)(vec3_t origin, int testground);
	int (* BotMovementViewTarget)(int movestate, struct bot_goal_q3_t* goal, int travelflags, float lookahead, vec3_t target);
	int (* BotPredictVisiblePosition)(vec3_t origin, int areanum, struct bot_goal_q3_t* goal, int travelflags, vec3_t target);
} ai_export_t;

//bot AI library imported functions
typedef struct botlib_export_s
{
	//Area Awareness System functions
	aas_export_t aas;
	//Elementary Action functions
	ea_export_t ea;
	//AI functions
	ai_export_t ai;
	//setup the bot library, returns BLERR_
	int (* BotLibSetup)(void);
	//shutdown the bot library, returns BLERR_
	int (* BotLibShutdown)(void);

	//start a frame in the bot library
	int (* BotLibStartFrame)(float time);
	//load a new map in the bot library
	int (* BotLibLoadMap)(const char* mapname);
	//entity updates
	int (* BotLibUpdateEntity)(int ent, bot_entitystate_t* state);
} botlib_export_t;

//linking of bot library
botlib_export_t* GetBotLibAPI(int apiVersion, botlib_import_t* import);

/* Library variables:

name:						default:			module(s):			description:

"basedir"					""					l_utils.c			Quake2 base directory
"gamedir"					""					l_utils.c			Quake2 game directory
"cddir"						""					l_utils.c			Quake2 CD directory

"autolaunchbspc"			"0"					be_aas_load.c		automatically launch (Win)BSPC
"log"						"0"					l_log.c				enable/disable creating a log file
"maxclients"				"4"					be_interface.c		maximum number of clients
"maxentities"				"1024"				be_interface.c		maximum number of entities

"sv_friction"				"6"					be_aas_move.c		ground friction
"sv_stopspeed"				"100"				be_aas_move.c		stop speed
"sv_gravity"				"800"				be_aas_move.c		gravity value
"sv_waterfriction"			"1"					be_aas_move.c		water friction
"sv_watergravity"			"400"				be_aas_move.c		gravity in water
"sv_maxvelocity"			"300"				be_aas_move.c		maximum velocity
"sv_maxwalkvelocity"		"300"				be_aas_move.c		maximum walk velocity
"sv_maxcrouchvelocity"		"100"				be_aas_move.c		maximum crouch velocity
"sv_maxswimvelocity"		"150"				be_aas_move.c		maximum swim velocity
"sv_walkaccelerate"			"10"				be_aas_move.c		walk acceleration
"sv_airaccelerate"			"1"					be_aas_move.c		air acceleration
"sv_swimaccelerate"			"4"					be_aas_move.c		swim acceleration
"sv_maxstep"				"18"				be_aas_move.c		maximum step height
"sv_maxbarrier"				"32"				be_aas_move.c		maximum barrier height
"sv_maxsteepness"			"0.7"				be_aas_move.c		maximum floor steepness
"sv_jumpvel"				"270"				be_aas_move.c		jump z velocity
"sv_maxwaterjump"			"20"				be_aas_move.c		maximum waterjump height

"max_aaslinks"				"4096"				be_aas_sample.c		maximum links in the AAS
"max_bsplinks"				"4096"				be_aas_bsp.c		maximum links in the BSP

"notspawnflags"				"2048"				be_ai_goal.c		entities with these spawnflags will be removed
"itemconfig"				"items.c"			be_ai_goal.c		item configuration file
"weaponconfig"				"weapons.c"			be_ai_weap.c		weapon configuration file
"synfile"					"syn.c"				be_ai_chat.c		file with synonyms
"rndfile"					"rnd.c"				be_ai_chat.c		file with random strings
"matchfile"					"match.c"			be_ai_chat.c		file with match strings
"max_messages"				"1024"				be_ai_chat.c		console message heap size
"max_weaponinfo"			"32"				be_ai_weap.c		maximum number of weapon info
"max_projectileinfo"		"32"				be_ai_weap.c		maximum number of projectile info
"max_iteminfo"				"256"				be_ai_goal.c		maximum number of item info
"max_levelitems"			"256"				be_ai_goal.c		maximum number of level items
"framereachability"			""					be_aas_reach.c		number of reachabilities to calucate per frame
"forceclustering"			"0"					be_aas_main.c		force recalculation of clusters
"forcereachability"			"0"					be_aas_main.c		force recalculation of reachabilities
"forcewrite"				"0"					be_aas_main.c		force writing of aas file
"nooptimize"				"0"					be_aas_main.c		no aas optimization

"laserhook"					"0"					be_ai_move.c		0 = CTF hook, 1 = laser hook

*/
