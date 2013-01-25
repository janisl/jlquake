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

#include "../tech3/local.h"
#include "local.h"

bool idWolfMPEntity::GetEFlagViewingCamera() const {
	return !!( gentity->eFlags & WMEF_VIEWING_CAMERA );
}

bool idWolfMPEntity::GetEFlagDead() const {
	return !!( gentity->eFlags & WMEF_DEAD );
}

void idWolfMPEntity::SetEFlagNoDraw() {
	gentity->eFlags |= WMEF_NODRAW;
}

int idWolfMPEntity::GetModelIndex() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->s.modelindex;
}

void idWolfMPEntity::SetModelIndex( int value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->s.modelindex = value;
}

int idWolfMPEntity::GetSolid() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->s.solid;
}

void idWolfMPEntity::SetSolid( int value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->s.solid = value;
}

int idWolfMPEntity::GetGeneric1() const {
	return 0;
}

void idWolfMPEntity::SetGeneric1( int value ) {
}

bool idWolfMPEntity::GetLinked() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.linked;
}

void idWolfMPEntity::SetLinked( bool value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->r.linked = value;
}

void idWolfMPEntity::IncLinkCount() {
	reinterpret_cast<wssharedEntity_t*>( gentity )->r.linkcount++;
}

int idWolfMPEntity::GetSvFlags() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags;
}

void idWolfMPEntity::SetSvFlags( int value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags = value;
}

bool idWolfMPEntity::GetSvFlagCapsule() const {
	return !!( reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_CAPSULE );
}

bool idWolfMPEntity::GetSvFlagCastAI() const {
	return !!( reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags & WMSVF_CASTAI );
}

bool idWolfMPEntity::GetSvFlagNoServerInfo() const {
	return !!( reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_NOSERVERINFO );
}

bool idWolfMPEntity::GetSvFlagSelfPortal() const {
	return false;
}

bool idWolfMPEntity::GetSvFlagSelfPortalExclusive() const {
	return false;
}

bool idWolfMPEntity::GetSvFlagSingleClient() const {
	return !!( reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_SINGLECLIENT );
}

bool idWolfMPEntity::GetSvFlagNotSingleClient() const {
	return !!( reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_NOTSINGLECLIENT );
}

bool idWolfMPEntity::GetSvFlagClientMask() const {
	return false;
}

bool idWolfMPEntity::GetSvFlagIgnoreBModelExtents() const {
	return false;
}

bool idWolfMPEntity::GetSvFlagVisDummy() const {
	return !!( reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_VISDUMMY );
}

bool idWolfMPEntity::GetSvFlagVisDummyMultiple() const {
	return !!( reinterpret_cast<wmsharedEntity_t*>( gentity )->r.svFlags & WOLFSVF_VISDUMMY_MULTIPLE );
}

int idWolfMPEntity::GetSingleClient() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.singleClient;
}

void idWolfMPEntity::SetSingleClient( int value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->r.singleClient = value;
}

bool idWolfMPEntity::GetBModel() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.bmodel;
}

void idWolfMPEntity::SetBModel( bool value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->r.bmodel = value;
}

const float* idWolfMPEntity::GetMins() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.mins;
}

void idWolfMPEntity::SetMins( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<wmsharedEntity_t*>( gentity )->r.mins );
}

const float* idWolfMPEntity::GetMaxs() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.maxs;
}

void idWolfMPEntity::SetMaxs( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<wmsharedEntity_t*>( gentity )->r.maxs );
}

int idWolfMPEntity::GetContents() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.contents;
}

void idWolfMPEntity::SetContents( int value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->r.contents = value;
}

float* idWolfMPEntity::GetAbsMin() {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.absmin;
}

void idWolfMPEntity::SetAbsMin( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<wmsharedEntity_t*>( gentity )->r.absmin );
}

float* idWolfMPEntity::GetAbsMax() {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.absmax;
}

void idWolfMPEntity::SetAbsMax( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<wmsharedEntity_t*>( gentity )->r.absmax );
}

const float* idWolfMPEntity::GetCurrentOrigin() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.currentOrigin;
}

void idWolfMPEntity::SetCurrentOrigin( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<wmsharedEntity_t*>( gentity )->r.currentOrigin );
}

const float* idWolfMPEntity::GetCurrentAngles() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.currentAngles;
}

void idWolfMPEntity::SetCurrentAngles( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<wmsharedEntity_t*>( gentity )->r.currentAngles );
}

int idWolfMPEntity::GetOwnerNum() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.ownerNum;
}

void idWolfMPEntity::SetOwnerNum( int value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->r.ownerNum = value;
}

int idWolfMPEntity::GetEventTime() const {
	return reinterpret_cast<wmsharedEntity_t*>( gentity )->r.eventTime;
}

void idWolfMPEntity::SetEventTime( int value ) {
	reinterpret_cast<wmsharedEntity_t*>( gentity )->r.eventTime = value;
}

bool idWolfMPEntity::GetSnapshotCallback() const {
	return false;
}

void idWolfMPEntity::SetSnapshotCallback( bool value ) {
}

void idWolfMPEntity::SetTempBoxModelContents( clipHandle_t clipHandle ) const {
	CM_SetTempBoxModelContents( clipHandle, reinterpret_cast<wmsharedEntity_t*>( gentity )->r.contents );
}

bool idWolfMPEntity::IsETypeProp() const {
	return false;
}
