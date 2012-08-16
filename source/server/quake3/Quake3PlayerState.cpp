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

float idQuake3PlayerState::GetLeanf() const
{
	return 0;
}

void idQuake3PlayerState::SetLeanf(float value)
{
}

int idQuake3PlayerState::GetClientNum() const
{
	return reinterpret_cast<q3playerState_t*>(ps)->clientNum;
}

void idQuake3PlayerState::SetClientNum(int value)
{
	reinterpret_cast<q3playerState_t*>(ps)->clientNum = value;
}

const float* idQuake3PlayerState::GetViewAngles() const
{
	return reinterpret_cast<q3playerState_t*>(ps)->viewangles;
}

void idQuake3PlayerState::SetViewAngles(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<q3playerState_t*>(ps)->viewangles);
}

int idQuake3PlayerState::GetViewHeight() const
{
	return reinterpret_cast<q3playerState_t*>(ps)->viewheight;
}

void idQuake3PlayerState::SetViewHeight(int value)
{
	reinterpret_cast<q3playerState_t*>(ps)->viewheight = value;
}

int idQuake3PlayerState::GetPersistantScore() const
{
	return reinterpret_cast<q3playerState_t*>(ps)->persistant[Q3PERS_SCORE];
}

int idQuake3PlayerState::GetPing() const
{
	return reinterpret_cast<q3playerState_t*>(ps)->ping;
}

void idQuake3PlayerState::SetPing(int value)
{
	reinterpret_cast<q3playerState_t*>(ps)->ping = value;
}
