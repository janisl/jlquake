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

float idWolfSPPlayerState::GetLeanf() const
{
	return reinterpret_cast<wsplayerState_t*>(ps)->leanf;
}

void idWolfSPPlayerState::SetLeanf(float value)
{
	reinterpret_cast<wsplayerState_t*>(ps)->leanf = value;
}

int idWolfSPPlayerState::GetClientNum() const
{
	return reinterpret_cast<wsplayerState_t*>(ps)->clientNum;
}

void idWolfSPPlayerState::SetClientNum(int value)
{
	reinterpret_cast<wsplayerState_t*>(ps)->clientNum = value;
}

const float* idWolfSPPlayerState::GetViewAngles() const
{
	return reinterpret_cast<wsplayerState_t*>(ps)->viewangles;
}

void idWolfSPPlayerState::SetViewAngles(const vec3_t value)
{
	VectorCopy(value, reinterpret_cast<wsplayerState_t*>(ps)->viewangles);
}

int idWolfSPPlayerState::GetViewHeight() const
{
	return reinterpret_cast<wsplayerState_t*>(ps)->viewheight;
}

void idWolfSPPlayerState::SetViewHeight(int value)
{
	reinterpret_cast<wsplayerState_t*>(ps)->viewheight = value;
}

int idWolfSPPlayerState::GetPing() const
{
	return reinterpret_cast<wsplayerState_t*>(ps)->ping;
}

void idWolfSPPlayerState::SetPing(int value)
{
	reinterpret_cast<wsplayerState_t*>(ps)->ping = value;
}
