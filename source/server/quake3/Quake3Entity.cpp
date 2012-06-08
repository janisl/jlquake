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

int idQuake3Entity::GetSolid() const
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->s.solid;
}

void idQuake3Entity::SetSolid(int value)
{
	reinterpret_cast<q3sharedEntity_t*>(gentity)->s.solid = value;
}

bool idQuake3Entity::GetLinked() const
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.linked;
}

void idQuake3Entity::SetLinked(bool value)
{
	reinterpret_cast<q3sharedEntity_t*>(gentity)->r.linked = value;
}

void idQuake3Entity::IncLinkCount()
{
	reinterpret_cast<q3sharedEntity_t*>(gentity)->r.linkcount++;
}

bool idQuake3Entity::GetBModel() const
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.bmodel;
}

void idQuake3Entity::SetBModel(bool value)
{
	reinterpret_cast<q3sharedEntity_t*>(gentity)->r.bmodel = value;
}

const float* idQuake3Entity::GetMins() const
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.mins;
}

void idQuake3Entity::SetMins(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<q3sharedEntity_t*>(gentity)->r.mins);
}

const float* idQuake3Entity::GetMaxs() const
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.maxs;
}

void idQuake3Entity::SetMaxs(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<q3sharedEntity_t*>(gentity)->r.maxs);
}

int idQuake3Entity::GetContents() const
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.contents;
}

void idQuake3Entity::SetContents(int value)
{
	reinterpret_cast<q3sharedEntity_t*>(gentity)->r.contents = value;
}

float* idQuake3Entity::GetAbsMin()
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.absmin;
}

void idQuake3Entity::SetAbsMin(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<q3sharedEntity_t*>(gentity)->r.absmin);
}

float* idQuake3Entity::GetAbsMax()
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.absmax;
}

void idQuake3Entity::SetAbsMax(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<q3sharedEntity_t*>(gentity)->r.absmax);
}

const float* idQuake3Entity::GetCurrentOrigin() const
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.currentOrigin;
}

void idQuake3Entity::SetCurrentOrigin(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<q3sharedEntity_t*>(gentity)->r.currentOrigin);
}

const float* idQuake3Entity::GetCurrentAngles() const
{
	return reinterpret_cast<q3sharedEntity_t*>(gentity)->r.currentAngles;
}

void idQuake3Entity::SetCurrentAngles(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<q3sharedEntity_t*>(gentity)->r.currentAngles);
}
