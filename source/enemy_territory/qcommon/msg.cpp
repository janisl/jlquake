/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../game/q_shared.h"
#include "qcommon.h"

int pcount[256];
int wastedbits = 0;

extern int oldsize;

/*
==============================================================================

            MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

void MSG_Init(QMsg* buf, byte* data, int length)
{
	buf->Init(data, length);
}

void MSG_InitOOB(QMsg* buf, byte* data, int length)
{
	buf->InitOOB(data, length);
}

void MSG_Uncompressed(QMsg* buf)
{
	// align to byte-boundary
	buf->bit = (buf->bit + 7) & ~7;
	buf->oob = qtrue;
}

void MSG_BeginReadingUncompressed(QMsg* buf)
{
	// align to byte-boundary
	buf->bit = (buf->bit + 7) & ~7;
	buf->oob = qtrue;
}

/*
=============================================================================

delta functions

=============================================================================
*/

extern Cvar* cl_shownet;

#define LOG(x) if (cl_shownet && cl_shownet->integer == 4) { Com_Printf("%s ", x); };

void MSG_WriteDelta(QMsg* msg, int oldV, int newV, int bits)
{
	if (oldV == newV)
	{
		msg->WriteBits(0, 1);
		return;
	}
	msg->WriteBits(1, 1);
	msg->WriteBits(newV, bits);
}

int MSG_ReadDelta(QMsg* msg, int oldV, int bits)
{
	if (msg->ReadBits(1))
	{
		return msg->ReadBits(bits);
	}
	return oldV;
}

void MSG_WriteDeltaFloat(QMsg* msg, float oldV, float newV)
{
	if (oldV == newV)
	{
		msg->WriteBits(0, 1);
		return;
	}
	msg->WriteBits(1, 1);
	msg->WriteBits(*(int*)&newV, 32);
}

float MSG_ReadDeltaFloat(QMsg* msg, float oldV)
{
	if (msg->ReadBits(1))
	{
		float newV;

		*(int*)&newV = msg->ReadBits(32);
		return newV;
	}
	return oldV;
}

/*
=============================================================================

delta functions with keys

=============================================================================
*/

