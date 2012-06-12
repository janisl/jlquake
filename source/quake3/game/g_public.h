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

#ifndef _QUAKE3_G_PUBLIC_H
#define _QUAKE3_G_PUBLIC_H

//
// system traps provided by the main engine
//
enum
{
	//============== general Quake services ==================

	Q3G_PRINT,		// ( const char *string );
	// print message on the local console

	Q3G_ERROR,		// ( const char *string );
	// abort the game

	Q3G_MILLISECONDS,	// ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	// console variable interaction
	Q3G_CVAR_REGISTER,	// ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	Q3G_CVAR_UPDATE,	// ( vmCvar_t *vmCvar );
	Q3G_CVAR_SET,		// ( const char *var_name, const char *value );
	Q3G_CVAR_VARIABLE_INTEGER_VALUE,	// ( const char *var_name );

	Q3G_CVAR_VARIABLE_STRING_BUFFER,	// ( const char *var_name, char *buffer, int bufsize );

	Q3G_ARGC,			// ( void );
	// ClientCommand and ServerCommand parameter access

	Q3G_ARGV,			// ( int n, char *buffer, int bufferLength );

	Q3G_FS_FOPEN_FILE,	// ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	Q3G_FS_READ,		// ( void *buffer, int len, fileHandle_t f );
	Q3G_FS_WRITE,		// ( const void *buffer, int len, fileHandle_t f );
	Q3G_FS_FCLOSE_FILE,		// ( fileHandle_t f );

	Q3G_SEND_CONSOLE_COMMAND,	// ( const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc


	//=========== server specific functionality =============

	Q3G_LOCATE_GAME_DATA,		// ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							q3playerState_t *clients, int sizeofGameClient );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface

	Q3G_DROP_CLIENT,		// ( int clientNum, const char *reason );
	// kick a client off the server with a message

	Q3G_SEND_SERVER_COMMAND,	// ( int clientNum, const char *fmt, ... );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients

	Q3G_SET_CONFIGSTRING,	// ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.

	Q3G_GET_CONFIGSTRING,	// ( int num, char *buffer, int bufferSize );

	Q3G_GET_USERINFO,		// ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset

	Q3G_SET_USERINFO,		// ( int num, const char *buffer );

	Q3G_GET_SERVERINFO,	// ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers

	Q3G_SET_BRUSH_MODEL,	// ( gentity_t *ent, const char *name );
	// sets mins and maxs based on the brushmodel name

	Q3G_TRACE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities

	Q3G_POINT_CONTENTS,	// ( const vec3_t point, int passEntityNum );
	// point contents against all linked entities

	Q3G_IN_PVS,			// ( const vec3_t p1, const vec3_t p2 );

	Q3G_IN_PVS_IGNORE_PORTALS,	// ( const vec3_t p1, const vec3_t p2 );

	Q3G_ADJUST_AREA_PORTAL_STATE,	// ( gentity_t *ent, qboolean open );

	Q3G_AREAS_CONNECTED,	// ( int area1, int area2 );

	Q3G_LINKENTITY,		// ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.

	Q3G_UNLINKENTITY,		// ( gentity_t *ent );
	// call before removing an interactive entity

	Q3G_ENTITIES_IN_BOX,	// ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact

	Q3G_ENTITY_CONTACT,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape

	// access for bots to get and free a server client (FIXME?)
	Q3G_BOT_ALLOCATE_CLIENT,	// ( void );

	Q3G_BOT_FREE_CLIENT,	// ( int clientNum );

	Q3G_GET_USERCMD,	// ( int clientNum, q3usercmd_t *cmd )

	Q3G_GET_ENTITY_TOKEN,	// qboolean ( char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.
	// This should only be done at GAME_INIT time.

	Q3G_FS_GETFILELIST,
	Q3G_DEBUG_POLYGON_CREATE,
	Q3G_DEBUG_POLYGON_DELETE,
	Q3G_REAL_TIME,
	Q3G_SNAPVECTOR,

