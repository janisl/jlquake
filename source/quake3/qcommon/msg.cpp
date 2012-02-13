/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "../game/q_shared.h"
#include "qcommon.h"

extern huffman_t		msgHuff;

static qboolean			msgInit = qfalse;

int pcount[256];

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

extern int oldsize;

void MSG_initHuffman();

void MSG_Init( QMsg *buf, byte *data, int length ) {
	if (!msgInit) {
		MSG_initHuffman();
	}
	buf->Init(data, length);
	buf->allowoverflow = true;
}

void MSG_InitOOB( QMsg *buf, byte *data, int length ) {
	if (!msgInit) {
		MSG_initHuffman();
	}
	buf->InitOOB(data, length);
	buf->allowoverflow = true;
}

/*
=============================================================================

bit functions
  
=============================================================================
*/

extern int	overflows;



/*
=============================================================================

delta functions
  
=============================================================================
*/

extern Cvar *cl_shownet;

#define	LOG(x) if( cl_shownet->integer == 4 ) { Com_Printf("%s ", x ); };

void MSG_WriteDelta( QMsg *msg, int oldV, int newV, int bits ) {
	if ( oldV == newV ) {
		msg->WriteBits(0, 1 );
		return;
	}
	msg->WriteBits(1, 1 );
	msg->WriteBits(newV, bits );
}

int	MSG_ReadDelta( QMsg *msg, int oldV, int bits ) {
	if ( msg->ReadBits(1 ) ) {
		return msg->ReadBits(bits );
	}
	return oldV;
}

void MSG_WriteDeltaFloat( QMsg *msg, float oldV, float newV ) {
	if ( oldV == newV ) {
		msg->WriteBits(0, 1 );
		return;
	}
	msg->WriteBits(1, 1 );
	msg->WriteBits(*(int *)&newV, 32 );
}

