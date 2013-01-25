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

#ifndef __idEntity3__
#define __idEntity3__

#include "../../common/clip_map/public.h"
#include "../../common/quake3defs.h"

//	This is initial portion of shared entity struct that is common in all
// Tech3 games.
struct sharedEntityCommon_t {
	int number;			// entity index
	int eType;			// entityType_t
	int eFlags;

	q3trajectory_t pos;	// for calculating position
	q3trajectory_t apos;	// for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum;	// shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum;	// -1 = in air

	int constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
};

class idEntity3 : public Interface {
public:
	idEntity3();

	void SetGEntity( void* newGEntity );

	int GetNumber() const;
	void SetNumber( int value );
	int GetEType() const;
	void SetEType( int value );
	/*
	int eFlags;

	q3trajectory_t pos;	// for calculating position
	q3trajectory_t apos;	// for calculating angles

	int time;
	int time2;
	*/
	const float* GetOrigin() const;
	void SetOrigin( const vec3_t value );
	const float* GetOrigin2() const;
	void SetOrigin2( const vec3_t value );
	const float* GetAngles() const;
	void SetAngles( const vec3_t value );
	/*
	vec3_t angles2;
	*/
	int GetOtherEntityNum() const;
	void SetOtherEntityNum( int value );
	/*
	int otherEntityNum2;

	int groundEntityNum;	// -1 = in air

	int constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
	 */
	virtual bool GetEFlagViewingCamera() const = 0;
	virtual bool GetEFlagDead() const = 0;
	virtual void SetEFlagNoDraw() = 0;
	virtual int GetModelIndex() const = 0;
	virtual void SetModelIndex( int value ) = 0;
	virtual int GetSolid() const = 0;
	virtual void SetSolid( int value ) = 0;
	virtual int GetGeneric1() const = 0;
	virtual void SetGeneric1( int value ) = 0;
	virtual bool GetLinked() const = 0;
	virtual void SetLinked( bool value ) = 0;
	virtual void IncLinkCount() = 0;
	virtual int GetSvFlags() const = 0;
	virtual void SetSvFlags( int value ) = 0;
	virtual bool GetSvFlagCapsule() const = 0;
	virtual bool GetSvFlagCastAI() const = 0;
	virtual bool GetSvFlagNoServerInfo() const = 0;
	virtual bool GetSvFlagSelfPortal() const = 0;
	virtual bool GetSvFlagSelfPortalExclusive() const = 0;
	virtual bool GetSvFlagSingleClient() const = 0;
	virtual bool GetSvFlagNotSingleClient() const = 0;
	virtual bool GetSvFlagClientMask() const = 0;
	virtual bool GetSvFlagIgnoreBModelExtents() const = 0;
	virtual bool GetSvFlagVisDummy() const = 0;
	virtual bool GetSvFlagVisDummyMultiple() const = 0;
	virtual int GetSingleClient() const = 0;
	virtual void SetSingleClient( int value ) = 0;
	virtual bool GetBModel() const = 0;
	virtual void SetBModel( bool value ) = 0;
	virtual const float* GetMins() const = 0;
	virtual void SetMins( const vec3_t value ) = 0;
	virtual const float* GetMaxs() const = 0;
	virtual void SetMaxs( const vec3_t value ) = 0;
	virtual int GetContents() const = 0;
	virtual void SetContents( int value ) = 0;
	virtual float* GetAbsMin() = 0;
	virtual void SetAbsMin( const vec3_t value ) = 0;
	virtual float* GetAbsMax() = 0;
	virtual void SetAbsMax( const vec3_t value ) = 0;
	virtual const float* GetCurrentOrigin() const = 0;
	virtual void SetCurrentOrigin( const vec3_t value ) = 0;
	virtual const float* GetCurrentAngles() const = 0;
	virtual void SetCurrentAngles( const vec3_t value ) = 0;
	virtual int GetOwnerNum() const = 0;
	virtual void SetOwnerNum( int value ) = 0;
	virtual int GetEventTime() const = 0;
	virtual void SetEventTime( int value ) = 0;
	virtual bool GetSnapshotCallback() const = 0;
	virtual void SetSnapshotCallback( bool value ) = 0;

	virtual void SetTempBoxModelContents( clipHandle_t clipHandle ) const = 0;
	virtual bool IsETypeProp() const = 0;

protected:
	sharedEntityCommon_t* gentity;
};

inline idEntity3::idEntity3()
	: gentity( NULL ) {
}

inline void idEntity3::SetGEntity( void* newGEntity ) {
	gentity = reinterpret_cast<sharedEntityCommon_t*>( newGEntity );
}

inline int idEntity3::GetNumber() const {
	return gentity->number;
}

inline void idEntity3::SetNumber( int value ) {
	gentity->number = value;
}

inline int idEntity3::GetEType() const {
	return gentity->eType;
}

inline void idEntity3::SetEType( int value ) {
	gentity->eType = value;
}

inline const float* idEntity3::GetOrigin() const {
	return gentity->origin;
}

inline void idEntity3::SetOrigin( const vec3_t value ) {
	VectorCopy( value, gentity->origin );
}

inline const float* idEntity3::GetOrigin2() const {
	return gentity->origin2;
}

inline void idEntity3::SetOrigin2( const vec3_t value ) {
	VectorCopy( value, gentity->origin2 );
}

inline const float* idEntity3::GetAngles() const {
	return gentity->angles;
}

inline void idEntity3::SetAngles( const vec3_t value ) {
	VectorCopy( value, gentity->angles );
}

inline int idEntity3::GetOtherEntityNum() const {
	return gentity->otherEntityNum;
}

inline void idEntity3::SetOtherEntityNum( int value ) {
	gentity->otherEntityNum = value;
}

#endif