int kbitmask[32] = {
	0x00000001, 0x00000003, 0x00000007, 0x0000000F,
	0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
	0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
	0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
	0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
	0x001FFFFf, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
	0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
	0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

void MSG_WriteDeltaKey(QMsg* msg, int key, int oldV, int newV, int bits)
{
	if (oldV == newV)
	{
		msg->WriteBits(0, 1);
		return;
	}
	msg->WriteBits(1, 1);
	msg->WriteBits(newV ^ key, bits);
}

int MSG_ReadDeltaKey(QMsg* msg, int key, int oldV, int bits)
{
	if (msg->ReadBits(1))
	{
		return msg->ReadBits(bits) ^ (key & kbitmask[bits]);
	}
	return oldV;
}

void MSG_WriteDeltaKeyFloat(QMsg* msg, int key, float oldV, float newV)
{
	if (oldV == newV)
	{
		msg->WriteBits(0, 1);
		return;
	}
	msg->WriteBits(1, 1);
	msg->WriteBits((*(int*)&newV) ^ key, 32);
}

float MSG_ReadDeltaKeyFloat(QMsg* msg, int key, float oldV)
{
	if (msg->ReadBits(1))
	{
		float newV;

		*(int*)&newV = msg->ReadBits(32) ^ key;
		return newV;
	}
	return oldV;
}


/*
============================================================================

etusercmd_t communication

============================================================================
*/

// ms is allways sent, the others are optional
#define CM_ANGLE1   (1 << 0)
#define CM_ANGLE2   (1 << 1)
#define CM_ANGLE3   (1 << 2)
#define CM_FORWARD  (1 << 3)
#define CM_SIDE     (1 << 4)
#define CM_UP       (1 << 5)
#define CM_BUTTONS  (1 << 6)
#define CM_WEAPON   (1 << 7)

/*
=====================
MSG_WriteDeltaUsercmd
=====================
*/
void MSG_WriteDeltaUsercmd(QMsg* msg, etusercmd_t* from, etusercmd_t* to)
{
	if (to->serverTime - from->serverTime < 256)
	{
		msg->WriteBits(1, 1);
		msg->WriteBits(to->serverTime - from->serverTime, 8);
	}
	else
	{
		msg->WriteBits(0, 1);
		msg->WriteBits(to->serverTime, 32);
	}
	MSG_WriteDelta(msg, from->angles[0], to->angles[0], 16);
	MSG_WriteDelta(msg, from->angles[1], to->angles[1], 16);
	MSG_WriteDelta(msg, from->angles[2], to->angles[2], 16);
	MSG_WriteDelta(msg, from->forwardmove, to->forwardmove, 8);
	MSG_WriteDelta(msg, from->rightmove, to->rightmove, 8);
	MSG_WriteDelta(msg, from->upmove, to->upmove, 8);
	MSG_WriteDelta(msg, from->buttons, to->buttons, 8);
	MSG_WriteDelta(msg, from->wbuttons, to->wbuttons, 8);
	MSG_WriteDelta(msg, from->weapon, to->weapon, 8);
	MSG_WriteDelta(msg, from->flags, to->flags, 8);
	MSG_WriteDelta(msg, from->doubleTap, to->doubleTap, 3);
	MSG_WriteDelta(msg, from->identClient, to->identClient, 8);				// NERVE - SMF
}


/*
=====================
MSG_ReadDeltaUsercmd
=====================
*/
void MSG_ReadDeltaUsercmd(QMsg* msg, etusercmd_t* from, etusercmd_t* to)
{
	if (msg->ReadBits(1))
	{
		to->serverTime = from->serverTime + msg->ReadBits(8);
	}
	else
	{
		to->serverTime = msg->ReadBits(32);
	}
	to->angles[0] = MSG_ReadDelta(msg, from->angles[0], 16);
	to->angles[1] = MSG_ReadDelta(msg, from->angles[1], 16);
	to->angles[2] = MSG_ReadDelta(msg, from->angles[2], 16);
	to->forwardmove = MSG_ReadDelta(msg, from->forwardmove, 8);
	to->rightmove = MSG_ReadDelta(msg, from->rightmove, 8);
	to->upmove = MSG_ReadDelta(msg, from->upmove, 8);
	to->buttons = MSG_ReadDelta(msg, from->buttons, 8);
	to->wbuttons = MSG_ReadDelta(msg, from->wbuttons, 8);
	to->weapon = MSG_ReadDelta(msg, from->weapon, 8);
	to->flags = MSG_ReadDelta(msg, from->flags, 8);
	to->doubleTap = MSG_ReadDelta(msg, from->doubleTap, 3) & 0x7;
	to->identClient = MSG_ReadDelta(msg, from->identClient, 8);		// NERVE - SMF
}

/*
=====================
MSG_WriteDeltaUsercmd
=====================
*/
void MSG_WriteDeltaUsercmdKey(QMsg* msg, int key, etusercmd_t* from, etusercmd_t* to)
{
	if (to->serverTime - from->serverTime < 256)
	{
		msg->WriteBits(1, 1);
		msg->WriteBits(to->serverTime - from->serverTime, 8);
	}
	else
	{
		msg->WriteBits(0, 1);
		msg->WriteBits(to->serverTime, 32);
	}
	if (from->angles[0] == to->angles[0] &&
		from->angles[1] == to->angles[1] &&
		from->angles[2] == to->angles[2] &&
		from->forwardmove == to->forwardmove &&
		from->rightmove == to->rightmove &&
		from->upmove == to->upmove &&
		from->buttons == to->buttons &&
		from->wbuttons == to->wbuttons &&
		from->weapon == to->weapon &&
		from->flags == to->flags &&
		from->doubleTap == to->doubleTap &&
		from->identClient == to->identClient)		// NERVE - SMF
	{
		msg->WriteBits(0, 1);					// no change
		oldsize += 7;
		return;
	}
	key ^= to->serverTime;
	msg->WriteBits(1, 1);
	MSG_WriteDeltaKey(msg, key, from->angles[0], to->angles[0], 16);
	MSG_WriteDeltaKey(msg, key, from->angles[1], to->angles[1], 16);
	MSG_WriteDeltaKey(msg, key, from->angles[2], to->angles[2], 16);
	MSG_WriteDeltaKey(msg, key, from->forwardmove, to->forwardmove, 8);
	MSG_WriteDeltaKey(msg, key, from->rightmove, to->rightmove, 8);
	MSG_WriteDeltaKey(msg, key, from->upmove, to->upmove, 8);
	MSG_WriteDeltaKey(msg, key, from->buttons, to->buttons, 8);
	MSG_WriteDeltaKey(msg, key, from->wbuttons, to->wbuttons, 8);
	MSG_WriteDeltaKey(msg, key, from->weapon, to->weapon, 8);
	MSG_WriteDeltaKey(msg, key, from->flags, to->flags, 8);
	MSG_WriteDeltaKey(msg, key, from->doubleTap, to->doubleTap, 3);
	MSG_WriteDeltaKey(msg, key, from->identClient, to->identClient, 8);			// NERVE - SMF
}


/*
=====================
MSG_ReadDeltaUsercmd
=====================
*/
void MSG_ReadDeltaUsercmdKey(QMsg* msg, int key, etusercmd_t* from, etusercmd_t* to)
{
	if (msg->ReadBits(1))
	{
		to->serverTime = from->serverTime + msg->ReadBits(8);
	}
	else
	{
		to->serverTime = msg->ReadBits(32);
	}
	if (msg->ReadBits(1))
	{
		key ^= to->serverTime;
		to->angles[0] = MSG_ReadDeltaKey(msg, key, from->angles[0], 16);
		to->angles[1] = MSG_ReadDeltaKey(msg, key, from->angles[1], 16);
		to->angles[2] = MSG_ReadDeltaKey(msg, key, from->angles[2], 16);
		to->forwardmove = MSG_ReadDeltaKey(msg, key, from->forwardmove, 8);
		to->rightmove = MSG_ReadDeltaKey(msg, key, from->rightmove, 8);
		to->upmove = MSG_ReadDeltaKey(msg, key, from->upmove, 8);
		to->buttons = MSG_ReadDeltaKey(msg, key, from->buttons, 8);
		to->wbuttons = MSG_ReadDeltaKey(msg, key, from->wbuttons, 8);
		to->weapon = MSG_ReadDeltaKey(msg, key, from->weapon, 8);
		to->flags = MSG_ReadDeltaKey(msg, key, from->flags, 8);
		to->doubleTap = MSG_ReadDeltaKey(msg, key, from->doubleTap, 3) & 0x7;
		to->identClient = MSG_ReadDeltaKey(msg, key, from->identClient, 8);				// NERVE - SMF
	}
	else
	{
		to->angles[0] = from->angles[0];
		to->angles[1] = from->angles[1];
		to->angles[2] = from->angles[2];
		to->forwardmove = from->forwardmove;
		to->rightmove = from->rightmove;
		to->upmove = from->upmove;
		to->buttons = from->buttons;
		to->wbuttons = from->wbuttons;
		to->weapon = from->weapon;
		to->flags = from->flags;
		to->doubleTap = from->doubleTap;
		to->identClient = from->identClient;			// NERVE - SMF
	}
}


/*
=============================================================================

etentityState_t communication

=============================================================================
*/

/*
=================
MSG_ReportChangeVectors_f

Prints out a table from the current statistics for copying to code
=================
*/
void MSG_ReportChangeVectors_f(void)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		if (pcount[i])
		{
			Com_Printf("%d used %d\n", i, pcount[i]);
		}
	}
}

