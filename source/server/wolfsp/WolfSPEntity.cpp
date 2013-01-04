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

bool idWolfSPEntity::GetEFlagViewingCamera() const
{
	return !!(gentity->eFlags & WSEF_VIEWING_CAMERA);
}

bool idWolfSPEntity::GetEFlagDead() const
{
	return !!(gentity->eFlags & WSEF_DEAD);
}

void idWolfSPEntity::SetEFlagNoDraw()
{
	gentity->eFlags |= WSEF_NODRAW;
}

int idWolfSPEntity::GetModelIndex() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->s.modelindex;
}

void idWolfSPEntity::SetModelIndex(int value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->s.modelindex = value;
}

int idWolfSPEntity::GetSolid() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->s.solid;
}

void idWolfSPEntity::SetSolid(int value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->s.solid = value;
}

int idWolfSPEntity::GetGeneric1() const
{
	return 0;
}

void idWolfSPEntity::SetGeneric1(int value)
{
}

bool idWolfSPEntity::GetLinked() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.linked;
}

void idWolfSPEntity::SetLinked(bool value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->r.linked = value;
}

void idWolfSPEntity::IncLinkCount()
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->r.linkcount++;
}

int idWolfSPEntity::GetSvFlags() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags;
}

void idWolfSPEntity::SetSvFlags(int value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags = value;
}

bool idWolfSPEntity::GetSvFlagCapsule() const
{
	return !!(reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags & WOLFSVF_CAPSULE);
}

bool idWolfSPEntity::GetSvFlagCastAI() const
{
	return !!(reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags & WSSVF_CASTAI);
}

bool idWolfSPEntity::GetSvFlagNoServerInfo() const
{
	return !!(reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags & WOLFSVF_NOSERVERINFO);
}

bool idWolfSPEntity::GetSvFlagSelfPortal() const
{
	return false;
}

bool idWolfSPEntity::GetSvFlagSelfPortalExclusive() const
{
	return false;
}

bool idWolfSPEntity::GetSvFlagSingleClient() const
{
	return !!(reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags & WOLFSVF_SINGLECLIENT);
}

bool idWolfSPEntity::GetSvFlagNotSingleClient() const
{
	return !!(reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags & WOLFSVF_NOTSINGLECLIENT);
}

bool idWolfSPEntity::GetSvFlagClientMask() const
{
	return false;
}

bool idWolfSPEntity::GetSvFlagIgnoreBModelExtents() const
{
	return false;
}

bool idWolfSPEntity::GetSvFlagVisDummy() const
{
	return !!(reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags & WOLFSVF_VISDUMMY);
}

bool idWolfSPEntity::GetSvFlagVisDummyMultiple() const
{
	return !!(reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags & WOLFSVF_VISDUMMY_MULTIPLE);
}

int idWolfSPEntity::GetSingleClient() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.singleClient;
}

void idWolfSPEntity::SetSingleClient(int value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->r.singleClient = value;
}

bool idWolfSPEntity::GetBModel() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.bmodel;
}

void idWolfSPEntity::SetBModel(bool value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->r.bmodel = value;
}

const float* idWolfSPEntity::GetMins() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.mins;
}

void idWolfSPEntity::SetMins(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<wssharedEntity_t*>(gentity)->r.mins);
}

const float* idWolfSPEntity::GetMaxs() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.maxs;
}

void idWolfSPEntity::SetMaxs(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<wssharedEntity_t*>(gentity)->r.maxs);
}

int idWolfSPEntity::GetContents() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.contents;
}

void idWolfSPEntity::SetContents(int value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->r.contents = value;
}

float* idWolfSPEntity::GetAbsMin()
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.absmin;
}

void idWolfSPEntity::SetAbsMin(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<wssharedEntity_t*>(gentity)->r.absmin);
}

float* idWolfSPEntity::GetAbsMax()
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.absmax;
}

void idWolfSPEntity::SetAbsMax(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<wssharedEntity_t*>(gentity)->r.absmax);
}

const float* idWolfSPEntity::GetCurrentOrigin() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.currentOrigin;
}

void idWolfSPEntity::SetCurrentOrigin(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<wssharedEntity_t*>(gentity)->r.currentOrigin);
}

const float* idWolfSPEntity::GetCurrentAngles() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.currentAngles;
}

void idWolfSPEntity::SetCurrentAngles(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<wssharedEntity_t*>(gentity)->r.currentAngles);
}

int idWolfSPEntity::GetOwnerNum() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.ownerNum;
}

void idWolfSPEntity::SetOwnerNum(int value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->r.ownerNum = value;
}

int idWolfSPEntity::GetEventTime() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->r.eventTime;
}

void idWolfSPEntity::SetEventTime(int value)
{
	reinterpret_cast<wssharedEntity_t*>(gentity)->r.eventTime = value;
}

bool idWolfSPEntity::GetSnapshotCallback() const
{
	return false;
}

void idWolfSPEntity::SetSnapshotCallback(bool value)
{
}

void idWolfSPEntity::SetTempBoxModelContents(clipHandle_t) const
{
}

bool idWolfSPEntity::IsETypeProp() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->s.eType == WSET_PROP;
}
