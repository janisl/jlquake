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

#ifndef _ET_G_PUBLIC_H
#define _ET_G_PUBLIC_H

//
// system traps provided by the main engine
//
enum
{
	//============== general Quake services ==================

	ETG_PRINT,		// ( const char *string );
	// print message on the local console

	ETG_ERROR,		// ( const char *string );
	// abort the game

	ETG_MILLISECONDS,	// ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	// console variable interaction
	ETG_CVAR_REGISTER,	// ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	ETG_CVAR_UPDATE,	// ( vmCvar_t *vmCvar );
	ETG_CVAR_SET,		// ( const char *var_name, const char *value );
	ETG_CVAR_VARIABLE_INTEGER_VALUE,	// ( const char *var_name );

	ETG_CVAR_VARIABLE_STRING_BUFFER,	// ( const char *var_name, char *buffer, int bufsize );

	ETG_CVAR_LATCHEDVARIABLESTRINGBUFFER,

	ETG_ARGC,			// ( void );
	// ClientCommand and ServerCommand parameter access

	ETG_ARGV,			// ( int n, char *buffer, int bufferLength );

	ETG_FS_FOPEN_FILE,	// ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	ETG_FS_READ,		// ( void *buffer, int len, fileHandle_t f );
	ETG_FS_WRITE,		// ( const void *buffer, int len, fileHandle_t f );
	ETG_FS_RENAME,
	ETG_FS_FCLOSE_FILE,		// ( fileHandle_t f );

	ETG_SEND_CONSOLE_COMMAND,	// ( const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc


	//=========== server specific functionality =============

	ETG_LOCATE_GAME_DATA,		// ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							etplayerState_t *clients, int sizeofGameClient );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface

	ETG_DROP_CLIENT,		// ( int clientNum, const char *reason );
	// kick a client off the server with a message

	ETG_SEND_SERVER_COMMAND,	// ( int clientNum, const char *fmt, ... );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients

	ETG_SET_CONFIGSTRING,	// ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.

	ETG_GET_CONFIGSTRING,	// ( int num, char *buffer, int bufferSize );

	ETG_GET_USERINFO,		// ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset

	ETG_SET_USERINFO,		// ( int num, const char *buffer );

	ETG_GET_SERVERINFO,	// ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers

	ETG_SET_BRUSH_MODEL,	// ( gentity_t *ent, const char *name );
	// sets mins and maxs based on the brushmodel name

	ETG_TRACE,	// ( q3trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities

	ETG_POINT_CONTENTS,	// ( const vec3_t point, int passEntityNum );
	// point contents against all linked entities

	ETG_IN_PVS,			// ( const vec3_t p1, const vec3_t p2 );

	ETG_IN_PVS_IGNORE_PORTALS,	// ( const vec3_t p1, const vec3_t p2 );

	ETG_ADJUST_AREA_PORTAL_STATE,	// ( gentity_t *ent, qboolean open );

	ETG_AREAS_CONNECTED,	// ( int area1, int area2 );

	ETG_LINKENTITY,		// ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.

	ETG_UNLINKENTITY,		// ( gentity_t *ent );
	// call before removing an interactive entity

	ETG_ENTITIES_IN_BOX,	// ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact

	ETG_ENTITY_CONTACT,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape

	// access for bots to get and free a server client (FIXME?)
	ETG_BOT_ALLOCATE_CLIENT,	// ( int clientNum );

	ETG_BOT_FREE_CLIENT,	// ( int clientNum );

	ETG_GET_USERCMD,	// ( int clientNum, etusercmd_t *cmd )

	ETG_GET_ENTITY_TOKEN,	// qboolean ( char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.
	// This should only be done at GAME_INIT time.

	ETG_FS_GETFILELIST,
	ETG_DEBUG_POLYGON_CREATE,
	ETG_DEBUG_POLYGON_DELETE,
	ETG_REAL_TIME,
	ETG_SNAPVECTOR,
// MrE:

	ETG_TRACECAPSULE,	// ( q3trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection using capsule against all linked entities

