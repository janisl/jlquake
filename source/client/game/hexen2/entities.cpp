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
Cvar* clhw_teamcolor;

h2entity_state_t clh2_baselines[MAX_EDICTS_H2];

h2entity_t h2cl_entities[MAX_EDICTS_H2];
static h2entity_t h2cl_static_entities[MAX_STATIC_ENTITIES_H2];

static float RTint[256];
static float GTint[256];
static float BTint[256];

qhandle_t clh2_player_models[MAX_PLAYER_CLASS];
static image_t* clh2_playertextures[H2BIGGEST_MAX_CLIENTS];	// color translated skins

static image_t* clh2_extra_textures[H2MAX_EXTRA_TEXTURES];	// generic textures for models

int clhw_playerindex[MAX_PLAYER_CLASS];

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
		CLH2_EntityNum(k);
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

void CLHW_ParsePlayerinfo(QMsg& message)
{
	int msec;
	int flags;
	h2player_info_t* info;
	hwplayer_state_t* state;
	int num;
	int i;
	qboolean playermodel = false;

	num = message.ReadByte();
	if (num > HWMAX_CLIENTS)
	{
		common->FatalError("CLHW_ParsePlayerinfo: bad num");
	}

	info = &cl.h2_players[num];

	state = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].playerstate[num];

	flags = state->flags = message.ReadShort();

	state->messagenum = cl.qh_parsecount;
	state->origin[0] = message.ReadCoord();
	state->origin[1] = message.ReadCoord();
	state->origin[2] = message.ReadCoord();
	VectorCopy(state->origin, info->origin);

	state->frame = message.ReadByte();

	// the other player's last move was likely some time
	// before the packet was sent out, so accurately track
	// the exact time it was valid at
	if (flags & HWPF_MSEC)
	{
		msec = message.ReadByte();
		state->state_time = cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].senttime - msec * 0.001;
	}
	else
	{
		state->state_time = cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].senttime;
	}

	if (flags & HWPF_COMMAND)
	{
		MSGHW_ReadUsercmd(&message, &state->command, false);
	}

	for (i = 0; i < 3; i++)
	{
		if (flags & (HWPF_VELOCITY1 << i))
		{
			state->velocity[i] = message.ReadShort();
		}
		else
		{
			state->velocity[i] = 0;
		}
	}

	if (flags & HWPF_MODEL)
	{
		state->modelindex = message.ReadShort();
	}
	else
	{
		playermodel = true;
		i = info->playerclass;
		if (i >= 1 && i <= MAX_PLAYER_CLASS)
		{
			state->modelindex = clhw_playerindex[i - 1];
		}
		else
		{
			state->modelindex = clhw_playerindex[0];
		}
	}

	if (flags & HWPF_SKINNUM)
	{
		state->skinnum = message.ReadByte();
	}
	else
	{
		if (info->siege_team == HWST_ATTACKER && playermodel)
		{
			state->skinnum = 1;	//using a playermodel and attacker - skin is set to 1
		}
		else
		{
			state->skinnum = 0;
		}
	}

	if (flags & HWPF_EFFECTS)
	{
		state->effects = message.ReadByte();
	}
	else
	{
		state->effects = 0;
	}

	if (flags & HWPF_EFFECTS2)
	{
		state->effects |= (message.ReadByte() << 8);
	}
	else
	{
		state->effects &= 0xff;
	}

	if (flags & HWPF_WEAPONFRAME)
	{
		state->weaponframe = message.ReadByte();
	}
	else
	{
		state->weaponframe = 0;
	}

	if (flags & HWPF_DRAWFLAGS)
	{
		state->drawflags = message.ReadByte();
	}
	else
	{
		state->drawflags = 0;
	}

	if (flags & HWPF_SCALE)
	{
		state->scale = message.ReadByte();
	}
	else
	{
		state->scale = 0;
	}

	if (flags & HWPF_ABSLIGHT)
	{
		state->abslight = message.ReadByte();
	}
	else
	{
		state->abslight = 0;
	}

	if (flags & HWPF_SOUND)
	{
		i = message.ReadShort();
		S_StartSound(state->origin, num, 1, cl.sound_precache[i], 1.0, 1.0);
	}

	VectorCopy(state->command.angles, state->viewangles);
}