float MSG_ReadDeltaFloat( QMsg *msg, float oldV ) {
	if ( msg->ReadBits(1 ) ) {
		float	newV;

		*(int *)&newV = msg->ReadBits(32 );
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
	0x0000001F,	0x0000003F,	0x0000007F,	0x000000FF,
	0x000001FF,	0x000003FF,	0x000007FF,	0x00000FFF,
	0x00001FFF,	0x00003FFF,	0x00007FFF,	0x0000FFFF,
	0x0001FFFF,	0x0003FFFF,	0x0007FFFF,	0x000FFFFF,
	0x001FFFFf,	0x003FFFFF,	0x007FFFFF,	0x00FFFFFF,
	0x01FFFFFF,	0x03FFFFFF,	0x07FFFFFF,	0x0FFFFFFF,
	0x1FFFFFFF,	0x3FFFFFFF,	0x7FFFFFFF,	0xFFFFFFFF,
};

void MSG_WriteDeltaKey( QMsg *msg, int key, int oldV, int newV, int bits ) {
	if ( oldV == newV ) {
		msg->WriteBits(0, 1 );
		return;
	}
	msg->WriteBits(1, 1 );
	msg->WriteBits(newV ^ key, bits );
}

int	MSG_ReadDeltaKey( QMsg *msg, int key, int oldV, int bits ) {
	if ( msg->ReadBits(1 ) ) {
		return msg->ReadBits(bits ) ^ (key & kbitmask[bits]);
	}
	return oldV;
}

void MSG_WriteDeltaKeyFloat( QMsg *msg, int key, float oldV, float newV ) {
	if ( oldV == newV ) {
		msg->WriteBits(0, 1 );
		return;
	}
	msg->WriteBits(1, 1 );
	msg->WriteBits((*(int *)&newV) ^ key, 32 );
}

float MSG_ReadDeltaKeyFloat( QMsg *msg, int key, float oldV ) {
	if ( msg->ReadBits(1 ) ) {
		float	newV;

		*(int *)&newV = msg->ReadBits(32 ) ^ key;
		return newV;
	}
	return oldV;
}


/*
============================================================================

q3usercmd_t communication

============================================================================
*/

// ms is allways sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE2 	(1<<1)
#define	CM_ANGLE3 	(1<<2)
#define	CM_FORWARD	(1<<3)
#define	CM_SIDE		(1<<4)
#define	CM_UP		(1<<5)
#define	CM_BUTTONS	(1<<6)
#define CM_WEAPON	(1<<7)

/*
=====================
MSG_WriteDeltaUsercmd
=====================
*/
void MSG_WriteDeltaUsercmd( QMsg *msg, q3usercmd_t *from, q3usercmd_t *to ) {
	if ( to->serverTime - from->serverTime < 256 ) {
		msg->WriteBits(1, 1 );
		msg->WriteBits(to->serverTime - from->serverTime, 8 );
	} else {
		msg->WriteBits(0, 1 );
		msg->WriteBits(to->serverTime, 32 );
	}
	MSG_WriteDelta( msg, from->angles[0], to->angles[0], 16 );
	MSG_WriteDelta( msg, from->angles[1], to->angles[1], 16 );
	MSG_WriteDelta( msg, from->angles[2], to->angles[2], 16 );
	MSG_WriteDelta( msg, from->forwardmove, to->forwardmove, 8 );
	MSG_WriteDelta( msg, from->rightmove, to->rightmove, 8 );
	MSG_WriteDelta( msg, from->upmove, to->upmove, 8 );
	MSG_WriteDelta( msg, from->buttons, to->buttons, 16 );
	MSG_WriteDelta( msg, from->weapon, to->weapon, 8 );
}


/*
=====================
MSG_ReadDeltaUsercmd
=====================
*/
void MSG_ReadDeltaUsercmd( QMsg *msg, q3usercmd_t *from, q3usercmd_t *to ) {
	if ( msg->ReadBits(1 ) ) {
		to->serverTime = from->serverTime + msg->ReadBits(8 );
	} else {
		to->serverTime = msg->ReadBits(32 );
	}
	to->angles[0] = MSG_ReadDelta( msg, from->angles[0], 16);
	to->angles[1] = MSG_ReadDelta( msg, from->angles[1], 16);
	to->angles[2] = MSG_ReadDelta( msg, from->angles[2], 16);
	to->forwardmove = MSG_ReadDelta( msg, from->forwardmove, 8);
	to->rightmove = MSG_ReadDelta( msg, from->rightmove, 8);
	to->upmove = MSG_ReadDelta( msg, from->upmove, 8);
	to->buttons = MSG_ReadDelta( msg, from->buttons, 16);
	to->weapon = MSG_ReadDelta( msg, from->weapon, 8);
}

/*
=====================
MSG_WriteDeltaUsercmd
=====================
*/
void MSG_WriteDeltaUsercmdKey( QMsg *msg, int key, q3usercmd_t *from, q3usercmd_t *to ) {
	if ( to->serverTime - from->serverTime < 256 ) {
		msg->WriteBits(1, 1 );
		msg->WriteBits(to->serverTime - from->serverTime, 8 );
	} else {
		msg->WriteBits(0, 1 );
		msg->WriteBits(to->serverTime, 32 );
	}
	if (from->angles[0] == to->angles[0] &&
		from->angles[1] == to->angles[1] &&
		from->angles[2] == to->angles[2] &&
		from->forwardmove == to->forwardmove &&
		from->rightmove == to->rightmove &&
		from->upmove == to->upmove &&
		from->buttons == to->buttons &&
		from->weapon == to->weapon) {
			msg->WriteBits(0, 1 );				// no change
			oldsize += 7;
			return;
	}
	key ^= to->serverTime;
	msg->WriteBits(1, 1 );
	MSG_WriteDeltaKey( msg, key, from->angles[0], to->angles[0], 16 );
	MSG_WriteDeltaKey( msg, key, from->angles[1], to->angles[1], 16 );
	MSG_WriteDeltaKey( msg, key, from->angles[2], to->angles[2], 16 );
	MSG_WriteDeltaKey( msg, key, from->forwardmove, to->forwardmove, 8 );
	MSG_WriteDeltaKey( msg, key, from->rightmove, to->rightmove, 8 );
	MSG_WriteDeltaKey( msg, key, from->upmove, to->upmove, 8 );
	MSG_WriteDeltaKey( msg, key, from->buttons, to->buttons, 16 );
	MSG_WriteDeltaKey( msg, key, from->weapon, to->weapon, 8 );
}


/*
=====================
MSG_ReadDeltaUsercmd
=====================
*/
void MSG_ReadDeltaUsercmdKey( QMsg *msg, int key, q3usercmd_t *from, q3usercmd_t *to ) {
	if ( msg->ReadBits(1 ) ) {
		to->serverTime = from->serverTime + msg->ReadBits(8 );
	} else {
		to->serverTime = msg->ReadBits(32 );
	}
	if ( msg->ReadBits(1 ) ) {
		key ^= to->serverTime;
		to->angles[0] = MSG_ReadDeltaKey( msg, key, from->angles[0], 16);
		to->angles[1] = MSG_ReadDeltaKey( msg, key, from->angles[1], 16);
		to->angles[2] = MSG_ReadDeltaKey( msg, key, from->angles[2], 16);
		to->forwardmove = MSG_ReadDeltaKey( msg, key, from->forwardmove, 8);
		to->rightmove = MSG_ReadDeltaKey( msg, key, from->rightmove, 8);
		to->upmove = MSG_ReadDeltaKey( msg, key, from->upmove, 8);
		to->buttons = MSG_ReadDeltaKey( msg, key, from->buttons, 16);
		to->weapon = MSG_ReadDeltaKey( msg, key, from->weapon, 8);
	} else {
		to->angles[0] = from->angles[0];
		to->angles[1] = from->angles[1];
		to->angles[2] = from->angles[2];
		to->forwardmove = from->forwardmove;
		to->rightmove = from->rightmove;
		to->upmove = from->upmove;
		to->buttons = from->buttons;
		to->weapon = from->weapon;
	}
}

/*
=============================================================================

q3entityState_t communication
  
=============================================================================
*/

/*
=================
MSG_ReportChangeVectors_f

Prints out a table from the current statistics for copying to code
=================
*/
void MSG_ReportChangeVectors_f( void ) {
	int i;
	for(i=0;i<256;i++) {
		if (pcount[i]) {
			Com_Printf("%d used %d\n", i, pcount[i]);
		}
	}
}

typedef struct {
	const char	*name;
	int		offset;
	int		bits;		// 0 = float
} netField_t;

// using the stringizing operator to save typing...
#define	NETF(x) #x,(qintptr)&((q3entityState_t*)0)->x

netField_t	entityStateFields[] = 
{
{ NETF(pos.trTime), 32 },
{ NETF(pos.trBase[0]), 0 },
{ NETF(pos.trBase[1]), 0 },
{ NETF(pos.trDelta[0]), 0 },
{ NETF(pos.trDelta[1]), 0 },
{ NETF(pos.trBase[2]), 0 },
{ NETF(apos.trBase[1]), 0 },
{ NETF(pos.trDelta[2]), 0 },
{ NETF(apos.trBase[0]), 0 },
{ NETF(event), 10 },
{ NETF(angles2[1]), 0 },
{ NETF(eType), 8 },
{ NETF(torsoAnim), 8 },
{ NETF(eventParm), 8 },
{ NETF(legsAnim), 8 },
{ NETF(groundEntityNum), GENTITYNUM_BITS_Q3 },
{ NETF(pos.trType), 8 },
{ NETF(eFlags), 19 },
{ NETF(otherEntityNum), GENTITYNUM_BITS_Q3 },
{ NETF(weapon), 8 },
{ NETF(clientNum), 8 },
{ NETF(angles[1]), 0 },
{ NETF(pos.trDuration), 32 },
{ NETF(apos.trType), 8 },
{ NETF(origin[0]), 0 },
{ NETF(origin[1]), 0 },
{ NETF(origin[2]), 0 },
{ NETF(solid), 24 },
{ NETF(powerups), 16 },
{ NETF(modelindex), 8 },
{ NETF(otherEntityNum2), GENTITYNUM_BITS_Q3 },
{ NETF(loopSound), 8 },
{ NETF(generic1), 8 },
{ NETF(origin2[2]), 0 },
{ NETF(origin2[0]), 0 },
{ NETF(origin2[1]), 0 },
{ NETF(modelindex2), 8 },
{ NETF(angles[0]), 0 },
{ NETF(time), 32 },
{ NETF(apos.trTime), 32 },
{ NETF(apos.trDuration), 32 },
{ NETF(apos.trBase[2]), 0 },
{ NETF(apos.trDelta[0]), 0 },
{ NETF(apos.trDelta[1]), 0 },
{ NETF(apos.trDelta[2]), 0 },
{ NETF(time2), 32 },
{ NETF(angles[2]), 0 },
{ NETF(angles2[0]), 0 },
{ NETF(angles2[2]), 0 },
{ NETF(constantLight), 32 },
{ NETF(frame), 16 }
};


// if (int)f == f and (int)f + ( 1<<(FLOAT_INT_BITS-1) ) < ( 1 << FLOAT_INT_BITS )
// the float will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define	FLOAT_INT_BITS	13
#define	FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))

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
void MSG_WriteDeltaEntity( QMsg *msg, q3entityState_t* from, q3entityState_t* to, 
						   qboolean force ) {
	int			i, lc;
	int			numFields;
	netField_t	*field;
	int			trunc;
	float		fullFloat;
	int			*fromF, *toF;

	numFields = sizeof(entityStateFields)/sizeof(entityStateFields[0]);

	// all fields should be 32 bits to avoid any compiler packing issues
	// the "number" field is not part of the field list
	// if this assert fails, someone added a field to the q3entityState_t
	// struct without updating the message fields
	assert( numFields + 1 == sizeof( *from )/4 );

	// a NULL to is a delta remove message
	if ( to == NULL ) {
		if ( from == NULL ) {
			return;
		}
		msg->WriteBits(from->number, GENTITYNUM_BITS_Q3 );
		msg->WriteBits(1, 1 );
		return;
	}

	if ( to->number < 0 || to->number >= MAX_GENTITIES_Q3 ) {
		Com_Error (ERR_FATAL, "MSG_WriteDeltaEntity: Bad entity number: %i", to->number );
	}

	lc = 0;
	// build the change vector as bytes so it is endien independent
	for ( i = 0, field = entityStateFields ; i < numFields ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		if ( *fromF != *toF ) {
			lc = i+1;
		}
	}

	if ( lc == 0 ) {
		// nothing at all changed
		if ( !force ) {
			return;		// nothing at all
		}
		// write two bits for no change
		msg->WriteBits(to->number, GENTITYNUM_BITS_Q3 );
		msg->WriteBits(0, 1 );		// not removed
		msg->WriteBits(0, 1 );		// no delta
		return;
	}

	msg->WriteBits(to->number, GENTITYNUM_BITS_Q3 );
	msg->WriteBits(0, 1 );			// not removed
	msg->WriteBits(1, 1 );			// we have a delta

	msg->WriteByte(lc );	// # of changes

	oldsize += numFields;

	for ( i = 0, field = entityStateFields ; i < lc ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( *fromF == *toF ) {
			msg->WriteBits(0, 1 );	// no change
			continue;
		}

		msg->WriteBits(1, 1 );	// changed

		if ( field->bits == 0 ) {
			// float
			fullFloat = *(float *)toF;
			trunc = (int)fullFloat;

			if (fullFloat == 0.0f) {
					msg->WriteBits(0, 1 );
					oldsize += FLOAT_INT_BITS;
			} else {
				msg->WriteBits(1, 1 );
				if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 && 
					trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) ) {
					// send as small integer
					msg->WriteBits(0, 1 );
					msg->WriteBits(trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
				} else {
					// send as full floating point value
					msg->WriteBits(1, 1 );
					msg->WriteBits(*toF, 32 );
				}
			}
		} else {
			if (*toF == 0) {
				msg->WriteBits(0, 1 );
			} else {
				msg->WriteBits(1, 1 );
				// integer
				msg->WriteBits(*toF, field->bits );
			}
		}
	}
}

