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

float idETPlayerState::GetLeanf() const {
	return reinterpret_cast<etplayerState_t*>( ps )->leanf;
}

void idETPlayerState::SetLeanf( float value ) {
	reinterpret_cast<etplayerState_t*>( ps )->leanf = value;
}

int idETPlayerState::GetClientNum() const {
	return reinterpret_cast<etplayerState_t*>( ps )->clientNum;
}

void idETPlayerState::SetClientNum( int value ) {
	reinterpret_cast<etplayerState_t*>( ps )->clientNum = value;
}

const float* idETPlayerState::GetViewAngles() const {
	return reinterpret_cast<etplayerState_t*>( ps )->viewangles;
}

void idETPlayerState::SetViewAngles( const vec3_t value ) {
	VectorCopy( value, reinterpret_cast<etplayerState_t*>( ps )->viewangles );
}

int idETPlayerState::GetViewHeight() const {
	return reinterpret_cast<etplayerState_t*>( ps )->viewheight;
}

void idETPlayerState::SetViewHeight( int value ) {
	reinterpret_cast<etplayerState_t*>( ps )->viewheight = value;
}

int idETPlayerState::GetPersistantScore() const {
	return reinterpret_cast<etplayerState_t*>( ps )->persistant[ ETPERS_SCORE ];
}

int idETPlayerState::GetPing() const {
	return reinterpret_cast<etplayerState_t*>( ps )->ping;
}

void idETPlayerState::SetPing( int value ) {
	reinterpret_cast<etplayerState_t*>( ps )->ping = value;
}
