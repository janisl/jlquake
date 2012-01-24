//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "progsvm.h"

idEntVarDef entFieldCameraMode;
idEntVarDef entFieldMoveChain;
idEntVarDef entFieldChainMoved;
idEntVarDef entFieldGravity;
idEntVarDef entFieldSiegeTeam;

void idEntVarDef::Init(const char* name, int offset)
{
	this->name = name;
	this->offset = offset;
}

int ED_InitEntityFields()
{
	int offset;
	if (GGameType & GAME_Quake)
	{
		if (GGameType & GAME_QuakeWorld)
		{
			offset = 408;
		}
		else
		{
			offset = 420;
		}
	}
	else
	{
		if (GGameType & GAME_HexenWorld)
		{
			offset = 676;
		}
		else
		{
			offset = 648;
		}
	}

	if (GGameType & GAME_Hexen2)
	{
		entFieldCameraMode.Init("cameramode", offset);
		offset += 4;
		entFieldMoveChain.Init("movechain", offset);
		offset += 4;
		entFieldChainMoved.Init("chainmoved", offset);
		offset += 4;
		//	string_index
		offset += 4;
		if (GGameType & GAME_HexenWorld)
		{
			entFieldGravity.Init("gravity", offset);
			offset += 4;
			entFieldSiegeTeam.Init("siege_team", offset);
			offset += 4;
		}
	}
	return offset;
}