/*
==================
MSG_ReadDeltaEntity

The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, q3entityState_t->number will be set to MAX_GENTITIES_Q3-1

Can go from either a baseline or a previous packet_entity
==================
*/
extern	Cvar	*cl_shownet;

void MSG_ReadDeltaEntity( QMsg *msg, q3entityState_t *from, q3entityState_t *to, 
						 int number) {
	int			i, lc;
	int			numFields;
	netField_t	*field;
	int			*fromF, *toF;
	int			print;
	int			trunc;
	int			startBit, endBit;

	if ( number < 0 || number >= MAX_GENTITIES_Q3) {
		Com_Error( ERR_DROP, "Bad delta entity number: %i", number );
	}

	if ( msg->bit == 0 ) {
		startBit = msg->readcount * 8 - GENTITYNUM_BITS_Q3;
	} else {
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
	}

	// check for a remove
	if ( msg->ReadBits(1 ) == 1 ) {
		Com_Memset( to, 0, sizeof( *to ) );	
		to->number = MAX_GENTITIES_Q3 - 1;
		if ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) {
			Com_Printf( "%3i: #%-3i remove\n", msg->readcount, number );
		}
		return;
	}

	// check for no delta
	if ( msg->ReadBits(1 ) == 0 ) {
		*to = *from;
		to->number = number;
		return;
	}

	numFields = sizeof(entityStateFields)/sizeof(entityStateFields[0]);
	lc = msg->ReadByte();

	// shownet 2/3 will interleave with other printed info, -1 will
	// just print the delta records`
	if ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) {
		print = 1;
		Com_Printf( "%3i: #%-3i ", msg->readcount, to->number );
	} else {
		print = 0;
	}

	to->number = number;

	for ( i = 0, field = entityStateFields ; i < lc ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( ! msg->ReadBits(1 ) ) {
			// no change
			*toF = *fromF;
		} else {
			if ( field->bits == 0 ) {
				// float
				if ( msg->ReadBits(1 ) == 0 ) {
						*(float *)toF = 0.0f; 
				} else {
					if ( msg->ReadBits(1 ) == 0 ) {
						// integral float
						trunc = msg->ReadBits(FLOAT_INT_BITS );
						// bias to allow equal parts positive and negative
						trunc -= FLOAT_INT_BIAS;
						*(float *)toF = trunc; 
						if ( print ) {
							Com_Printf( "%s:%i ", field->name, trunc );
						}
					} else {
						// full floating point value
						*toF = msg->ReadBits(32 );
						if ( print ) {
							Com_Printf( "%s:%f ", field->name, *(float *)toF );
						}
					}
				}
			} else {
				if ( msg->ReadBits(1 ) == 0 ) {
					*toF = 0;
				} else {
					// integer
					*toF = msg->ReadBits(field->bits );
					if ( print ) {
						Com_Printf( "%s:%i ", field->name, *toF );
					}
				}
			}
//			pcount[i]++;
		}
	}
	for ( i = lc, field = &entityStateFields[lc] ; i < numFields ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		// no change
		*toF = *fromF;
	}

	if ( print ) {
		if ( msg->bit == 0 ) {
			endBit = msg->readcount * 8 - GENTITYNUM_BITS_Q3;
		} else {
			endBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
		}
		Com_Printf( " (%i bits)\n", endBit - startBit  );
	}
}