	Q3G_TRACECAPSULE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	Q3G_ENTITY_CONTACTCAPSULE,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );

	// 1.32
	Q3G_FS_SEEK,

	Q3BOTLIB_SETUP = 200,				// ( void );
	Q3BOTLIB_SHUTDOWN,				// ( void );
	Q3BOTLIB_LIBVAR_SET,
	Q3BOTLIB_LIBVAR_GET,
	Q3BOTLIB_PC_ADD_GLOBAL_DEFINE,
	Q3BOTLIB_START_FRAME,
	Q3BOTLIB_LOAD_MAP,
	Q3BOTLIB_UPDATENTITY,
	Q3BOTLIB_TEST,

	Q3BOTLIB_GET_SNAPSHOT_ENTITY,		// ( int client, int ent );
	Q3BOTLIB_GET_CONSOLE_MESSAGE,		// ( int client, char *message, int size );
	Q3BOTLIB_USER_COMMAND,			// ( int client, q3usercmd_t *ucmd );

	Q3BOTLIB_AAS_ENABLE_ROUTING_AREA = 300,
	Q3BOTLIB_AAS_BBOX_AREAS,
	Q3BOTLIB_AAS_AREA_INFO,
	Q3BOTLIB_AAS_ENTITY_INFO,

	Q3BOTLIB_AAS_INITIALIZED,
	Q3BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX,
	Q3BOTLIB_AAS_TIME,

	Q3BOTLIB_AAS_POINT_AREA_NUM,
	Q3BOTLIB_AAS_TRACE_AREAS,

	Q3BOTLIB_AAS_POINT_CONTENTS,
	Q3BOTLIB_AAS_NEXT_BSP_ENTITY,
	Q3BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY,
	Q3BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY,
	Q3BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY,
	Q3BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY,

	Q3BOTLIB_AAS_AREA_REACHABILITY,

	Q3BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA,

	Q3BOTLIB_AAS_SWIMMING,
	Q3BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT,

	Q3BOTLIB_EA_SAY = 400,
	Q3BOTLIB_EA_SAY_TEAM,
	Q3BOTLIB_EA_COMMAND,

	Q3BOTLIB_EA_ACTION,
	Q3BOTLIB_EA_GESTURE,
	Q3BOTLIB_EA_TALK,
	Q3BOTLIB_EA_ATTACK,
	Q3BOTLIB_EA_USE,
	Q3BOTLIB_EA_RESPAWN,
	Q3BOTLIB_EA_CROUCH,
	Q3BOTLIB_EA_MOVE_UP,
	Q3BOTLIB_EA_MOVE_DOWN,
	Q3BOTLIB_EA_MOVE_FORWARD,
	Q3BOTLIB_EA_MOVE_BACK,
	Q3BOTLIB_EA_MOVE_LEFT,
	Q3BOTLIB_EA_MOVE_RIGHT,

	Q3BOTLIB_EA_SELECT_WEAPON,
	Q3BOTLIB_EA_JUMP,
	Q3BOTLIB_EA_DELAYED_JUMP,
	Q3BOTLIB_EA_MOVE,
	Q3BOTLIB_EA_VIEW,

	Q3BOTLIB_EA_END_REGULAR,
	Q3BOTLIB_EA_GET_INPUT,
	Q3BOTLIB_EA_RESET_INPUT,


	Q3BOTLIB_AI_LOAD_CHARACTER = 500,
	Q3BOTLIB_AI_FREE_CHARACTER,
	Q3BOTLIB_AI_CHARACTERISTIC_FLOAT,
	Q3BOTLIB_AI_CHARACTERISTIC_BFLOAT,
	Q3BOTLIB_AI_CHARACTERISTIC_INTEGER,
	Q3BOTLIB_AI_CHARACTERISTIC_BINTEGER,
	Q3BOTLIB_AI_CHARACTERISTIC_STRING,

	Q3BOTLIB_AI_ALLOC_CHAT_STATE,
	Q3BOTLIB_AI_FREE_CHAT_STATE,
	Q3BOTLIB_AI_QUEUE_CONSOLE_MESSAGE,
	Q3BOTLIB_AI_REMOVE_CONSOLE_MESSAGE,
	Q3BOTLIB_AI_NEXT_CONSOLE_MESSAGE,
	Q3BOTLIB_AI_NUM_CONSOLE_MESSAGE,
	Q3BOTLIB_AI_INITIAL_CHAT,
	Q3BOTLIB_AI_REPLY_CHAT,
	Q3BOTLIB_AI_CHAT_LENGTH,
	Q3BOTLIB_AI_ENTER_CHAT,
	Q3BOTLIB_AI_STRING_CONTAINS,
	Q3BOTLIB_AI_FIND_MATCH,
	Q3BOTLIB_AI_MATCH_VARIABLE,
	Q3BOTLIB_AI_UNIFY_WHITE_SPACES,
	Q3BOTLIB_AI_REPLACE_SYNONYMS,
	Q3BOTLIB_AI_LOAD_CHAT_FILE,
	Q3BOTLIB_AI_SET_CHAT_GENDER,
	Q3BOTLIB_AI_SET_CHAT_NAME,

	Q3BOTLIB_AI_RESET_GOAL_STATE,
	Q3BOTLIB_AI_RESET_AVOID_GOALS,
	Q3BOTLIB_AI_PUSH_GOAL,
	Q3BOTLIB_AI_POP_GOAL,
	Q3BOTLIB_AI_EMPTY_GOAL_STACK,
	Q3BOTLIB_AI_DUMP_AVOID_GOALS,
	Q3BOTLIB_AI_DUMP_GOAL_STACK,
	Q3BOTLIB_AI_GOAL_NAME,
	Q3BOTLIB_AI_GET_TOP_GOAL,
	Q3BOTLIB_AI_GET_SECOND_GOAL,
	Q3BOTLIB_AI_CHOOSE_LTG_ITEM,
	Q3BOTLIB_AI_CHOOSE_NBG_ITEM,
	Q3BOTLIB_AI_TOUCHING_GOAL,
	Q3BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE,
	Q3BOTLIB_AI_GET_LEVEL_ITEM_GOAL,
	Q3BOTLIB_AI_AVOID_GOAL_TIME,
	Q3BOTLIB_AI_INIT_LEVEL_ITEMS,
	Q3BOTLIB_AI_UPDATE_ENTITY_ITEMS,
	Q3BOTLIB_AI_LOAD_ITEM_WEIGHTS,
	Q3BOTLIB_AI_FREE_ITEM_WEIGHTS,
	Q3BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC,
	Q3BOTLIB_AI_ALLOC_GOAL_STATE,
	Q3BOTLIB_AI_FREE_GOAL_STATE,

	Q3BOTLIB_AI_RESET_MOVE_STATE,
	Q3BOTLIB_AI_MOVE_TO_GOAL,
	Q3BOTLIB_AI_MOVE_IN_DIRECTION,
	Q3BOTLIB_AI_RESET_AVOID_REACH,
	Q3BOTLIB_AI_RESET_LAST_AVOID_REACH,
	Q3BOTLIB_AI_REACHABILITY_AREA,
	Q3BOTLIB_AI_MOVEMENT_VIEW_TARGET,
	Q3BOTLIB_AI_ALLOC_MOVE_STATE,
	Q3BOTLIB_AI_FREE_MOVE_STATE,
	Q3BOTLIB_AI_INIT_MOVE_STATE,

	Q3BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON,
	Q3BOTLIB_AI_GET_WEAPON_INFO,
	Q3BOTLIB_AI_LOAD_WEAPON_WEIGHTS,
	Q3BOTLIB_AI_ALLOC_WEAPON_STATE,
	Q3BOTLIB_AI_FREE_WEAPON_STATE,
	Q3BOTLIB_AI_RESET_WEAPON_STATE,

	Q3BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION,
	Q3BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC,
	Q3BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC,
	Q3BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL,
	Q3BOTLIB_AI_GET_MAP_LOCATION_GOAL,
	Q3BOTLIB_AI_NUM_INITIAL_CHATS,
	Q3BOTLIB_AI_GET_CHAT_MESSAGE,
	Q3BOTLIB_AI_REMOVE_FROM_AVOID_GOALS,
	Q3BOTLIB_AI_PREDICT_VISIBLE_POSITION,

	Q3BOTLIB_AI_SET_AVOID_GOAL_TIME,
	Q3BOTLIB_AI_ADD_AVOID_SPOT,
	Q3BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL,
	Q3BOTLIB_AAS_PREDICT_ROUTE,
	Q3BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX,

	Q3BOTLIB_PC_LOAD_SOURCE,
	Q3BOTLIB_PC_FREE_SOURCE,
	Q3BOTLIB_PC_READ_TOKEN,
	Q3BOTLIB_PC_SOURCE_FILE_AND_LINE

};

//
// functions exported by the game subsystem
//
enum
{
	Q3GAME_INIT,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.

	Q3GAME_SHUTDOWN,	// (void);

	Q3GAME_CLIENT_CONNECT,	// ( int clientNum, qboolean firstTime, qboolean isBot );
	// return NULL if the client is allowed to connect, otherwise return
	// a text string with the reason for denial

	Q3GAME_CLIENT_BEGIN,				// ( int clientNum );

	Q3GAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum );

	Q3GAME_CLIENT_DISCONNECT,			// ( int clientNum );

	Q3GAME_CLIENT_COMMAND,			// ( int clientNum );

	Q3GAME_CLIENT_THINK,				// ( int clientNum );

	Q3GAME_RUN_FRAME,					// ( int levelTime );

	Q3GAME_CONSOLE_COMMAND,			// ( void );
	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return qfalse if the game doesn't recognize it as a command.

	Q3BOTAI_START_FRAME				// ( int time );
};

#endif
