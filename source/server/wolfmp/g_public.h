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

#ifndef _WOLFMP_G_PUBLIC_H
#define _WOLFMP_G_PUBLIC_H

//
// system traps provided by the main engine
//
enum
{
	//============== general Quake services ==================

	WMG_PRINT,		// ( const char *string );
	// print message on the local console

	WMG_ERROR,		// ( const char *string );
	// abort the game

	WMG_MILLISECONDS,	// ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	// console variable interaction
	WMG_CVAR_REGISTER,	// ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	WMG_CVAR_UPDATE,	// ( vmCvar_t *vmCvar );
	WMG_CVAR_SET,		// ( const char *var_name, const char *value );
	WMG_CVAR_VARIABLE_INTEGER_VALUE,	// ( const char *var_name );

	WMG_CVAR_VARIABLE_STRING_BUFFER,	// ( const char *var_name, char *buffer, int bufsize );

	WMG_ARGC,			// ( void );
	// ClientCommand and ServerCommand parameter access

	WMG_ARGV,			// ( int n, char *buffer, int bufferLength );

	WMG_FS_FOPEN_FILE,	// ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	WMG_FS_READ,		// ( void *buffer, int len, fileHandle_t f );
	WMG_FS_WRITE,		// ( const void *buffer, int len, fileHandle_t f );
	WMG_FS_RENAME,
	WMG_FS_FCLOSE_FILE,		// ( fileHandle_t f );

	WMG_SEND_CONSOLE_COMMAND,	// ( const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc


	//=========== server specific functionality =============

	WMG_LOCATE_GAME_DATA,		// ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							wmplayerState_t *clients, int sizeofGameClient );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface

	WMG_DROP_CLIENT,		// ( int clientNum, const char *reason );
	// kick a client off the server with a message

	WMG_SEND_SERVER_COMMAND,	// ( int clientNum, const char *fmt, ... );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients

	WMG_SET_CONFIGSTRING,	// ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.

	WMG_GET_CONFIGSTRING,	// ( int num, char *buffer, int bufferSize );

	WMG_GET_USERINFO,		// ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset

	WMG_SET_USERINFO,		// ( int num, const char *buffer );

	WMG_GET_SERVERINFO,	// ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers

	WMG_SET_BRUSH_MODEL,	// ( gentity_t *ent, const char *name );
	// sets mins and maxs based on the brushmodel name

	WMG_TRACE,	// ( q3trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities

	WMG_POINT_CONTENTS,	// ( const vec3_t point, int passEntityNum );
	// point contents against all linked entities

	WMG_IN_PVS,			// ( const vec3_t p1, const vec3_t p2 );

	WMG_IN_PVS_IGNORE_PORTALS,	// ( const vec3_t p1, const vec3_t p2 );

	WMG_ADJUST_AREA_PORTAL_STATE,	// ( gentity_t *ent, qboolean open );

	WMG_AREAS_CONNECTED,	// ( int area1, int area2 );

	WMG_LINKENTITY,		// ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.

	WMG_UNLINKENTITY,		// ( gentity_t *ent );
	// call before removing an interactive entity

	WMG_ENTITIES_IN_BOX,	// ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact

	WMG_ENTITY_CONTACT,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape

	// access for bots to get and free a server client (FIXME?)
	WMG_BOT_ALLOCATE_CLIENT,	// ( void );

	WMG_BOT_FREE_CLIENT,	// ( int clientNum );

	WMG_GET_USERCMD,	// ( int clientNum, wmusercmd_t *cmd )

	WMG_GET_ENTITY_TOKEN,	// qboolean ( char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.
	// This should only be done at GAME_INIT time.

	WMG_FS_GETFILELIST,
	WMG_DEBUG_POLYGON_CREATE,
	WMG_DEBUG_POLYGON_DELETE,
	WMG_REAL_TIME,
	WMG_SNAPVECTOR,
// MrE:

	WMG_TRACECAPSULE,	// ( q3trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection using capsule against all linked entities

	WMG_ENTITY_CONTACTCAPSULE,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape
// done.

	WMG_GETTAG,

	WMBOTLIB_SETUP = 200,				// ( void );
	WMBOTLIB_SHUTDOWN,				// ( void );
	WMBOTLIB_LIBVAR_SET,
	WMBOTLIB_LIBVAR_GET,
	WMBOTLIB_PC_ADD_GLOBAL_DEFINE,
	WMBOTLIB_START_FRAME,
	WMBOTLIB_LOAD_MAP,
	WMBOTLIB_UPDATENTITY,
	WMBOTLIB_TEST,

	WMBOTLIB_GET_SNAPSHOT_ENTITY,		// ( int client, int ent );
	WMBOTLIB_GET_CONSOLE_MESSAGE,		// ( int client, char *message, int size );
	WMBOTLIB_USER_COMMAND,			// ( int client, wmusercmd_t *ucmd );