/*
============================================================================

plyer_state_t communication

============================================================================
*/

// using the stringizing operator to save typing...
#define	PSF(x) #x,(qintptr)&((q3playerState_t*)0)->x

netField_t	playerStateFields[] = 
{
{ PSF(commandTime), 32 },				
{ PSF(origin[0]), 0 },
{ PSF(origin[1]), 0 },
{ PSF(bobCycle), 8 },
{ PSF(velocity[0]), 0 },
{ PSF(velocity[1]), 0 },
{ PSF(viewangles[1]), 0 },
{ PSF(viewangles[0]), 0 },
{ PSF(weaponTime), -16 },
{ PSF(origin[2]), 0 },
{ PSF(velocity[2]), 0 },
{ PSF(legsTimer), 8 },
{ PSF(pm_time), -16 },
{ PSF(eventSequence), 16 },
{ PSF(torsoAnim), 8 },
{ PSF(movementDir), 4 },
{ PSF(events[0]), 8 },
{ PSF(legsAnim), 8 },
{ PSF(events[1]), 8 },
{ PSF(pm_flags), 16 },
{ PSF(groundEntityNum), GENTITYNUM_BITS_Q3 },
{ PSF(weaponstate), 4 },
{ PSF(eFlags), 16 },
{ PSF(externalEvent), 10 },
{ PSF(gravity), 16 },
{ PSF(speed), 16 },
{ PSF(delta_angles[1]), 16 },
{ PSF(externalEventParm), 8 },
{ PSF(viewheight), -8 },
{ PSF(damageEvent), 8 },
{ PSF(damageYaw), 8 },
{ PSF(damagePitch), 8 },
{ PSF(damageCount), 8 },
{ PSF(generic1), 8 },
{ PSF(pm_type), 8 },					
{ PSF(delta_angles[0]), 16 },
{ PSF(delta_angles[2]), 16 },
{ PSF(torsoTimer), 12 },
{ PSF(eventParms[0]), 8 },
{ PSF(eventParms[1]), 8 },
{ PSF(clientNum), 8 },
{ PSF(weapon), 5 },
{ PSF(viewangles[2]), 0 },
{ PSF(grapplePoint[0]), 0 },
{ PSF(grapplePoint[1]), 0 },
{ PSF(grapplePoint[2]), 0 },
{ PSF(jumppad_ent), 10 },
{ PSF(loopSound), 16 }
};

