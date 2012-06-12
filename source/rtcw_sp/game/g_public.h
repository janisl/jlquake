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

#ifndef _WOLFSP_G_PUBLIC_H
#define _WOLFSP_G_PUBLIC_H

//
// system traps provided by the main engine
//
enum
{
	//============== general Quake services ==================

	WSG_PRINT,		// ( const char *string );
	// print message on the local console

	WSG_ERROR,		// ( const char *string );
	// abort the game

	WSG_ENDGAME,		// ( void );	//----(SA)	added
	// exit to main menu and start "endgame" menu

	WSG_MILLISECONDS,	// ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	// console variable interaction
	WSG_CVAR_REGISTER,	// ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	WSG_CVAR_UPDATE,	// ( vmCvar_t *vmCvar );
	WSG_CVAR_SET,		// ( const char *var_name, const char *value );
	WSG_CVAR_VARIABLE_INTEGER_VALUE,	// ( const char *var_name );

	WSG_CVAR_VARIABLE_STRING_BUFFER,	// ( const char *var_name, char *buffer, int bufsize );

	WSG_ARGC,			// ( void );
	// ClientCommand and ServerCommand parameter access

	WSG_ARGV,			// ( int n, char *buffer, int bufferLength );

	WSG_FS_FOPEN_FILE,	// ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	WSG_FS_READ,		// ( void *buffer, int len, fileHandle_t f );
	WSG_FS_WRITE,		// ( const void *buffer, int len, fileHandle_t f );
	WSG_FS_RENAME,
	WSG_FS_FCLOSE_FILE,		// ( fileHandle_t f );

	WSG_SEND_CONSOLE_COMMAND,	// ( const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc


	//=========== server specific functionality =============

	WSG_LOCATE_GAME_DATA,		// ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							wsplayerState_t *clients, int sizeofGameClient );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface

	WSG_DROP_CLIENT,		// ( int clientNum, const char *reason );
	// kick a client off the server with a message

	WSG_SEND_SERVER_COMMAND,	// ( int clientNum, const char *fmt, ... );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients

	WSG_SET_CONFIGSTRING,	// ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.

	WSG_GET_CONFIGSTRING,	// ( int num, char *buffer, int bufferSize );

	WSG_GET_USERINFO,		// ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset

	WSG_SET_USERINFO,		// ( int num, const char *buffer );

	WSG_GET_SERVERINFO,	// ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers

	WSG_SET_BRUSH_MODEL,	// ( gentity_t *ent, const char *name );
	// sets mins and maxs based on the brushmodel name

	WSG_TRACE,	// ( q3trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities

	WSG_POINT_CONTENTS,	// ( const vec3_t point, int passEntityNum );
	// point contents against all linked entities

	WSG_IN_PVS,			// ( const vec3_t p1, const vec3_t p2 );

	WSG_IN_PVS_IGNORE_PORTALS,	// ( const vec3_t p1, const vec3_t p2 );

	WSG_ADJUST_AREA_PORTAL_STATE,	// ( gentity_t *ent, qboolean open );

	WSG_AREAS_CONNECTED,	// ( int area1, int area2 );

	WSG_LINKENTITY,		// ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.

	WSG_UNLINKENTITY,		// ( gentity_t *ent );
	// call before removing an interactive entity

	WSG_ENTITIES_IN_BOX,	// ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact

	WSG_ENTITY_CONTACT,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape

	// access for bots to get and free a server client (FIXME?)
	WSG_BOT_ALLOCATE_CLIENT,	// ( void );

	WSG_BOT_FREE_CLIENT,	// ( int clientNum );

	WSG_GET_USERCMD,	// ( int clientNum, wsusercmd_t *cmd )

	WSG_GET_ENTITY_TOKEN,	// qboolean ( char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.
	// This should only be done at GAME_INIT time.

	WSG_FS_GETFILELIST,
	WSG_DEBUG_POLYGON_CREATE,
	WSG_DEBUG_POLYGON_DELETE,
	WSG_REAL_TIME,
	WSG_SNAPVECTOR,
// MrE:

	WSG_TRACECAPSULE,	// ( q3trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection using capsule against all linked entities

	WSG_ENTITY_CONTACTCAPSULE,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape
// done.

	WSG_GETTAG,

	WSBOTLIB_SETUP = 200,				// ( void );
	WSBOTLIB_SHUTDOWN,				// ( void );
	WSBOTLIB_LIBVAR_SET,
	WSBOTLIB_LIBVAR_GET,
	WSBOTLIB_PC_ADD_GLOBAL_DEFINE,
	WSBOTLIB_START_FRAME,
	WSBOTLIB_LOAD_MAP,
	WSBOTLIB_UPDATENTITY,
	WSBOTLIB_TEST,

	WSBOTLIB_GET_SNAPSHOT_ENTITY,		// ( int client, int ent );
	WSBOTLIB_GET_CONSOLE_MESSAGE,		// ( int client, char *message, int size );
	WSBOTLIB_USER_COMMAND,			// ( int client, wsusercmd_t *ucmd );

