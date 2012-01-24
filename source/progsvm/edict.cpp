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

idEntVarDef entFieldRingsActive;
idEntVarDef entFieldRingsLow;
idEntVarDef entFieldArtifacts;
idEntVarDef entFieldArtifactActive;
idEntVarDef entFieldArtifactLow;
idEntVarDef entFieldHasted;
idEntVarDef entFieldInventory;
idEntVarDef entFieldCntTorch;
idEntVarDef entFieldCntHBoost;
idEntVarDef entFieldCntSHBoost;
idEntVarDef entFieldCntManaBoost;
idEntVarDef entFieldCntTeleport;
idEntVarDef entFieldCntTome;
idEntVarDef entFieldCntSummon;
idEntVarDef entFieldCntInvisibility;
idEntVarDef entFieldCntGlyph;
idEntVarDef entFieldCntHaste;
idEntVarDef entFieldCntBlast;
idEntVarDef entFieldCntPolyMorph;
idEntVarDef entFieldCntFlight;
idEntVarDef entFieldCntCubeOfForce;
idEntVarDef entFieldCntInvincibility;
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
			offset = 584;
		}
		else
		{
			offset = 556;
		}
	}

	if (GGameType & GAME_Hexen2)
	{
		//	rings
		offset += 4;
		entFieldRingsActive.Init("rings_active", offset);
		offset += 4;
		entFieldRingsLow.Init("rings_low", offset);
		offset += 4;
		entFieldArtifacts.Init("artifacts", offset);
		offset += 4;
		entFieldArtifactActive.Init("artifact_active", offset);
		offset += 4;
		entFieldArtifactLow.Init("artifact_low", offset);
		offset += 4;
		entFieldHasted.Init("hasted", offset);
		offset += 4;
		entFieldInventory.Init("inventory", offset);
		offset += 4;
		entFieldCntTorch.Init("cnt_torch", offset);
		offset += 4;
		entFieldCntHBoost.Init("cnt_h_boost", offset);
		offset += 4;
		entFieldCntSHBoost.Init("cnt_sh_boost", offset);
		offset += 4;
		entFieldCntManaBoost.Init("cnt_mana_boost", offset);
		offset += 4;
		entFieldCntTeleport.Init("cnt_teleport", offset);
		offset += 4;
		entFieldCntTome.Init("cnt_tome", offset);
		offset += 4;
		entFieldCntSummon.Init("cnt_summon", offset);
		offset += 4;
		entFieldCntInvisibility.Init("cnt_invisibility", offset);
		offset += 4;
		entFieldCntGlyph.Init("cnt_glyph", offset);
		offset += 4;
		entFieldCntHaste.Init("cnt_haste", offset);
		offset += 4;
		entFieldCntBlast.Init("cnt_blast", offset);
		offset += 4;
		entFieldCntPolyMorph.Init("cnt_polymorph", offset);
		offset += 4;
		entFieldCntFlight.Init("cnt_flight", offset);
		offset += 4;
		entFieldCntCubeOfForce.Init("cnt_cubeofforce", offset);
		offset += 4;
		entFieldCntInvincibility.Init("cnt_invincibility", offset);
		offset += 4;
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
