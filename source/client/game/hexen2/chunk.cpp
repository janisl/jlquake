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

#include "../../client.h"
#include "local.h"

static void CLH2_InitChunkGlass(float final, qhandle_t* model)
{
	if (final < 0.20)
	{
		*model = R_RegisterModel("models/shard1.mdl");
	}
	else if (final < 0.40)
	{
		*model = R_RegisterModel("models/shard2.mdl");
	}
	else if (final < 0.60)
	{
		*model = R_RegisterModel("models/shard3.mdl");
	}
	else if (final < 0.80)
	{
		*model = R_RegisterModel("models/shard4.mdl");
	}
	else 
	{
		*model = R_RegisterModel("models/shard5.mdl");
	}
}

static void CLH2_InitChunkClearGlass(float final, qhandle_t* model, int* skinNum, int* drawFlags)
{
	CLH2_InitChunkGlass(final, model);
	*skinNum = 1;
	*drawFlags |= H2DRF_TRANSLUCENT;
}

static void CLH2_InitChunkRedGlass(float final, qhandle_t* model, int* skinNum)
{
	CLH2_InitChunkGlass(final, model);
	*skinNum = 2;
}

static void CLH2_InitChunkWebs(float final, qhandle_t* model, int* skinNum, int* drawFlags)
{
	CLH2_InitChunkGlass(final, model);
	*skinNum = 3;
	*drawFlags |= H2DRF_TRANSLUCENT;
}

static void CLH2_InitChunkWood(float final, qhandle_t* model)
{
	if (final < 0.25)
	{
		*model = R_RegisterModel("models/splnter1.mdl");
	}
	else if (final < 0.50)
	{
		*model = R_RegisterModel("models/splnter2.mdl");
	}
	else if (final < 0.75)
	{
		*model = R_RegisterModel("models/splnter3.mdl");
	}
	else
	{
		*model = R_RegisterModel("models/splnter4.mdl");
	}
}

static void CLH2_InitChunkMetal(float final, qhandle_t* model)
{
	if (final < 0.25)
	{
		*model = R_RegisterModel("models/metlchk1.mdl");
	}
	else if (final < 0.50)
	{
		*model = R_RegisterModel("models/metlchk2.mdl");
	}
	else if (final < 0.75)
	{
		*model = R_RegisterModel("models/metlchk3.mdl");
	}
	else 
	{
		*model = R_RegisterModel("models/metlchk4.mdl");
	}
}

static void CLH2_InitChunkFlesh(float final, qhandle_t* model)
{
	if (final < 0.33)
	{
		*model = R_RegisterModel("models/flesh1.mdl");
	}
	else if (final < 0.66)
	{
		*model = R_RegisterModel("models/flesh2.mdl");
	}
	else
	{
		*model = R_RegisterModel("models/flesh3.mdl");
	}
}

static void CLH2_InitChunkGreyStone(float final, qhandle_t* model, int* skinNum)
{
	if (final < 0.25)
	{
		*model = R_RegisterModel("models/schunk1.mdl");
	}
	else if (final < 0.50)
	{
		*model = R_RegisterModel("models/schunk2.mdl");
	}
	else if (final < 0.75)
	{
		*model = R_RegisterModel("models/schunk3.mdl");
	}
	else 
	{
		*model = R_RegisterModel("models/schunk4.mdl");
	}
	*skinNum = 0;
}

static void CLH2_InitChunkBrownStone(float final, qhandle_t* model, int* skinNum)
{
	CLH2_InitChunkGreyStone(final, model, skinNum);
	*skinNum = 1;
}

static void CLH2_InitChunkClay(float final, qhandle_t* model)
{
	if (final < 0.25)
	{
		*model = R_RegisterModel("models/clshard1.mdl");
	}
	else if (final < 0.50)
	{
		*model = R_RegisterModel("models/clshard2.mdl");
	}
	else if (final < 0.75)
	{
		*model = R_RegisterModel("models/clshard3.mdl");
	}
	else 
	{
		*model = R_RegisterModel("models/clshard4.mdl");
	}
}

static void CLH2_InitChunkBone(float final, qhandle_t* model, int* skinNum)
{
	CLH2_InitChunkClay(final, model);
	*skinNum = 1;
}

static void CLH2_InitChunkLeaves(float final, qhandle_t* model)
{
	if (final < 0.33)
	{
		*model = R_RegisterModel("models/leafchk1.mdl");
	}
	else if (final < 0.66)
	{
		*model = R_RegisterModel("models/leafchk2.mdl");
	}
	else
	{
		*model = R_RegisterModel("models/leafchk3.mdl");
	}
}

static void CLH2_InitChunkHay(float final, qhandle_t* model)
{
	if (final < 0.33)
	{
		*model = R_RegisterModel("models/hay1.mdl");
	}
	else if (final < 0.66)
	{
		*model = R_RegisterModel("models/hay2.mdl");
	}
	else
	{
		*model = R_RegisterModel("models/hay3.mdl");
	}
}