/*
=============
MSG_WriteDeltaPlayerstate

=============
*/
void MSG_WriteDeltaPlayerstate( QMsg *msg, q3playerState_t* from, q3playerState_t* to ) {
	int				i;
	q3playerState_t	dummy;
	int				statsbits;
	int				persistantbits;
	int				ammobits;
	int				powerupbits;
	int				numFields;
	int				c;
	netField_t		*field;
	int				*fromF, *toF;
	float			fullFloat;
	int				trunc, lc;

	if (!from) {
		from = &dummy;
		Com_Memset (&dummy, 0, sizeof(dummy));
	}

	c = msg->cursize;

	numFields = sizeof( playerStateFields ) / sizeof( playerStateFields[0] );

	lc = 0;
	for ( i = 0, field = playerStateFields ; i < numFields ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		if ( *fromF != *toF ) {
			lc = i+1;
		}
	}

	msg->WriteByte(lc );	// # of changes

	oldsize += numFields - lc;

	for ( i = 0, field = playerStateFields ; i < lc ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( *fromF == *toF ) {
			msg->WriteBits(0, 1 );	// no change
			continue;
		}

		msg->WriteBits(1, 1 );	// changed
//		pcount[i]++;

		if ( field->bits == 0 ) {
			// float
			fullFloat = *(float *)toF;
			trunc = (int)fullFloat;

			if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 && 
				trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) ) {
				// send as small integer
				msg->WriteBits(0, 1 );
				msg->WriteBits(trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
			} else {
				// send as full floating point value
				msg->WriteBits(1, 1 );
				msg->WriteBits(*toF, 32 );
			}
		} else {
			// integer
			msg->WriteBits(*toF, field->bits );
		}
	}
	c = msg->cursize - c;


	//
	// send the arrays
	//
	statsbits = 0;
	for (i=0 ; i<16 ; i++) {
		if (to->stats[i] != from->stats[i]) {
			statsbits |= 1<<i;
		}
	}
	persistantbits = 0;
	for (i=0 ; i<16 ; i++) {
		if (to->persistant[i] != from->persistant[i]) {
			persistantbits |= 1<<i;
		}
	}
	ammobits = 0;
	for (i=0 ; i<16 ; i++) {
		if (to->ammo[i] != from->ammo[i]) {
			ammobits |= 1<<i;
		}
	}
	powerupbits = 0;
	for (i=0 ; i<16 ; i++) {
		if (to->powerups[i] != from->powerups[i]) {
			powerupbits |= 1<<i;
		}
	}

	if (!statsbits && !persistantbits && !ammobits && !powerupbits) {
		msg->WriteBits(0, 1 );	// no change
		oldsize += 4;
		return;
	}
	msg->WriteBits(1, 1 );	// changed

	if ( statsbits ) {
		msg->WriteBits(1, 1 );	// changed
		msg->WriteShort(statsbits );
		for (i=0 ; i<16 ; i++)
			if (statsbits & (1<<i) )
				msg->WriteShort(to->stats[i]);
	} else {
		msg->WriteBits(0, 1 );	// no change
	}


	if ( persistantbits ) {
		msg->WriteBits(1, 1 );	// changed
		msg->WriteShort(persistantbits );
		for (i=0 ; i<16 ; i++)
			if (persistantbits & (1<<i) )
				msg->WriteShort(to->persistant[i]);
	} else {
		msg->WriteBits(0, 1 );	// no change
	}


	if ( ammobits ) {
		msg->WriteBits(1, 1 );	// changed
		msg->WriteShort(ammobits );
		for (i=0 ; i<16 ; i++)
			if (ammobits & (1<<i) )
				msg->WriteShort(to->ammo[i]);
	} else {
		msg->WriteBits(0, 1 );	// no change
	}


	if ( powerupbits ) {
		msg->WriteBits(1, 1 );	// changed
		msg->WriteShort(powerupbits );
		for (i=0 ; i<16 ; i++)
			if (powerupbits & (1<<i) )
				msg->WriteLong(to->powerups[i] );
	} else {
		msg->WriteBits(0, 1 );	// no change
	}
}