typedef struct
{
	const char* name;
	int offset;
	int bits;			// 0 = float
	int used;
} netField_t;

// using the stringizing operator to save typing...
#define NETF(x) # x,(qintptr) & ((etentityState_t*)0)->x

netField_t entityStateFields[] = {
	{ NETF(eType), 8 },
	{ NETF(eFlags), 24 },
	{ NETF(pos.trType), 8 },
	{ NETF(pos.trTime), 32 },
	{ NETF(pos.trDuration), 32 },
	{ NETF(pos.trBase[0]), 0 },
	{ NETF(pos.trBase[1]), 0 },
	{ NETF(pos.trBase[2]), 0 },
	{ NETF(pos.trDelta[0]), 0 },
	{ NETF(pos.trDelta[1]), 0 },
	{ NETF(pos.trDelta[2]), 0 },
	{ NETF(apos.trType), 8 },
	{ NETF(apos.trTime), 32 },
	{ NETF(apos.trDuration), 32 },
	{ NETF(apos.trBase[0]), 0 },
	{ NETF(apos.trBase[1]), 0 },
	{ NETF(apos.trBase[2]), 0 },
	{ NETF(apos.trDelta[0]), 0 },
	{ NETF(apos.trDelta[1]), 0 },
	{ NETF(apos.trDelta[2]), 0 },
	{ NETF(time), 32 },
	{ NETF(time2), 32 },
	{ NETF(origin[0]), 0 },
	{ NETF(origin[1]), 0 },
	{ NETF(origin[2]), 0 },
	{ NETF(origin2[0]), 0 },
	{ NETF(origin2[1]), 0 },
	{ NETF(origin2[2]), 0 },
	{ NETF(angles[0]), 0 },
	{ NETF(angles[1]), 0 },
	{ NETF(angles[2]), 0 },
	{ NETF(angles2[0]), 0 },
	{ NETF(angles2[1]), 0 },
	{ NETF(angles2[2]), 0 },
	{ NETF(otherEntityNum), GENTITYNUM_BITS_Q3 },
	{ NETF(otherEntityNum2), GENTITYNUM_BITS_Q3 },
	{ NETF(groundEntityNum), GENTITYNUM_BITS_Q3 },
	{ NETF(loopSound), 8 },
	{ NETF(constantLight), 32 },
	{ NETF(dl_intensity), 32 },		//----(SA)	longer now to carry the corona colors
	{ NETF(modelindex), 9 },
	{ NETF(modelindex2), 9 },
	{ NETF(frame), 16 },
	{ NETF(clientNum), 8 },
	{ NETF(solid), 24 },
	{ NETF(event), 10 },
	{ NETF(eventParm), 8 },
	{ NETF(eventSequence), 8 },		// warning: need to modify cg_event.c at "// check the sequencial list" if you change this
	{ NETF(events[0]), 8 },
	{ NETF(events[1]), 8 },
	{ NETF(events[2]), 8 },
	{ NETF(events[3]), 8 },
	{ NETF(eventParms[0]), 8 },
	{ NETF(eventParms[1]), 8 },
	{ NETF(eventParms[2]), 8 },
	{ NETF(eventParms[3]), 8 },
	{ NETF(powerups), 16 },
	{ NETF(weapon), 8 },
	{ NETF(legsAnim), ANIM_BITS },
	{ NETF(torsoAnim), ANIM_BITS },
	{ NETF(density), 10},
	{ NETF(dmgFlags), 32 },		//----(SA)	additional info flags for damage
	{ NETF(onFireStart), 32},
	{ NETF(onFireEnd), 32},
	{ NETF(nextWeapon), 8},
	{ NETF(teamNum), 8},
	{ NETF(effect1Time), 32},
	{ NETF(effect2Time), 32},
	{ NETF(effect3Time), 32},
	{ NETF(animMovetype), 4},
	{ NETF(aiState), 2},
};


static int QDECL qsort_entitystatefields(const void* a, const void* b)
{
	int aa, bb;

	aa = *((int*)a);
	bb = *((int*)b);

	if (entityStateFields[aa].used > entityStateFields[bb].used)
	{
		return -1;
	}
	if (entityStateFields[bb].used > entityStateFields[aa].used)
	{
		return 1;
	}
	return 0;
}

void MSG_PrioritiseEntitystateFields(void)
{
	int fieldorders[sizeof(entityStateFields) / sizeof(entityStateFields[0])];
	int numfields = sizeof(entityStateFields) / sizeof(entityStateFields[0]);
	int i;

	for (i = 0; i < numfields; i++)
	{
		fieldorders[i] = i;
	}

	qsort(fieldorders, numfields, sizeof(int), qsort_entitystatefields);

	Com_Printf("Entitystate fields in order of priority\n");
	Com_Printf("netField_t entityStateFields[] = {\n");
	for (i = 0; i < numfields; i++)
	{
		Com_Printf("{ NETF (%s), %i },\n", entityStateFields[fieldorders[i]].name, entityStateFields[fieldorders[i]].bits);
	}
	Com_Printf("};\n");
}

// if (int)f == f and (int)f + ( 1<<(FLOAT_INT_BITS-1) ) < ( 1 << FLOAT_INT_BITS )
// the float will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define FLOAT_INT_BITS  13
#define FLOAT_INT_BIAS  (1 << (FLOAT_INT_BITS - 1))

