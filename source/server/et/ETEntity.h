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

class idETEntity : public idEntity3
{
	/*
	int dl_intensity;		// used for coronas
	int loopSound;			// constantly loop this sound
*/
	virtual int GetModelIndex() const;
	virtual void SetModelIndex(int value);
/*	int modelindex2;
	int clientNum;			// 0 to (MAX_CLIENTS_ET - 1), for players and corpses
	int frame;
*/
	virtual int GetSolid() const;
	virtual void SetSolid(int value);
/*
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
	 */
	virtual bool GetLinked() const;
	virtual void SetLinked(bool value);
	virtual void IncLinkCount();
	virtual int GetSvFlags() const;
	virtual void SetSvFlags(int value);
	virtual bool GetSvFlagCapsule() const;
	virtual bool GetSvFlagCastAI() const;
	/*
	int singleClient;				// only send to this client when SVF_SINGLECLIENT is set
*/
	virtual bool GetBModel() const;
	virtual void SetBModel(bool value);
	virtual const float* GetMins() const;
	virtual void SetMins(const vec3_t value);
	virtual const float* GetMaxs() const;
	virtual void SetMaxs(const vec3_t value);
	virtual int GetContents() const;
	virtual void SetContents(int value);
	virtual float* GetAbsMin();
	virtual void SetAbsMin(const vec3_t value);
	virtual float* GetAbsMax();
	virtual void SetAbsMax(const vec3_t value);
	virtual const float* GetCurrentOrigin() const;
	virtual void SetCurrentOrigin(const vec3_t value);
	virtual const float* GetCurrentAngles() const;
	virtual void SetCurrentAngles(const vec3_t value);
	virtual int GetOwnerNum() const;
	virtual void SetOwnerNum(int value);
/*
	int eventTime;

	int worldflags;				// DHM - Nerve

	qboolean snapshotCallback;
	 */

	virtual void SetTempBoxModelContents(clipHandle_t clipHandle) const;
	virtual bool IsETypeProp() const;
};