	ETG_ENTITY_CONTACTCAPSULE,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape
// done.

	ETG_GETTAG,

	ETG_REGISTERTAG,
	// Gordon: load a serverside tag

	ETG_REGISTERSOUND,	// xkan, 10/28/2002 - register the sound
	ETG_GET_SOUND_LENGTH,	// xkan, 10/28/2002 - get the length of the sound

	ETBOTLIB_SETUP = 200,				// ( void );
	ETBOTLIB_SHUTDOWN,				// ( void );
	ETBOTLIB_LIBVAR_SET,
	ETBOTLIB_LIBVAR_GET,
	ETBOTLIB_PC_ADD_GLOBAL_DEFINE,
	ETBOTLIB_START_FRAME,
	ETBOTLIB_LOAD_MAP,
	ETBOTLIB_UPDATENTITY,
	ETBOTLIB_TEST,

	ETBOTLIB_GET_SNAPSHOT_ENTITY,		// ( int client, int ent );
	ETBOTLIB_GET_CONSOLE_MESSAGE,		// ( int client, char *message, int size );
	ETBOTLIB_USER_COMMAND,			// ( int client, etusercmd_t *ucmd );

	ETBOTLIB_AAS_ENTITY_VISIBLE = 300,	//FIXME: remove
	ETBOTLIB_AAS_IN_FIELD_OF_VISION,		//FIXME: remove
	ETBOTLIB_AAS_VISIBLE_CLIENTS,			//FIXME: remove
	ETBOTLIB_AAS_ENTITY_INFO,

	ETBOTLIB_AAS_INITIALIZED,
	ETBOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX,
	ETBOTLIB_AAS_TIME,

	// Ridah
	ETBOTLIB_AAS_SETCURRENTWORLD,
	// done.

	ETBOTLIB_AAS_POINT_AREA_NUM,
	ETBOTLIB_AAS_TRACE_AREAS,
	ETBOTLIB_AAS_BBOX_AREAS,
	ETBOTLIB_AAS_AREA_CENTER,
	ETBOTLIB_AAS_AREA_WAYPOINT,

	ETBOTLIB_AAS_POINT_CONTENTS,
	ETBOTLIB_AAS_NEXT_BSP_ENTITY,
	ETBOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY,
	ETBOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY,
	ETBOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY,
	ETBOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY,

	ETBOTLIB_AAS_AREA_REACHABILITY,
	ETBOTLIB_AAS_AREA_LADDER,

	ETBOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA,

	ETBOTLIB_AAS_SWIMMING,
	ETBOTLIB_AAS_PREDICT_CLIENT_MOVEMENT,

	// Ridah, route-tables
	ETBOTLIB_AAS_RT_SHOWROUTE,
	//BOTLIB_AAS_RT_GETHIDEPOS,
	//BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE,
	ETBOTLIB_AAS_NEARESTHIDEAREA,
	ETBOTLIB_AAS_LISTAREASINRANGE,
	ETBOTLIB_AAS_AVOIDDANGERAREA,
	ETBOTLIB_AAS_RETREAT,
	ETBOTLIB_AAS_ALTROUTEGOALS,
	ETBOTLIB_AAS_SETAASBLOCKINGENTITY,
	ETBOTLIB_AAS_RECORDTEAMDEATHAREA,
	// done.

	ETBOTLIB_EA_SAY = 400,
	ETBOTLIB_EA_SAY_TEAM,
	ETBOTLIB_EA_USE_ITEM,
	ETBOTLIB_EA_DROP_ITEM,
	ETBOTLIB_EA_USE_INV,
	ETBOTLIB_EA_DROP_INV,
	ETBOTLIB_EA_GESTURE,
	ETBOTLIB_EA_COMMAND,