/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is NULL, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
==================
*/
void MSG_WriteDeltaEntity(QMsg* msg, struct etentityState_t* from, struct etentityState_t* to,
	qboolean force)
{
	int i, lc;
	int numFields;
	netField_t* field;
	int trunc;
	float fullFloat;
	int* fromF, * toF;

	numFields = sizeof(entityStateFields) / sizeof(entityStateFields[0]);

	// all fields should be 32 bits to avoid any compiler packing issues
	// the "number" field is not part of the field list
	// if this assert fails, someone added a field to the etentityState_t
	// struct without updating the message fields
	assert(numFields + 1 == sizeof(*from) / 4);

	// a NULL to is a delta remove message
	if (to == NULL)
	{
		if (from == NULL)
		{
			return;
		}
		if (cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -1))
		{
			Com_Printf("W|%3i: #%-3i remove\n", msg->cursize, from->number);
		}
		msg->WriteBits(from->number, GENTITYNUM_BITS_Q3);
		msg->WriteBits(1, 1);
		return;
	}

	if (to->number < 0 || to->number >= MAX_GENTITIES_Q3)
	{
		Com_Error(ERR_FATAL, "MSG_WriteDeltaEntity: Bad entity number: %i", to->number);
	}

	lc = 0;
	// build the change vector as bytes so it is endien independent
	for (i = 0, field = entityStateFields; i < numFields; i++, field++)
	{
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);
		if (*fromF != *toF)
		{
			lc = i + 1;

			field->used++;
		}
	}

	if (lc == 0)
	{
		// nothing at all changed
		if (!force)
		{
			return;		// nothing at all
		}
		// write two bits for no change
		msg->WriteBits(to->number, GENTITYNUM_BITS_Q3);
		msg->WriteBits(0, 1);		// not removed
		msg->WriteBits(0, 1);		// no delta
		return;
	}

	msg->WriteBits(to->number, GENTITYNUM_BITS_Q3);
	msg->WriteBits(0, 1);			// not removed
	msg->WriteBits(1, 1);			// we have a delta

	msg->WriteByte(lc);		// # of changes

	oldsize += numFields;

//	Com_Printf( "Delta for ent %i: ", to->number );

	for (i = 0, field = entityStateFields; i < lc; i++, field++)
	{
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);

		if (*fromF == *toF)
		{
			msg->WriteBits(0, 1);	// no change

			wastedbits++;

			continue;
		}

		msg->WriteBits(1, 1);	// changed

		if (field->bits == 0)
		{
			// float
			fullFloat = *(float*)toF;
			trunc = (int)fullFloat;

			if (fullFloat == 0.0f)
			{
				msg->WriteBits(0, 1);
				oldsize += FLOAT_INT_BITS;
			}
			else
			{
				msg->WriteBits(1, 1);
				if (trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 &&
					trunc + FLOAT_INT_BIAS < (1 << FLOAT_INT_BITS))
				{
					// send as small integer
					msg->WriteBits(0, 1);
					msg->WriteBits(trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS);
//					if ( print ) {
//						Com_Printf( "%s:%i ", field->name, trunc );
//					}
				}
				else
				{
					// send as full floating point value
					msg->WriteBits(1, 1);
					msg->WriteBits(*toF, 32);
//					if ( print ) {
//						Com_Printf( "%s:%f ", field->name, *(float *)toF );
//					}
				}
			}
		}
		else
		{
			if (*toF == 0)
			{
				msg->WriteBits(0, 1);
			}
			else
			{
				msg->WriteBits(1, 1);
				// integer
				msg->WriteBits(*toF, field->bits);
//				if ( print ) {
//					Com_Printf( "%s:%i ", field->name, *toF );
//				}
			}
		}
	}

//	Com_Printf( "\n" );

/*
    c = msg->cursize - c;

    if ( print ) {
        if ( msg->bit == 0 ) {
            endBit = msg->cursize * 8 - GENTITYNUM_BITS_Q3;
        } else {
            endBit = ( msg->cursize - 1 ) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
        }
        Com_Printf( " (%i bits)\n", endBit - startBit  );
    }
*/
}

/*
==================
MSG_ReadDeltaEntity

The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, etentityState_t->number will be set to MAX_GENTITIES_Q3-1

Can go from either a baseline or a previous packet_entity
==================
*/
extern Cvar* cl_shownet;

void MSG_ReadDeltaEntity(QMsg* msg, etentityState_t* from, etentityState_t* to,
	int number)
{
	int i, lc;
	int numFields;
	netField_t* field;
	int* fromF, * toF;
	int print;
	int trunc;
	int startBit, endBit;

	if (number < 0 || number >= MAX_GENTITIES_Q3)
	{
		Com_Error(ERR_DROP, "Bad delta entity number: %i", number);
	}

	if (msg->bit == 0)
	{
		startBit = msg->readcount * 8 - GENTITYNUM_BITS_Q3;
	}
	else
	{
		startBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
	}

	// check for a remove
	if (msg->ReadBits(1) == 1)
	{
		memset(to, 0, sizeof(*to));
		to->number = MAX_GENTITIES_Q3 - 1;
		if (cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -1))
		{
			Com_Printf("%3i: #%-3i remove\n", msg->readcount, number);
		}
		return;
	}

	// check for no delta
	if (msg->ReadBits(1) == 0)
	{
		*to = *from;
		to->number = number;
		return;
	}

	numFields = sizeof(entityStateFields) / sizeof(entityStateFields[0]);
	lc = msg->ReadByte();

	if (lc > numFields)		//DAJ FIXME have gotten lc = 76 which is bigger than numFields!
	{
		lc = numFields;
	}

	// shownet 2/3 will interleave with other printed info, -1 will
	// just print the delta records`
	if (cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -1))
	{
		print = 1;
		Com_Printf("%3i: #%-3i ", msg->readcount, to->number);
	}
	else
	{
		print = 0;
	}

	to->number = number;

	for (i = 0, field = entityStateFields; i < lc; i++, field++)
	{
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);

		if (!msg->ReadBits(1))
		{
			// no change
			*toF = *fromF;
		}
		else
		{
			if (field->bits == 0)
			{
				// float
				if (msg->ReadBits(1) == 0)
				{
					*(float*)toF = 0.0f;
				}
				else
				{
					if (msg->ReadBits(1) == 0)
					{
						// integral float
						trunc = msg->ReadBits(FLOAT_INT_BITS);
						// bias to allow equal parts positive and negative
						trunc -= FLOAT_INT_BIAS;
						*(float*)toF = trunc;
						if (print)
						{
							Com_Printf("%s:%i ", field->name, trunc);
						}
					}
					else
					{
						// full floating point value
						*toF = msg->ReadBits(32);
						if (print)
						{
							Com_Printf("%s:%f ", field->name, *(float*)toF);
						}
					}
				}
			}
			else
			{
				if (msg->ReadBits(1) == 0)
				{
					*toF = 0;
				}
				else
				{
					// integer
					*toF = msg->ReadBits(field->bits);
					if (print)
					{
						Com_Printf("%s:%i ", field->name, *toF);
					}
				}
			}
//			pcount[i]++;
		}
	}
	for (i = lc, field = &entityStateFields[lc]; i < numFields; i++, field++)
	{
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);
		// no change
		*toF = *fromF;
	}

	if (print)
	{
		if (msg->bit == 0)
		{
			endBit = msg->readcount * 8 - GENTITYNUM_BITS_Q3;
		}
		else
		{
			endBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
		}
		Com_Printf(" (%i bits)\n", endBit - startBit);
	}
}


