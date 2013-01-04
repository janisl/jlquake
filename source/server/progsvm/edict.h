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

#ifndef __PROGSVM_EDICT_H__
#define __PROGSVM_EDICT_H__

#include "../../common/console_variable.h"
#include "../../common/quakedefs.h"
#include "../../common/hexen2defs.h"
#include "../link.h"

#define MAX_ENT_LEAFS   16

typedef int func_t;
typedef int string_t;

union eval_t
{
	string_t string;
	float _float;
	float vector[3];
	func_t function;
	int _int;
	int edict;
};

class idEntVarDef
{
public:
	void Init(const char* name, int offset);

	const char* name;
	int offset;
};

struct entvars_t
{
	float modelindex;
	vec3_t absmin;
	vec3_t absmax;
	float ltime;
};

extern idEntVarDef entFieldLastRunTime;
extern idEntVarDef entFieldMoveType;
extern idEntVarDef entFieldSolid;
extern idEntVarDef entFieldOrigin;
extern idEntVarDef entFieldOldOrigin;
extern idEntVarDef entFieldVelocity;
extern idEntVarDef entFieldAngles;
extern idEntVarDef entFieldAVelocity;
extern idEntVarDef entFieldPunchAngle;
extern idEntVarDef entFieldClassName;
extern idEntVarDef entFieldModel;
extern idEntVarDef entFieldFrame;
extern idEntVarDef entFieldSkin;
extern idEntVarDef entFieldEffects;
extern idEntVarDef entFieldScale;
extern idEntVarDef entFieldDrawFlags;
extern idEntVarDef entFieldAbsLight;
extern idEntVarDef entFieldMins;
extern idEntVarDef entFieldMaxs;
extern idEntVarDef entFieldSize;
extern idEntVarDef entFieldHull;
extern idEntVarDef entFieldTouch;
extern idEntVarDef entFieldUse;
extern idEntVarDef entFieldThink;
extern idEntVarDef entFieldBlocked;
extern idEntVarDef entFieldNextThink;
extern idEntVarDef entFieldGroundEntity;
extern idEntVarDef entFieldHealth;
extern idEntVarDef entFieldStatsRestored;
extern idEntVarDef entFieldFrags;
extern idEntVarDef entFieldWeapon;
extern idEntVarDef entFieldWeaponModel;
extern idEntVarDef entFieldWeaponFrame;
extern idEntVarDef entFieldMaxHealth;
extern idEntVarDef entFieldCurrentAmmo;
extern idEntVarDef entFieldAmmoShells;
extern idEntVarDef entFieldAmmoNails;
extern idEntVarDef entFieldAmmoRockets;
extern idEntVarDef entFieldAmmoCells;
extern idEntVarDef entFieldPlayerClass;
extern idEntVarDef entFieldNextPlayerClass;
extern idEntVarDef entFieldHasPortals;
extern idEntVarDef entFieldBlueMana;
extern idEntVarDef entFieldGreenMana;
extern idEntVarDef entFieldMaxMana;
extern idEntVarDef entFieldArmorAmulet;
extern idEntVarDef entFieldArmorBracer;
extern idEntVarDef entFieldArmorBreastPlate;
extern idEntVarDef entFieldArmorHelmet;
extern idEntVarDef entFieldLevel;
extern idEntVarDef entFieldIntelligence;
extern idEntVarDef entFieldWisdom;
extern idEntVarDef entFieldDexterity;
extern idEntVarDef entFieldStrength;
extern idEntVarDef entFieldExperience;
extern idEntVarDef entFieldRingFlight;
extern idEntVarDef entFieldRingWater;
extern idEntVarDef entFieldRingTurning;
extern idEntVarDef entFieldRingRegeneration;
extern idEntVarDef entFieldHasteTime;
extern idEntVarDef entFieldTomeTime;
extern idEntVarDef entFieldPuzzleInv1;
extern idEntVarDef entFieldPuzzleInv2;
extern idEntVarDef entFieldPuzzleInv3;
extern idEntVarDef entFieldPuzzleInv4;
extern idEntVarDef entFieldPuzzleInv5;
extern idEntVarDef entFieldPuzzleInv6;
extern idEntVarDef entFieldPuzzleInv7;
extern idEntVarDef entFieldPuzzleInv8;
extern idEntVarDef entFieldItems;
extern idEntVarDef entFieldTakeDamage;
extern idEntVarDef entFieldChain;
extern idEntVarDef entFieldDeadFlag;
extern idEntVarDef entFieldViewOfs;
extern idEntVarDef entFieldButton0;
extern idEntVarDef entFieldButton2;
extern idEntVarDef entFieldImpulse;
extern idEntVarDef entFieldFixAngle;
extern idEntVarDef entFieldVAngle;
extern idEntVarDef entFieldIdealPitch;
extern idEntVarDef entFieldIdealRoll;
extern idEntVarDef entFieldHoverZ;
extern idEntVarDef entFieldNetName;
extern idEntVarDef entFieldEnemy;
extern idEntVarDef entFieldFlags;
extern idEntVarDef entFieldFlags2;
extern idEntVarDef entFieldColorMap;
extern idEntVarDef entFieldTeam;
extern idEntVarDef entFieldLightLevel;
extern idEntVarDef entFieldWpnSound;
extern idEntVarDef entFieldTargAng;
extern idEntVarDef entFieldTargPitch;
extern idEntVarDef entFieldTargDist;
extern idEntVarDef entFieldTeleportTime;
extern idEntVarDef entFieldArmorValue;
extern idEntVarDef entFieldWaterLevel;
extern idEntVarDef entFieldWaterType;
extern idEntVarDef entFieldFriction;
extern idEntVarDef entFieldIdealYaw;
extern idEntVarDef entFieldYawSpeed;
extern idEntVarDef entFieldGoalEntity;
extern idEntVarDef entFieldSpawnFlags;
extern idEntVarDef entFieldDmgTake;
extern idEntVarDef entFieldDmgSave;
extern idEntVarDef entFieldDmgInflictor;
extern idEntVarDef entFieldOwner;
extern idEntVarDef entFieldMoveDir;
extern idEntVarDef entFieldMessage;
extern idEntVarDef entFieldSounds;
extern idEntVarDef entFieldSoundType;	//	Could merge with entFieldSounds
extern idEntVarDef entFieldRingsActive;
extern idEntVarDef entFieldRingsLow;
extern idEntVarDef entFieldArtifacts;
extern idEntVarDef entFieldArtifactActive;
extern idEntVarDef entFieldArtifactLow;
extern idEntVarDef entFieldHasted;
extern idEntVarDef entFieldInventory;
extern idEntVarDef entFieldCntTorch;
extern idEntVarDef entFieldCntHBoost;
extern idEntVarDef entFieldCntSHBoost;
extern idEntVarDef entFieldCntManaBoost;
extern idEntVarDef entFieldCntTeleport;
extern idEntVarDef entFieldCntTome;
extern idEntVarDef entFieldCntSummon;
extern idEntVarDef entFieldCntInvisibility;
extern idEntVarDef entFieldCntGlyph;
extern idEntVarDef entFieldCntHaste;
extern idEntVarDef entFieldCntBlast;
extern idEntVarDef entFieldCntPolyMorph;
extern idEntVarDef entFieldCntFlight;
extern idEntVarDef entFieldCntCubeOfForce;
extern idEntVarDef entFieldCntInvincibility;
extern idEntVarDef entFieldCameraMode;
extern idEntVarDef entFieldMoveChain;
extern idEntVarDef entFieldChainMoved;
extern idEntVarDef entFieldGravity;
extern idEntVarDef entFieldSiegeTeam;