void CLHW_SavePlayer(QMsg& message)
{
	int num = message.ReadByte();

	if (num > HWMAX_CLIENTS)
	{
		common->Error("CLHW_ParsePlayerinfo: bad num");
	}

	hwplayer_state_t* state = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].playerstate[num];

	state->messagenum = cl.qh_parsecount;
	state->state_time = cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].senttime;
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

static void CLH2_LinkStaticEntities()
{
	h2entity_t* pent = h2cl_static_entities;
	for (int i = 0; i < cl.qh_num_statics; i++, pent++)
	{
		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(pent->state.origin, rent.origin);
		rent.hModel = cl.model_draw[pent->state.modelindex];
		rent.frame = pent->state.frame;
		rent.skinNum = pent->state.skinnum;
		rent.syncBase = pent->syncbase;
		CLH2_SetRefEntAxis(&rent, pent->state.angles, vec3_origin, pent->state.scale,
			pent->state.colormap, pent->state.abslight, pent->state.drawflags);
		CLH2_HandleCustomSkin(&rent, -1);
		R_AddRefEntityToScene(&rent);
	}
}

static void CLH2_RelinkEntities()
{
	h2entity_t* ent;
	int i, j;
	float frac, f, d;
	vec3_t delta;
	//float		bobjrotate;
	vec3_t oldorg;
	int c;

	c = 0;
	// determine partial update time
	frac = CLQH_LerpPoint();

	R_ClearScene();

	//
	// interpolate player info
	//
	for (i = 0; i < 3; i++)
	{
		cl.qh_velocity[i] = cl.qh_mvelocity[1][i] +
			frac * (cl.qh_mvelocity[0][i] - cl.qh_mvelocity[1][i]);
	}

	if (clc.demoplaying && !h2intro_playing)
	{
		// interpolate the angles
		for (j = 0; j < 3; j++)
		{
			d = cl.qh_mviewangles[0][j] - cl.qh_mviewangles[1][j];
			if (d > 180)
			{
				d -= 360;
			}
			else if (d < -180)
			{
				d += 360;
			}
			cl.viewangles[j] = cl.qh_mviewangles[1][j] + frac * d;
		}
	}

	// start on the entity after the world
	for (i = 1,ent = h2cl_entities + 1; i < cl.qh_num_entities; i++,ent++)
	{
		if (!ent->state.modelindex)
		{
			// empty slot
			continue;
		}

		// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.qh_mtime[0] && !(clh2_baselines[i].flags & BE_ON))
		{
			ent->state.modelindex = 0;
			continue;
		}

		VectorCopy(ent->state.origin, oldorg);

		if (ent->msgtime != cl.qh_mtime[0])
		{	// the entity was not updated in the last message
			// so move to the final spot
			VectorCopy(ent->msg_origins[0], ent->state.origin);
			VectorCopy(ent->msg_angles[0], ent->state.angles);
		}
		else
		{	// if the delta is large, assume a teleport and don't lerp
			f = frac;
			for (j = 0; j < 3; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];

				if (delta[j] > 100 || delta[j] < -100)
				{
					f = 1;		// assume a teleportation, not a motion
				}
			}

			// interpolate the origin and angles
			for (j = 0; j < 3; j++)
			{
				ent->state.origin[j] = ent->msg_origins[1][j] + f * delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
				{
					d -= 360;
				}
				else if (d < -180)
				{
					d += 360;
				}
				ent->state.angles[j] = ent->msg_angles[1][j] + f * d;
			}
		}

		c++;
		if (ent->state.effects & H2EF_DARKFIELD)
		{
			CLH2_DarkFieldParticles(ent->state.origin);
		}
		if (ent->state.effects & H2EF_MUZZLEFLASH)
		{
			CLH2_MuzzleFlashLight(i, ent->state.origin, ent->state.angles, true);
		}
		if (ent->state.effects & H2EF_BRIGHTLIGHT)
		{
			CLH2_BrightLight(i, ent->state.origin);
		}
		if (ent->state.effects & H2EF_DIMLIGHT)
		{
			CLH2_DimLight(i, ent->state.origin);
		}
		if (ent->state.effects & H2EF_DARKLIGHT)
		{
			CLH2_DarkLight(i, ent->state.origin);
		}
		if (ent->state.effects & H2EF_LIGHT)
		{
			CLH2_Light(i, ent->state.origin);
		}

		int ModelFlags = R_ModelFlags(cl.model_draw[ent->state.modelindex]);
		if (ModelFlags & H2MDLEF_GIB)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_blood);
		}
		else if (ModelFlags & H2MDLEF_ZOMGIB)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_slight_blood);
		}
		else if (ModelFlags & H2MDLEF_BLOODSHOT)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_bloodshot);
		}
		else if (ModelFlags & H2MDLEF_TRACER)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_tracer);
		}
		else if (ModelFlags & H2MDLEF_TRACER2)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_tracer2);
		}
		else if (ModelFlags & H2MDLEF_ROCKET)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_rocket_trail);
		}
		else if (ModelFlags & H2MDLEF_FIREBALL)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_fireball);
			CLH2_FireBallLight(i, ent->state.origin);
		}
		else if (ModelFlags & H2MDLEF_ACIDBALL)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_acidball);
			CLH2_FireBallLight(i, ent->state.origin);
		}
		else if (ModelFlags & H2MDLEF_ICE)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_ice);
		}
		else if (ModelFlags & H2MDLEF_SPIT)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_spit);
			CLH2_SpitLight(i, ent->state.origin);
		}
		else if (ModelFlags & H2MDLEF_SPELL)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_spell);
		}
		else if (ModelFlags & H2MDLEF_GRENADE)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_smoke);
		}
		else if (ModelFlags & H2MDLEF_TRACER3)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_voor_trail);
		}
		else if (ModelFlags & H2MDLEF_VORP_MISSILE)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_vorpal);
		}
		else if (ModelFlags & H2MDLEF_SET_STAFF)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin,rt_setstaff);
		}
		else if (ModelFlags & H2MDLEF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
			{
				CLH2_TrailParticles(oldorg, ent->state.origin, rt_magicmissile);
			}
		}
		else if (ModelFlags & H2MDLEF_BONESHARD)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_boneshard);
		}
		else if (ModelFlags & H2MDLEF_SCARAB)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_scarab);
		}

		if (ent->state.effects & H2EF_NODRAW)
		{
			continue;
		}

		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(ent->state.origin, rent.origin);
		rent.hModel = cl.model_draw[ent->state.modelindex];
		rent.frame = ent->state.frame;
		rent.syncBase = ent->syncbase;
		rent.skinNum = ent->state.skinnum;
		CLH2_SetRefEntAxis(&rent, ent->state.angles, vec3_origin, ent->state.scale, ent->state.colormap, ent->state.abslight, ent->state.drawflags);
		CLH2_HandleCustomSkin(&rent, i <= cl.qh_maxclients ? i - 1 : -1);
		if (i == cl.viewentity && !chase_active->value)
		{
			rent.renderfx |= RF_THIRD_PERSON;
		}
		R_AddRefEntityToScene(&rent);
	}
}

