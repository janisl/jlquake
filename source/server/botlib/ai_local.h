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

//location in the map "target_location"
struct maplocation_t
{
	vec3_t origin;
	int areanum;
	char name[MAX_EPAIRKEY];
	maplocation_t* next;
};

//camp spots "info_camp"
struct campspot_t
{
	vec3_t origin;
	int areanum;
	char name[MAX_EPAIRKEY];
	float range;
	float weight;
	float wait;
	float random;
	campspot_t* next;
};

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
extern maplocation_t* maplocations;
extern campspot_t* campspots;
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

//free cached bot characters
void BotShutdownCharacters();

//setup the weapon AI
int BotSetupWeaponAI();
//shut down the weapon AI
void BotShutdownWeaponAI();

//escape character
#define ESCAPE_CHAR             0x01	//'_'
//
// "hi ", people, " ", 0, " entered the game"
//becomes:
// "hi _rpeople_ _v0_ entered the game"
//

//match piece types
#define MT_VARIABLE                 1		//variable match piece
#define MT_STRING                   2		//string match piece
//reply chat key flags
#define RCKFL_AND                   1		//key must be present
#define RCKFL_NOT                   2		//key must be absent
#define RCKFL_NAME                  4		//name of bot must be present
#define RCKFL_STRING                8		//key is a string
#define RCKFL_VARIABLES             16		//key is a match template
#define RCKFL_BOTNAMES              32		//key is a series of botnames
#define RCKFL_GENDERFEMALE          64		//bot must be female
#define RCKFL_GENDERMALE            128		//bot must be male
#define RCKFL_GENDERLESS            256		//bot must be genderless
//time to ignore a chat message after using it
#define CHATMESSAGE_RECENTTIME  20

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

//random string
struct bot_randomstring_t
{
	char* string;
	bot_randomstring_t* next;
};

//list with random strings
struct bot_randomlist_t
{
	char* string;
	int numstrings;
	bot_randomstring_t* firstrandomstring;
	bot_randomlist_t* next;
};

//synonym
struct bot_synonym_t
{
	char* string;
	float weight;
	bot_synonym_t* next;
};

//list with synonyms
struct bot_synonymlist_t
{
	unsigned int context;
	float totalweight;
	bot_synonym_t* firstsynonym;
	bot_synonymlist_t* next;
};

//fixed match string
struct bot_matchstring_t
{
	char* string;
	bot_matchstring_t* next;
};

//piece of a match template
struct bot_matchpiece_t
{
	int type;
	bot_matchstring_t* firststring;
	int variable;
	bot_matchpiece_t* next;
};

//match template
struct bot_matchtemplate_t
{
	unsigned int context;
	int type;
	int subtype;
	bot_matchpiece_t* first;
	bot_matchtemplate_t* next;
};

//reply chat key
struct bot_replychatkey_t
{
	int flags;
	char* string;
	bot_matchpiece_t* match;
	bot_replychatkey_t* next;
};

//reply chat
struct bot_replychat_t
{
	bot_replychatkey_t* keys;
	float priority;
	int numchatmessages;
	bot_chatmessage_t* firstchatmessage;
	bot_replychat_t* next;
};

//string list
struct bot_stringlist_t
{
	char* string;
	bot_stringlist_t* next;
};


struct bot_ichatdata_t
{
	bot_chat_t* chat;
	char filename[MAX_QPATH];
	char chatname[MAX_QPATH];
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

extern bot_chatstate_t* botchatstates[MAX_BOTLIB_CLIENTS_ARRAY + 1];
extern bot_consolemessage_t* consolemessageheap;
extern bot_consolemessage_t* freeconsolemessages;
extern bot_matchtemplate_t* matchtemplates;
extern bot_synonymlist_t* synonyms;
extern bot_randomlist_t* randomstrings;
extern bot_replychat_t* replychats;
extern bot_ichatdata_t* ichatdata[MAX_BOTLIB_CLIENTS_ARRAY];

bot_chatstate_t* BotChatStateFromHandle(int handle);
void InitConsoleMessageHeap();
void FreeConsoleMessage(bot_consolemessage_t* message);
void BotRemoveTildes(char* message);
char* StringContainsWord(char* str1, const char* str2, bool casesensitive);
bot_synonymlist_t* BotLoadSynonyms(const char* filename);
void BotReplaceWeightedSynonyms(char* string, unsigned int context);
void BotReplaceReplySynonyms(char* string, unsigned int context);
