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

#define MAX_CLIENTS_WS         128		// absolute limit
#define MAX_CLIENTS_WM         64	// JPW NERVE back to q3ta default was 128		// absolute limit
#define MAX_CLIENTS_ET         64	// JPW NERVE back to q3ta default was 128		// absolute limit

#define MAX_CONFIGSTRINGS_WS   2048
#define MAX_CONFIGSTRINGS_WM   2048
#define MAX_CONFIGSTRINGS_ET   1024
#define BIGGEST_MAX_CONFIGSTRINGS_T3   2048

// wsusercmd_t is sent to the server each client frame
struct wsusercmd_t
{
	int serverTime;
	byte buttons;
	byte wbuttons;
	byte weapon;
	byte holdable;			//----(SA)	added
	int angles[3];


	signed char forwardmove, rightmove, upmove;
	signed char wolfkick;		// RF, we should move this over to a wbutton, this is a huge waste of bandwidth

	unsigned short cld;			// NERVE - SMF - send client damage in usercmd instead of as a server command
};

// wmusercmd_t is sent to the server each client frame
struct wmusercmd_t
{
	int serverTime;
	byte buttons;
	byte wbuttons;
	byte weapon;
	byte holdable;			//----(SA)	added
	int angles[3];

	signed char forwardmove, rightmove, upmove;
	signed char wolfkick;		// RF, we should move this over to a wbutton, this is a huge waste of bandwidth

	char mpSetup;				// NERVE - SMF
	char identClient;			// NERVE - SMF
};

// etusercmd_t is sent to the server each client frame
struct etusercmd_t
{
	int serverTime;
	byte buttons;
	byte wbuttons;
	byte weapon;
	byte flags;
	int angles[3];

	signed char forwardmove, rightmove, upmove;
	byte doubleTap;				// Arnout: only 3 bits used

	// rain - in ET, this can be any entity, and it's used as an array
	// index, so make sure it's unsigned
	byte identClient;			// NERVE - SMF
};

#define MAX_WEAPONS_WS             64	// (SA) and yet more!
#define MAX_WEAPONS_WM             64	// (SA) and yet more!
#define MAX_WEAPONS_ET             64	// (SA) and yet more!

#define MAX_HOLDABLE_WS            16

// ACK: I'd really like to make this 4, but that seems to cause network problems
#define MAX_EVENTS_WS              4	// max events per frame before we drop events
#define MAX_EVENTS_WM              4	// max events per frame before we drop events
#define MAX_EVENTS_ET              4	// max events per frame before we drop events

// wsplayerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// wsplayerState_t is a full superset of entityState_t as it is used by players,
// so if a wsplayerState_t is transmitted, the entityState_t can be fully derived
// from it.
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
struct wsplayerState_t
{
	int commandTime;			// cmd->serverTime of last executed command
	int pm_type;
	int bobCycle;				// for view bobbing and footstep generation
	int pm_flags;				// ducked, jump_held, etc
	int pm_time;

	vec3_t origin;
	vec3_t velocity;
	int weaponTime;
	int weaponDelay;			// for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...)
	int grenadeTimeLeft;			// for delayed grenade throwing.  this is set to a #define for grenade
									// lifetime when the attack button goes down, then when attack is released
									// this is the amount of time left before the grenade goes off (or if it
									// gets to 0 while in players hand, it explodes)


	int gravity;
	float leanf;				// amount of 'lean' when player is looking around corner //----(SA)	added

	int speed;
	int delta_angles[3];			// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	int groundEntityNum;		// Q3ENTITYNUM_NONE = in air

	int legsTimer;				// don't change low priority animations until this runs out
	int legsAnim;				// mask off ANIM_TOGGLEBIT

	int torsoTimer;				// don't change low priority animations until this runs out
	int torsoAnim;				// mask off ANIM_TOGGLEBIT

	int movementDir;			// a number 0 to 7 that represents the reletive angle
								// of movement to the view angle (axial and diagonals)
								// when at rest, the value will remain unchanged
								// used to twist the legs during strafing



	int eFlags;					// copied to entityState_t->eFlags