	WSBOTLIB_AAS_ENTITY_VISIBLE = 300,	//FIXME: remove
	WSBOTLIB_AAS_IN_FIELD_OF_VISION,		//FIXME: remove
	WSBOTLIB_AAS_VISIBLE_CLIENTS,			//FIXME: remove
	WSBOTLIB_AAS_ENTITY_INFO,

	WSBOTLIB_AAS_INITIALIZED,
	WSBOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX,
	WSBOTLIB_AAS_TIME,

	// Ridah
	WSBOTLIB_AAS_SETCURRENTWORLD,
	// done.

	WSBOTLIB_AAS_POINT_AREA_NUM,
	WSBOTLIB_AAS_TRACE_AREAS,

	WSBOTLIB_AAS_POINT_CONTENTS,
	WSBOTLIB_AAS_NEXT_BSP_ENTITY,
	WSBOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY,
	WSBOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY,
	WSBOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY,
	WSBOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY,

	WSBOTLIB_AAS_AREA_REACHABILITY,

	WSBOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA,

	WSBOTLIB_AAS_SWIMMING,
	WSBOTLIB_AAS_PREDICT_CLIENT_MOVEMENT,

	// Ridah, route-tables
	WSBOTLIB_AAS_RT_SHOWROUTE,
	WSBOTLIB_AAS_RT_GETHIDEPOS,
	WSBOTLIB_AAS_FINDATTACKSPOTWITHINRANGE,
	WSBOTLIB_AAS_GETROUTEFIRSTVISPOS,
	WSBOTLIB_AAS_SETAASBLOCKINGENTITY,
	// done.

	WSBOTLIB_EA_SAY = 400,
	WSBOTLIB_EA_SAY_TEAM,
	WSBOTLIB_EA_USE_ITEM,
	WSBOTLIB_EA_DROP_ITEM,
	WSBOTLIB_EA_USE_INV,
	WSBOTLIB_EA_DROP_INV,
	WSBOTLIB_EA_GESTURE,
	WSBOTLIB_EA_COMMAND,

	WSBOTLIB_EA_SELECT_WEAPON,
	WSBOTLIB_EA_TALK,
	WSBOTLIB_EA_ATTACK,
	WSBOTLIB_EA_RELOAD,
	WSBOTLIB_EA_USE,
	WSBOTLIB_EA_RESPAWN,
	WSBOTLIB_EA_JUMP,
	WSBOTLIB_EA_DELAYED_JUMP,
	WSBOTLIB_EA_CROUCH,
	WSBOTLIB_EA_MOVE_UP,
	WSBOTLIB_EA_MOVE_DOWN,
	WSBOTLIB_EA_MOVE_FORWARD,
	WSBOTLIB_EA_MOVE_BACK,
	WSBOTLIB_EA_MOVE_LEFT,
	WSBOTLIB_EA_MOVE_RIGHT,
	WSBOTLIB_EA_MOVE,
	WSBOTLIB_EA_VIEW,

	WSBOTLIB_EA_END_REGULAR,
	WSBOTLIB_EA_GET_INPUT,
	WSBOTLIB_EA_RESET_INPUT,


	WSBOTLIB_AI_LOAD_CHARACTER = 500,
	WSBOTLIB_AI_FREE_CHARACTER,
	WSBOTLIB_AI_CHARACTERISTIC_FLOAT,
	WSBOTLIB_AI_CHARACTERISTIC_BFLOAT,
	WSBOTLIB_AI_CHARACTERISTIC_INTEGER,
	WSBOTLIB_AI_CHARACTERISTIC_BINTEGER,
	WSBOTLIB_AI_CHARACTERISTIC_STRING,

	WSBOTLIB_AI_ALLOC_CHAT_STATE,
	WSBOTLIB_AI_FREE_CHAT_STATE,
	WSBOTLIB_AI_QUEUE_CONSOLE_MESSAGE,
	WSBOTLIB_AI_REMOVE_CONSOLE_MESSAGE,
	WSBOTLIB_AI_NEXT_CONSOLE_MESSAGE,
	WSBOTLIB_AI_NUM_CONSOLE_MESSAGE,
	WSBOTLIB_AI_INITIAL_CHAT,
	WSBOTLIB_AI_REPLY_CHAT,
	WSBOTLIB_AI_CHAT_LENGTH,
	WSBOTLIB_AI_ENTER_CHAT,
	WSBOTLIB_AI_STRING_CONTAINS,
	WSBOTLIB_AI_FIND_MATCH,
	WSBOTLIB_AI_MATCH_VARIABLE,
	WSBOTLIB_AI_UNIFY_WHITE_SPACES,
	WSBOTLIB_AI_REPLACE_SYNONYMS,
	WSBOTLIB_AI_LOAD_CHAT_FILE,
	WSBOTLIB_AI_SET_CHAT_GENDER,
	WSBOTLIB_AI_SET_CHAT_NAME,

