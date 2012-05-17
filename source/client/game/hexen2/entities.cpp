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

#include "../../client.h"
#include "local.h"
#include "../../../common/file_formats/spr.h"

#define H2MAX_EXTRA_TEXTURES 156	// 255-100+1

Cvar* clh2_playerclass;

h2entity_state_t clh2_baselines[MAX_EDICTS_H2];

h2entity_t h2cl_entities[MAX_EDICTS_H2];
h2entity_t h2cl_static_entities[MAX_STATIC_ENTITIES_H2];

static float RTint[256];
static float GTint[256];
static float BTint[256];

qhandle_t clh2_player_models[MAX_PLAYER_CLASS];
static image_t* clh2_playertextures[H2BIGGEST_MAX_CLIENTS];	// color translated skins

static image_t* clh2_extra_textures[H2MAX_EXTRA_TEXTURES];	// generic textures for models

static const char* parsedelta_strings[] =
{
	"HWU_ANGLE1",	//0
	"HWU_ANGLE3",	//1
	"HWU_SCALE",	//2
	"HWU_COLORMAP",	//3
	"HWU_SKIN",	//4
	"HWU_EFFECTS",	//5
	"HWU_MODEL16",	//6
	"HWU_MOREBITS2",			//7
	"",			//8 is unused
	"HWU_ORIGIN1",	//9
	"HWU_ORIGIN2",	//10
	"HWU_ORIGIN3",	//11
	"HWU_ANGLE2",	//12
	"HWU_FRAME",	//13
	"HWU_REMOVE",	//14
	"HWU_MOREBITS",	//15
	"HWU_MODEL",//16
	"HWU_SOUND",//17
	"HWU_DRAWFLAGS",//18
	"HWU_ABSLIGHT"	//19
};

void CLH2_InitColourShadeTables()
{
	for (int i = 0; i < 16; i++)
	{
		int c = ColorIndex[i];

		int r = r_palette[c][0];
		int g = r_palette[c][1];
		int b = r_palette[c][2];

		for (int p = 0; p < 16; p++)
		{
			RTint[i * 16 + p] = ((float)r) / ((float)ColorPercent[15 - p]);
			GTint[i * 16 + p] = ((float)g) / ((float)ColorPercent[15 - p]);
			BTint[i * 16 + p] = ((float)b) / ((float)ColorPercent[15 - p]);
		}
	}
}

void CLH2_ClearEntityTextureArrays()
{
	Com_Memset(clh2_playertextures, 0, sizeof(clh2_playertextures));
	Com_Memset(clh2_extra_textures, 0, sizeof(clh2_extra_textures));
}

int CLH2_GetMaxPlayerClasses()
{
	if (GGameType & GAME_HexenWorld)
	{
		return MAX_PLAYER_CLASS;
	}
	if (GGameType & GAME_H2Portals)
	{
		return NUM_CLASSES_H2MP;
	}
	return NUM_CLASSES_H2;
}

//	This error checks and tracks the total number of entities
h2entity_t* CLH2_EntityNum(int num)
{
	if (num >= cl.qh_num_entities)
	{
		if (num >= MAX_EDICTS_H2)
		{
			common->Error("CLH2_EntityNum: %i is an invalid number",num);
		}
		while (cl.qh_num_entities <= num)
		{
			cl.qh_num_entities++;
		}
	}

	return &h2cl_entities[num];
}

static void CLH2_ParseBaseline(QMsg& message, h2entity_state_t* es)
{
	es->modelindex = message.ReadShort();
	es->frame = message.ReadByte();
	es->colormap = message.ReadByte();
	es->skinnum = message.ReadByte();
	es->scale = message.ReadByte();
	es->drawflags = message.ReadByte();
	es->abslight = message.ReadByte();

	for (int i = 0; i < 3; i++)
	{
		es->origin[i] = message.ReadCoord();
		es->angles[i] = message.ReadAngle();
	}
}

void CLH2_ParseSpawnBaseline(QMsg& message)
{
	int i = message.ReadShort();
	if (!(GGameType & GAME_HexenWorld))
	{
		// must use CLH2_EntityNum() to force cl.num_entities up
		CLH2_EntityNum(i);
	}
	CLH2_ParseBaseline(message, &clh2_baselines[i]);
}

