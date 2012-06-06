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

#define MAX_AVOIDGOALS          256
#define MAX_GOALSTACK           8

#define MAX_MESSAGE_SIZE        256

//the actuall chat messages
struct bot_chatmessage_t
{
	char* chatmessage;					//chat message string
	float time;							//last time used
	bot_chatmessage_t* next;		//next chat message in a list
};

//bot chat type with chat lines
struct bot_chattype_t
{
	char name[MAX_CHATTYPE_NAME];
	int numchatmessages;
	bot_chatmessage_t* firstchatmessage;
	bot_chattype_t* next;
};

//bot chat lines
struct bot_chat_t
{
	bot_chattype_t* types;
};

struct bot_consolemessage_t
{
	int handle;
	float time;							//message time
	int type;							//message type
	char message[MAX_MESSAGE_SIZE];		//message
	bot_consolemessage_t* prev, * next;	//prev and next in list
};

//chat state of a bot
struct bot_chatstate_t
{
	int gender;											//0=it, 1=female, 2=male
	int client;											//client number
	char name[32];										//name of the bot
	char chatmessage[MAX_MESSAGE_SIZE];
	int handle;
	//the console messages visible to the bot
	bot_consolemessage_t* firstmessage;			//first message is the first typed message
	bot_consolemessage_t* lastmessage;			//last message is the last typed message, bottom of console
	//number of console messages stored in the state
	int numconsolemessages;
	//the bot chat lines
	bot_chat_t* chat;
};

bot_chatstate_t* BotChatStateFromHandle(int handle);
void BotRemoveTildes(char* message);
//setup the chat AI
int BotSetupChatAI();
//shutdown the chat AI
void BotShutdownChatAI();

//free cached bot characters
void BotShutdownCharacters();

//a bot goal
//!!!! Do not change so that mem-copy can be used with game versions.
struct bot_goal_t
{
	vec3_t origin;				//origin of the goal
	int areanum;				//area number of the goal
	vec3_t mins, maxs;			//mins and maxs of the goal
	int entitynum;				//number of the goal entity
	int number;					//goal number
	int flags;					//goal flags
	int iteminfo;				//item information
	int urgency;				//how urgent is the goal? should we be allowed to exit autonomy range to reach the goal?
	int goalEndTime;			// When is the shortest time this can end?
};

//minimum avoid goal time
#define AVOID_MINIMUM_TIME      10
//default avoid goal time
#define AVOID_DEFAULT_TIME      30
//avoid dropped goal time
#define AVOID_DROPPED_TIME      10
//avoid dropped goal time
#define AVOID_DROPPED_TIME_WOLF 5
#define TRAVELTIME_SCALE        0.01

//item flags
#define IFL_NOTFREE             1		//not in free for all
#define IFL_NOTTEAM             2		//not in team play
#define IFL_NOTSINGLE           4		//not in single player
#define IFL_NOTBOT              8		//bot should never go for this
#define IFL_ROAM                16		//bot roam goal

struct levelitem_t
{
	int number;							//number of the level item
	int iteminfo;						//index into the item info
	int flags;							//item flags
	float weight;						//fixed roam weight
	vec3_t origin;						//origin of the item
	int goalareanum;					//area the item is in
	vec3_t goalorigin;					//goal origin within the area
	int entitynum;						//entity number
	float timeout;						//item is removed after this time
	levelitem_t* prev;
	levelitem_t* next;
};

struct iteminfo_t
{
	char classname[32];					//classname of the item
	char name[MAX_STRINGFIELD];			//name of the item
	char model[MAX_STRINGFIELD];		//model of the item
	int modelindex;						//model index
	int type;							//item type
	int index;							//index in the inventory
	float respawntime;					//respawn time
	vec3_t mins;						//mins of the item
	vec3_t maxs;						//maxs of the item
	int number;							//number of the item info
};

struct itemconfig_t
{
	int numiteminfo;
	iteminfo_t* iteminfo;
};

//goal state
struct weightconfig_t;
struct bot_goalstate_t
{
	weightconfig_t* itemweightconfig;	//weight config
	int* itemweightindex;						//index from item to weight
	//
	int client;									//client using this goal state
	int lastreachabilityarea;					//last area with reachabilities the bot was in
	//
	bot_goal_t goalstack[MAX_GOALSTACK];		//goal stack
	int goalstacktop;							//the top of the goal stack
	//
	int avoidgoals[MAX_AVOIDGOALS];				//goals to avoid
	float avoidgoaltimes[MAX_AVOIDGOALS];		//times to avoid the goals
};

extern itemconfig_t* itemconfig;
extern levelitem_t* levelitems;
extern int numlevelitems;
extern int g_gametype;
extern bool g_singleplayer;
extern libvar_t* droppedweight;

bot_goalstate_t* BotGoalStateFromHandle(int handle);
void InitLevelItemHeap();
levelitem_t* AllocLevelItem();
void FreeLevelItem(levelitem_t* li);
void AddLevelItemToList(levelitem_t* li);
void RemoveLevelItemFromList(levelitem_t* li);
void BotFreeInfoEntities();
//setup the goal AI
int BotSetupGoalAI(bool singleplayer);
//shut down the goal AI
void BotShutdownGoalAI();
void BotInitInfoEntities();
void BotAddToAvoidGoals(bot_goalstate_t* gs, int number, float avoidtime);