/*
===================
MSG_ReadDeltaPlayerstate
===================
*/
void MSG_ReadDeltaPlayerstate (QMsg *msg, q3playerState_t *from, q3playerState_t *to ) {
	int			i, lc;
	int			bits;
	netField_t	*field;
	int			numFields;
	int			startBit, endBit;
	int			print;
	int			*fromF, *toF;
	int			trunc;
	q3playerState_t	dummy;

	if ( !from ) {
		from = &dummy;
		Com_Memset( &dummy, 0, sizeof( dummy ) );
	}
	*to = *from;

	if ( msg->bit == 0 ) {
		startBit = msg->readcount * 8 - GENTITYNUM_BITS_Q3;
	} else {
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
	}

	// shownet 2/3 will interleave with other printed info, -2 will
	// just print the delta records
	if ( cl_shownet->integer >= 2 || cl_shownet->integer == -2 ) {
		print = 1;
		Com_Printf( "%3i: playerstate ", msg->readcount );
	} else {
		print = 0;
	}

	numFields = sizeof( playerStateFields ) / sizeof( playerStateFields[0] );
	lc = msg->ReadByte();

	for ( i = 0, field = playerStateFields ; i < lc ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( ! msg->ReadBits(1 ) ) {
			// no change
			*toF = *fromF;
		} else {
			if ( field->bits == 0 ) {
				// float
				if ( msg->ReadBits(1 ) == 0 ) {
					// integral float
					trunc = msg->ReadBits(FLOAT_INT_BITS );
					// bias to allow equal parts positive and negative
					trunc -= FLOAT_INT_BIAS;
					*(float *)toF = trunc; 
					if ( print ) {
						Com_Printf( "%s:%i ", field->name, trunc );
					}
				} else {
					// full floating point value
					*toF = msg->ReadBits(32 );
					if ( print ) {
						Com_Printf( "%s:%f ", field->name, *(float *)toF );
					}
				}
			} else {
				// integer
				*toF = msg->ReadBits(field->bits );
				if ( print ) {
					Com_Printf( "%s:%i ", field->name, *toF );
				}
			}
		}
	}
	for ( i=lc,field = &playerStateFields[lc];i<numFields; i++, field++) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		// no change
		*toF = *fromF;
	}


	// read the arrays
	if (msg->ReadBits(1 ) ) {
		// parse stats
		if ( msg->ReadBits(1 ) ) {
			LOG("PS_STATS");
			bits = msg->ReadShort();
			for (i=0 ; i<16 ; i++) {
				if (bits & (1<<i) ) {
					to->stats[i] = msg->ReadShort();
				}
			}
		}

		// parse persistant stats
		if ( msg->ReadBits(1 ) ) {
			LOG("PS_PERSISTANT");
			bits = msg->ReadShort();
			for (i=0 ; i<16 ; i++) {
				if (bits & (1<<i) ) {
					to->persistant[i] = msg->ReadShort();
				}
			}
		}

		// parse ammo
		if ( msg->ReadBits(1 ) ) {
			LOG("PS_AMMO");
			bits = msg->ReadShort();
			for (i=0 ; i<16 ; i++) {
				if (bits & (1<<i) ) {
					to->ammo[i] = msg->ReadShort();
				}
			}
		}

		// parse powerups
		if ( msg->ReadBits(1 ) ) {
			LOG("PS_POWERUPS");
			bits = msg->ReadShort();
			for (i=0 ; i<16 ; i++) {
				if (bits & (1<<i) ) {
					to->powerups[i] = msg->ReadLong();
				}
			}
		}
	}

	if ( print ) {
		if ( msg->bit == 0 ) {
			endBit = msg->readcount * 8 - GENTITYNUM_BITS_Q3;
		} else {
			endBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS_Q3;
		}
		Com_Printf( " (%i bits)\n", endBit - startBit  );
	}
}