	int eventSequence;			// pmove generated events
	int events[MAX_EVENTS_WS];
	int eventParms[MAX_EVENTS_WS];
	int oldEventSequence;			// so we can see which events have been added since we last converted to entityState_t

	int externalEvent;			// events set on player from another source
	int externalEventParm;
	int externalEventTime;

	int clientNum;				// ranges from 0 to MAX_CLIENTS_WS-1

	// weapon info
	int weapon;					// copied to entityState_t->weapon
	int weaponstate;

	// item info
	int item;

	vec3_t viewangles;			// for fixed views
	int viewheight;

	// damage feedback
	int damageEvent;			// when it changes, latch the other parms
	int damageYaw;
	int damagePitch;
	int damageCount;

	int stats[MAX_STATS_Q3];
	int persistant[MAX_PERSISTANT_Q3];			// stats that aren't cleared on death
	int powerups[MAX_POWERUPS_Q3];			// level.time that the powerup runs out
	int ammo[MAX_WEAPONS_WS];				// total amount of ammo
	int ammoclip[MAX_WEAPONS_WS];			// ammo in clip
	int holdable[MAX_HOLDABLE_WS];
	int holding;						// the current item in holdable[] that is selected (held)
	int weapons[MAX_WEAPONS_WS / (sizeof(int) * 8)];		// 64 bits for weapons held

	// Ridah, allow for individual bounding boxes
	vec3_t mins, maxs;
	float crouchMaxZ;
	float crouchViewHeight, standViewHeight, deadViewHeight;
	// variable movement speed
	float runSpeedScale, sprintSpeedScale, crouchSpeedScale;
	// done.

	// Ridah, view locking for mg42
	int viewlocked;
	int viewlocked_entNum;

	// Ridah, need this to fix friction problems with slow zombie's whereby
	// the friction prevents them from accelerating to their full potential
	float friction;

	// Ridah, AI character id is used for weapon association
	int aiChar;
	int teamNum;

	// Rafael
	int gunfx;

	// RF, burning effect is required for view blending effect
	int onFireStart;

	int serverCursorHint;				// what type of cursor hint the server is dictating
	int serverCursorHintVal;			// a value (0-255) associated with the above

	q3trace_t serverCursorHintTrace;		// not communicated over net, but used to store the current server-side cursorhint trace

	// ----------------------------------------------------------------------
	// not communicated over the net at all
	// FIXME: this doesn't get saved between predicted frames on the clients-side (cg.predictedPlayerState)
	// So to use persistent variables here, which don't need to come from the server,
	// we could use a marker variable, and use that to store everything after it
	// before we read in the new values for the predictedPlayerState, then restore them
	// after copying the structure recieved from the server.

	// (SA) yeah.  this is causing me a little bit of trouble too.  can we go ahead with the above suggestion or find an alternative?

	int ping;					// server to game info for scoreboard
	int pmove_framecount;			// FIXME: don't transmit over the network
	int entityEventSequence;

	int sprintTime;
	int sprintExertTime;

	// JPW NERVE -- value for all multiplayer classes with regenerating "class weapons" -- ie LT artillery, medic medpack, engineer build points, etc
	int classWeaponTime;
	int jumpTime;			// used in SP/MP to prevent jump accel
	// jpw

	int weapAnimTimer;				// don't change low priority animations until this runs out
	int weapAnim;				// mask off ANIM_TOGGLEBIT

	qboolean releasedFire;

	float aimSpreadScaleFloat;			// (SA) the server-side aimspreadscale that lets it track finer changes but still only
										// transmit the 8bit int to the client
	int aimSpreadScale;			// 0 - 255 increases with angular movement
	int lastFireTime;			// used by server to hold last firing frame briefly when randomly releasing trigger (AI)

	int quickGrenTime;

	int leanStopDebounceTime;

	int weapHeat[MAX_WEAPONS_WS];			// some weapons can overheat.  this tracks (server-side) how hot each weapon currently is.
	int curWeapHeat;					// value for the currently selected weapon (for transmission to client)

	int venomTime;

//----(SA)	added
	int accShowBits;			// RF (changed from short), these should all be 32 bit
	int accHideBits;
//----(SA)	end

