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

#define FIELD_FLOAT(name) \
	float Get ## name() \
	{ \
		return GetFloatField(entField ## name); \
	} \
	void Set ## name(float value) \
	{ \
		SetFloatField(entField ## name, value); \
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