void CLH2_ParseSpawnStatic(QMsg& message)
{
	int i = cl.qh_num_statics;
	if (i >= MAX_STATIC_ENTITIES_H2)
	{
		common->Error("Too many static entities");
	}
	h2entity_t* ent = &h2cl_static_entities[i];
	cl.qh_num_statics++;

	CLH2_ParseBaseline(message, &ent->state);
	ent->state.colormap = 0;
}

void CLH2_ParseReference(QMsg& message)
{
	cl.h2_last_sequence = cl.h2_current_sequence;
	cl.h2_current_frame = message.ReadByte();
	cl.h2_current_sequence = message.ReadByte();
	if (cl.h2_need_build == 2)
	{
		cl.h2_frames[0].count = cl.h2_frames[1].count = cl.h2_frames[2].count = 0;
		cl.h2_need_build = 1;
		cl.h2_reference_frame = cl.h2_current_frame;
	}
	else if (cl.h2_last_sequence != cl.h2_current_sequence)
	{
		if (cl.h2_reference_frame >= 1 && cl.h2_reference_frame <= H2MAX_FRAMES)
		{
			short RemovePlace, OrigPlace, NewPlace, AddedIndex;
			RemovePlace = OrigPlace = NewPlace = AddedIndex = 0;
			for (int i = 0; i < cl.qh_num_entities; i++)
			{
				if (RemovePlace >= cl.h2_NumToRemove || cl.h2_RemoveList[RemovePlace] != i)
				{
					if (NewPlace < cl.h2_frames[1].count &&
						cl.h2_frames[1].states[NewPlace].number == i)
					{
						cl.h2_frames[2].states[AddedIndex] = cl.h2_frames[1].states[NewPlace];
						AddedIndex++;
						cl.h2_frames[2].count++;
					}
					else if (OrigPlace < cl.h2_frames[0].count &&
								cl.h2_frames[0].states[OrigPlace].number == i)
					{
						cl.h2_frames[2].states[AddedIndex] = cl.h2_frames[0].states[OrigPlace];
						AddedIndex++;
						cl.h2_frames[2].count++;
					}
				}
				else
				{
					RemovePlace++;
				}

				if (cl.h2_frames[0].states[OrigPlace].number == i)
				{
					OrigPlace++;
				}
				if (cl.h2_frames[1].states[NewPlace].number == i)
				{
					NewPlace++;
				}
			}
			cl.h2_frames[0] = cl.h2_frames[2];
		}
		cl.h2_frames[1].count = cl.h2_frames[2].count = 0;
		cl.h2_need_build = 1;
		cl.h2_reference_frame = cl.h2_current_frame;
	}
	else
	{
		cl.h2_need_build = 0;
	}

	for (int i = 1; i < cl.qh_num_entities; i++)
	{
		clh2_baselines[i].flags &= ~BE_ON;
	}

	for (int i = 0; i < cl.h2_frames[0].count; i++)
	{
		h2entity_t* ent = CLH2_EntityNum(cl.h2_frames[0].states[i].number);
		ent->state.modelindex = cl.h2_frames[0].states[i].modelindex;
		clh2_baselines[cl.h2_frames[0].states[i].number].flags |= BE_ON;
	}
}

void CLH2_ParseClearEdicts(QMsg& message)
{
	int j = message.ReadByte();
	if (cl.h2_need_build)
	{
		cl.h2_NumToRemove = j;
	}
	for (int i = 0; i < j; i++)
	{
		int k = message.ReadShort();
		if (cl.h2_need_build)
		{
			cl.h2_RemoveList[i] = k;
		}
		h2entity_t* ent = CLH2_EntityNum(k);
		clh2_baselines[k].flags &= ~BE_ON;
	}
}