static void CLH2_InitChunkCloth(float final, qhandle_t* model)
{
	if (final < 0.33)
	{
		*model = R_RegisterModel("models/clthchk1.mdl");
	}
	else if (final < 0.66)
	{
		*model = R_RegisterModel("models/clthchk2.mdl");
	}
	else
	{
		*model = R_RegisterModel("models/clthchk3.mdl");
	}
}

static void CLH2_InitChunkWoodAndLeaf(float final, qhandle_t* model)
{
	if (final < 0.14)
	{
		*model = R_RegisterModel("models/splnter1.mdl");
	}
	else if (final < 0.28)
	{
		*model = R_RegisterModel("models/leafchk1.mdl");
	}
	else if (final < 0.42)
	{
		*model = R_RegisterModel("models/splnter2.mdl");
	}
	else if (final < 0.56)
	{
		*model = R_RegisterModel("models/leafchk2.mdl");
	}
	else if (final < 0.70)
	{
		*model = R_RegisterModel("models/splnter3.mdl");
	}
	else if (final < 0.84)
	{
		*model = R_RegisterModel("models/leafchk3.mdl");
	}
	else
	{
		*model = R_RegisterModel("models/splnter4.mdl");
	}
}

static void CLH2_InitChunkWoodAndMetal(float final, qhandle_t* model)
{
	if (final < 0.125)
	{
		*model = R_RegisterModel("models/splnter1.mdl");
	}
	else if (final < 0.25)
	{
		*model = R_RegisterModel("models/metlchk1.mdl");
	}
	else if (final < 0.375)
	{
		*model = R_RegisterModel("models/splnter2.mdl");
	}
	else if (final < 0.50)
	{
		*model = R_RegisterModel("models/metlchk2.mdl");
	}
	else if (final < 0.625)
	{
		*model = R_RegisterModel("models/splnter3.mdl");
	}
	else if (final < 0.75)
	{
		*model = R_RegisterModel("models/metlchk3.mdl");
	}
	else if (final < 0.875)
	{
		*model = R_RegisterModel("models/splnter4.mdl");
	}
	else
	{
		*model = R_RegisterModel("models/metlchk4.mdl");
	}
}

static void CLH2_InitChunkWoodAndStone(float final, qhandle_t* model)
{
	if (final < 0.125)
	{
		*model = R_RegisterModel("models/splnter1.mdl");
	}
	else if (final < 0.25)
	{
		*model = R_RegisterModel("models/schunk1.mdl");
	}
	else if (final < 0.375)
	{
		*model = R_RegisterModel("models/splnter2.mdl");
	}
	else if (final < 0.50)
	{
		*model = R_RegisterModel("models/schunk2.mdl");
	}
	else if (final < 0.625)
	{
		*model = R_RegisterModel("models/splnter3.mdl");
	}
	else if (final < 0.75)
	{
		*model = R_RegisterModel("models/schunk3.mdl");
	}
	else if (final < 0.875)
	{
		*model = R_RegisterModel("models/splnter4.mdl");
	}
	else 
	{
		*model = R_RegisterModel("models/schunk4.mdl");
	}
}

static void CLH2_InitChunkMetalAndStone(float final, qhandle_t* model)
{
	if (final < 0.125)
	{
		*model = R_RegisterModel("models/metlchk1.mdl");
	}
	else if (final < 0.25)
	{
		*model = R_RegisterModel("models/schunk1.mdl");
	}
	else if (final < 0.375)
	{
		*model = R_RegisterModel("models/metlchk2.mdl");
	}
	else if (final < 0.50)
	{
		*model = R_RegisterModel("models/schunk2.mdl");
	}
	else if (final < 0.625)
	{
		*model = R_RegisterModel("models/metlchk3.mdl");
	}
	else if (final < 0.75)
	{
		*model = R_RegisterModel("models/schunk3.mdl");
	}
	else if (final < 0.875)
	{
		*model = R_RegisterModel("models/metlchk4.mdl");
	}
	else 
	{
		*model = R_RegisterModel("models/schunk4.mdl");
	}
}

static void CLH2_InitChunkMetalAndCloth(float final, qhandle_t* model)
{
	if (final < 0.14)
	{
		*model = R_RegisterModel("models/metlchk1.mdl");
	}
	else if (final < 0.28)
	{
		*model = R_RegisterModel("models/clthchk1.mdl");
	}
	else if (final < 0.42)
	{
		*model = R_RegisterModel("models/metlchk2.mdl");
	}
	else if (final < 0.56)
	{
		*model = R_RegisterModel("models/clthchk2.mdl");
	}
	else if (final < 0.70)
	{
		*model = R_RegisterModel("models/metlchk3.mdl");
	}
	else if (final < 0.84)
	{
		*model = R_RegisterModel("models/clthchk3.mdl");
	}
	else 
	{
		*model = R_RegisterModel("models/metlchk4.mdl");
	}
}

