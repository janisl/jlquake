/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//!!!!!!!!!!!!!!! Used by game VM !!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
/*****************************************************************************
 * name:		botlib.h
 *
 * desc:		bot AI library
 *
 * $Archive: /source/code/game/botai.h $
 *
 *****************************************************************************/

#define BOTLIB_API_VERSION      2

struct aas_clientmove_s;
struct aas_altroutegoal_s;
struct bot_entitystate_t;

//debug line colors
#define LINECOLOR_NONE          -1
#define LINECOLOR_RED           1	//0xf2f2f0f0L
#define LINECOLOR_GREEN         2	//0xd0d1d2d3L
#define LINECOLOR_BLUE          3	//0xf3f3f1f1L
#define LINECOLOR_YELLOW        4	//0xdcdddedfL
#define LINECOLOR_ORANGE        5	//0xe0e1e2e3L

#ifndef BSPTRACE

#define BSPTRACE

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

#endif	// BSPTRACE

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
} botlib_import_t;

typedef struct aas_export_s
{
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	int (* AAS_PointContents)(vec3_t point);
	//--------------------------------------------
	// be_aas_altroute.c
	//--------------------------------------------
	int (* AAS_AlternativeRouteGoals)(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
		struct aas_altroutegoal_s* altroutegoals, int maxaltroutegoals,
		int type);
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
} aas_export_t;

typedef struct ea_export_s
{
	//ClientCommand elementary actions
	void (* EA_Command)(int client, char* command);
	void (* EA_Say)(int client, char* str);
	void (* EA_SayTeam)(int client, char* str);
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
	int (* BotChooseLTGItem)(int goalstate, vec3_t origin, int* inventory, int travelflags);
	int (* BotChooseNBGItem)(int goalstate, vec3_t origin, int* inventory, int travelflags,
		struct bot_goal_q3_t* ltg, float maxtime);
	int (* BotItemGoalInVisButNotVisible)(int viewer, vec3_t eye, vec3_t viewangles, struct bot_goal_q3_t* goal);
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

"basedir"					""					l_utils.c			base directory
"gamedir"					""					l_utils.c			game directory
"cddir"						""					l_utils.c			CD directory

"log"						"0"					l_log.c				enable/disable creating a log file
"maxclients"				"4"					be_interface.c		maximum number of clients
"maxentities"				"1024"				be_interface.c		maximum number of entities
"bot_developer"				"0"					be_interface.c		bot developer mode

"phys_friction"				"6"					be_aas_move.c		ground friction
"phys_stopspeed"			"100"				be_aas_move.c		stop speed
"phys_gravity"				"800"				be_aas_move.c		gravity value
"phys_waterfriction"		"1"					be_aas_move.c		water friction
"phys_watergravity"			"400"				be_aas_move.c		gravity in water
"phys_maxvelocity"			"320"				be_aas_move.c		maximum velocity
"phys_maxwalkvelocity"		"320"				be_aas_move.c		maximum walk velocity
"phys_maxcrouchvelocity"	"100"				be_aas_move.c		maximum crouch velocity
"phys_maxswimvelocity"		"150"				be_aas_move.c		maximum swim velocity
"phys_walkaccelerate"		"10"				be_aas_move.c		walk acceleration
"phys_airaccelerate"		"1"					be_aas_move.c		air acceleration
"phys_swimaccelerate"		"4"					be_aas_move.c		swim acceleration
"phys_maxstep"				"18"				be_aas_move.c		maximum step height
"phys_maxsteepness"			"0.7"				be_aas_move.c		maximum floor steepness
"phys_maxbarrier"			"32"				be_aas_move.c		maximum barrier height
"phys_maxwaterjump"			"19"				be_aas_move.c		maximum waterjump height
"phys_jumpvel"				"270"				be_aas_move.c		jump z velocity
"phys_falldelta5"			"40"				be_aas_move.c
"phys_falldelta10"			"60"				be_aas_move.c
"rs_waterjump"				"400"				be_aas_move.c
"rs_teleport"				"50"				be_aas_move.c
"rs_barrierjump"			"100"				be_aas_move.c
"rs_startcrouch"			"300"				be_aas_move.c
"rs_startgrapple"			"500"				be_aas_move.c
"rs_startwalkoffledge"		"70"				be_aas_move.c
"rs_startjump"				"300"				be_aas_move.c
"rs_rocketjump"				"500"				be_aas_move.c
"rs_bfgjump"				"500"				be_aas_move.c
"rs_jumppad"				"250"				be_aas_move.c
"rs_aircontrolledjumppad"	"300"				be_aas_move.c
"rs_funcbob"				"300"				be_aas_move.c
"rs_startelevator"			"50"				be_aas_move.c
"rs_falldamage5"			"300"				be_aas_move.c
"rs_falldamage10"			"500"				be_aas_move.c
"rs_maxjumpfallheight"		"450"				be_aas_move.c

"max_aaslinks"				"4096"				be_aas_sample.c		maximum links in the AAS
"max_routingcache"			"4096"				be_aas_route.c		maximum routing cache size in KB
"forceclustering"			"0"					be_aas_main.c		force recalculation of clusters
"forcereachability"			"0"					be_aas_main.c		force recalculation of reachabilities
"forcewrite"				"0"					be_aas_main.c		force writing of aas file
"aasoptimize"				"0"					be_aas_main.c		enable aas optimization
"sv_mapChecksum"			"0"					be_aas_main.c		BSP file checksum
"bot_visualizejumppads"		"0"					be_aas_reach.c		visualize jump pads

"bot_reloadcharacters"		"0"					-					reload bot character files
"ai_gametype"				"0"					be_ai_goal.c		game type
"droppedweight"				"1000"				be_ai_goal.c		additional dropped item weight
"weapindex_rocketlauncher"	"5"					be_ai_move.c		rl weapon index for rocket jumping
"weapindex_bfg10k"			"9"					be_ai_move.c		bfg weapon index for bfg jumping
"weapindex_grapple"			"10"				be_ai_move.c		grapple weapon index for grappling
"entitytypemissile"			"3"					be_ai_move.c		ET_MISSILE
"offhandgrapple"			"0"					be_ai_move.c		enable off hand grapple hook
"cmd_grappleon"				"grappleon"			be_ai_move.c		command to activate off hand grapple
"cmd_grappleoff"			"grappleoff"		be_ai_move.c		command to deactivate off hand grapple
"itemconfig"				"items.c"			be_ai_goal.c		item configuration file
"weaponconfig"				"weapons.c"			be_ai_weap.c		weapon configuration file
"synfile"					"syn.c"				be_ai_chat.c		file with synonyms
"rndfile"					"rnd.c"				be_ai_chat.c		file with random strings
"matchfile"					"match.c"			be_ai_chat.c		file with match strings
"nochat"					"0"					be_ai_chat.c		disable chats
"max_messages"				"1024"				be_ai_chat.c		console message heap size
"max_weaponinfo"			"32"				be_ai_weap.c		maximum number of weapon info
"max_projectileinfo"		"32"				be_ai_weap.c		maximum number of projectile info
"max_iteminfo"				"256"				be_ai_goal.c		maximum number of item info
"max_levelitems"			"256"				be_ai_goal.c		maximum number of level items

*/