/*
Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked.  Other attributes can change without relinking.
*/
void CLH2_ParseUpdate(QMsg& message, int bits)
{
	if (clc.qh_signon == SIGNONS - 1)
	{
		// first update is the final signon stage
		clc.qh_signon = SIGNONS;
		CLH2_SignonReply();
	}

	if (bits & H2U_MOREBITS)
	{
		int i = message.ReadByte();
		bits |= (i << 8);
	}

	if (bits & H2U_MOREBITS2)
	{
		int i = message.ReadByte();
		bits |= (i << 16);
	}

	int num;
	if (bits & H2U_LONGENTITY)
	{
		num = message.ReadShort();
	}
	else
	{
		num = message.ReadByte();
	}
	h2entity_t* ent = CLH2_EntityNum(num);
	h2entity_state_t& baseline = clh2_baselines[num];

	baseline.flags |= BE_ON;

	h2entity_state_t* ref_ent = NULL;

	for (int i = 0; i < cl.h2_frames[0].count; i++)
	{
		if (cl.h2_frames[0].states[i].number == num)
		{
			ref_ent = &cl.h2_frames[0].states[i];
			break;
		}
	}

	h2entity_state_t build_ent;
	if (!ref_ent)
	{
		ref_ent = &build_ent;

		build_ent.number = num;
		build_ent.origin[0] = baseline.origin[0];
		build_ent.origin[1] = baseline.origin[1];
		build_ent.origin[2] = baseline.origin[2];
		build_ent.angles[0] = baseline.angles[0];
		build_ent.angles[1] = baseline.angles[1];
		build_ent.angles[2] = baseline.angles[2];
		build_ent.modelindex = baseline.modelindex;
		build_ent.frame = baseline.frame;
		build_ent.colormap = baseline.colormap;
		build_ent.skinnum = baseline.skinnum;
		build_ent.effects = baseline.effects;
		build_ent.scale = baseline.scale;
		build_ent.drawflags = baseline.drawflags;
		build_ent.abslight = baseline.abslight;
	}

	h2entity_state_t* set_ent;
	h2entity_state_t dummy;
	if (cl.h2_need_build)
	{
		// new sequence, first valid frame
		set_ent = &cl.h2_frames[1].states[cl.h2_frames[1].count];
		cl.h2_frames[1].count++;
	}
	else
	{
		set_ent = &dummy;
	}

	if (bits & H2U_CLEAR_ENT)
	{
		Com_Memset(ent, 0, sizeof(h2entity_t));
		Com_Memset(ref_ent, 0, sizeof(*ref_ent));
		ref_ent->number = num;
	}

	*set_ent = *ref_ent;

	bool forcelink;
	if (ent->msgtime != cl.qh_mtime[1])
	{
		forcelink = true;	// no previous frame to lerp from
	}
	else
	{
		forcelink = false;
	}

	ent->msgtime = cl.qh_mtime[0];

	int modnum;
	if (bits & H2U_MODEL)
	{
		modnum = message.ReadShort();
		if (modnum >= MAX_MODELS_H2)
		{
			common->Error("CL_ParseModel: bad modnum");
		}
	}
	else
	{
		modnum = ref_ent->modelindex;
	}

	qhandle_t model = cl.model_draw[modnum];
	set_ent->modelindex = modnum;
	if (modnum != ent->state.modelindex)
	{
		ent->state.modelindex = modnum;

		// automatic animation (torches, etc) can be either all together
		// or randomized
		if (model)
		{
			if (R_ModelSyncType(model) == ST_RAND)
			{
				ent->syncbase = rand() * (1.0 / RAND_MAX);	//(float)(rand()&0x7fff) / 0x7fff;
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
		if (num > 0 && num <= cl.qh_maxclients)
		{
			CLH2_TranslatePlayerSkin(num - 1);
		}
	}

	if (bits & H2U_FRAME)
	{
		set_ent->frame = ent->state.frame = message.ReadByte();
	}
	else
	{
		ent->state.frame = ref_ent->frame;
	}

	if (bits & H2U_COLORMAP)
	{
		set_ent->colormap = ent->state.colormap = message.ReadByte();
	}
	else
	{
		ent->state.colormap = ref_ent->colormap;
	}

	if (bits & H2U_SKIN)
	{
		set_ent->skinnum = ent->state.skinnum = message.ReadByte();
		set_ent->drawflags = ent->state.drawflags = message.ReadByte();
	}
	else
	{
		ent->state.skinnum = ref_ent->skinnum;
		ent->state.drawflags = ref_ent->drawflags;
	}

	if (bits & H2U_EFFECTS)
	{
		set_ent->effects = ent->state.effects = message.ReadByte();
	}
	else
	{
		ent->state.effects = ref_ent->effects;
	}

	// shift the known values for interpolation
	VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & H2U_ORIGIN1)
	{
		set_ent->origin[0] = ent->msg_origins[0][0] = message.ReadCoord();
		//if (num == 2) fprintf(FH,"Read origin[0] %f\n",set_ent->angles[0]);
	}
	else
	{
		ent->msg_origins[0][0] = ref_ent->origin[0];
		//if (num == 2) fprintf(FH,"Restored origin[0] %f\n",ref_ent->angles[0]);
	}
	if (bits & H2U_ANGLE1)
	{
		set_ent->angles[0] = ent->msg_angles[0][0] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][0] = ref_ent->angles[0];
	}

	if (bits & H2U_ORIGIN2)
	{
		set_ent->origin[1] = ent->msg_origins[0][1] = message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][1] = ref_ent->origin[1];
	}
	if (bits & H2U_ANGLE2)
	{
		set_ent->angles[1] = ent->msg_angles[0][1] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][1] = ref_ent->angles[1];
	}

	if (bits & H2U_ORIGIN3)
	{
		set_ent->origin[2] = ent->msg_origins[0][2] = message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][2] = ref_ent->origin[2];
	}
	if (bits & H2U_ANGLE3)
	{
		set_ent->angles[2] = ent->msg_angles[0][2] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][2] = ref_ent->angles[2];
	}

	if (bits & H2U_SCALE)
	{
		set_ent->scale = ent->state.scale = message.ReadByte();
		set_ent->abslight = ent->state.abslight = message.ReadByte();
	}
	else
	{
		ent->state.scale = ref_ent->scale;
		ent->state.abslight = ref_ent->abslight;
	}

	if (bits & H2U_NOLERP)
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

