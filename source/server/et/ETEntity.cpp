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

#include "../../common/qcommon.h"
#include "../tech3/local.h"
#include "local.h"

bool idETEntity::GetEFlagViewingCamera() const {
	return false;
}

bool idETEntity::GetEFlagDead() const {
	return false;
}

void idETEntity::SetEFlagNoDraw() {
	gentity->eFlags |= ETEF_NODRAW;
}

int idETEntity::GetModelIndex() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->s.modelindex;
}

void idETEntity::SetModelIndex( int value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->s.modelindex = value;
}

int idETEntity::GetSolid() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->s.solid;
}

void idETEntity::SetSolid( int value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->s.solid = value;
}

int idETEntity::GetGeneric1() const {
	return 0;
}

void idETEntity::SetGeneric1( int value ) {
}

bool idETEntity::GetLinked() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.linked;
}

void idETEntity::SetLinked( bool value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.linked = value;
}

void idETEntity::IncLinkCount() {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.linkcount++;
}

int idETEntity::GetSvFlags() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags;
}

void idETEntity::SetSvFlags( int value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags = value;
}

bool idETEntity::GetSvFlagCapsule() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_CAPSULE );
}

bool idETEntity::GetSvFlagCastAI() const {
	return false;
}

bool idETEntity::GetSvFlagNoServerInfo() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_NOSERVERINFO );
}

bool idETEntity::GetSvFlagSelfPortal() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & ETSVF_SELF_PORTAL );
}

bool idETEntity::GetSvFlagSelfPortalExclusive() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & ETSVF_SELF_PORTAL_EXCLUSIVE );
}

bool idETEntity::GetSvFlagSingleClient() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_SINGLECLIENT );
}

bool idETEntity::GetSvFlagNotSingleClient() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_NOTSINGLECLIENT );
}

bool idETEntity::GetSvFlagClientMask() const {
	return false;
}

bool idETEntity::GetSvFlagIgnoreBModelExtents() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & ETSVF_IGNOREBMODELEXTENTS );
}

bool idETEntity::GetSvFlagVisDummy() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_VISDUMMY );
}

bool idETEntity::GetSvFlagVisDummyMultiple() const {
	return !!( reinterpret_cast<etsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_VISDUMMY_MULTIPLE );
}

int idETEntity::GetSingleClient() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.singleClient;
}

void idETEntity::SetSingleClient( int value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.singleClient = value;
}

bool idETEntity::GetBModel() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.bmodel;
}

void idETEntity::SetBModel( bool value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.bmodel = value;
}

const float* idETEntity::GetMins() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.mins;
}

void idETEntity::SetMins( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<etsharedEntity_t*>( gentity )->r.mins );
}

const float* idETEntity::GetMaxs() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.maxs;
}

void idETEntity::SetMaxs( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<etsharedEntity_t*>( gentity )->r.maxs );
}

int idETEntity::GetContents() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.contents;
}

void idETEntity::SetContents( int value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.contents = value;
}

float* idETEntity::GetAbsMin() {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.absmin;
}

void idETEntity::SetAbsMin( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<etsharedEntity_t*>( gentity )->r.absmin );
}

float* idETEntity::GetAbsMax() {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.absmax;
}

void idETEntity::SetAbsMax( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<etsharedEntity_t*>( gentity )->r.absmax );
}

const float* idETEntity::GetCurrentOrigin() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.currentOrigin;
}

void idETEntity::SetCurrentOrigin( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<etsharedEntity_t*>( gentity )->r.currentOrigin );
}

const float* idETEntity::GetCurrentAngles() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.currentAngles;
}

void idETEntity::SetCurrentAngles( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<etsharedEntity_t*>( gentity )->r.currentAngles );
}

int idETEntity::GetOwnerNum() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.ownerNum;
}

void idETEntity::SetOwnerNum( int value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.ownerNum = value;
}

int idETEntity::GetEventTime() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.eventTime;
}

void idETEntity::SetEventTime( int value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.eventTime = value;
}

bool idETEntity::GetSnapshotCallback() const {
	return reinterpret_cast<etsharedEntity_t*>( gentity )->r.snapshotCallback;
}

void idETEntity::SetSnapshotCallback( bool value ) {
	reinterpret_cast<etsharedEntity_t*>( gentity )->r.snapshotCallback = value;
}

void idETEntity::SetTempBoxModelContents( clipHandle_t clipHandle ) const {
	CM_SetTempBoxModelContents( clipHandle, reinterpret_cast<etsharedEntity_t*>( gentity )->r.contents );
}

bool idETEntity::IsETypeProp() const {
	return false;
}