	WMBOTLIB_AAS_ENTITY_VISIBLE = 300,	//FIXME: remove
	WMBOTLIB_AAS_IN_FIELD_OF_VISION,		//FIXME: remove
	WMBOTLIB_AAS_VISIBLE_CLIENTS,			//FIXME: remove
	WMBOTLIB_AAS_ENTITY_INFO,

	WMBOTLIB_AAS_INITIALIZED,
	WMBOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX,
	WMBOTLIB_AAS_TIME,

	// Ridah
	WMBOTLIB_AAS_SETCURRENTWORLD,
	// done.

	WMBOTLIB_AAS_POINT_AREA_NUM,
	WMBOTLIB_AAS_TRACE_AREAS,

	WMBOTLIB_AAS_POINT_CONTENTS,
	WMBOTLIB_AAS_NEXT_BSP_ENTITY,
	WMBOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY,
	WMBOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY,
	WMBOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY,
	WMBOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY,

	WMBOTLIB_AAS_AREA_REACHABILITY,

	WMBOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA,

	WMBOTLIB_AAS_SWIMMING,
	WMBOTLIB_AAS_PREDICT_CLIENT_MOVEMENT,

	// Ridah, route-tables
	WMBOTLIB_AAS_RT_SHOWROUTE,
	WMBOTLIB_AAS_RT_GETHIDEPOS,
	WMBOTLIB_AAS_FINDATTACKSPOTWITHINRANGE,
	WMBOTLIB_AAS_SETAASBLOCKINGENTITY,
	// done.

	WMBOTLIB_EA_SAY = 400,
	WMBOTLIB_EA_SAY_TEAM,
	WMBOTLIB_EA_USE_ITEM,
	WMBOTLIB_EA_DROP_ITEM,
	WMBOTLIB_EA_USE_INV,
	WMBOTLIB_EA_DROP_INV,
	WMBOTLIB_EA_GESTURE,
	WMBOTLIB_EA_COMMAND,

	WMBOTLIB_EA_SELECT_WEAPON,
	WMBOTLIB_EA_TALK,
	WMBOTLIB_EA_ATTACK,
	WMBOTLIB_EA_RELOAD,
	WMBOTLIB_EA_USE,
	WMBOTLIB_EA_RESPAWN,
	WMBOTLIB_EA_JUMP,
	WMBOTLIB_EA_DELAYED_JUMP,
	WMBOTLIB_EA_CROUCH,
	WMBOTLIB_EA_MOVE_UP,
	WMBOTLIB_EA_MOVE_DOWN,
	WMBOTLIB_EA_MOVE_FORWARD,
	WMBOTLIB_EA_MOVE_BACK,
	WMBOTLIB_EA_MOVE_LEFT,
	WMBOTLIB_EA_MOVE_RIGHT,
	WMBOTLIB_EA_MOVE,
	WMBOTLIB_EA_VIEW,

	WMBOTLIB_EA_END_REGULAR,
	WMBOTLIB_EA_GET_INPUT,
	WMBOTLIB_EA_RESET_INPUT,


	WMBOTLIB_AI_LOAD_CHARACTER = 500,
	WMBOTLIB_AI_FREE_CHARACTER,
	WMBOTLIB_AI_CHARACTERISTIC_FLOAT,
	WMBOTLIB_AI_CHARACTERISTIC_BFLOAT,
	WMBOTLIB_AI_CHARACTERISTIC_INTEGER,
	WMBOTLIB_AI_CHARACTERISTIC_BINTEGER,
	WMBOTLIB_AI_CHARACTERISTIC_STRING,

	WMBOTLIB_AI_ALLOC_CHAT_STATE,
	WMBOTLIB_AI_FREE_CHAT_STATE,
	WMBOTLIB_AI_QUEUE_CONSOLE_MESSAGE,
	WMBOTLIB_AI_REMOVE_CONSOLE_MESSAGE,
	WMBOTLIB_AI_NEXT_CONSOLE_MESSAGE,
	WMBOTLIB_AI_NUM_CONSOLE_MESSAGE,
	WMBOTLIB_AI_INITIAL_CHAT,
	WMBOTLIB_AI_REPLY_CHAT,
	WMBOTLIB_AI_CHAT_LENGTH,
	WMBOTLIB_AI_ENTER_CHAT,
	WMBOTLIB_AI_STRING_CONTAINS,
	WMBOTLIB_AI_FIND_MATCH,
	WMBOTLIB_AI_MATCH_VARIABLE,
	WMBOTLIB_AI_UNIFY_WHITE_SPACES,
	WMBOTLIB_AI_REPLACE_SYNONYMS,
	WMBOTLIB_AI_LOAD_CHAT_FILE,
	WMBOTLIB_AI_SET_CHAT_GENDER,
	WMBOTLIB_AI_SET_CHAT_NAME,

