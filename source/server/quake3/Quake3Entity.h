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

class idQuake3Entity : public idEntity3
{
	/*
	int loopSound;		// constantly loop this sound
*/
	virtual int GetModelIndex() const;
	virtual void SetModelIndex(int value);
	/*
	int modelindex2;
	int clientNum;		// 0 to (MAX_CLIENTS_Q3 - 1), for players and corpses
	int frame;
*/
	virtual int GetSolid() const;
	virtual void SetSolid(int value);
/*
	int event;			// impulse events -- muzzle flashes, footsteps, etc
	int eventParm;

	// for players
	int powerups;		// bit flags
	int weapon;			// determines weapon and flash model, etc
	int legsAnim;		// mask off ANIM_TOGGLEBIT
	int torsoAnim;		// mask off ANIM_TOGGLEBIT

	int generic1;
	 */
	/*
	q3entityState_t s;				// communicated by server to clients
*/
	virtual bool GetLinked() const;
	virtual void SetLinked(bool value);
	virtual void IncLinkCount();
/*
	int svFlags;					// Q3SVF_NOCLIENT, Q3SVF_BROADCAST, etc
*/
	virtual bool GetSvFlagCapsule() const;
/*
	// only send to this client when SVF_SINGLECLIENT is set
	// if Q3SVF_CLIENTMASK is set, use bitmask for clients to send to (maxclients must be <= 32, up to the mod to enforce this)
	int singleClient;
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

	virtual void SetTempBoxModelContents(clipHandle_t clipHandle) const;
	virtual bool IsETypeProp() const;
};