static void ShowNetParseDelta(int x)
{
	if (cl_shownet->value != 2)
	{
		return;
	}

	char orstring[2];
	orstring[0] = 0;
	orstring[1] = 0;

	common->Printf("bits: ");
	for (int i = 0; i <= 19; i++)
	{
		if (x != 8)
		{
			if (x & (1 << i))
			{
				common->Printf("%s%s",orstring,parsedelta_strings[i]);
				orstring[0] = '|';
			}
		}
	}
	common->Printf("\n");
}

static void CLHW_ParseDelta(QMsg& message, h2entity_state_t* from, h2entity_state_t* to, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	to->number = bits & 511;
	bits &= ~511;

	if (bits & HWU_MOREBITS)
	{	// read in the low order bits
		int i = message.ReadByte();
		bits |= i;
	}

	if (bits & HWU_MOREBITS2)
	{
		int i = message.ReadByte();
		bits |= (i << 16);
	}

	ShowNetParseDelta(bits);

	// count the bits for net profiling
	for (int i = 0; i < 19; i++)
	{
		if (bits & (1 << i))
		{
			bitcounts[i]++;
		}
	}

	to->flags = bits;

	if (bits & HWU_MODEL)
	{
		if (bits & HWU_MODEL16)
		{
			to->modelindex = message.ReadShort();
		}
		else
		{
			to->modelindex = message.ReadByte();
		}
	}

	if (bits & HWU_FRAME)
	{
		to->frame = message.ReadByte();
	}

	if (bits & HWU_COLORMAP)
	{
		to->colormap = message.ReadByte();
	}

	if (bits & HWU_SKIN)
	{
		to->skinnum = message.ReadByte();
	}

	if (bits & HWU_DRAWFLAGS)
	{
		to->drawflags = message.ReadByte();
	}

	if (bits & HWU_EFFECTS)
	{
		to->effects = message.ReadLong();
	}

	if (bits & HWU_ORIGIN1)
	{
		to->origin[0] = message.ReadCoord();
	}

	if (bits & HWU_ANGLE1)
	{
		to->angles[0] = message.ReadAngle();
	}

	if (bits & HWU_ORIGIN2)
	{
		to->origin[1] = message.ReadCoord();
	}

	if (bits & HWU_ANGLE2)
	{
		to->angles[1] = message.ReadAngle();
	}

	if (bits & HWU_ORIGIN3)
	{
		to->origin[2] = message.ReadCoord();
	}

	if (bits & HWU_ANGLE3)
	{
		to->angles[2] = message.ReadAngle();
	}

	if (bits & HWU_SCALE)
	{
		to->scale = message.ReadByte();
	}
	if (bits & HWU_ABSLIGHT)
	{
		to->abslight = message.ReadByte();
	}
	if (bits & HWU_SOUND)
	{
		int i = message.ReadShort();
		S_StartSound(to->origin, to->number, 1, cl.sound_precache[i], 1.0, 1.0);
	}
}

