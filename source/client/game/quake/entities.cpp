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
#include "../../../core/file_formats/spr.h"
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

//	Parse an entity update message from the server
//	If an entities model or origin changes from frame to frame, it must be
// relinked.  Other attributes can change without relinking.
void CLQ1_ParseUpdate(QMsg& message, int bits)
{
	if (clc_common->qh_signon == SIGNONS - 1)
	{
		// first update is the final signon stage
		clc_common->qh_signon = SIGNONS;
		CLQ1_SignonReply ();
	}

	if (bits & Q1U_MOREBITS)
	{
		int i = message.ReadByte();
		bits |= (i << 8);
	}

	int num;
	if (bits & Q1U_LONGENTITY)
	{
		num = message.ReadShort();
	}
	else
	{
		num = message.ReadByte();
	}

	q1entity_t* ent = CLQ1_EntityNum(num);
	const q1entity_state_t& baseline = clq1_baselines[num];

	for (int i = 0; i < 32; i++)
	{
		if (bits & (1 << i))
		{
			bitcounts[i]++;
		}
	}

	bool forcelink = ent->msgtime != cl_common->qh_mtime[1];	// no previous frame to lerp from

	ent->msgtime = cl_common->qh_mtime[0];

	int modnum;
	if (bits & Q1U_MODEL)
	{
		modnum = message.ReadByte();
		if (modnum >= MAX_MODELS_Q1)
		{
			throw DropException("CL_ParseModel: bad modnum");
		}
	}
	else
	{
		modnum = baseline.modelindex;
	}

	qhandle_t model = cl_common->model_draw[modnum];
	if (modnum != ent->state.modelindex)
	{
		ent->state.modelindex = modnum;
		// automatic animation (torches, etc) can be either all together
		// or randomized
		if (model)
		{
			if (R_ModelSyncType(model) == ST_RAND)
			{
				ent->syncbase = (float)(rand() & 0x7fff) / 0x7fff;
			}
			else
			{
				ent->syncbase = 0.0;
			}
		}
		else
		{
			forcelink = true;	// hack to make null model players work
		}
		if (num > 0 && num <= cl_common->qh_maxclients)
		{
			CLQ1_TranslatePlayerSkin(num - 1);
		}
	}
	
	if (bits & Q1U_FRAME)
	{
		ent->state.frame = message.ReadByte();
	}
	else
	{
		ent->state.frame = baseline.frame;
	}

	if (bits & Q1U_COLORMAP)
	{
		ent->state.colormap = message.ReadByte();
	}
	else
	{
		ent->state.colormap = baseline.colormap;
	}
	if (ent->state.colormap > cl_common->qh_maxclients)
	{
		throw Exception("i >= cl.maxclients");
	}

	int skin;
	if (bits & Q1U_SKIN)
	{
		skin = message.ReadByte();
	}
	else
	{
		skin = baseline.skinnum;
	}
	if (skin != ent->state.skinnum)
	{
		ent->state.skinnum = skin;
		if (num > 0 && num <= cl_common->qh_maxclients)
		{
			CLQ1_TranslatePlayerSkin(num - 1);
		}
	}

	if (bits & Q1U_EFFECTS)
	{
		ent->state.effects = message.ReadByte();
	}
	else
	{
		ent->state.effects = baseline.effects;
	}

	// shift the known values for interpolation
	VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & Q1U_ORIGIN1)
	{
		ent->msg_origins[0][0] = message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][0] = baseline.origin[0];
	}
	if (bits & Q1U_ANGLE1)
	{
		ent->msg_angles[0][0] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][0] = baseline.angles[0];
	}

	if (bits & Q1U_ORIGIN2)
	{
		ent->msg_origins[0][1] = message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][1] = baseline.origin[1];
	}
	if (bits & Q1U_ANGLE2)
	{
		ent->msg_angles[0][1] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][1] = baseline.angles[1];
	}

	if (bits & Q1U_ORIGIN3)
	{
		ent->msg_origins[0][2] = message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][2] = baseline.origin[2];
	}
	if (bits & Q1U_ANGLE3)
	{
		ent->msg_angles[0][2] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][2] = baseline.angles[2];
	}

	if (bits & Q1U_NOLERP)
	{
		forcelink = true;
	}

	if (forcelink)
	{
		// didn't have an update last message
		VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy(ent->msg_origins[0], ent->state.origin);
		VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy(ent->msg_angles[0], ent->state.angles);
	}
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