int msg_hData[256] = {
250315,			// 0
41193,			// 1
6292,			// 2
7106,			// 3
3730,			// 4
3750,			// 5
6110,			// 6
23283,			// 7
33317,			// 8
6950,			// 9
7838,			// 10
9714,			// 11
9257,			// 12
17259,			// 13
3949,			// 14
1778,			// 15
8288,			// 16
1604,			// 17
1590,			// 18
1663,			// 19
1100,			// 20
1213,			// 21
1238,			// 22
1134,			// 23
1749,			// 24
1059,			// 25
1246,			// 26
1149,			// 27
1273,			// 28
4486,			// 29
2805,			// 30
3472,			// 31
21819,			// 32
1159,			// 33
1670,			// 34
1066,			// 35
1043,			// 36
1012,			// 37
1053,			// 38
1070,			// 39
1726,			// 40
888,			// 41
1180,			// 42
850,			// 43
960,			// 44
780,			// 45
1752,			// 46
3296,			// 47
10630,			// 48
4514,			// 49
5881,			// 50
2685,			// 51
4650,			// 52
3837,			// 53
2093,			// 54
1867,			// 55
2584,			// 56
1949,			// 57
1972,			// 58
940,			// 59
1134,			// 60
1788,			// 61
1670,			// 62
1206,			// 63
5719,			// 64
6128,			// 65
7222,			// 66
6654,			// 67
3710,			// 68
3795,			// 69
1492,			// 70
1524,			// 71
2215,			// 72
1140,			// 73
1355,			// 74
971,			// 75
2180,			// 76
1248,			// 77
1328,			// 78
1195,			// 79
1770,			// 80
1078,			// 81
1264,			// 82
1266,			// 83
1168,			// 84
965,			// 85
1155,			// 86
1186,			// 87
1347,			// 88
1228,			// 89
1529,			// 90
1600,			// 91
2617,			// 92
2048,			// 93
2546,			// 94
3275,			// 95
2410,			// 96
3585,			// 97
2504,			// 98
2800,			// 99
2675,			// 100
6146,			// 101
3663,			// 102
2840,			// 103
14253,			// 104
3164,			// 105
2221,			// 106
1687,			// 107
3208,			// 108
2739,			// 109
3512,			// 110
4796,			// 111
4091,			// 112
3515,			// 113
5288,			// 114
4016,			// 115
7937,			// 116
6031,			// 117
5360,			// 118
3924,			// 119
4892,			// 120
3743,			// 121
4566,			// 122
4807,			// 123
5852,			// 124
6400,			// 125
6225,			// 126
8291,			// 127
23243,			// 128
7838,			// 129
7073,			// 130
8935,			// 131
5437,			// 132
4483,			// 133
3641,			// 134
5256,			// 135
5312,			// 136
5328,			// 137
5370,			// 138
3492,			// 139
2458,			// 140
1694,			// 141
1821,			// 142
2121,			// 143
1916,			// 144
1149,			// 145
1516,			// 146
1367,			// 147
1236,			// 148
1029,			// 149
1258,			// 150
1104,			// 151
1245,			// 152
1006,			// 153
1149,			// 154
1025,			// 155
1241,			// 156
952,			// 157
1287,			// 158
997,			// 159
1713,			// 160
1009,			// 161
1187,			// 162
879,			// 163
1099,			// 164
929,			// 165
1078,			// 166
951,			// 167
1656,			// 168
930,			// 169
1153,			// 170
1030,			// 171
1262,			// 172
1062,			// 173
1214,			// 174
1060,			// 175
1621,			// 176
930,			// 177
1106,			// 178
912,			// 179
1034,			// 180
892,			// 181
1158,			// 182
990,			// 183
1175,			// 184
850,			// 185
1121,			// 186
903,			// 187
1087,			// 188
920,			// 189
1144,			// 190
1056,			// 191
3462,			// 192
2240,			// 193
4397,			// 194
12136,			// 195
7758,			// 196
1345,			// 197
1307,			// 198
3278,			// 199
1950,			// 200
886,			// 201
1023,			// 202
1112,			// 203
1077,			// 204
1042,			// 205
1061,			// 206
1071,			// 207
1484,			// 208
1001,			// 209
1096,			// 210
915,			// 211
1052,			// 212
995,			// 213
1070,			// 214
876,			// 215
1111,			// 216
851,			// 217
1059,			// 218
805,			// 219
1112,			// 220
923,			// 221
1103,			// 222
817,			// 223
1899,			// 224
1872,			// 225
976,			// 226
841,			// 227
1127,			// 228
956,			// 229
1159,			// 230
950,			// 231
7791,			// 232
954,			// 233
1289,			// 234
933,			// 235
1127,			// 236
3207,			// 237
1020,			// 238
927,			// 239
1355,			// 240
768,			// 241
1040,			// 242
745,			// 243
952,			// 244
805,			// 245
1073,			// 246
740,			// 247
1013,			// 248
805,			// 249
1008,			// 250
796,			// 251
996,			// 252
1057,			// 253
11457,			// 254
13504,			// 255
};