static void FlushEntityPacket(QMsg& message)
{
	int word;
	h2entity_state_t olde, newe;

	common->DPrintf("FlushEntityPacket\n");

	Com_Memset(&olde, 0, sizeof(olde));

	cl.qh_validsequence = 0;		// can't render a frame
	cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW].invalid = true;

	// read it all, but ignore it
	while (1)
	{
		word = (unsigned short)message.ReadShort();
		if (message.badread)
		{	// something didn't parse right...
			common->Error("msg_badread in packetentities\n");
			return;
		}

		if (!word)
		{
			break;	// done

		}
		CLHW_ParseDelta(message, &olde, &newe, word);
	}
}

//	An hwsvc_packetentities has just been parsed, deal with the
// rest of the data stream.
static void CLHW_ParsePacketEntities(QMsg& message, bool delta)
{
	int oldpacket, newpacket;
	hwpacket_entities_t* oldp, * newp, dummy;
	int oldindex, newindex;
	int word, newnum, oldnum;
	qboolean full;
	byte from;

	newpacket = clc.netchan.incomingSequence & UPDATE_MASK_HW;
	newp = &cl.hw_frames[newpacket].packet_entities;
	cl.hw_frames[newpacket].invalid = false;

	if (delta)
	{
		from = message.ReadByte();

		oldpacket = cl.hw_frames[newpacket].delta_sequence;

		if ((from & UPDATE_MASK_HW) != (oldpacket & UPDATE_MASK_HW))
		{
			common->DPrintf("WARNING: from mismatch\n");
		}
	}
	else
	{
		oldpacket = -1;
	}

	full = false;
	if (oldpacket != -1)
	{
		if (clc.netchan.outgoingSequence - oldpacket >= UPDATE_BACKUP_HW - 1)
		{	// we can't use this, it is too old
			FlushEntityPacket(message);
			return;
		}
		cl.qh_validsequence = clc.netchan.incomingSequence;
		oldp = &cl.hw_frames[oldpacket & UPDATE_MASK_HW].packet_entities;
	}
	else
	{	// this is a full update that we can start delta compressing from now
		oldp = &dummy;
		dummy.num_entities = 0;
		cl.qh_validsequence = clc.netchan.incomingSequence;
		full = true;
	}

	oldindex = 0;
	newindex = 0;
	newp->num_entities = 0;

	while (1)
	{
		word = (unsigned short)message.ReadShort();
		if (message.badread)
		{	// something didn't parse right...
			common->Error("msg_badread in packetentities\n");
			return;
		}

		if (!word)
		{
			while (oldindex < oldp->num_entities)
			{
				// copy all the rest of the entities from the old packet
				if (newindex >= HWMAX_PACKET_ENTITIES)
				{
					common->Error("CLHW_ParsePacketEntities: newindex == HWMAX_PACKET_ENTITIES");
				}
				newp->entities[newindex] = oldp->entities[oldindex];
				newindex++;
				oldindex++;
			}
			break;
		}
		newnum = word & 511;
		oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;

		while (newnum > oldnum)
		{
			if (full)
			{
				common->Printf("WARNING: oldcopy on full update");
				FlushEntityPacket(message);
				return;
			}

			// copy one of the old entities over to the new packet unchanged
			if (newindex >= HWMAX_PACKET_ENTITIES)
			{
				common->Error("CLHW_ParsePacketEntities: newindex == HWMAX_PACKET_ENTITIES");
			}
			newp->entities[newindex] = oldp->entities[oldindex];
			newindex++;
			oldindex++;
			oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;
		}

		if (newnum < oldnum)
		{
			// new from baseline
			if (word & HWU_REMOVE)
			{
				if (full)
				{
					cl.qh_validsequence = 0;
					common->Printf("WARNING: HWU_REMOVE on full update\n");
					FlushEntityPacket(message);
					return;
				}
				continue;
			}
			if (newindex >= HWMAX_PACKET_ENTITIES)
			{
				common->Error("CLHW_ParsePacketEntities: newindex == HWMAX_PACKET_ENTITIES");
			}
			CLHW_ParseDelta(message, &clh2_baselines[newnum], &newp->entities[newindex], word);
			newindex++;
			continue;
		}

		if (newnum == oldnum)
		{	// delta from previous
			if (full)
			{
				cl.qh_validsequence = 0;
				common->Printf("WARNING: delta on full update");
			}
			if (word & HWU_REMOVE)
			{
				oldindex++;
				continue;
			}
			CLHW_ParseDelta(message, &oldp->entities[oldindex], &newp->entities[newindex], word);
			newindex++;
			oldindex++;
		}

	}

	newp->num_entities = newindex;
}

