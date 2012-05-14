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

#include "progsvm.h"

idEntVarDef entFieldLastRunTime;
idEntVarDef entFieldMoveType;
idEntVarDef entFieldSolid;
idEntVarDef entFieldOrigin;
idEntVarDef entFieldOldOrigin;
idEntVarDef entFieldVelocity;
idEntVarDef entFieldAngles;
idEntVarDef entFieldAVelocity;
idEntVarDef entFieldPunchAngle;
idEntVarDef entFieldClassName;
idEntVarDef entFieldModel;
idEntVarDef entFieldFrame;
idEntVarDef entFieldSkin;
idEntVarDef entFieldEffects;
idEntVarDef entFieldScale;
idEntVarDef entFieldDrawFlags;
idEntVarDef entFieldAbsLight;
idEntVarDef entFieldMins;
idEntVarDef entFieldMaxs;
idEntVarDef entFieldSize;
idEntVarDef entFieldHull;
idEntVarDef entFieldTouch;
idEntVarDef entFieldUse;
idEntVarDef entFieldThink;
idEntVarDef entFieldBlocked;
idEntVarDef entFieldNextThink;
idEntVarDef entFieldGroundEntity;
idEntVarDef entFieldHealth;
idEntVarDef entFieldStatsRestored;
idEntVarDef entFieldFrags;
idEntVarDef entFieldWeapon;
idEntVarDef entFieldWeaponModel;
idEntVarDef entFieldWeaponFrame;
idEntVarDef entFieldMaxHealth;
idEntVarDef entFieldCurrentAmmo;
idEntVarDef entFieldAmmoShells;
idEntVarDef entFieldAmmoNails;
idEntVarDef entFieldAmmoRockets;
idEntVarDef entFieldAmmoCells;
idEntVarDef entFieldPlayerClass;
idEntVarDef entFieldNextPlayerClass;
idEntVarDef entFieldHasPortals;
idEntVarDef entFieldBlueMana;
idEntVarDef entFieldGreenMana;
idEntVarDef entFieldMaxMana;
idEntVarDef entFieldArmorAmulet;
idEntVarDef entFieldArmorBracer;
idEntVarDef entFieldArmorBreastPlate;
idEntVarDef entFieldArmorHelmet;
idEntVarDef entFieldLevel;
idEntVarDef entFieldIntelligence;
idEntVarDef entFieldWisdom;
idEntVarDef entFieldDexterity;
idEntVarDef entFieldStrength;
idEntVarDef entFieldExperience;
idEntVarDef entFieldRingFlight;
idEntVarDef entFieldRingWater;
idEntVarDef entFieldRingTurning;
idEntVarDef entFieldRingRegeneration;
idEntVarDef entFieldHasteTime;
idEntVarDef entFieldTomeTime;
idEntVarDef entFieldPuzzleInv1;
idEntVarDef entFieldPuzzleInv2;
idEntVarDef entFieldPuzzleInv3;
idEntVarDef entFieldPuzzleInv4;
idEntVarDef entFieldPuzzleInv5;
idEntVarDef entFieldPuzzleInv6;
idEntVarDef entFieldPuzzleInv7;
idEntVarDef entFieldPuzzleInv8;
idEntVarDef entFieldItems;
idEntVarDef entFieldTakeDamage;
idEntVarDef entFieldChain;
idEntVarDef entFieldDeadFlag;
idEntVarDef entFieldViewOfs;
idEntVarDef entFieldButton0;
idEntVarDef entFieldButton2;
idEntVarDef entFieldImpulse;
idEntVarDef entFieldFixAngle;
idEntVarDef entFieldVAngle;
idEntVarDef entFieldIdealPitch;
idEntVarDef entFieldIdealRoll;
idEntVarDef entFieldHoverZ;
idEntVarDef entFieldNetName;
idEntVarDef entFieldEnemy;
idEntVarDef entFieldFlags;
idEntVarDef entFieldFlags2;
idEntVarDef entFieldColorMap;
idEntVarDef entFieldTeam;
idEntVarDef entFieldLightLevel;
idEntVarDef entFieldWpnSound;
idEntVarDef entFieldTargAng;
idEntVarDef entFieldTargPitch;
idEntVarDef entFieldTargDist;
idEntVarDef entFieldTeleportTime;
idEntVarDef entFieldArmorValue;
idEntVarDef entFieldWaterLevel;
idEntVarDef entFieldWaterType;
idEntVarDef entFieldFriction;
idEntVarDef entFieldIdealYaw;
idEntVarDef entFieldYawSpeed;
idEntVarDef entFieldGoalEntity;
idEntVarDef entFieldSpawnFlags;
idEntVarDef entFieldDmgTake;
idEntVarDef entFieldDmgSave;
idEntVarDef entFieldDmgInflictor;
idEntVarDef entFieldOwner;
idEntVarDef entFieldMoveDir;
idEntVarDef entFieldMessage;
idEntVarDef entFieldSounds;
idEntVarDef entFieldSoundType;
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
	int offset = 32;

	if (((GGameType & GAME_Quake) && (GGameType & GAME_QuakeWorld)) ||
		((GGameType & GAME_Hexen2) && (GGameType & GAME_HexenWorld)))
	{
		entFieldLastRunTime.Init("lastruntime", offset);
		offset += 4;
	}
	entFieldMoveType.Init("movetype", offset);
	offset += 4;
	entFieldSolid.Init("solid", offset);
	offset += 4;
	entFieldOrigin.Init("origin", offset);
	offset += 12;
	entFieldOldOrigin.Init("oldorigin", offset);
	offset += 12;
	entFieldVelocity.Init("velocity", offset);
	offset += 12;
	entFieldAngles.Init("angles", offset);
	offset += 12;
	entFieldAVelocity.Init("avelocity", offset);
	offset += 12;
	if (!(GGameType & GAME_QuakeWorld))
	{
		entFieldPunchAngle.Init("punchangle", offset);
		offset += 12;
	}
	entFieldClassName.Init("classname", offset);
	offset += 4;
	entFieldModel.Init("model", offset);
	offset += 4;
	entFieldFrame.Init("frame", offset);
	offset += 4;
	entFieldSkin.Init("skin", offset);
	offset += 4;
	entFieldEffects.Init("effects", offset);
	offset += 4;
	if (GGameType & GAME_Hexen2)
	{
		entFieldScale.Init("scale", offset);
		offset += 4;
		entFieldDrawFlags.Init("drawflags", offset);
		offset += 4;
		entFieldAbsLight.Init("abslight", offset);
		offset += 4;
	}
	entFieldMins.Init("mins", offset);
	offset += 12;
	entFieldMaxs.Init("maxs", offset);
	offset += 12;
	entFieldSize.Init("size", offset);
	offset += 12;
	if (GGameType & GAME_Hexen2)
	{
		entFieldHull.Init("hull", offset);
		offset += 4;
	}
	entFieldTouch.Init("touch", offset);
	offset += 4;
	entFieldUse.Init("use", offset);
	offset += 4;
	entFieldThink.Init("think", offset);
	offset += 4;
	entFieldBlocked.Init("blocked", offset);
	offset += 4;
	entFieldNextThink.Init("nextthink", offset);
	offset += 4;
	entFieldGroundEntity.Init("groundentity", offset);
	offset += 4;
	if (GGameType & GAME_Quake)
	{
		entFieldHealth.Init("health", offset);
		offset += 4;
	}
	if (GGameType & GAME_Hexen2)
	{
		entFieldStatsRestored.Init("stats_restored", offset);
		offset += 4;
	}
	entFieldFrags.Init("frags", offset);
	offset += 4;
	entFieldWeapon.Init("weapon", offset);
	offset += 4;
	entFieldWeaponModel.Init("weaponmodel", offset);
	offset += 4;
	entFieldWeaponFrame.Init("weaponframe", offset);
	offset += 4;
	if (GGameType & GAME_Quake)
	{
		entFieldCurrentAmmo.Init("currentammo", offset);
		offset += 4;
		entFieldAmmoShells.Init("ammo_shells", offset);
		offset += 4;
		entFieldAmmoNails.Init("ammo_nails", offset);
		offset += 4;
		entFieldAmmoRockets.Init("ammo_rockets", offset);
		offset += 4;
		entFieldAmmoCells.Init("ammo_cells", offset);
		offset += 4;
	}
	if (GGameType & GAME_Hexen2)
	{
		entFieldHealth.Init("health", offset);
		offset += 4;
		entFieldMaxHealth.Init("max_health", offset);
		offset += 4;
		entFieldPlayerClass.Init("playerclass", offset);
		offset += 4;
		if (GGameType & GAME_HexenWorld)
		{
			entFieldNextPlayerClass.Init("next_playerclass", offset);
			offset += 4;
			entFieldHasPortals.Init("has_portals", offset);
			offset += 4;
		}
		entFieldBlueMana.Init("bluemana", offset);
		offset += 4;
		entFieldGreenMana.Init("greenmana", offset);
		offset += 4;
		entFieldMaxMana.Init("max_mana", offset);
		offset += 4;
		entFieldArmorAmulet.Init("armor_amulet", offset);
		offset += 4;
		entFieldArmorBracer.Init("armor_bracer", offset);
		offset += 4;
		entFieldArmorBreastPlate.Init("armor_breastplate", offset);
		offset += 4;
		entFieldArmorHelmet.Init("armor_helmet", offset);
		offset += 4;
		entFieldLevel.Init("level", offset);
		offset += 4;
		entFieldIntelligence.Init("intelligence", offset);
		offset += 4;
		entFieldWisdom.Init("wisdom", offset);
		offset += 4;
		entFieldDexterity.Init("dexterity", offset);
		offset += 4;
		entFieldStrength.Init("strength", offset);
		offset += 4;
		entFieldExperience.Init("experience", offset);
		offset += 4;
		entFieldRingFlight.Init("ring_flight", offset);
		offset += 4;
		entFieldRingWater.Init("ring_water", offset);
		offset += 4;
		entFieldRingTurning.Init("ring_turning", offset);
		offset += 4;
		entFieldRingRegeneration.Init("ring_regeneration", offset);
		offset += 4;
		entFieldHasteTime.Init("haste_time", offset);
		offset += 4;
		entFieldTomeTime.Init("tome_time", offset);
		offset += 4;
		entFieldPuzzleInv1.Init("puzzle_inv1", offset);
		offset += 4;
		entFieldPuzzleInv2.Init("puzzle_inv2", offset);
		offset += 4;
		entFieldPuzzleInv3.Init("puzzle_inv3", offset);
		offset += 4;
		entFieldPuzzleInv4.Init("puzzle_inv4", offset);
		offset += 4;
		entFieldPuzzleInv5.Init("puzzle_inv5", offset);
		offset += 4;
		entFieldPuzzleInv6.Init("puzzle_inv6", offset);
		offset += 4;
		entFieldPuzzleInv7.Init("puzzle_inv7", offset);
		offset += 4;
		entFieldPuzzleInv8.Init("puzzle_inv8", offset);
		offset += 4;
		//	experience_value
		offset += 4;
	}
	entFieldItems.Init("items", offset);
	offset += 4;
	entFieldTakeDamage.Init("takedamage", offset);
	offset += 4;
	entFieldChain.Init("chain", offset);
	offset += 4;
	entFieldDeadFlag.Init("deadflag", offset);
	offset += 4;
	entFieldViewOfs.Init("view_ofs", offset);
	offset += 12;
	entFieldButton0.Init("button0", offset);
	offset += 4;
	//	button1
	offset += 4;
	entFieldButton2.Init("button2", offset);
	offset += 4;
	entFieldImpulse.Init("impulse", offset);
	offset += 4;
	entFieldFixAngle.Init("fixangle", offset);
	offset += 4;
	entFieldVAngle.Init("v_angle", offset);
	offset += 12;
	if (!(GGameType & GAME_QuakeWorld))
	{
		entFieldIdealPitch.Init("idealpitch", offset);
		offset += 4;
	}
	if (GGameType & GAME_Hexen2)
	{
		entFieldIdealRoll.Init("idealroll", offset);
		offset += 4;
		entFieldHoverZ.Init("hoverz", offset);
		offset += 4;
	}
	entFieldNetName.Init("netname", offset);
	offset += 4;
	entFieldEnemy.Init("enemy", offset);
	offset += 4;
	entFieldFlags.Init("flags", offset);
	offset += 4;
	if (GGameType & GAME_Hexen2)
	{
		entFieldFlags2.Init("flags2", offset);
		offset += 4;
		//	artifact_flags
		offset += 4;
	}
	entFieldColorMap.Init("colormap", offset);
	offset += 4;
	entFieldTeam.Init("team", offset);
	offset += 4;
	if (GGameType & GAME_Quake)
	{
		entFieldMaxHealth.Init("max_health", offset);
		offset += 4;
	}
	if (GGameType & GAME_Hexen2)
	{
		entFieldLightLevel.Init("light_level", offset);
		offset += 4;
		if (GGameType & GAME_HexenWorld)
		{
			entFieldWpnSound.Init("wpn_sound", offset);
			offset += 4;
			entFieldTargAng.Init("targAng", offset);
			offset += 4;
			entFieldTargPitch.Init("targPitch", offset);
			offset += 4;
			entFieldTargDist.Init("targDist", offset);
			offset += 4;
		}
	}
	entFieldTeleportTime.Init("teleport_time", offset);
	offset += 4;
	//	armortype
	offset += 4;
	entFieldArmorValue.Init("armorvalue", offset);
	offset += 4;
	entFieldWaterLevel.Init("waterlevel", offset);
	offset += 4;
	entFieldWaterType.Init("watertype", offset);
	offset += 4;
	if (GGameType & GAME_Hexen2)
	{
		entFieldFriction.Init("friction", offset);
		offset += 4;
	}
	entFieldIdealYaw.Init("ideal_yaw", offset);
	offset += 4;
	entFieldYawSpeed.Init("yaw_speed", offset);
	offset += 4;
	if (GGameType & GAME_Quake)
	{
		//	aiment
		offset += 4;
	}
	entFieldGoalEntity.Init("goalentity", offset);
	offset += 4;
	entFieldSpawnFlags.Init("spawnflags", offset);
	offset += 4;
	//	target
	offset += 4;
	//	targetname
	offset += 4;
	entFieldDmgTake.Init("dmg_take", offset);
	offset += 4;
	entFieldDmgSave.Init("dmg_save", offset);
	offset += 4;
	entFieldDmgInflictor.Init("dmg_inflictor", offset);
	offset += 4;
	entFieldOwner.Init("owner", offset);
	offset += 4;
	entFieldMoveDir.Init("movedir", offset);
	offset += 12;
	entFieldMessage.Init("message", offset);
	offset += 4;
	if (GGameType & GAME_Quake)
	{
		entFieldSounds.Init("sounds", offset);
		offset += 4;
	}
	if (GGameType & GAME_Hexen2)
	{
		entFieldSoundType.Init("soundtype", offset);
		offset += 4;
	}
	//	noise
	offset += 4;
	//	noise1
	offset += 4;
	//	noise2
	offset += 4;
	//	noise3
	offset += 4;
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