static void CLH2_InitChunkIce(qhandle_t* model, int* skinNum, int* drawFlags, int* frame, int* absoluteLight)
{
	*model = R_RegisterModel("models/shard.mdl");
	*skinNum=0;
	*frame = rand() % 2;
	*drawFlags |= H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT;
	*absoluteLight = 0.5;
}

static void CLH2_InitChunkMeteor(qhandle_t* model, int* skinNum)
{
	*model = R_RegisterModel("models/tempmetr.mdl");
	*skinNum = 0;
}

static void CLH2_InitChunkAcid(qhandle_t* model, int* skinNum)
{
	// no spinning if possible...
	*model = R_RegisterModel("models/sucwp2p.mdl");
	*skinNum = 0;
}

static void CLH2_InitChunkGreenFlesh(float final, qhandle_t* model, int* skinNum)
{
	// spider guts
	if (final < 0.33)
	{
		*model = R_RegisterModel("models/sflesh1.mdl");
	}
	else if (final < 0.66)
	{
		*model = R_RegisterModel("models/sflesh2.mdl");
	}
	else
	{
		*model = R_RegisterModel("models/sflesh3.mdl");
	}
	*skinNum = 0;
}

void CLH2_InitChunkModel(int chType, int* model, int* skinNum, int* drawFlags, int* frame, int* absoluteLight)
{
	float final = (rand() % 100) * .01;
	if (chType == H2THINGTYPE_GLASS)
	{
		CLH2_InitChunkGlass(final, model);
	}
	else if (chType == H2THINGTYPE_CLEARGLASS)
	{
		CLH2_InitChunkClearGlass(final, model, skinNum, drawFlags);
	}
	else if (chType == H2THINGTYPE_REDGLASS)
	{
		CLH2_InitChunkRedGlass(final, model, skinNum);
	}
	else if (chType == H2THINGTYPE_WEBS)
	{
		CLH2_InitChunkWebs(final, model, skinNum, drawFlags);
	}
	else if (chType == H2THINGTYPE_WOOD)
	{
		CLH2_InitChunkWood(final, model);
	}
	else if (chType == H2THINGTYPE_METAL)
	{
		CLH2_InitChunkMetal(final, model);
	}
	else if (chType == H2THINGTYPE_FLESH)
	{
		CLH2_InitChunkFlesh(final, model);
	}
	else if (chType == H2THINGTYPE_BROWNSTONE ||
		((GGameType & GAME_HexenWorld) && chType == H2THINGTYPE_DIRT))
	{
		CLH2_InitChunkBrownStone(final, model, skinNum);
	}
	else if (chType == H2THINGTYPE_CLAY)
	{
		CLH2_InitChunkClay(final, model);
	}
	else if (!(GGameType & GAME_HexenWorld) && chType == H2THINGTYPE_BONE)
	{
		CLH2_InitChunkBone(final, model, skinNum);
	}
	else if (chType == H2THINGTYPE_LEAVES)
	{
		CLH2_InitChunkLeaves(final, model);
	}
	else if (chType == H2THINGTYPE_HAY)
	{
		CLH2_InitChunkHay(final, model);
	}
	else if (chType == H2THINGTYPE_CLOTH)
	{
		CLH2_InitChunkCloth(final, model);
	}
	else if (chType == H2THINGTYPE_WOOD_LEAF)
	{
		CLH2_InitChunkWoodAndLeaf(final, model);
	}
	else if (chType == H2THINGTYPE_WOOD_METAL)
	{
		CLH2_InitChunkWoodAndMetal(final, model);
	}
	else if (chType == H2THINGTYPE_WOOD_STONE)
	{
		CLH2_InitChunkWoodAndStone(final, model);
	}
	else if (chType == H2THINGTYPE_METAL_STONE)
	{
		CLH2_InitChunkMetalAndStone(final, model);
	}
	else if (chType == H2THINGTYPE_METAL_CLOTH)
	{
		CLH2_InitChunkMetalAndCloth(final, model);
	}
	else if (chType == H2THINGTYPE_ICE)
	{
		CLH2_InitChunkIce(model, skinNum, drawFlags, frame, absoluteLight);
	}
	else if (chType == H2THINGTYPE_METEOR)
	{
		CLH2_InitChunkMeteor(model, skinNum);
	}
	else if (chType == H2THINGTYPE_ACID)
	{
		CLH2_InitChunkAcid(model, skinNum);
	}
	else if (chType == H2THINGTYPE_GREENFLESH)
	{
		CLH2_InitChunkGreenFlesh(final, model, skinNum);
	}
	else
	{
		CLH2_InitChunkGreyStone(final, model, skinNum);
	}
}