static void HandleEffects(int effects, int number, refEntity_t* ent, const vec3_t angles, vec3_t angleAdd)
{
	bool rotateSet = false;

	// Effect Flags
	if (effects & HWEF_BRIGHTFIELD)
	{
		// show that this guy is cool or something...
		CLH2_BrightFieldLight(number, ent->origin);
		CLHW_BrightFieldParticles(ent->origin);
	}
	if (effects & H2EF_DARKFIELD)
	{
		CLH2_DarkFieldParticles(ent->origin);
	}
	if (effects & H2EF_MUZZLEFLASH)
	{
		CLH2_MuzzleFlashLight(number, ent->origin, angles, true);
	}
	if (effects & H2EF_BRIGHTLIGHT)
	{
		CLH2_BrightLight(number, ent->origin);
	}
	if (effects & H2EF_DIMLIGHT)
	{
		CLH2_DimLight(number, ent->origin);
	}
	if (effects & H2EF_LIGHT)
	{
		CLH2_Light(number, ent->origin);
	}

	if (effects & HWEF_POISON_GAS)
	{
		CLHW_UpdatePoisonGas(ent->origin, angles);
	}
	if (effects & HWEF_ACIDBLOB)
	{
		angleAdd[0] = 0;
		angleAdd[1] = 0;
		angleAdd[2] = 200 * cl.qh_serverTimeFloat;

		rotateSet = true;

		CLHW_UpdateAcidBlob(ent->origin, angles);
	}
	if (effects & HWEF_ONFIRE)
	{
		CLHW_UpdateOnFire(ent, angles, number);
	}
	if (effects & HWEF_POWERFLAMEBURN)
	{
		CLHW_UpdatePowerFlameBurn(ent, number);
	}
	if (effects & HWEF_ICESTORM_EFFECT)
	{
		CLHW_UpdateIceStorm(ent, number);
	}
	if (effects & HWEF_HAMMER_EFFECTS)
	{
		angleAdd[0] = 200 * cl.qh_serverTimeFloat;
		angleAdd[1] = 0;
		angleAdd[2] = 0;

		rotateSet = true;

		CLHW_UpdateHammer(ent, number);
	}
	if (effects & HWEF_BEETLE_EFFECTS)
	{
		CLHW_UpdateBug(ent);
	}
	if (effects & H2EF_DARKLIGHT)	//EF_INVINC_CIRC)
	{
		CLHW_SuccubusInvincibleParticles(ent->origin);
	}

	if (effects & HWEF_UPDATESOUND)
	{
		S_UpdateSoundPos(number, 7, ent->origin);
	}

	if (!rotateSet)
	{
		angleAdd[0] = 0;
		angleAdd[1] = 0;
		angleAdd[2] = 0;
	}
}