void MSG_initHuffman() {
	int i,j;

	msgInit = qtrue;
	Huff_Init(&msgHuff);
	for(i=0;i<256;i++) {
		for (j=0;j<msg_hData[i];j++) {
			Huff_addRef(&msgHuff.compressor,	(byte)i);			// Do update
			Huff_addRef(&msgHuff.decompressor,	(byte)i);			// Do update
		}
	}
}

/*
void MSG_NUinitHuffman() {
	byte	*data;
	int		size, i, ch;
	int		array[256];

	msgInit = qtrue;

	Huff_Init(&msgHuff);
	// load it in
	size = FS_ReadFile( "netchan/netchan.bin", (void **)&data );

	for(i=0;i<256;i++) {
		array[i] = 0;
	}
	for(i=0;i<size;i++) {
		ch = data[i];
		Huff_addRef(&msgHuff.compressor,	ch);			// Do update
		Huff_addRef(&msgHuff.decompressor,	ch);			// Do update
		array[ch]++;
	}
	Com_Printf("msg_hData {\n");
	for(i=0;i<256;i++) {
		if (array[i] == 0) {
			Huff_addRef(&msgHuff.compressor,	i);			// Do update
			Huff_addRef(&msgHuff.decompressor,	i);			// Do update
		}
		Com_Printf("%d,			// %d\n", array[i], i);
	}
	Com_Printf("};\n");
	FS_FreeFile( data );
	Cbuf_AddText( "condump dump.txt\n" );
}
*/

//===========================================================================