void CLHW_ParsePacketEntities(QMsg& message)
{
	CLHW_ParsePacketEntities(message, false);
}

void CLHW_ParseDeltaPacketEntities(QMsg& message)
{
	CLHW_ParsePacketEntities(message, true);
}

void CLH2_SetRefEntAxis(refEntity_t* entity, vec3_t entityAngles, vec3_t angleAdd, int scale, int colourShade, int absoluteLight, int drawFlags)
{
	if (drawFlags & H2DRF_TRANSLUCENT)
	{
		entity->renderfx |= RF_WATERTRANS;
	}

	vec3_t angles;
	if (R_IsMeshModel(entity->hModel))
	{
		if (R_ModelFlags(entity->hModel) & H2MDLEF_FACE_VIEW)
		{
			//	yaw and pitch must be 0 so that renderer can safely multply matrices.
			angles[PITCH] = 0;
			angles[YAW] = 0;
			angles[ROLL] = entityAngles[ROLL];

			AnglesToAxis(angles, entity->axis);
		}
		else
		{
			if (R_ModelFlags(entity->hModel) & H2MDLEF_ROTATE)
			{
				angles[YAW] = AngleMod((entity->origin[0] + entity->origin[1]) * 0.8 + (108 * cl.serverTime * 0.001));
			}
			else
			{
				angles[YAW] = entityAngles[YAW];
			}
			angles[ROLL] = entityAngles[ROLL];
			// stupid quake bug
			angles[PITCH] = -entityAngles[PITCH];

			if (angleAdd[0] || angleAdd[1] || angleAdd[2])
			{
				float BaseAxis[3][3];
				AnglesToAxis(angles, BaseAxis);

				// For clientside rotation stuff
				float AddAxis[3][3];
				AnglesToAxis(angleAdd, AddAxis);

				MatrixMultiply(AddAxis, BaseAxis, entity->axis);
			}
			else
			{
				AnglesToAxis(angles, entity->axis);
			}
		}

		if ((R_ModelFlags(entity->hModel) & H2MDLEF_ROTATE) || (scale != 0 && scale != 100))
		{
			entity->renderfx |= RF_LIGHTING_ORIGIN;
			VectorCopy(entity->origin, entity->lightingOrigin);
		}

		if (R_ModelFlags(entity->hModel) & H2MDLEF_ROTATE)
		{
			// Floating motion
			float delta = sin(entity->origin[0] + entity->origin[1] + (cl.serverTime * 0.001 * 3)) * 5.5;
			VectorMA(entity->origin, delta, entity->axis[2], entity->origin);
			absoluteLight = 60 + 34 + sin(entity->origin[0] + entity->origin[1] + (cl.serverTime * 0.001 * 3.8)) * 34;
			drawFlags |= H2MLS_ABSLIGHT;
		}

		if (scale != 0 && scale != 100)
		{
			float entScale = (float)scale / 100.0;
			float esx;
			float esy;
			float esz;
			switch (drawFlags & H2SCALE_TYPE_MASKIN)
			{
			case H2SCALE_TYPE_UNIFORM:
				esx = entScale;
				esy = entScale;
				esz = entScale;
				break;
			case H2SCALE_TYPE_XYONLY:
				esx = entScale;
				esy = entScale;
				esz = 1;
				break;
			case H2SCALE_TYPE_ZONLY:
				esx = 1;
				esy = 1;
				esz = entScale;
				break;
			}
			float etz;
			switch (drawFlags & H2SCALE_ORIGIN_MASKIN)
			{
			case H2SCALE_ORIGIN_CENTER:
				etz = 0.5;
				break;
			case H2SCALE_ORIGIN_BOTTOM:
				etz = 0;
				break;
			case H2SCALE_ORIGIN_TOP:
				etz = 1.0;
				break;
			}

			vec3_t Out;
			R_CalculateModelScaleOffset(entity->hModel, esx, esy, esz, etz, Out);
			VectorMA(entity->origin, Out[0], entity->axis[0], entity->origin);
			VectorMA(entity->origin, Out[1], entity->axis[1], entity->origin);
			VectorMA(entity->origin, Out[2], entity->axis[2], entity->origin);
			VectorScale(entity->axis[0], esx, entity->axis[0]);
			VectorScale(entity->axis[1], esy, entity->axis[1]);
			VectorScale(entity->axis[2], esz, entity->axis[2]);
			entity->nonNormalizedAxes = true;
		}
	}
	else
	{
		angles[YAW] = entityAngles[YAW];
		angles[ROLL] = entityAngles[ROLL];
		angles[PITCH] = entityAngles[PITCH];

		AnglesToAxis(angles, entity->axis);
	}

	if (colourShade)
	{
		entity->renderfx |= RF_COLORSHADE;
		entity->shaderRGBA[0] = (int)(RTint[colourShade] * 255);
		entity->shaderRGBA[1] = (int)(GTint[colourShade] * 255);
		entity->shaderRGBA[2] = (int)(BTint[colourShade] * 255);
	}

	int mls = drawFlags & H2MLS_MASKIN;
	if (mls == H2MLS_ABSLIGHT)
	{
		entity->renderfx |= RF_ABSOLUTE_LIGHT;
		entity->radius = absoluteLight / 256.0;
	}
	else if (mls != H2MLS_NONE)
	{
		// Use a model light style (25-30)
		entity->renderfx |= RF_ABSOLUTE_LIGHT;
		entity->radius = cl_lightstyle[24 + mls].value[0] / 2;
	}
}