/*
============================================================================

player_state_t communication

============================================================================
*/

// using the stringizing operator to save typing...
#define PSF(x) # x,(qintptr) & ((etplayerState_t*)0)->x

netField_t playerStateFields[] = {
	{ PSF(commandTime), 32 },
	{ PSF(pm_type), 8 },
	{ PSF(bobCycle), 8 },
	{ PSF(pm_flags), 16 },
	{ PSF(pm_time), -16 },
	{ PSF(origin[0]), 0 },
	{ PSF(origin[1]), 0 },
	{ PSF(origin[2]), 0 },
	{ PSF(velocity[0]), 0 },
	{ PSF(velocity[1]), 0 },
	{ PSF(velocity[2]), 0 },
	{ PSF(weaponTime), -16 },
	{ PSF(weaponDelay), -16 },
	{ PSF(grenadeTimeLeft), -16 },
	{ PSF(gravity), 16 },
	{ PSF(leanf), 0 },
	{ PSF(speed), 16 },
	{ PSF(delta_angles[0]), 16 },
	{ PSF(delta_angles[1]), 16 },
	{ PSF(delta_angles[2]), 16 },
	{ PSF(groundEntityNum), GENTITYNUM_BITS_Q3 },
	{ PSF(legsTimer), 16 },
	{ PSF(torsoTimer), 16 },
	{ PSF(legsAnim), ANIM_BITS },
	{ PSF(torsoAnim), ANIM_BITS },
	{ PSF(movementDir), 8 },
	{ PSF(eFlags), 24 },
	{ PSF(eventSequence), 8 },
	{ PSF(events[0]), 8 },
	{ PSF(events[1]), 8 },
	{ PSF(events[2]), 8 },
	{ PSF(events[3]), 8 },
	{ PSF(eventParms[0]), 8 },
	{ PSF(eventParms[1]), 8 },
	{ PSF(eventParms[2]), 8 },
	{ PSF(eventParms[3]), 8 },
	{ PSF(clientNum), 8 },
	{ PSF(weapons[0]), 32 },
	{ PSF(weapons[1]), 32 },
	{ PSF(weapon), 7 },		// (SA) yup, even more
	{ PSF(weaponstate), 4 },
	{ PSF(weapAnim), 10 },
	{ PSF(viewangles[0]), 0 },
	{ PSF(viewangles[1]), 0 },
	{ PSF(viewangles[2]), 0 },
	{ PSF(viewheight), -8 },
	{ PSF(damageEvent), 8 },
	{ PSF(damageYaw), 8 },
	{ PSF(damagePitch), 8 },
	{ PSF(damageCount), 8 },
	{ PSF(mins[0]), 0 },
	{ PSF(mins[1]), 0 },
	{ PSF(mins[2]), 0 },
	{ PSF(maxs[0]), 0 },
	{ PSF(maxs[1]), 0 },
	{ PSF(maxs[2]), 0 },
	{ PSF(crouchMaxZ), 0 },
	{ PSF(crouchViewHeight), 0 },
	{ PSF(standViewHeight), 0 },
	{ PSF(deadViewHeight), 0 },
	{ PSF(runSpeedScale), 0 },
	{ PSF(sprintSpeedScale), 0 },
	{ PSF(crouchSpeedScale), 0 },
	{ PSF(friction), 0 },
	{ PSF(viewlocked), 8 },
	{ PSF(viewlocked_entNum), 16 },
	{ PSF(nextWeapon), 8 },
	{ PSF(teamNum), 8 },
//{ PSF(gunfx), 8},
	{ PSF(onFireStart), 32},
	{ PSF(curWeapHeat), 8 },
	{ PSF(aimSpreadScale), 8},
	{ PSF(serverCursorHint), 8},	//----(SA)	added
	{ PSF(serverCursorHintVal), 8},		//----(SA)	added
	{ PSF(classWeaponTime), 32},	// JPW NERVE
	{ PSF(identifyClient), 8 },
	{ PSF(identifyClientHealth), 8 },
	{ PSF(aiState), 2},
};

static int QDECL qsort_playerstatefields(const void* a, const void* b)
{
	int aa, bb;

	aa = *((int*)a);
	bb = *((int*)b);

	if (playerStateFields[aa].used > playerStateFields[bb].used)
	{
		return -1;
	}
	if (playerStateFields[bb].used > playerStateFields[aa].used)
	{
		return 1;
	}
	return 0;
}

