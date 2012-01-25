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

#define	MAX_ENT_LEAFS	16

extern idEntVarDef entFieldMaxHealth;
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

#ifdef HEXEN2_EDICT
	h2entity_state_t baseline;
#else
	q1entity_state_t baseline;
#endif
	
	float freetime;			// sv.time when the object was freed
#ifdef HEXEN2_EDICT
	float alloctime;		// sv.time when the object was allocated
#endif
	entvars_t v;			// C exported fields from progs
	// other fields from progs come immediately after

	int GetIntField(idEntVarDef& field);
	void SetIntField(idEntVarDef& field, int value);
	float GetFloatField(idEntVarDef& field);
	void SetFloatField(idEntVarDef& field, float value);
	float* GetVectorField(idEntVarDef& field);
	void SetVectorField(idEntVarDef& field, vec3_t value);

#define FIELD_FLOAT(name) \
	float Get ## name() \
	{ \
		return GetFloatField(entField ## name); \
	} \
	void Set ## name(float value) \
	{ \
		SetFloatField(entField ## name, value); \
	}
#define FIELD_VECTOR(name) \
	float* Get ## name() \
	{ \
		return GetVectorField(entField ## name); \
	} \
	void Set ## name(vec3_t value) \
	{ \
		SetVectorField(entField ## name, value); \
	}
#define FIELD_STRING(name) \
	string_t Get ## name() \
	{ \
		return GetIntField(entField ## name); \
	} \
	void Set ## name(string_t value) \
	{ \
		SetIntField(entField ## name, value); \
	}
#define FIELD_FUNC(name) \
	func_t Get ## name() \
	{ \
		return GetIntField(entField ## name); \
	} \
	void Set ## name(func_t value) \
	{ \
		SetIntField(entField ## name, value); \
	}
#define FIELD_ENTITY(name) \
	int Get ## name() \
	{ \
		return GetIntField(entField ## name); \
	} \
	void Set ## name(int value) \
	{ \
		SetIntField(entField ## name, value); \
	}

	//	Hexen 2 and HexenWorld
	FIELD_FLOAT(MaxHealth)
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
#undef FIELD_FUNC
#undef FIELD_ENTITY
};

int ED_InitEntityFields();

inline int qhedict_t::GetIntField(idEntVarDef& field)
{
	return *(int*)((byte*)&v + field.offset);
}

inline void qhedict_t::SetIntField(idEntVarDef& field, int value)
{
	*(int*)((byte*)&v + field.offset) = value;
}

inline float qhedict_t::GetFloatField(idEntVarDef& field)
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

inline void qhedict_t::SetVectorField(idEntVarDef& field, vec3_t value)
{
	VectorCopy(value, (float*)((byte*)&v + field.offset));
}