struct qhedict_t
{
	bool free;
	link_t area;			// linked to a division node or leaf

	int num_leafs;
	int LeafNums[MAX_ENT_LEAFS];

	q1entity_state_t q1_baseline;
	h2entity_state_t h2_baseline;

	float freetime;			// sv.time when the object was freed
	float alloctime;		// sv.time when the object was allocated
	entvars_t v;			// C exported fields from progs
	// other fields from progs come immediately after

	int GetIntField(idEntVarDef& field) const;
	void SetIntField(idEntVarDef& field, int value);
	float GetFloatField(idEntVarDef& field) const;
	void SetFloatField(idEntVarDef& field, float value);
	float* GetVectorField(idEntVarDef& field);
	void SetVectorField(idEntVarDef& field, const vec3_t value);

#define FIELD_FLOAT(name) \
	float Get ## name() const	\
	{ \
		return GetFloatField(entField ## name);	\
	} \
	void Set ## name(float value) \
	{ \
		SetFloatField(entField ## name, value);	\
	}
#define FIELD_VECTOR(name) \
	float* Get ## name() \
	{ \
		return GetVectorField(entField ## name); \
	} \
	void Set ## name(const vec3_t value) \
	{ \
		SetVectorField(entField ## name, value); \
	}
#define FIELD_STRING(name) \
	string_t Get ## name() const \
	{ \
		return GetIntField(entField ## name); \
	} \
	void Set ## name(string_t value) \
	{ \
		SetIntField(entField ## name, value); \
	}
#define FIELD_FUNC(name) \
	func_t Get ## name() const \
	{ \
		return GetIntField(entField ## name); \
	} \
	void Set ## name(func_t value) \
	{ \
		SetIntField(entField ## name, value); \
	}
#define FIELD_ENTITY(name) \
	int Get ## name() const \
	{ \
		return GetIntField(entField ## name); \
	} \
	void Set ## name(int value)	\
	{ \
		SetIntField(entField ## name, value); \
	}

	//	QuakeWorld and HexenWorld
	FIELD_FLOAT(LastRunTime)
	//	All games
	FIELD_FLOAT(MoveType)
	FIELD_FLOAT(Solid)
	FIELD_VECTOR(Origin)
	FIELD_VECTOR(OldOrigin)
	FIELD_VECTOR(Velocity)
	FIELD_VECTOR(Angles)
	FIELD_VECTOR(AVelocity)
	//	Quake, Hexen 2 and HexenWorld
	FIELD_VECTOR(PunchAngle)
	//	All games
	FIELD_STRING(ClassName)
	FIELD_STRING(Model)
	FIELD_FLOAT(Frame)
	FIELD_FLOAT(Skin)
	FIELD_FLOAT(Effects)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(Scale)
	FIELD_FLOAT(DrawFlags)
	FIELD_FLOAT(AbsLight)
	//	All games
	FIELD_VECTOR(Mins)
	FIELD_VECTOR(Maxs)
	FIELD_VECTOR(Size)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(Hull)
	//	All games
	FIELD_FUNC(Touch)
	FIELD_FUNC(Use)
	FIELD_FUNC(Think)
	FIELD_FUNC(Blocked)
	FIELD_FLOAT(NextThink)
	FIELD_ENTITY(GroundEntity)
	FIELD_FLOAT(Health)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(StatsRestored)
	//	All games
	FIELD_FLOAT(Frags)
	FIELD_FLOAT(Weapon)
	FIELD_STRING(WeaponModel)
	FIELD_FLOAT(WeaponFrame)
	FIELD_FLOAT(MaxHealth)
	//	Quake and QuakeWorld
	FIELD_FLOAT(CurrentAmmo)
	FIELD_FLOAT(AmmoShells)
	FIELD_FLOAT(AmmoNails)
	FIELD_FLOAT(AmmoRockets)
	FIELD_FLOAT(AmmoCells)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(PlayerClass)
	//	HexenWorld
	FIELD_FLOAT(NextPlayerClass)
	FIELD_FLOAT(HasPortals)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(BlueMana)
	FIELD_FLOAT(GreenMana)
	FIELD_FLOAT(MaxMana)
	FIELD_FLOAT(ArmorAmulet)
	FIELD_FLOAT(ArmorBracer)
	FIELD_FLOAT(ArmorBreastPlate)
	FIELD_FLOAT(ArmorHelmet)
	FIELD_FLOAT(Level)
	FIELD_FLOAT(Intelligence)
	FIELD_FLOAT(Wisdom)
	FIELD_FLOAT(Dexterity)
	FIELD_FLOAT(Strength)
	FIELD_FLOAT(Experience)
	FIELD_FLOAT(RingFlight)
	FIELD_FLOAT(RingWater)
	FIELD_FLOAT(RingTurning)
	FIELD_FLOAT(RingRegeneration)
	FIELD_FLOAT(HasteTime)
	FIELD_FLOAT(TomeTime)
	FIELD_STRING(PuzzleInv1)
	FIELD_STRING(PuzzleInv2)
	FIELD_STRING(PuzzleInv3)
	FIELD_STRING(PuzzleInv4)
	FIELD_STRING(PuzzleInv5)
	FIELD_STRING(PuzzleInv6)
	FIELD_STRING(PuzzleInv7)
	FIELD_STRING(PuzzleInv8)
	//	All games
	FIELD_FLOAT(Items)
	FIELD_FLOAT(TakeDamage)
	FIELD_ENTITY(Chain)
	FIELD_FLOAT(DeadFlag)
	FIELD_VECTOR(ViewOfs)
	FIELD_FLOAT(Button0)
	FIELD_FLOAT(Button2)
	FIELD_FLOAT(Impulse)
	FIELD_FLOAT(FixAngle)
	FIELD_VECTOR(VAngle)
	//	Quake, Hexen 2 and HexenWorld
	FIELD_FLOAT(IdealPitch)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(IdealRoll)
	FIELD_FLOAT(HoverZ)
	//	All games
	FIELD_STRING(NetName)
	FIELD_ENTITY(Enemy)
	FIELD_FLOAT(Flags)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(Flags2)
	//	All games
	FIELD_FLOAT(ColorMap)
	FIELD_FLOAT(Team)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(LightLevel)
	//	HexenWorld
	FIELD_FLOAT(WpnSound)
	FIELD_FLOAT(TargAng)
	FIELD_FLOAT(TargPitch)
	FIELD_FLOAT(TargDist)
	//	All games
	FIELD_FLOAT(TeleportTime)
	FIELD_FLOAT(ArmorValue)
	FIELD_FLOAT(WaterLevel)
	FIELD_FLOAT(WaterType)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(Friction)
	//	All games
	FIELD_FLOAT(IdealYaw)
	FIELD_FLOAT(YawSpeed)
	FIELD_ENTITY(GoalEntity)
	FIELD_FLOAT(SpawnFlags)
	FIELD_FLOAT(DmgTake)
	FIELD_FLOAT(DmgSave)
	FIELD_ENTITY(DmgInflictor)
	FIELD_ENTITY(Owner)
	FIELD_VECTOR(MoveDir)
	FIELD_STRING(Message)
	//	Quake and QuakeWorld
	FIELD_FLOAT(Sounds)
	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(SoundType)
	FIELD_FLOAT(RingsActive)
	FIELD_FLOAT(RingsLow)
	FIELD_FLOAT(Artifacts)
	FIELD_FLOAT(ArtifactActive)
	FIELD_FLOAT(ArtifactLow)
	FIELD_FLOAT(Hasted)
	FIELD_FLOAT(Inventory)
	FIELD_FLOAT(CntTorch)
	FIELD_FLOAT(CntHBoost)
	FIELD_FLOAT(CntSHBoost)
	FIELD_FLOAT(CntManaBoost)
	FIELD_FLOAT(CntTeleport)
	FIELD_FLOAT(CntTome)
	FIELD_FLOAT(CntSummon)
	FIELD_FLOAT(CntInvisibility)
	FIELD_FLOAT(CntGlyph)
	FIELD_FLOAT(CntHaste)
	FIELD_FLOAT(CntBlast)
	FIELD_FLOAT(CntPolyMorph)
	FIELD_FLOAT(CntFlight)
	FIELD_FLOAT(CntCubeOfForce)
	FIELD_FLOAT(CntInvincibility)
	FIELD_ENTITY(CameraMode)
	FIELD_ENTITY(MoveChain)
	FIELD_FUNC(ChainMoved)
	//	HexenWorld
	FIELD_FLOAT(Gravity)
	FIELD_FLOAT(SiegeTeam)

#undef FIELD_FLOAT
#undef FIELD_VECTOR
#undef FIELD_STRING
#undef FIELD_FUNC
#undef FIELD_ENTITY
};

extern Cvar* max_temp_edicts;

int ED_InitEntityFields();

qhedict_t* QH_EDICT_NUM(int n);
int QH_NUM_FOR_EDICT(const qhedict_t* e);

#define NEXT_EDICT(e) ((qhedict_t*)((byte*)e + pr_edict_size))

#define EDICT_TO_PROG(e) ((byte*)e - (byte*)sv.qh_edicts)
#define PROG_TO_EDICT(e) ((qhedict_t*)((byte*)sv.qh_edicts + e))

eval_t* GetEdictFieldValue(qhedict_t* ed, const char* field);
void ED_ClearGEFVCache();

void ED_ClearEdict(qhedict_t* e);

void ED_Print(const qhedict_t* ed);
void ED_PrintNum(int ent);
void ED_PrintEdicts();
void ED_PrintEdict_f();
void ED_Write(fileHandle_t f, const qhedict_t* ed);
const char* ED_ParseEdict(const char* data, qhedict_t* ent);
void ED_Count();
qhedict_t* ED_Alloc();
void ED_Free(qhedict_t* ed);
qhedict_t* ED_Alloc_Temp();
void ED_LoadFromFile(const char* data);

inline int qhedict_t::GetIntField(idEntVarDef& field) const
{
	return *(int*)((byte*)&v + field.offset);
}

inline void qhedict_t::SetIntField(idEntVarDef& field, int value)
{
	*(int*)((byte*)&v + field.offset) = value;
}

inline float qhedict_t::GetFloatField(idEntVarDef& field) const
{
	return *(float*)((byte*)&v + field.offset);
}

inline void qhedict_t::SetFloatField(idEntVarDef& field, float value)
{
	*(float*)((byte*)&v + field.offset) = value;
}

inline float* qhedict_t::GetVectorField(idEntVarDef& field)
{
	return (float*)((byte*)&v + field.offset);
}

inline void qhedict_t::SetVectorField(idEntVarDef& field, const vec3_t value)
{
	VectorCopy(value, (float*)((byte*)&v + field.offset));
}

#endif