	WMBOTLIB_AI_RESET_GOAL_STATE,
	WMBOTLIB_AI_RESET_AVOID_GOALS,
	WMBOTLIB_AI_PUSH_GOAL,
	WMBOTLIB_AI_POP_GOAL,
	WMBOTLIB_AI_EMPTY_GOAL_STACK,
	WMBOTLIB_AI_DUMP_AVOID_GOALS,
	WMBOTLIB_AI_DUMP_GOAL_STACK,
	WMBOTLIB_AI_GOAL_NAME,
	WMBOTLIB_AI_GET_TOP_GOAL,
	WMBOTLIB_AI_GET_SECOND_GOAL,
	WMBOTLIB_AI_CHOOSE_LTG_ITEM,
	WMBOTLIB_AI_CHOOSE_NBG_ITEM,
	WMBOTLIB_AI_TOUCHING_GOAL,
	WMBOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE,
	WMBOTLIB_AI_GET_LEVEL_ITEM_GOAL,
	WMBOTLIB_AI_AVOID_GOAL_TIME,
	WMBOTLIB_AI_INIT_LEVEL_ITEMS,
	WMBOTLIB_AI_UPDATE_ENTITY_ITEMS,
	WMBOTLIB_AI_LOAD_ITEM_WEIGHTS,
	WMBOTLIB_AI_FREE_ITEM_WEIGHTS,
	WMBOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC,
	WMBOTLIB_AI_ALLOC_GOAL_STATE,
	WMBOTLIB_AI_FREE_GOAL_STATE,

	WMBOTLIB_AI_RESET_MOVE_STATE,
	WMBOTLIB_AI_MOVE_TO_GOAL,
	WMBOTLIB_AI_MOVE_IN_DIRECTION,
	WMBOTLIB_AI_RESET_AVOID_REACH,
	WMBOTLIB_AI_RESET_LAST_AVOID_REACH,
	WMBOTLIB_AI_REACHABILITY_AREA,
	WMBOTLIB_AI_MOVEMENT_VIEW_TARGET,
	WMBOTLIB_AI_ALLOC_MOVE_STATE,
	WMBOTLIB_AI_FREE_MOVE_STATE,
	WMBOTLIB_AI_INIT_MOVE_STATE,
	// Ridah
	WMBOTLIB_AI_INIT_AVOID_REACH,
	// done.

	WMBOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON,
	WMBOTLIB_AI_GET_WEAPON_INFO,
	WMBOTLIB_AI_LOAD_WEAPON_WEIGHTS,
	WMBOTLIB_AI_ALLOC_WEAPON_STATE,
	WMBOTLIB_AI_FREE_WEAPON_STATE,
	WMBOTLIB_AI_RESET_WEAPON_STATE,

	WMBOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION,
	WMBOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC,
	WMBOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC,
	WMBOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL,
	WMBOTLIB_AI_GET_MAP_LOCATION_GOAL,
	WMBOTLIB_AI_NUM_INITIAL_CHATS,
	WMBOTLIB_AI_GET_CHAT_MESSAGE,
	WMBOTLIB_AI_REMOVE_FROM_AVOID_GOALS,
	WMBOTLIB_AI_PREDICT_VISIBLE_POSITION,

	WMBOTLIB_AI_SET_AVOID_GOAL_TIME,
	WMBOTLIB_AI_ADD_AVOID_SPOT,
	WMBOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL,
	WMBOTLIB_AAS_PREDICT_ROUTE,
	WMBOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX,

	WMBOTLIB_PC_LOAD_SOURCE,
	WMBOTLIB_PC_FREE_SOURCE,
	WMBOTLIB_PC_READ_TOKEN,
	WMBOTLIB_PC_SOURCE_FILE_AND_LINE
};

//
// functions exported by the game subsystem
//
enum
{
	WMGAME_INIT,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.

	WMGAME_SHUTDOWN,	// (void);

	WMGAME_CLIENT_CONNECT,	// ( int clientNum, qboolean firstTime, qboolean isBot );
	// return NULL if the client is allowed to connect, otherwise return
	// a text string with the reason for denial

	WMGAME_CLIENT_BEGIN,				// ( int clientNum );

	WMGAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum );

	WMGAME_CLIENT_DISCONNECT,			// ( int clientNum );

	WMGAME_CLIENT_COMMAND,			// ( int clientNum );

	WMGAME_CLIENT_THINK,				// ( int clientNum );

	WMGAME_RUN_FRAME,					// ( int levelTime );

	WMGAME_CONSOLE_COMMAND,			// ( void );
	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return qfalse if the game doesn't recognize it as a command.

	WMBOTAI_START_FRAME,				// ( int time );

	// Ridah, Cast AI
	WMAICAST_VISIBLEFROMPOS,
	WMAICAST_CHECKATTACKATPOS,

	WMGAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT
};

#endif
