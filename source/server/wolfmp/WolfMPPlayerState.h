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

class idWolfMPPlayerState : public idPlayerState3
{
public:
/*
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
*/
};