	int aiState;

	float footstepCount;
};

// wmplayerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// wmplayerState_t is a full superset of entityState_t as it is used by players,
// so if a wmplayerState_t is transmitted, the entityState_t can be fully derived
// from it.
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
struct wmplayerState_t
{
	int commandTime;			// cmd->serverTime of last executed command
	int pm_type;
	int bobCycle;				// for view bobbing and footstep generation
	int pm_flags;				// ducked, jump_held, etc
	int pm_time;

	vec3_t origin;
	vec3_t velocity;
	int weaponTime;
	int weaponDelay;			// for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...)
	int grenadeTimeLeft;			// for delayed grenade throwing.  this is set to a #define for grenade
									// lifetime when the attack button goes down, then when attack is released
									// this is the amount of time left before the grenade goes off (or if it
									// gets to 0 while in players hand, it explodes)


	int gravity;
	float leanf;				// amount of 'lean' when player is looking around corner //----(SA)	added

	int speed;
	int delta_angles[3];			// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	int groundEntityNum;		// Q3ENTITYNUM_NONE = in air

	int legsTimer;				// don't change low priority animations until this runs out
	int legsAnim;				// mask off ANIM_TOGGLEBIT

	int torsoTimer;				// don't change low priority animations until this runs out
	int torsoAnim;				// mask off ANIM_TOGGLEBIT

	int movementDir;			// a number 0 to 7 that represents the reletive angle
								// of movement to the view angle (axial and diagonals)
								// when at rest, the value will remain unchanged
								// used to twist the legs during strafing



	int eFlags;					// copied to entityState_t->eFlags

	int eventSequence;			// pmove generated events
	int events[MAX_EVENTS_WM];
	int eventParms[MAX_EVENTS_WM];
	int oldEventSequence;			// so we can see which events have been added since we last converted to entityState_t

	int externalEvent;			// events set on player from another source
	int externalEventParm;
	int externalEventTime;

	int clientNum;				// ranges from 0 to MAX_CLIENTS_WM-1

	// weapon info
	int weapon;					// copied to entityState_t->weapon
	int weaponstate;

	// item info
	int item;

	vec3_t viewangles;			// for fixed views
	int viewheight;

	// damage feedback
	int damageEvent;			// when it changes, latch the other parms
	int damageYaw;
	int damagePitch;
	int damageCount;

	int stats[MAX_STATS_Q3];
	int persistant[MAX_PERSISTANT_Q3];			// stats that aren't cleared on death
	int powerups[MAX_POWERUPS_Q3];			// level.time that the powerup runs out
	int ammo[MAX_WEAPONS_WM];				// total amount of ammo
	int ammoclip[MAX_WEAPONS_WM];			// ammo in clip
	int holdable[16];
	int holding;						// the current item in holdable[] that is selected (held)
	int weapons[MAX_WEAPONS_WM / (sizeof(int) * 8)];		// 64 bits for weapons held

	// Ridah, allow for individual bounding boxes
	vec3_t mins, maxs;
	float crouchMaxZ;
	float crouchViewHeight, standViewHeight, deadViewHeight;
	// variable movement speed
	float runSpeedScale, sprintSpeedScale, crouchSpeedScale;
	// done.

	// Ridah, view locking for mg42
	int viewlocked;
	int viewlocked_entNum;

	// Ridah, need this to fix friction problems with slow zombie's whereby
	// the friction prevents them from accelerating to their full potential
	float friction;

	// Ridah, AI character id is used for weapon association
	int aiChar;
	int teamNum;

	// Rafael
	int gunfx;

	// RF, burning effect is required for view blending effect
	int onFireStart;

	int serverCursorHint;				// what type of cursor hint the server is dictating
	int serverCursorHintVal;			// a value (0-255) associated with the above

	q3trace_t serverCursorHintTrace;		// not communicated over net, but used to store the current server-side cursorhint trace