	ETBOTLIB_EA_SELECT_WEAPON,
	ETBOTLIB_EA_TALK,
	ETBOTLIB_EA_ATTACK,
	ETBOTLIB_EA_RELOAD,
	ETBOTLIB_EA_USE,
	ETBOTLIB_EA_RESPAWN,
	ETBOTLIB_EA_JUMP,
	ETBOTLIB_EA_DELAYED_JUMP,
	ETBOTLIB_EA_CROUCH,
	ETBOTLIB_EA_WALK,
	ETBOTLIB_EA_MOVE_UP,
	ETBOTLIB_EA_MOVE_DOWN,
	ETBOTLIB_EA_MOVE_FORWARD,
	ETBOTLIB_EA_MOVE_BACK,
	ETBOTLIB_EA_MOVE_LEFT,
	ETBOTLIB_EA_MOVE_RIGHT,
	ETBOTLIB_EA_MOVE,
	ETBOTLIB_EA_VIEW,
	// START	xkan, 9/16/2002
	ETBOTLIB_EA_PRONE,
	// END		xkan, 9/16/2002

	ETBOTLIB_EA_END_REGULAR,
	ETBOTLIB_EA_GET_INPUT,
	ETBOTLIB_EA_RESET_INPUT,


	ETBOTLIB_AI_LOAD_CHARACTER = 500,
	ETBOTLIB_AI_FREE_CHARACTER,
	ETBOTLIB_AI_CHARACTERISTIC_FLOAT,
	ETBOTLIB_AI_CHARACTERISTIC_BFLOAT,
	ETBOTLIB_AI_CHARACTERISTIC_INTEGER,
	ETBOTLIB_AI_CHARACTERISTIC_BINTEGER,
	ETBOTLIB_AI_CHARACTERISTIC_STRING,

	ETBOTLIB_AI_ALLOC_CHAT_STATE,
	ETBOTLIB_AI_FREE_CHAT_STATE,
	ETBOTLIB_AI_QUEUE_CONSOLE_MESSAGE,
	ETBOTLIB_AI_REMOVE_CONSOLE_MESSAGE,
	ETBOTLIB_AI_NEXT_CONSOLE_MESSAGE,
	ETBOTLIB_AI_NUM_CONSOLE_MESSAGE,
	ETBOTLIB_AI_INITIAL_CHAT,
	ETBOTLIB_AI_REPLY_CHAT,
	ETBOTLIB_AI_CHAT_LENGTH,
	ETBOTLIB_AI_ENTER_CHAT,
	ETBOTLIB_AI_STRING_CONTAINS,
	ETBOTLIB_AI_FIND_MATCH,
	ETBOTLIB_AI_MATCH_VARIABLE,
	ETBOTLIB_AI_UNIFY_WHITE_SPACES,
	ETBOTLIB_AI_REPLACE_SYNONYMS,
	ETBOTLIB_AI_LOAD_CHAT_FILE,
	ETBOTLIB_AI_SET_CHAT_GENDER,
	ETBOTLIB_AI_SET_CHAT_NAME,

	ETBOTLIB_AI_RESET_GOAL_STATE,
	ETBOTLIB_AI_RESET_AVOID_GOALS,
	ETBOTLIB_AI_PUSH_GOAL,
	ETBOTLIB_AI_POP_GOAL,
	ETBOTLIB_AI_EMPTY_GOAL_STACK,
	ETBOTLIB_AI_DUMP_AVOID_GOALS,
	ETBOTLIB_AI_DUMP_GOAL_STACK,
	ETBOTLIB_AI_GOAL_NAME,
	ETBOTLIB_AI_GET_TOP_GOAL,
	ETBOTLIB_AI_GET_SECOND_GOAL,
	ETBOTLIB_AI_CHOOSE_LTG_ITEM,
	ETBOTLIB_AI_CHOOSE_NBG_ITEM,
	ETBOTLIB_AI_TOUCHING_GOAL,
	ETBOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE,
	ETBOTLIB_AI_GET_LEVEL_ITEM_GOAL,
	ETBOTLIB_AI_AVOID_GOAL_TIME,
	ETBOTLIB_AI_INIT_LEVEL_ITEMS,
	ETBOTLIB_AI_UPDATE_ENTITY_ITEMS,
	ETBOTLIB_AI_LOAD_ITEM_WEIGHTS,
	ETBOTLIB_AI_FREE_ITEM_WEIGHTS,
	ETBOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC,
	ETBOTLIB_AI_ALLOC_GOAL_STATE,
	ETBOTLIB_AI_FREE_GOAL_STATE,

