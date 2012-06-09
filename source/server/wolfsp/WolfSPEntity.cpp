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

#include "../server.h"
#include "../tech3/local.h"
#include "local.h"

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

bool idWolfSPEntity::GetSvFlagCapsule() const
{
	return !!(reinterpret_cast<wssharedEntity_t*>(gentity)->r.svFlags & WOLFSVF_CAPSULE);
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

void idWolfSPEntity::SetTempBoxModelContents(clipHandle_t) const
{
}

bool idWolfSPEntity::IsETypeProp() const
{
	return reinterpret_cast<wssharedEntity_t*>(gentity)->s.eType == WSET_PROP;
}