	// ----------------------------------------------------------------------
	// not communicated over the net at all
	// FIXME: this doesn't get saved between predicted frames on the clients-side (cg.predictedPlayerState)
	// So to use persistent variables here, which don't need to come from the server,
	// we could use a marker variable, and use that to store everything after it
	// before we read in the new values for the predictedPlayerState, then restore them
	// after copying the structure recieved from the server.

	// (SA) yeah.  this is causing me a little bit of trouble too.  can we go ahead with the above suggestion or find an alternative?

	int ping;					// server to game info for scoreboard
	int pmove_framecount;			// FIXME: don't transmit over the network
	int entityEventSequence;

	int sprintTime;
	int sprintExertTime;

	// JPW NERVE -- value for all multiplayer classes with regenerating "class weapons" -- ie LT artillery, medic medpack, engineer build points, etc
	int classWeaponTime;
	int jumpTime;			// used in MP to prevent jump accel
	// jpw

	int weapAnimTimer;				// don't change low priority animations until this runs out		//----(SA)	added
	int weapAnim;				// mask off ANIM_TOGGLEBIT										//----(SA)	added

	qboolean releasedFire;

	float aimSpreadScaleFloat;			// (SA) the server-side aimspreadscale that lets it track finer changes but still only
										// transmit the 8bit int to the client
	int aimSpreadScale;			// 0 - 255 increases with angular movement
	int lastFireTime;			// used by server to hold last firing frame briefly when randomly releasing trigger (AI)

	int quickGrenTime;

	int leanStopDebounceTime;

//----(SA)	added

	// seems like heat and aimspread could be tied together somehow, however, they (appear to) change at different rates and
	// I can't currently see how to optimize this to one server->client transmission "weapstatus" value.
	int weapHeat[MAX_WEAPONS_WM];			// some weapons can overheat.  this tracks (server-side) how hot each weapon currently is.
	int curWeapHeat;					// value for the currently selected weapon (for transmission to client)

	int venomTime;			//----(SA)	added
//----(SA)	end

	int aiState;

	int identifyClient;					// NERVE - SMF
};

// etplayerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c
// (Gordon: unless it doesnt need transmitted over the network, in which case it should prolly go in the new pmext struct anyway)

// etplayerState_t is a full superset of entityState_t as it is used by players,
// so if a etplayerState_t is transmitted, the entityState_t can be fully derived
// from it.
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
struct etplayerState_t
{
	int commandTime;			// cmd->serverTime of last executed command
	int pm_type;
	int bobCycle;				// for view bobbing and footstep generation
	int pm_flags;				// ducked, jump_held, etc
	int pm_time;

	vec3_t origin;
	vec3_t velocity;
	int weaponTime;
	int weaponDelay;			// for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...)
	int grenadeTimeLeft;			// for delayed grenade throwing.  this is set to a #define for grenade
									// lifetime when the attack button goes down, then when attack is released
									// this is the amount of time left before the grenade goes off (or if it
									// gets to 0 while in players hand, it explodes)


	int gravity;
	float leanf;				// amount of 'lean' when player is looking around corner //----(SA)	added

	int speed;
	int delta_angles[3];			// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	int groundEntityNum;		// Q3ENTITYNUM_NONE = in air

	int legsTimer;				// don't change low priority animations until this runs out
	int legsAnim;				// mask off ANIM_TOGGLEBIT

	int torsoTimer;				// don't change low priority animations until this runs out
	int torsoAnim;				// mask off ANIM_TOGGLEBIT

	int movementDir;			// a number 0 to 7 that represents the reletive angle
								// of movement to the view angle (axial and diagonals)
								// when at rest, the value will remain unchanged
								// used to twist the legs during strafing



	int eFlags;					// copied to entityState_t->eFlags

	int eventSequence;			// pmove generated events
	int events[MAX_EVENTS_ET];
	int eventParms[MAX_EVENTS_ET];
	int oldEventSequence;			// so we can see which events have been added since we last converted to entityState_t

	int externalEvent;			// events set on player from another source
	int externalEventParm;
	int externalEventTime;

	int clientNum;				// ranges from 0 to MAX_CLIENTS_ET-1

	// weapon info
	int weapon;					// copied to entityState_t->weapon
	int weaponstate;