	ETBOTLIB_AI_RESET_MOVE_STATE,
	ETBOTLIB_AI_MOVE_TO_GOAL,
	ETBOTLIB_AI_MOVE_IN_DIRECTION,
	ETBOTLIB_AI_RESET_AVOID_REACH,
	ETBOTLIB_AI_RESET_LAST_AVOID_REACH,
	ETBOTLIB_AI_REACHABILITY_AREA,
	ETBOTLIB_AI_MOVEMENT_VIEW_TARGET,
	ETBOTLIB_AI_ALLOC_MOVE_STATE,
	ETBOTLIB_AI_FREE_MOVE_STATE,
	ETBOTLIB_AI_INIT_MOVE_STATE,
	// Ridah
	ETBOTLIB_AI_INIT_AVOID_REACH,
	// done.

	ETBOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON,
	ETBOTLIB_AI_GET_WEAPON_INFO,
	ETBOTLIB_AI_LOAD_WEAPON_WEIGHTS,
	ETBOTLIB_AI_ALLOC_WEAPON_STATE,
	ETBOTLIB_AI_FREE_WEAPON_STATE,
	ETBOTLIB_AI_RESET_WEAPON_STATE,

	ETBOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION,
	ETBOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC,
	ETBOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC,
	ETBOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL,
	ETBOTLIB_AI_GET_MAP_LOCATION_GOAL,
	ETBOTLIB_AI_NUM_INITIAL_CHATS,
	ETBOTLIB_AI_GET_CHAT_MESSAGE,
	ETBOTLIB_AI_REMOVE_FROM_AVOID_GOALS,
	ETBOTLIB_AI_PREDICT_VISIBLE_POSITION,

	ETBOTLIB_AI_SET_AVOID_GOAL_TIME,
	ETBOTLIB_AI_ADD_AVOID_SPOT,
	ETBOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL,
	ETBOTLIB_AAS_PREDICT_ROUTE,
	ETBOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX,

	ETBOTLIB_PC_LOAD_SOURCE,
	ETBOTLIB_PC_FREE_SOURCE,
	ETBOTLIB_PC_READ_TOKEN,
	ETBOTLIB_PC_SOURCE_FILE_AND_LINE,
	ETBOTLIB_PC_UNREAD_TOKEN,

	ETPB_STAT_REPORT,

	// zinx
	ETG_SENDMESSAGE,
	ETG_MESSAGESTATUS,
	// -zinx
};

//
// functions exported by the game subsystem
//
enum
{
	ETGAME_INIT,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.

	ETGAME_SHUTDOWN,	// (void);

	ETGAME_CLIENT_CONNECT,	// ( int clientNum, qboolean firstTime, qboolean isBot );
	// return NULL if the client is allowed to connect, otherwise return
	// a text string with the reason for denial

	ETGAME_CLIENT_BEGIN,				// ( int clientNum );

	ETGAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum );

	ETGAME_CLIENT_DISCONNECT,			// ( int clientNum );

	ETGAME_CLIENT_COMMAND,			// ( int clientNum );

	ETGAME_CLIENT_THINK,				// ( int clientNum );

	ETGAME_RUN_FRAME,					// ( int levelTime );

	ETGAME_CONSOLE_COMMAND,			// ( void );
	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return false if the game doesn't recognize it as a command.

	ETGAME_SNAPSHOT_CALLBACK,			// ( int entityNum, int clientNum ); // return false if you don't want it to be added

	ETBOTAI_START_FRAME,				// ( int time );

	// Ridah, Cast AI
	ETBOT_VISIBLEFROMPOS,
	ETBOT_CHECKATTACKATPOS,
	// done.

	// zinx
	ETGAME_MESSAGERECEIVED,			// ( int cno, const char *buf, int buflen, int commandTime );
	// -zinx
};

#endif