	WSBOTLIB_AI_RESET_GOAL_STATE,
	WSBOTLIB_AI_RESET_AVOID_GOALS,
	WSBOTLIB_AI_PUSH_GOAL,
	WSBOTLIB_AI_POP_GOAL,
	WSBOTLIB_AI_EMPTY_GOAL_STACK,
	WSBOTLIB_AI_DUMP_AVOID_GOALS,
	WSBOTLIB_AI_DUMP_GOAL_STACK,
	WSBOTLIB_AI_GOAL_NAME,
	WSBOTLIB_AI_GET_TOP_GOAL,
	WSBOTLIB_AI_GET_SECOND_GOAL,
	WSBOTLIB_AI_CHOOSE_LTG_ITEM,
	WSBOTLIB_AI_CHOOSE_NBG_ITEM,
	WSBOTLIB_AI_TOUCHING_GOAL,
	WSBOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE,
	WSBOTLIB_AI_GET_LEVEL_ITEM_GOAL,
	WSBOTLIB_AI_AVOID_GOAL_TIME,
	WSBOTLIB_AI_INIT_LEVEL_ITEMS,
	WSBOTLIB_AI_UPDATE_ENTITY_ITEMS,
	WSBOTLIB_AI_LOAD_ITEM_WEIGHTS,
	WSBOTLIB_AI_FREE_ITEM_WEIGHTS,
	WSBOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC,
	WSBOTLIB_AI_ALLOC_GOAL_STATE,
	WSBOTLIB_AI_FREE_GOAL_STATE,

	WSBOTLIB_AI_RESET_MOVE_STATE,
	WSBOTLIB_AI_MOVE_TO_GOAL,
	WSBOTLIB_AI_MOVE_IN_DIRECTION,
	WSBOTLIB_AI_RESET_AVOID_REACH,
	WSBOTLIB_AI_RESET_LAST_AVOID_REACH,
	WSBOTLIB_AI_REACHABILITY_AREA,
	WSBOTLIB_AI_MOVEMENT_VIEW_TARGET,
	WSBOTLIB_AI_ALLOC_MOVE_STATE,
	WSBOTLIB_AI_FREE_MOVE_STATE,
	WSBOTLIB_AI_INIT_MOVE_STATE,
	// Ridah
	WSBOTLIB_AI_INIT_AVOID_REACH,
	// done.

	WSBOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON,
	WSBOTLIB_AI_GET_WEAPON_INFO,
	WSBOTLIB_AI_LOAD_WEAPON_WEIGHTS,
	WSBOTLIB_AI_ALLOC_WEAPON_STATE,
	WSBOTLIB_AI_FREE_WEAPON_STATE,
	WSBOTLIB_AI_RESET_WEAPON_STATE,

	WSBOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION,
	WSBOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC,
	WSBOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC,
	WSBOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL,
	WSBOTLIB_AI_GET_MAP_LOCATION_GOAL,
	WSBOTLIB_AI_NUM_INITIAL_CHATS,
	WSBOTLIB_AI_GET_CHAT_MESSAGE,
	WSBOTLIB_AI_REMOVE_FROM_AVOID_GOALS,
	WSBOTLIB_AI_PREDICT_VISIBLE_POSITION,

	WSBOTLIB_AI_SET_AVOID_GOAL_TIME,
	WSBOTLIB_AI_ADD_AVOID_SPOT,
	WSBOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL,
	WSBOTLIB_AAS_PREDICT_ROUTE,
	WSBOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX,

	WSBOTLIB_PC_LOAD_SOURCE,
	WSBOTLIB_PC_FREE_SOURCE,
	WSBOTLIB_PC_READ_TOKEN,
	WSBOTLIB_PC_SOURCE_FILE_AND_LINE,

	WSG_FS_COPY_FILE	//DAJ
};

//
// functions exported by the game subsystem
//
enum
{
	WSGAME_INIT,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.

	WSGAME_SHUTDOWN,	// (void);

	WSGAME_CLIENT_CONNECT,	// ( int clientNum, qboolean firstTime, qboolean isBot );
	// return NULL if the client is allowed to connect, otherwise return
	// a text string with the reason for denial

	WSGAME_CLIENT_BEGIN,				// ( int clientNum );

	WSGAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum );

	WSGAME_CLIENT_DISCONNECT,			// ( int clientNum );

	WSGAME_CLIENT_COMMAND,			// ( int clientNum );

	WSGAME_CLIENT_THINK,				// ( int clientNum );

	WSGAME_RUN_FRAME,					// ( int levelTime );

	WSGAME_CONSOLE_COMMAND,			// ( void );
	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return qfalse if the game doesn't recognize it as a command.

	WSBOTAI_START_FRAME,				// ( int time );

	// Ridah, Cast AI
	WSAICAST_VISIBLEFROMPOS,
	WSAICAST_CHECKATTACKATPOS,

	WSGAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT,
	WSGAME_GETMODELINFO
};

#endif