	// item info
	int item;

	vec3_t viewangles;			// for fixed views
	int viewheight;

	// damage feedback
	int damageEvent;			// when it changes, latch the other parms
	int damageYaw;
	int damagePitch;
	int damageCount;

	int stats[MAX_STATS_Q3];
	int persistant[MAX_PERSISTANT_Q3];			// stats that aren't cleared on death
	int powerups[MAX_POWERUPS_Q3];			// level.time that the powerup runs out
	int ammo[MAX_WEAPONS_ET];				// total amount of ammo
	int ammoclip[MAX_WEAPONS_ET];			// ammo in clip
	int holdable[16];
	int holding;						// the current item in holdable[] that is selected (held)
	int weapons[MAX_WEAPONS_ET / (sizeof(int) * 8)];		// 64 bits for weapons held

	// Ridah, allow for individual bounding boxes
	vec3_t mins, maxs;
	float crouchMaxZ;
	float crouchViewHeight, standViewHeight, deadViewHeight;
	// variable movement speed
	float runSpeedScale, sprintSpeedScale, crouchSpeedScale;
	// done.

	// Ridah, view locking for mg42
	int viewlocked;
	int viewlocked_entNum;

	float friction;

	int nextWeapon;
	int teamNum;						// Arnout: doesn't seem to be communicated over the net

	// Rafael
	//int			gunfx;

	// RF, burning effect is required for view blending effect
	int onFireStart;

	int serverCursorHint;				// what type of cursor hint the server is dictating
	int serverCursorHintVal;			// a value (0-255) associated with the above

	q3trace_t serverCursorHintTrace;		// not communicated over net, but used to store the current server-side cursorhint trace

	// ----------------------------------------------------------------------
	// So to use persistent variables here, which don't need to come from the server,
	// we could use a marker variable, and use that to store everything after it
	// before we read in the new values for the predictedPlayerState, then restore them
	// after copying the structure recieved from the server.

	// Arnout: use the pmoveExt_t structure in bg_public.h to store this kind of data now (presistant on client, not network transmitted)

	int ping;					// server to game info for scoreboard
	int pmove_framecount;
	int entityEventSequence;

	int sprintExertTime;

	// JPW NERVE -- value for all multiplayer classes with regenerating "class weapons" -- ie LT artillery, medic medpack, engineer build points, etc
	int classWeaponTime;				// Arnout : DOES get send over the network
	int jumpTime;					// used in MP to prevent jump accel
	// jpw

	int weapAnim;					// mask off ANIM_TOGGLEBIT										//----(SA)	added		// Arnout : DOES get send over the network

	qboolean releasedFire;

	float aimSpreadScaleFloat;			// (SA) the server-side aimspreadscale that lets it track finer changes but still only
										// transmit the 8bit int to the client
	int aimSpreadScale;					// 0 - 255 increases with angular movement		// Arnout : DOES get send over the network
	int lastFireTime;					// used by server to hold last firing frame briefly when randomly releasing trigger (AI)

	int quickGrenTime;

	int leanStopDebounceTime;

//----(SA)	added

	// seems like heat and aimspread could be tied together somehow, however, they (appear to) change at different rates and
	// I can't currently see how to optimize this to one server->client transmission "weapstatus" value.
	int weapHeat[MAX_WEAPONS_ET];			// some weapons can overheat.  this tracks (server-side) how hot each weapon currently is.
	int curWeapHeat;					// value for the currently selected weapon (for transmission to client)		// Arnout : DOES get send over the network
	int identifyClient;					// NERVE - SMF
	int identifyClientHealth;

	int aiState;			// xkan, 1/10/2003
};

// wsentityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)

struct wsentityState_t
{
	int number;				// entity index
	int eType;				// entityType_t
	int eFlags;

	q3trajectory_t pos;			// for calculating position
	q3trajectory_t apos;		// for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum;		// shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum;		// -1 = in air

	int constantLight;		// r + (g<<8) + (b<<16) + (intensity<<24)
	int dl_intensity;		// used for coronas
	int loopSound;			// constantly loop this sound