void MSG_PrioritisePlayerStateFields(void)
{
	int fieldorders[sizeof(playerStateFields) / sizeof(playerStateFields[0])];
	int numfields = sizeof(playerStateFields) / sizeof(playerStateFields[0]);
	int i;

	for (i = 0; i < numfields; i++)
	{
		fieldorders[i] = i;
	}

	qsort(fieldorders, numfields, sizeof(int), qsort_playerstatefields);

	Com_Printf("Playerstate fields in order of priority\n");
	Com_Printf("netField_t playerStateFields[] = {\n");
	for (i = 0; i < numfields; i++)
	{
		Com_Printf("{ PSF(%s), %i },\n", playerStateFields[fieldorders[i]].name, playerStateFields[fieldorders[i]].bits);
	}
	Com_Printf("};\n");
}



/*
=============
MSG_WriteDeltaPlayerstate

=============
*/
void MSG_WriteDeltaPlayerstate(QMsg* msg, struct etplayerState_t* from, struct etplayerState_t* to)
{
	int i, j, lc;
	etplayerState_t dummy;
	int statsbits;
	int persistantbits;
	int ammobits[4];				//----(SA)	modified
	int clipbits;					//----(SA)	added
	int powerupbits;
	int holdablebits;
	int numFields;
//bani - appears to have been debugging left in
//	int				c;
	netField_t* field;
	int* fromF, * toF;
	float fullFloat;
	int trunc;
	int startBit, endBit;
	int print;

	if (!from)
	{
		from = &dummy;
		memset(&dummy, 0, sizeof(dummy));
	}

	if (msg->bit == 0)
	{
		startBit = msg->cursize * 8 - GENTITYNUM_BITS_Q3;
	}
	else
	{
		startBit = (msg->cursize - 1) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
	}

	// shownet 2/3 will interleave with other printed info, -2 will
	// just print the delta records
	if (cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -2))
	{
		print = 1;
		Com_Printf("W|%3i: playerstate ", msg->cursize);
	}
	else
	{
		print = 0;
	}

//bani - appears to have been debugging left in
//	c = msg->cursize;

	numFields = sizeof(playerStateFields) / sizeof(playerStateFields[0]);

	lc = 0;
	for (i = 0, field = playerStateFields; i < numFields; i++, field++)
	{
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);
		if (*fromF != *toF)
		{
			lc = i + 1;

			field->used++;
		}
	}

	msg->WriteByte(lc);		// # of changes

	oldsize += numFields - lc;

	for (i = 0, field = playerStateFields; i < lc; i++, field++)
	{
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);

		if (*fromF == *toF)
		{
			wastedbits++;

			msg->WriteBits(0, 1);	// no change
			continue;
		}

		msg->WriteBits(1, 1);	// changed
//		pcount[i]++;

		if (field->bits == 0)
		{
			// float
			fullFloat = *(float*)toF;
			trunc = (int)fullFloat;

			if (trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 &&
				trunc + FLOAT_INT_BIAS < (1 << FLOAT_INT_BITS))
			{
				// send as small integer
				msg->WriteBits(0, 1);
				msg->WriteBits(trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS);
//				if ( print ) {
//					Com_Printf( "%s:%i ", field->name, trunc );
//				}
			}
			else
			{
				// send as full floating point value
				msg->WriteBits(1, 1);
				msg->WriteBits(*toF, 32);
//				if ( print ) {
//					Com_Printf( "%s:%f ", field->name, *(float *)toF );
//				}
			}
		}
		else
		{
			// integer
			msg->WriteBits(*toF, field->bits);
//			if ( print ) {
//				Com_Printf( "%s:%i ", field->name, *toF );
//			}
		}
	}
//bani - appears to have been debugging left in
//	c = msg->cursize - c;


	//
	// send the arrays
	//
	statsbits = 0;
	for (i = 0; i < 16; i++)
	{
		if (to->stats[i] != from->stats[i])
		{
			statsbits |= 1 << i;
		}
	}
	persistantbits = 0;
	for (i = 0; i < 16; i++)
	{
		if (to->persistant[i] != from->persistant[i])
		{
			persistantbits |= 1 << i;
		}
	}
	holdablebits = 0;
	for (i = 0; i < 16; i++)
	{
		if (to->holdable[i] != from->holdable[i])
		{
			holdablebits |= 1 << i;
		}
	}
	powerupbits = 0;
	for (i = 0; i < 16; i++)
	{
		if (to->powerups[i] != from->powerups[i])
		{
			powerupbits |= 1 << i;
		}
	}


	if (statsbits || persistantbits || holdablebits || powerupbits)
	{

		msg->WriteBits(1, 1);	// something changed

		if (statsbits)
		{
			msg->WriteBits(1, 1);	// changed
			msg->WriteShort(statsbits);
			for (i = 0; i < 16; i++)
				if (statsbits & (1 << i))
				{
					// RF, changed to long to allow more flexibility
//					MSG_WriteLong (msg, to->stats[i]);
					msg->WriteShort(to->stats[i]);		//----(SA)	back to short since weapon bits are handled elsewhere now
				}
		}
		else
		{
			msg->WriteBits(0, 1);	// no change to stats
		}


		if (persistantbits)
		{
			msg->WriteBits(1, 1);	// changed
			msg->WriteShort(persistantbits);
			for (i = 0; i < 16; i++)
				if (persistantbits & (1 << i))
				{
					msg->WriteShort(to->persistant[i]);
				}
		}
		else
		{
			msg->WriteBits(0, 1);	// no change to persistant
		}


		if (holdablebits)
		{
			msg->WriteBits(1, 1);	// changed
			msg->WriteShort(holdablebits);
			for (i = 0; i < 16; i++)
				if (holdablebits & (1 << i))
				{
					msg->WriteShort(to->holdable[i]);
				}
		}
		else
		{
			msg->WriteBits(0, 1);	// no change to holdables
		}


		if (powerupbits)
		{
			msg->WriteBits(1, 1);	// changed
			msg->WriteShort(powerupbits);
			for (i = 0; i < 16; i++)
				if (powerupbits & (1 << i))
				{
					msg->WriteLong(to->powerups[i]);
				}
		}
		else
		{
			msg->WriteBits(0, 1);	// no change to powerups
		}
	}
	else
	{
		msg->WriteBits(0, 1);	// no change to any
		oldsize += 4;
	}