#define MAX_AVOIDREACH                  1
#define MAX_AVOIDSPOTS                  32

//used to avoid reachability links for some time after being used
#define AVOIDREACH_TIME_Q3      6		//avoid links for 6 seconds after use
#define AVOIDREACH_TIME_ET      4
#define AVOIDREACH_TRIES        4

//prediction times
#define PREDICTIONTIME_JUMP 3		//in seconds
#define PREDICTIONTIME_MOVE 2		//in seconds

//hook commands
#define CMD_HOOKOFF             "hookoff"
#define CMD_HOOKON              "hookon"

//weapon indexes for weapon jumping
#define WEAPONINDEX_ROCKET_LAUNCHER     5
#define WEAPONINDEX_BFG                 9

#define MODELTYPE_FUNC_PLAT     1
#define MODELTYPE_FUNC_BOB      2
#define MODELTYPE_FUNC_DOOR     3
#define MODELTYPE_FUNC_STATIC   4

struct bot_avoidspot_t
{
	vec3_t origin;
	float radius;
	int type;
};

//movement state
//NOTE: the moveflags MFL_ONGROUND, MFL_TELEPORTED, MFL_WATERJUMP and
//		MFL_GRAPPLEPULL must be set outside the movement code
struct bot_movestate_t
{
	//input vars (all set outside the movement code)
	vec3_t origin;								//origin of the bot
	vec3_t velocity;							//velocity of the bot
	vec3_t viewoffset;							//view offset
	int entitynum;								//entity number of the bot
	int client;									//client number of the bot
	float thinktime;							//time the bot thinks
	int presencetype;							//presencetype of the bot
	vec3_t viewangles;							//view angles of the bot
	//state vars
	int areanum;								//area the bot is in
	int lastareanum;							//last area the bot was in
	int lastgoalareanum;						//last goal area number
	int lastreachnum;							//last reachability number
	vec3_t lastorigin;							//origin previous cycle
	float lasttime;
	int reachareanum;							//area number of the reachabilty
	int moveflags;								//movement flags
	int jumpreach;								//set when jumped
	float grapplevisible_time;					//last time the grapple was visible
	float lastgrappledist;						//last distance to the grapple end
	float reachability_time;					//time to use current reachability
	int avoidreach[MAX_AVOIDREACH];				//reachabilities to avoid
	float avoidreachtimes[MAX_AVOIDREACH];		//times to avoid the reachabilities
	int avoidreachtries[MAX_AVOIDREACH];		//number of tries before avoiding
	bot_avoidspot_t avoidspots[MAX_AVOIDSPOTS];	//spots to avoid
	int numavoidspots;
};

extern libvar_t* weapindex_grapple;
extern libvar_t* offhandgrapple;
extern libvar_t* cmd_grappleoff;
extern libvar_t* cmd_grappleon;
extern int modeltypes[MAX_MODELS_Q3];

bot_movestate_t* BotMoveStateFromHandle(int handle);
//must be called every map change
void BotSetBrushModelTypes();
void BotAddToAvoidReach(bot_movestate_t* ms, int number, float avoidtime);
void BotClearMoveResult(bot_moveresult_t* moveresult);
int GrappleState(bot_movestate_t* ms, aas_reachability_t* reach);
int BotReachabilityTime(aas_reachability_t* reach);
//setup movement AI
int BotSetupMoveAI();
//shutdown movement AI
void BotShutdownMoveAI();
int BotGetReachabilityToGoal(const vec3_t origin, int areanum,
	int lastgoalareanum, int lastareanum,
	const int* avoidreach, const float* avoidreachtimes, const int* avoidreachtries,
	const bot_goal_t* goal, int travelflags, int movetravelflags,
	const bot_avoidspot_t* avoidspots, int numavoidspots, int* flags);
int BotFuzzyPointReachabilityArea(const vec3_t origin);
int BotOnTopOfEntity(const bot_movestate_t* ms);
void BotCheckBlocked(const bot_movestate_t* ms, const vec3_t dir, int checkbottom, bot_moveresult_t* result);
bot_moveresult_t BotTravel_Walk(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_Walk(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_Crouch(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_BarrierJump(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_BarrierJump(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_Swim(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_WaterJump(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_WaterJump(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_WalkOffLedge(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_WalkOffLedge(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_Jump(bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_Jump(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_Ladder(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_Teleport(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_Elevator(bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_Elevator(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_FuncBobbing(bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_FuncBobbing(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_RocketJump(bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_BFGJump(bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_WeaponJump(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotTravel_JumpPad(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotFinishTravel_JumpPad(const bot_movestate_t* ms, const aas_reachability_t* reach);
bot_moveresult_t BotMoveInGoalArea(bot_movestate_t* ms, const bot_goal_t* goal);

//setup the weapon AI
int BotSetupWeaponAI();
//shut down the weapon AI
void BotShutdownWeaponAI();