	int modelindex;
	int modelindex2;
	int clientNum;			// 0 to (MAX_CLIENTS_WS - 1), for players and corpses
	int frame;

	int solid;				// for client side prediction, trap_linkentity sets this properly

	// old style events, in for compatibility only
	int event;
	int eventParm;

	int eventSequence;		// pmove generated events
	int events[MAX_EVENTS_WS];
	int eventParms[MAX_EVENTS_WS];

	// for players
	int powerups;			// bit flags
	int weapon;				// determines weapon and flash model, etc
	int legsAnim;			// mask off ANIM_TOGGLEBIT
	int torsoAnim;			// mask off ANIM_TOGGLEBIT
//	int		weapAnim;		// mask off ANIM_TOGGLEBIT	//----(SA)	removed (weap anims will be client-side only)

	int density;			// for particle effects

	int dmgFlags;			// to pass along additional information for damage effects for players/ Also used for cursorhints for non-player entities

	// Ridah
	int onFireStart, onFireEnd;

	int aiChar, teamNum;

	int effect1Time, effect2Time, effect3Time;

	int aiState;

	int animMovetype;		// clients can't derive movetype of other clients for anim scripting system
};

// wmentityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)

struct wmentityState_t
{
	int number;				// entity index
	int eType;				// entityType_t
	int eFlags;

	q3trajectory_t pos;			// for calculating position
	q3trajectory_t apos;		// for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum;		// shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum;		// -1 = in air

	int constantLight;		// r + (g<<8) + (b<<16) + (intensity<<24)
	int dl_intensity;		// used for coronas
	int loopSound;			// constantly loop this sound

	int modelindex;
	int modelindex2;
	int clientNum;			// 0 to (MAX_CLIENTS_WM - 1), for players and corpses
	int frame;

	int solid;				// for client side prediction, trap_linkentity sets this properly

	// old style events, in for compatibility only
	int event;
	int eventParm;

	int eventSequence;		// pmove generated events
	int events[MAX_EVENTS_WM];
	int eventParms[MAX_EVENTS_WM];

	// for players
	int powerups;			// bit flags
	int weapon;				// determines weapon and flash model, etc
	int legsAnim;			// mask off ANIM_TOGGLEBIT
	int torsoAnim;			// mask off ANIM_TOGGLEBIT
//	int		weapAnim;		// mask off ANIM_TOGGLEBIT	//----(SA)	removed (weap anims will be client-side only)

	int density;			// for particle effects

	int dmgFlags;			// to pass along additional information for damage effects for players/ Also used for cursorhints for non-player entities

	// Ridah
	int onFireStart, onFireEnd;

	int aiChar, teamNum;

	int effect1Time, effect2Time, effect3Time;

	int aiState;

	int animMovetype;		// clients can't derive movetype of other clients for anim scripting system
};

struct etentityState_t
{
	int number;						// entity index
	int eType;				// entityType_t
	int eFlags;

	q3trajectory_t pos;			// for calculating position
	q3trajectory_t apos;		// for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum;		// shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum;		// -1 = in air

	int constantLight;		// r + (g<<8) + (b<<16) + (intensity<<24)
	int dl_intensity;		// used for coronas
	int loopSound;			// constantly loop this sound

	int modelindex;
	int modelindex2;
	int clientNum;			// 0 to (MAX_CLIENTS_ET - 1), for players and corpses
	int frame;

	int solid;				// for client side prediction, trap_linkentity sets this properly

	// old style events, in for compatibility only
	int event;
	int eventParm;

	int eventSequence;		// pmove generated events
	int events[MAX_EVENTS_ET];
	int eventParms[MAX_EVENTS_ET];

	// for players
	int powerups;			// bit flags	// Arnout: used to store entState_t for non-player entities (so we know to draw them translucent clientsided)
	int weapon;				// determines weapon and flash model, etc
	int legsAnim;			// mask off ANIM_TOGGLEBIT
	int torsoAnim;			// mask off ANIM_TOGGLEBIT
//	int		weapAnim;		// mask off ANIM_TOGGLEBIT	//----(SA)	removed (weap anims will be client-side only)