#if 0
// RF, optimization
//		Send a single bit to signify whether or not the ammo/clip info changed.
//		If it did, send individual segments specifying offset values for each item.
	{
		int ammo_ofs;
		int clip_ofs;

		ammobits = 0;

		// ammo
		for (i = 0; i < 32; i++)
		{
			if (to->ammo[i] != from->ammo[i])
			{
				ammobits |= 1 << i;
			}
		}
		// ammoclip (just add these changes to the ammo changes. if either changes, we should send both, since they are likely to both change at once anyway)
		for (i = 0; i < 32; i++)
		{
			if (to->ammoclip[i] != from->ammoclip[i])
			{
				ammobits |= 1 << i;
			}
		}

		if (ammobits)
		{
			msg->WriteBits(1, 1);	// changed

			// send each changed item
			for (i = 0; i < 32; i++)
			{
				if (ammobits & (1 << i))
				{
					ammo_ofs = to->ammo[i] - from->ammo[i];
					clip_ofs = to->ammoclip[i] - from->ammoclip[i];

					while (ammo_ofs || clip_ofs)
					{
						msg->WriteBits(1, 1);	// signify that another index is present
						msg->WriteBits(i, 5);	// index number

						// ammo
						if (abs(ammo_ofs) > 127)
						{
							if (ammo_ofs > 0)
							{
								MSG_WriteChar(msg, 127);
								ammo_ofs -= 127;
							}
							else
							{
								MSG_WriteChar(msg, -127);
								ammo_ofs += 127;
							}
						}
						else
						{
							MSG_WriteChar(msg, ammo_ofs);
							ammo_ofs = 0;
						}

						// clip
						if (abs(clip_ofs) > 127)
						{
							if (clip_ofs > 0)
							{
								MSG_WriteChar(msg, 127);
								clip_ofs -= 127;
							}
							else
							{
								MSG_WriteChar(msg, -127);
								clip_ofs += 127;
							}
						}
						else
						{
							MSG_WriteChar(msg, clip_ofs);
							clip_ofs = 0;
						}
					}
				}
			}

			// signify the end of changes
			msg->WriteBits(0, 1);

		}
		else
		{
			msg->WriteBits(0, 1);	// no change
		}
	}

#else
//----(SA)	I split this into two groups using shorts so it wouldn't have
//			to use a long every time ammo changed for any weap.
//			this seemed like a much friendlier option than making it
//			read/write a long for any ammo change.

	// j == 0 : weaps 0-15
	// j == 1 : weaps 16-31
	// j == 2 : weaps 32-47	//----(SA)	now up to 64 (but still pretty net-friendly)
	// j == 3 : weaps 48-63

	// ammo stored
	for (j = 0; j < 4; j++)		//----(SA)	modified for 64 weaps
	{
		ammobits[j] = 0;
		for (i = 0; i < 16; i++)
		{
			if (to->ammo[i + (j * 16)] != from->ammo[i + (j * 16)])
			{
				ammobits[j] |= 1 << i;
			}
		}
	}

//----(SA)	also encapsulated ammo changes into one check.  clip values will change frequently,
	// but ammo will not.  (only when you get ammo/reload rather than each shot)
	if (ammobits[0] || ammobits[1] || ammobits[2] || ammobits[3])		// if any were set...
	{
		msg->WriteBits(1, 1);	// changed
		for (j = 0; j < 4; j++)
		{
			if (ammobits[j])
			{
				msg->WriteBits(1, 1);	// changed
				msg->WriteShort(ammobits[j]);
				for (i = 0; i < 16; i++)
					if (ammobits[j] & (1 << i))
					{
						msg->WriteShort(to->ammo[i + (j * 16)]);
					}
			}
			else
			{
				msg->WriteBits(0, 1);	// no change
			}
		}
	}
	else
	{
		msg->WriteBits(0, 1);	// no change
	}

	// ammo in clip
	for (j = 0; j < 4; j++)		//----(SA)	modified for 64 weaps
	{
		clipbits = 0;
		for (i = 0; i < 16; i++)
		{
			if (to->ammoclip[i + (j * 16)] != from->ammoclip[i + (j * 16)])
			{
				clipbits |= 1 << i;
			}
		}
		if (clipbits)
		{
			msg->WriteBits(1, 1);	// changed
			msg->WriteShort(clipbits);
			for (i = 0; i < 16; i++)
				if (clipbits & (1 << i))
				{
					msg->WriteShort(to->ammoclip[i + (j * 16)]);
				}
		}
		else
		{
			msg->WriteBits(0, 1);	// no change
		}
	}
#endif


	if (print)
	{
		if (msg->bit == 0)
		{
			endBit = msg->cursize * 8 - GENTITYNUM_BITS_Q3;
		}
		else
		{
			endBit = (msg->cursize - 1) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
		}
		Com_Printf(" (%i bits)\n", endBit - startBit);
	}

}


