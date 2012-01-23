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

Cvar* cl_doubleeyes;

q1entity_state_t clq1_baselines[MAX_EDICTS_Q1];
q1entity_t clq1_entities[MAX_EDICTS_Q1];
q1entity_t clq1_static_entities[MAX_STATIC_ENTITIES_Q1];

image_t* clq1_playertextures[BIGGEST_MAX_CLIENTS_Q1];	// color translated skins

int clq1_playerindex;

//	This error checks and tracks the total number of entities
q1entity_t* CLQ1_EntityNum(int number)
{
	if (number >= cl_common->qh_num_entities)
	{
		if (number >= MAX_EDICTS_Q1)
		{
			throw DropException(va("CLQ1_EntityNum: %i is an invalid number", number));
		}
		while (cl_common->qh_num_entities <= number)
		{
			clq1_entities[cl_common->qh_num_entities].state.colormap = 0;
			cl_common->qh_num_entities++;
		}
	}
	return &clq1_entities[number];
}

static void CLQ1_ParseBaseline(QMsg& message, q1entity_state_t *es)
{
	es->modelindex = message.ReadByte();
	es->frame = message.ReadByte();
	es->colormap = message.ReadByte();
	es->skinnum = message.ReadByte();
	for (int i = 0; i < 3; i++)
	{
		es->origin[i] = message.ReadCoord();
		es->angles[i] = message.ReadAngle();
	}
}

void CLQ1_ParseSpawnBaseline(QMsg& message)
{
	int i = message.ReadShort();
	if (!(GGameType & GAME_QuakeWorld))
	{
		// must use CLQ1_EntityNum() to force cl.num_entities up
		CLQ1_EntityNum(i);
	}
	CLQ1_ParseBaseline(message, &clq1_baselines[i]);
}

void CLQ1_ParseSpawnStatic(QMsg& message)
{
	int i = cl_common->qh_num_statics;
	if (i >= MAX_STATIC_ENTITIES_Q1)
	{
		throw DropException("Too many static entities");
	}
	q1entity_t* ent = &clq1_static_entities[i];
	cl_common->qh_num_statics++;

	CLQ1_ParseBaseline(message, &ent->state);
	ent->state.colormap = 0;
}

void CLQ1_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles)
{
	vec3_t angles;
	angles[YAW] = ent_angles[YAW];
	angles[ROLL] = ent_angles[ROLL];
	if (R_IsMeshModel(ent->hModel))
	{
		// stupid quake bug
		angles[PITCH] = -ent_angles[PITCH];
	}
	else
	{
		angles[PITCH] = ent_angles[PITCH];
	}

	AnglesToAxis(angles, ent->axis);

	if (!String::Cmp(R_ModelName(ent->hModel), "progs/eyes.mdl") &&
		((GGameType & GAME_QuakeWorld) || cl_doubleeyes->integer))
	{
		// double size of eyes, since they are really hard to see in gl
		ent->renderfx |= RF_LIGHTING_ORIGIN;
		VectorCopy(ent->origin, ent->lightingOrigin);
		ent->origin[2] -= (22 + 8);

		VectorScale(ent->axis[0], 2, ent->axis[0]);
		VectorScale(ent->axis[1], 2, ent->axis[1]);
		VectorScale(ent->axis[2], 2, ent->axis[2]);
		ent->nonNormalizedAxes = true;
	}

	// HACK HACK HACK -- no fullbright colors, so make torches full light
	if (!String::Cmp(R_ModelName(ent->hModel), "progs/flame2.mdl") ||
		!String::Cmp(R_ModelName(ent->hModel), "progs/flame.mdl"))
	{
		ent->renderfx |= RF_ABSOLUTE_LIGHT;
		ent->radius = 1.0;
	}
}

void CLQ1_LinkStaticEntities()
{
	q1entity_t* pent = clq1_static_entities;
	for (int i = 0; i < cl_common->qh_num_statics; i++, pent++)
	{
		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(pent->state.origin, rent.origin);
		rent.hModel = cl_common->model_draw[pent->state.modelindex];
		CLQ1_SetRefEntAxis(&rent, pent->state.angles);
		rent.frame = pent->state.frame;
		rent.skinNum = pent->state.skinnum;
		rent.shaderTime = pent->syncbase;
		R_AddRefEntityToScene(&rent);
	}
}

//	Translates a skin texture by the per-player color lookup
void CLQ1_TranslatePlayerSkin(int playernum)
{
	q1player_info_t* player = &cl_common->q1_players[playernum];
	if (GGameType & GAME_QuakeWorld)
	{
		if (!player->name[0])
		{
			return;
		}

		char s[512];
		String::Cpy(s, Info_ValueForKey(player->userinfo, "skin"));
		String::StripExtension(s, s);
		if (player->skin && !String::ICmp(s, player->skin->name))
		{
			player->skin = NULL;
		}

		if (player->_topcolor == player->topcolor &&
			player->_bottomcolor == player->bottomcolor && player->skin)
		{
			return;
		}

		player->_topcolor = player->topcolor;
		player->_bottomcolor = player->bottomcolor;
	}

	byte translate[256];
	CL_CalcQuakeSkinTranslation(player->topcolor, player->bottomcolor, translate);

	//
	// locate the original skin pixels
	//
	if (GGameType & GAME_QuakeWorld)
	{
		if (!player->skin)
		{
			CLQW_SkinFind(player);
		}
		byte* original = CLQW_SkinCache(player->skin);
		if (original != NULL)
		{
			//skin data width
			R_CreateOrUpdateTranslatedSkin(clq1_playertextures[playernum], va("*player%d", playernum),
				original, translate, 320, 200);
		}
		else
		{
			R_CreateOrUpdateTranslatedModelSkinQ1(clq1_playertextures[playernum], va("*player%d", playernum),
				cl_common->model_draw[clq1_playerindex], translate);
		}
	}
	else
	{
		q1entity_t* ent = &clq1_entities[1 + playernum];

		R_CreateOrUpdateTranslatedModelSkinQ1(clq1_playertextures[playernum], va("*player%d", playernum),
			cl_common->model_draw[ent->state.modelindex], translate);
	}
}
