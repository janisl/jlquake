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

class idQuake3PlayerState : public idPlayerState3 {
public:
/*
    int gravity;
    */
	virtual float GetLeanf() const;
	virtual void SetLeanf( float value );
/*
    int speed;
    int delta_angles[3];	// add to command angles to get view direction
                            // changed by spawns, rotating objects, and teleporters

    int groundEntityNum;// Q3ENTITYNUM_NONE = in air

    int legsTimer;		// don't change low priority animations until this runs out
    int legsAnim;		// mask off ANIM_TOGGLEBIT

    int torsoTimer;		// don't change low priority animations until this runs out
    int torsoAnim;		// mask off ANIM_TOGGLEBIT

    int movementDir;	// a number 0 to 7 that represents the reletive angle
                        // of movement to the view angle (axial and diagonals)
                        // when at rest, the value will remain unchanged
                        // used to twist the legs during strafing

    vec3_t grapplePoint;	// location of grapple to pull towards if PMF_GRAPPLE_PULL

    int eFlags;			// copied to q3entityState_t->eFlags

    int eventSequence;	// pmove generated events
    int events[MAX_PS_EVENTS_Q3];
    int eventParms[MAX_PS_EVENTS_Q3];

    int externalEvent;	// events set on player from another source
    int externalEventParm;
    int externalEventTime;
*/
	virtual int GetClientNum() const;
	virtual void SetClientNum( int value );
/*
    int weapon;			// copied to q3entityState_t->weapon
    int weaponstate;
*/
	virtual const float* GetViewAngles() const;
	virtual void SetViewAngles( const vec3_t value );
	virtual int GetViewHeight() const;
	virtual void SetViewHeight( int value );
	/*

	// damage feedback
	int damageEvent;	// when it changes, latch the other parms
	int damageYaw;
	int damagePitch;
	int damageCount;

	int stats[MAX_STATS_Q3];
	int persistant[MAX_PERSISTANT_Q3];	// stats that aren't cleared on death
	*/
	virtual int GetPersistantScore() const;
	/*
	int powerups[MAX_POWERUPS_Q3];	// level.time that the powerup runs out
	int ammo[MAX_WEAPONS_Q3];

	int generic1;
	int loopSound;
	int jumppad_ent;	// jumppad entity hit this frame
	*/
	virtual int GetPing() const;
	virtual void SetPing( int value );
/*
    int pmove_framecount;	// FIXME: don't transmit over the network
    int jumppad_frame;
    int entityEventSequence;
 */
};