static void CLHW_LinkPacketEntities()
{
	hwpacket_entities_t* pack = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW].packet_entities;
	hwpacket_entities_t* PrevPack = &cl.hw_frames[(clc.netchan.incomingSequence - 1) & UPDATE_MASK_HW].packet_entities;

	float f = 0;		// FIXME: no interpolation right now

	for (int pnum = 0; pnum < pack->num_entities; pnum++)
	{
		h2entity_state_t* s1 = &pack->entities[pnum];
		h2entity_state_t* s2 = s1;	// FIXME: no interpolation right now

		// if set to invisible, skip
		if (!s1->modelindex)
		{
			continue;
		}

		// create a new entity
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));

		ent.reType = RT_MODEL;
		qhandle_t model = cl.model_draw[s1->modelindex];
		ent.hModel = model;

		// set skin
		ent.skinNum = s1->skinnum;

		// set frame
		ent.frame = s1->frame;

		int drawflags = s1->drawflags;

		vec3_t angles;
		for (int i = 0; i < 3; i++)
		{
			float a1 = s1->angles[i];
			float a2 = s2->angles[i];
			if (a1 - a2 > 180)
			{
				a1 -= 360;
			}
			if (a1 - a2 < -180)
			{
				a1 += 360;
			}
			angles[i] = a2 + f * (a1 - a2);
		}

		// calculate origin
		for (int i = 0; i < 3; i++)
		{
			ent.origin[i] = s2->origin[i] + f * (s1->origin[i] - s2->origin[i]);
		}

		// scan the old entity display list for a matching
		vec3_t old_origin;
		int i;
		for (i = 0; i < PrevPack->num_entities; i++)
		{
			if (PrevPack->entities[i].number == s1->number)
			{
				VectorCopy(PrevPack->entities[i].origin, old_origin);
				break;
			}
		}
		if (i == PrevPack->num_entities)
		{
			CLH2_SetRefEntAxis(&ent, angles, vec3_origin, s1->scale, s1->colormap, s1->abslight, drawflags);
			R_AddRefEntityToScene(&ent);
			continue;		// not in last message
		}

		for (i = 0; i < 3; i++)
			//if ( abs(old_origin[i] - ent->origin[i]) > 128)
			if (abs(old_origin[i] - ent.origin[i]) > 512)	// this is an issue for laggy situations...
			{	// no trail if too far
				VectorCopy(ent.origin, old_origin);
				break;
			}

		// some of the effects need to know how far the thing has moved...

		if (clhw_siege)
		{
			if ((int)s1->effects & H2EF_NODRAW)
			{
				ent.skinNum = 101;	//ice, but in siege will be invis skin for dwarf to see
				drawflags |= H2DRF_TRANSLUCENT;
				s1->effects &= ~H2EF_NODRAW;
			}
		}

		vec3_t angleAdd;
		HandleEffects(s1->effects, s1->number, &ent, angles, angleAdd);
		CLH2_SetRefEntAxis(&ent, angles, angleAdd, s1->scale, s1->colormap, s1->abslight, drawflags);
		R_AddRefEntityToScene(&ent);

		// add automatic particle trails
		int ModelFlags = R_ModelFlags(ent.hModel);
		if (!ModelFlags)
		{
			continue;
		}

		// Model Flags
		if (ModelFlags & H2MDLEF_GIB)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_blood);
		}
		else if (ModelFlags & H2MDLEF_ZOMGIB)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_slight_blood);
		}
		else if (ModelFlags & H2MDLEF_TRACER)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_tracer);
		}
		else if (ModelFlags & H2MDLEF_TRACER2)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_tracer2);
		}
		else if (ModelFlags & H2MDLEF_ROCKET)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_rocket_trail);
		}
		else if (ModelFlags & H2MDLEF_FIREBALL)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_fireball);
			CLH2_FireBallLight(i, ent.origin);
		}
		else if (ModelFlags & H2MDLEF_ICE)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_ice);
		}
		else if (ModelFlags & H2MDLEF_SPIT)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_spit);
			CLH2_SpitLight(i, ent.origin);
		}
		else if (ModelFlags & H2MDLEF_SPELL)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_spell);
		}
		else if (ModelFlags & H2MDLEF_GRENADE)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_grensmoke);
		}
		else if (ModelFlags & H2MDLEF_TRACER3)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_voor_trail);
		}
		else if (ModelFlags & H2MDLEF_VORP_MISSILE)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_vorpal);
		}
		else if (ModelFlags & H2MDLEF_SET_STAFF)
		{
			CLH2_TrailParticles(old_origin, ent.origin,rt_setstaff);
		}
		else if (ModelFlags & H2MDLEF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
			{
				CLH2_TrailParticles(old_origin, ent.origin, rt_magicmissile);
			}
		}
		else if (ModelFlags & H2MDLEF_BONESHARD)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_boneshard);
		}
		else if (ModelFlags & H2MDLEF_SCARAB)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_scarab);
		}
		else if (ModelFlags & H2MDLEF_ACIDBALL)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_acidball);
		}
		else if (ModelFlags & H2MDLEF_BLOODSHOT)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_bloodshot);
		}
	}
}