/*
===================
MSG_ReadDeltaPlayerstate
===================
*/
void MSG_ReadDeltaPlayerstate(QMsg* msg, etplayerState_t* from, etplayerState_t* to)
{
	int i, j, lc;
	int bits;
	netField_t* field;
	int numFields;
	int startBit, endBit;
	int print;
	int* fromF, * toF;
	int trunc;
	etplayerState_t dummy;

	if (!from)
	{
		from = &dummy;
		memset(&dummy, 0, sizeof(dummy));
	}
	*to = *from;

	if (msg->bit == 0)
	{
		startBit = msg->readcount * 8 - GENTITYNUM_BITS_Q3;
	}
	else
	{
		startBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
	}

	// shownet 2/3 will interleave with other printed info, -2 will
	// just print the delta records
	if (cl_shownet && (cl_shownet->integer >= 2 || cl_shownet->integer == -2))
	{
		print = 1;
		Com_Printf("%3i: playerstate ", msg->readcount);
	}
	else
	{
		print = 0;
	}

	numFields = sizeof(playerStateFields) / sizeof(playerStateFields[0]);
	lc = msg->ReadByte();

	for (i = 0, field = playerStateFields; i < lc; i++, field++)
	{
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);

		if (!msg->ReadBits(1))
		{
			// no change
			*toF = *fromF;
		}
		else
		{
			if (field->bits == 0)
			{
				// float
				if (msg->ReadBits(1) == 0)
				{
					// integral float
					trunc = msg->ReadBits(FLOAT_INT_BITS);
					// bias to allow equal parts positive and negative
					trunc -= FLOAT_INT_BIAS;
					*(float*)toF = trunc;
					if (print)
					{
						Com_Printf("%s:%i ", field->name, trunc);
					}
				}
				else
				{
					// full floating point value
					*toF = msg->ReadBits(32);
					if (print)
					{
						Com_Printf("%s:%f ", field->name, *(float*)toF);
					}
				}
			}
			else
			{
				// integer
				*toF = msg->ReadBits(field->bits);
				if (print)
				{
					Com_Printf("%s:%i ", field->name, *toF);
				}
			}
		}
	}
	for (i = lc,field = &playerStateFields[lc]; i < numFields; i++, field++)
	{
		fromF = (int*)((byte*)from + field->offset);
		toF = (int*)((byte*)to + field->offset);
		// no change
		*toF = *fromF;
	}


	// read the arrays
	if (msg->ReadBits(1))		// one general bit tells if any of this infrequently changing stuff has changed
	{	// parse stats
		if (msg->ReadBits(1))
		{
			LOG("PS_STATS");
			bits = msg->ReadShort();
			for (i = 0; i < 16; i++)
			{
				if (bits & (1 << i))
				{
					// RF, changed to long to allow more flexibility
//					to->stats[i] = MSG_ReadLong(msg);
					to->stats[i] = msg->ReadShort();	//----(SA)	back to short since weapon bits are handled elsewhere now

				}
			}
		}

		// parse persistant stats
		if (msg->ReadBits(1))
		{
			LOG("PS_PERSISTANT");
			bits = msg->ReadShort();
			for (i = 0; i < 16; i++)
			{
				if (bits & (1 << i))
				{
					to->persistant[i] = msg->ReadShort();
				}
			}
		}

		// parse holdable stats
		if (msg->ReadBits(1))
		{
			LOG("PS_HOLDABLE");
			bits = msg->ReadShort();
			for (i = 0; i < 16; i++)
			{
				if (bits & (1 << i))
				{
					to->holdable[i] = msg->ReadShort();
				}
			}
		}

		// parse powerups
		if (msg->ReadBits(1))
		{
			LOG("PS_POWERUPS");
			bits = msg->ReadShort();
			for (i = 0; i < 16; i++)
			{
				if (bits & (1 << i))
				{
					to->powerups[i] = msg->ReadLong();
				}
			}
		}
	}

#if 0
// RF, optimization
//		Send a single bit to signify whether or not the ammo/clip info changed.
//		If it did, send individual segments specifying offset values for each item.

	if (msg->ReadBits(1))			// it changed
	{
		while (msg->ReadBits(1))
		{
			i = msg->ReadBits(5);		// read the index number
			// now read the offsets
			to->ammo[i] += MSG_ReadChar(msg);
			to->ammoclip[i] += MSG_ReadChar(msg);
		}
	}

#else
//----(SA)	I split this into two groups using shorts so it wouldn't have
//			to use a long every time ammo changed for any weap.
//			this seemed like a much friendlier option than making it
//			read/write a long for any ammo change.

	// parse ammo

	// j == 0 : weaps 0-15
	// j == 1 : weaps 16-31
	// j == 2 : weaps 32-47	//----(SA)	now up to 64 (but still pretty net-friendly)
	// j == 3 : weaps 48-63

	// ammo stored
	if (msg->ReadBits(1))			// check for any ammo change (0-63)
	{
		for (j = 0; j < 4; j++)
		{
			if (msg->ReadBits(1))
			{
				LOG("PS_AMMO");
				bits = msg->ReadShort();
				for (i = 0; i < 16; i++)
				{
					if (bits & (1 << i))
					{
						to->ammo[i + (j * 16)] = msg->ReadShort();
					}
				}
			}
		}
	}

	// ammo in clip
	for (j = 0; j < 4; j++)
	{
		if (msg->ReadBits(1))
		{
			LOG("PS_AMMOCLIP");
			bits = msg->ReadShort();
			for (i = 0; i < 16; i++)
			{
				if (bits & (1 << i))
				{
					to->ammoclip[i + (j * 16)] = msg->ReadShort();
				}
			}
		}
	}

#endif


	if (print)
	{
		if (msg->bit == 0)
		{
			endBit = msg->readcount * 8 - GENTITYNUM_BITS_Q3;
		}
		else
		{
			endBit = (msg->readcount - 1) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
		}
		Com_Printf(" (%i bits)\n", endBit - startBit);
	}
}