//	Translates a skin texture by the per-player color lookup
void CLH2_TranslatePlayerSkin(int playernum)
{
	h2player_info_t* player = &cl.h2_players[playernum];
	if (GGameType & GAME_HexenWorld)
	{
		if (!player->name[0])
		{
			return;
		}
		if (!player->playerclass)
		{
			return;
		}
		if (player->modelindex <= 0)
		{
			return;
		}
	}

	byte translate[256];
	CL_CalcHexen2SkinTranslation(player->topColour, player->bottomColour, player->playerclass, translate);

	//
	// locate the original skin pixels
	//
	int classIndex;
	if (player->playerclass >= 1 && player->playerclass <= CLH2_GetMaxPlayerClasses())
	{
		classIndex = player->playerclass - 1;
		player->Translated = true;
	}
	else
	{
		classIndex = 0;
	}

	R_CreateOrUpdateTranslatedModelSkinH2(clh2_playertextures[playernum], va("*player%d", playernum),
		clh2_player_models[classIndex], translate, classIndex);
}

void CLH2_HandleCustomSkin(refEntity_t* entity, int playerIndex)
{
	if (entity->skinNum >= 100)
	{
		if (entity->skinNum > 255)
		{
			throw Exception("skinnum > 255");
		}

		if (!clh2_extra_textures[entity->skinNum - 100])	// Need to load it in
		{
			char temp[40];
			String::Sprintf(temp, sizeof(temp), "gfx/skin%d.lmp", entity->skinNum);
			clh2_extra_textures[entity->skinNum - 100] = R_CachePic(temp);
		}

		entity->customSkin = R_GetImageHandle(clh2_extra_textures[entity->skinNum - 100]);
	}
	else if (playerIndex >= 0 && entity->hModel)
	{
		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
		//FIXME? What about Dwarf?
		if (entity->hModel == clh2_player_models[0] ||
			entity->hModel == clh2_player_models[1] ||
			entity->hModel == clh2_player_models[2] ||
			entity->hModel == clh2_player_models[3] ||
			entity->hModel == clh2_player_models[4])
		{
			if (!cl.h2_players[playerIndex].Translated)
			{
				CLH2_TranslatePlayerSkin(playerIndex);
			}

			entity->customSkin = R_GetImageHandle(clh2_playertextures[playerIndex]);
		}
	}
}