//	Create visible entities in the correct position
// for all current players
static void CLHW_LinkPlayers()
{
	double playertime = cls.realtime * 0.001 - cls.qh_latency + 0.02;
	if (playertime > cls.realtime * 0.001)
	{
		playertime = cls.realtime * 0.001;
	}

	hwframe_t* frame = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW];

	h2player_info_t* info = cl.h2_players;
	hwplayer_state_t* state = frame->playerstate;
	for (int j = 0; j < HWMAX_CLIENTS; j++, info++, state++)
	{
		info->shownames_off = true;
		if (state->messagenum != cl.qh_parsecount)
		{
			continue;	// not present this frame

		}
		if (!state->modelindex)
		{
			continue;
		}

		cl.h2_players[j].modelindex = state->modelindex;

		// the player object never gets added
		if (j == cl.playernum)
		{
			continue;
		}

		// grab an entity to fill in
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		ent.hModel = cl.model_draw[state->modelindex];
		ent.skinNum = state->skinnum;
		ent.frame = state->frame;

		int drawflags = state->drawflags;
		if (ent.hModel == clh2_player_models[0] ||
			ent.hModel == clh2_player_models[1] ||
			ent.hModel == clh2_player_models[2] ||
			ent.hModel == clh2_player_models[3] ||
			ent.hModel == clh2_player_models[4] ||	//mg-siege
			ent.hModel == clh2_player_models[5])
		{
			// use custom skin
			info->shownames_off = false;
		}

		//
		// angles
		//
		vec3_t angles;
		angles[PITCH] = -state->viewangles[PITCH] / 3;
		angles[YAW] = state->viewangles[YAW];
		angles[ROLL] = 0;
		angles[ROLL] = VQH_CalcRoll(angles, state->velocity) * 4;

		// only predict half the move to minimize overruns
		int msec = 500 * (playertime - state->state_time);
		if (msec <= 0 || (!cl_predict_players->value && !cl_predict_players2->value) || j == cl.playernum)
		{
			VectorCopy(state->origin, ent.origin);
		}
		else
		{
			// predict players movement
			if (msec > 255)
			{
				msec = 255;
			}
			state->command.msec = msec;

			int oldphysent = qh_pmove.numphysent;
			CLQHW_SetSolidPlayers(j);
			hwplayer_state_t exact;
			CLHW_PredictUsercmd(state, &exact, &state->command, false);
			qh_pmove.numphysent = oldphysent;
			VectorCopy(exact.origin, ent.origin);
		}

		if (clhw_siege)
		{
			if ((int)state->effects & H2EF_NODRAW)
			{
				ent.skinNum = 101;	//ice, but in siege will be invis skin for dwarf to see
				drawflags |= H2DRF_TRANSLUCENT;
				state->effects &= ~H2EF_NODRAW;
			}
		}

		vec3_t angleAdd;
		HandleEffects(state->effects, j + 1, &ent, angles, angleAdd);

		//	This uses behavior of software renderer as GL version was fucked
		// up because it didn't take into the account the fact that shadelight
		// has divided by 200 at this point.
		int colorshade = 0;
		if (!info->shownames_off)
		{
			int my_team = cl.h2_players[cl.playernum].siege_team;
			int ve_team = info->siege_team;
			float ambientlight = R_CalcEntityLight(&ent);
			float shadelight = ambientlight;

			// clamp lighting so it doesn't overbright as much
			if (ambientlight > 128)
			{
				ambientlight = 128;
			}
			if (ambientlight + shadelight > 192)
			{
				shadelight = 192 - ambientlight;
			}
			if ((ambientlight + shadelight) > 75 || (clhw_siege && my_team == ve_team))
			{
				info->shownames_off = false;
			}
			else
			{
				info->shownames_off = true;
			}
			if (clhw_siege)
			{
				if (cl.h2_players[cl.playernum].playerclass == CLASS_DWARF && ent.skinNum == 101)
				{
					colorshade = 141;
					info->shownames_off = false;
				}
				else if (cl.h2_players[cl.playernum].playerclass == CLASS_DWARF && (ambientlight + shadelight) < 151)
				{
					colorshade = 138 + (int)((ambientlight + shadelight) / 30);
					info->shownames_off = false;
				}
				else if (ve_team == HWST_DEFENDER)
				{
					//tint gold since we can't have seperate skins
					colorshade = 165;
				}
			}
			else
			{
				char client_team[16];
				String::NCpy(client_team, Info_ValueForKey(cl.h2_players[cl.playernum].userinfo, "team"), 16);
				client_team[15] = 0;
				if (client_team[0])
				{
					char this_team[16];
					String::NCpy(this_team, Info_ValueForKey(info->userinfo, "team"), 16);
					this_team[15] = 0;
					if (String::ICmp(client_team, this_team) == 0)
					{
						colorshade = clhw_teamcolor->value;
					}
				}
			}
		}

		CLH2_SetRefEntAxis(&ent, angles, angleAdd, state->scale, colorshade, state->abslight, drawflags);
		CLH2_HandleCustomSkin(&ent, j);
		R_AddRefEntityToScene(&ent);
	}
}

void CLH2_EmitEntities()
{
	CLH2_RelinkEntities();
	CLH2_UpdateTEnts();
	CLH2_LinkStaticEntities();
}

void CLHW_EmitEntities()
{
	if (cls.state != CA_ACTIVE)
	{
		return;
	}
	if (!cl.qh_validsequence)
	{
		return;
	}

	R_ClearScene();

	CLHW_LinkPlayers();
	CLHW_LinkPacketEntities();
	CLH2_LinkProjectiles();
	CLH2_LinkMissiles();
	CLH2_UpdateTEnts();
	CLH2_LinkStaticEntities();
}
