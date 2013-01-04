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

float idWolfMPPlayerState::GetLeanf() const
{
	return reinterpret_cast<wmplayerState_t*>(ps)->leanf;
}

void idWolfMPPlayerState::SetLeanf(float value)
{
	reinterpret_cast<wmplayerState_t*>(ps)->leanf = value;
}

int idWolfMPPlayerState::GetClientNum() const
{
	return reinterpret_cast<wmplayerState_t*>(ps)->clientNum;
}

void idWolfMPPlayerState::SetClientNum(int value)
{
	reinterpret_cast<wmplayerState_t*>(ps)->clientNum = value;
}

const float* idWolfMPPlayerState::GetViewAngles() const
{
	return reinterpret_cast<wmplayerState_t*>(ps)->viewangles;
}

void idWolfMPPlayerState::SetViewAngles(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<wmplayerState_t*>(ps)->viewangles);
}

int idWolfMPPlayerState::GetViewHeight() const
{
	return reinterpret_cast<wmplayerState_t*>(ps)->viewheight;
}

void idWolfMPPlayerState::SetViewHeight(int value)
{
	reinterpret_cast<wmplayerState_t*>(ps)->viewheight = value;
}

int idWolfMPPlayerState::GetPersistantScore() const
{
	return reinterpret_cast<wmplayerState_t*>(ps)->persistant[WMPERS_SCORE];
}

int idWolfMPPlayerState::GetPing() const
{
	return reinterpret_cast<wmplayerState_t*>(ps)->ping;
}

void idWolfMPPlayerState::SetPing(int value)
{
	reinterpret_cast<wmplayerState_t*>(ps)->ping = value;
}