	int density;			// for particle effects

	int dmgFlags;			// to pass along additional information for damage effects for players/ Also used for cursorhints for non-player entities

	// Ridah
	int onFireStart, onFireEnd;

	int nextWeapon;
	int teamNum;

	int effect1Time, effect2Time, effect3Time;

	int aiState;		// xkan, 1/10/2003
	int animMovetype;		// clients can't derive movetype of other clients for anim scripting system
};

// RF, increased this, seems to keep causing problems when set to 64, especially when loading
// a savegame, which is hard to fix on that side, since we can't really spread out a loadgame
// among several frames
#define MAX_RELIABLE_COMMANDS_WOLF   256	// bigger!
#define BIGGEST_MAX_RELIABLE_COMMANDS   256	// bigger!

// in_usercmd_t->button bits
#define WOLFBUTTON_ACTIVATE     BIT(6)
#define WOLFBUTTON_ANY          BIT(7)		// any key whatsoever
#define WOLFBUTTON_LEANLEFT     BIT(12)
#define WOLFBUTTON_LEANRIGHT    BIT(13)

#define MAX_BINARY_MESSAGE_ET  32768	// max length of binary message

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
enum
{
	WSPERS_SCORE = 0,		// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	// Rafael - mg42		// (SA) I don't understand these here.  can someone explain?
	WSPERS_HWEAPON_USE = 14,
};

enum
{
	WMPERS_SCORE = 0,		// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	// Rafael - mg42		// (SA) I don't understand these here.  can someone explain?
	WMPERS_HWEAPON_USE = 12,
};

enum
{
	ETPERS_SCORE = 0,		// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	// Rafael - mg42		// (SA) I don't understand these here.  can someone explain?
	ETPERS_HWEAPON_USE = 12,
};

// wsentityState_t->eFlags
#define WSEF_DEAD           0x00000001		// don't draw a foe marker over players with WSEF_DEAD
#define WSEF_NODRAW         0x00000080		// may have an event, but no model (unspawned items)
#define WSEF_VIEWING_CAMERA 0x00040000
// wmentityState_t->eFlags
#define WMEF_DEAD           0x00000001		// don't draw a foe marker over players with WMEF_DEAD
#define WMEF_NODRAW         0x00000080		// may have an event, but no model (unspawned items)
#define WMEF_VIEWING_CAMERA 0x00040000
// etentityState_t->eFlags
#define ETEF_NODRAW         0x00000040		// may have an event, but no model (unspawned items)

#define WMCS_WOLFINFO       36		// NERVE - SMF
#define ETCS_WOLFINFO       21		// NERVE - SMF

struct animModelInfo_t;

enum
{
	ETMESSAGE_EMPTY = 0,
	ETMESSAGE_WAITING,		// rate/packet limited
	ETMESSAGE_WAITING_OVERFLOW,	// packet too large with message
};

struct etgameInfo_t
{
	qboolean spEnabled;
	int spGameTypes;
	int defaultSPGameType;
	int coopGameTypes;
	int defaultCoopGameType;
	int defaultGameType;
	qboolean usesProfiles;
};

#define WSAUTHORIZE_SERVER_NAME "authorize.gmistudios.com"
#define WMAUTHORIZE_SERVER_NAME "wolfauthorize.idsoftware.com"

// bitmask
enum
{
	DL_FLAG_DISCON = 0,
	DL_FLAG_URL
};

#define WSPROTOCOL_VERSION  49	// TA value
// 1.33 - protocol 59
// 1.4 - protocol 60
#define WMPROTOCOL_VERSION  60
// 2.56 - protocol 83
// 2.4 - protocol 80
#define ETPROTOCOL_VERSION  84

#define WMGAMENAME_STRING   "wolfmp"
#define ETGAMENAME_STRING   "et"

#define WSMASTER_SERVER_NAME    "master.gmistudios.com"
#define WMMASTER_SERVER_NAME    "wolfmaster.idsoftware.com"
#define ETMASTER_SERVER_NAME    "etmaster.idsoftware.com"

#define ETCONFIG_NAME   "etconfig.cfg"
