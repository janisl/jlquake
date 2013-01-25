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

class idQuake3Entity : public idEntity3 {
	virtual bool GetEFlagViewingCamera() const;
	virtual bool GetEFlagDead() const;
	virtual void SetEFlagNoDraw();
	/*
	int loopSound;		// constantly loop this sound
	*/
	virtual int GetModelIndex() const;
	virtual void SetModelIndex( int value );
	/*
	int modelindex2;
	int clientNum;		// 0 to (MAX_CLIENTS_Q3 - 1), for players and corpses
	int frame;
	*/
	virtual int GetSolid() const;
	virtual void SetSolid( int value );
/*
    int event;			// impulse events -- muzzle flashes, footsteps, etc
    int eventParm;

    // for players
    int powerups;		// bit flags
    int weapon;			// determines weapon and flash model, etc
    int legsAnim;		// mask off ANIM_TOGGLEBIT
    int torsoAnim;		// mask off ANIM_TOGGLEBIT
     */
	virtual int GetGeneric1() const;
	virtual void SetGeneric1( int value );
	/*
	q3entityState_t s;				// communicated by server to clients
	*/
	virtual bool GetLinked() const;
	virtual void SetLinked( bool value );
	virtual void IncLinkCount();
	virtual int GetSvFlags() const;
	virtual void SetSvFlags( int value );
	virtual bool GetSvFlagCapsule() const;
	virtual bool GetSvFlagCastAI() const;
	virtual bool GetSvFlagNoServerInfo() const;
	virtual bool GetSvFlagSelfPortal() const;
	virtual bool GetSvFlagSelfPortalExclusive() const;
	virtual bool GetSvFlagSingleClient() const;
	virtual bool GetSvFlagNotSingleClient() const;
	virtual bool GetSvFlagClientMask() const;
	virtual bool GetSvFlagIgnoreBModelExtents() const;
	virtual bool GetSvFlagVisDummy() const;
	virtual bool GetSvFlagVisDummyMultiple() const;
	virtual int GetSingleClient() const;
	virtual void SetSingleClient( int value );
	virtual bool GetBModel() const;
	virtual void SetBModel( bool value );
	virtual const float* GetMins() const;
	virtual void SetMins( const vec3_t value );
	virtual const float* GetMaxs() const;
	virtual void SetMaxs( const vec3_t value );
	virtual int GetContents() const;
	virtual void SetContents( int value );
	virtual float* GetAbsMin();
	virtual void SetAbsMin( const vec3_t value );
	virtual float* GetAbsMax();
	virtual void SetAbsMax( const vec3_t value );
	virtual const float* GetCurrentOrigin() const;
	virtual void SetCurrentOrigin( const vec3_t value );
	virtual const float* GetCurrentAngles() const;
	virtual void SetCurrentAngles( const vec3_t value );
	virtual int GetOwnerNum() const;
	virtual void SetOwnerNum( int value );
	virtual int GetEventTime() const;
	virtual void SetEventTime( int value );
	virtual bool GetSnapshotCallback() const;
	virtual void SetSnapshotCallback( bool value );

	virtual void SetTempBoxModelContents( clipHandle_t clipHandle ) const;
	virtual bool IsETypeProp() const;
};
