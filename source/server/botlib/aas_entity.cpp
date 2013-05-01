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

#include "../../common/qcommon.h"
#include "../../common/common_defs.h"
#include "../quake3serverdefs.h"
#include "local.h"

// Ridah, always use the default world for entities
static aas_t* defaultaasworld = aasworlds;

void AAS_EntityInfo( int entnum, aas_entityinfo_t* info ) {
	if ( !defaultaasworld->initialized ) {
		BotImport_Print( PRT_FATAL, "AAS_EntityInfo: (*defaultaasworld) not initialized\n" );
		Com_Memset( info, 0, sizeof ( aas_entityinfo_t ) );
		return;
	}

	if ( entnum < 0 || entnum >= defaultaasworld->maxentities ) {
		// if it's not a bot game entity, then report it
		if ( !( GGameType & GAME_ET ) || !( entnum >= defaultaasworld->maxentities && entnum < defaultaasworld->maxentities + NUM_BOTGAMEENTITIES ) ) {
			BotImport_Print( PRT_FATAL, "AAS_EntityInfo: entnum %d out of range\n", entnum );
		}
		Com_Memset( info, 0, sizeof ( aas_entityinfo_t ) );
		return;
	}

	Com_Memcpy( info, &defaultaasworld->entities[ entnum ].i, sizeof ( aas_entityinfo_t ) );
}

void AAS_EntityOrigin( int entnum, vec3_t origin ) {
	if ( entnum < 0 || entnum >= defaultaasworld->maxentities ) {
		BotImport_Print( PRT_FATAL, "AAS_EntityOrigin: entnum %d out of range\n", entnum );
		VectorClear( origin );
		return;
	}

	VectorCopy( defaultaasworld->entities[ entnum ].i.origin, origin );
}

int AAS_EntityModelindex( int entnum ) {
	if ( entnum < 0 || entnum >= defaultaasworld->maxentities ) {
		BotImport_Print( PRT_FATAL, "AAS_EntityModelindex: entnum %d out of range\n", entnum );
		return 0;
	}
	return defaultaasworld->entities[ entnum ].i.modelindex;
}

int AAS_EntityType( int entnum ) {
	if ( !defaultaasworld->initialized ) {
		return 0;
	}

	if ( entnum < 0 || entnum >= defaultaasworld->maxentities ) {
		BotImport_Print( PRT_FATAL, "AAS_EntityType: entnum %d out of range\n", entnum );
		return 0;
	}
	return defaultaasworld->entities[ entnum ].i.type;
}

int AAS_EntityModelNum( int entnum ) {
	if ( !defaultaasworld->initialized ) {
		return 0;
	}

	if ( entnum < 0 || entnum >= defaultaasworld->maxentities ) {
		BotImport_Print( PRT_FATAL, "AAS_EntityModelNum: entnum %d out of range\n", entnum );
		return 0;
	}
	return defaultaasworld->entities[ entnum ].i.modelindex;
}

bool AAS_OriginOfMoverWithModelNum( int modelnum, vec3_t origin ) {
	for ( int i = 0; i < defaultaasworld->maxentities; i++ ) {
		aas_entity_t* ent = &defaultaasworld->entities[ i ];
		if ( ent->i.type == Q3ET_MOVER ) {
			if ( ent->i.modelindex == modelnum ) {
				VectorCopy( ent->i.origin, origin );
				return true;
			}
		}
	}
	return false;
}

void AAS_ResetEntityLinks() {
	for ( int i = 0; i < defaultaasworld->maxentities; i++ ) {
		defaultaasworld->entities[ i ].areas = NULL;
		defaultaasworld->entities[ i ].leaves = NULL;
	}
}

void AAS_InvalidateEntities() {
	for ( int i = 0; i < defaultaasworld->maxentities; i++ ) {
		defaultaasworld->entities[ i ].i.valid = false;
		defaultaasworld->entities[ i ].i.number = i;
	}
}

int AAS_NextEntity( int entnum ) {
	if ( !defaultaasworld->loaded ) {
		return 0;
	}

	if ( entnum < 0 ) {
		entnum = -1;
	}
	while ( ++entnum < defaultaasworld->maxentities ) {
		if ( defaultaasworld->entities[ entnum ].i.valid ) {
			return entnum;
		}
	}
	return 0;
}

// Ridah, used to find out if there is an entity touching the given area, if so, try and avoid it
bool AAS_IsEntityInArea( int entnumIgnore, int entnumIgnore2, int areanum ) {
	for ( aas_link_t* link = aasworld->arealinkedentities[ areanum ]; link; link = link->next_ent ) {
		//ignore the pass entity
		if ( link->entnum == entnumIgnore ) {
			continue;
		}
		if ( link->entnum == entnumIgnore2 ) {
			continue;
		}

		aas_entity_t* ent = &defaultaasworld->entities[ link->entnum ];
		if ( !ent->i.valid ) {
			continue;
		}
		if ( !ent->i.solid ) {
			continue;
		}
		return true;
	}
	return false;
}

int AAS_UpdateEntity( int entnum, const bot_entitystate_t* state ) {
	if ( !defaultaasworld->loaded ) {
		BotImport_Print( PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n" );
		return GGameType & GAME_Quake3 ? Q3BLERR_NOAASFILE : WOLFBLERR_NOAASFILE;
	}

	aas_entity_t* ent = &defaultaasworld->entities[ entnum ];

	if ( !state ) {
		//unlink the entity
		AAS_UnlinkFromAreas( ent->areas );
		ent->areas = NULL;
		ent->leaves = NULL;
		return BLERR_NOERROR;
	}

	ent->i.update_time = AAS_Time() - ent->i.ltime;
	ent->i.type = state->type;
	ent->i.flags = state->flags;
	ent->i.ltime = AAS_Time();
	VectorCopy( ent->i.origin, ent->i.lastvisorigin );
	VectorCopy( state->old_origin, ent->i.old_origin );
	ent->i.solid = state->solid;
	ent->i.groundent = state->groundent;
	ent->i.modelindex = state->modelindex;
	ent->i.modelindex2 = state->modelindex2;
	ent->i.frame = state->frame;
	if ( GGameType & GAME_Quake3 ) {
		ent->i.event = state->event;
	}
	ent->i.eventParm = state->eventParm;
	ent->i.powerups = state->powerups;
	ent->i.weapon = state->weapon;
	ent->i.legsAnim = state->legsAnim;
	ent->i.torsoAnim = state->torsoAnim;
	//number of the entity
	ent->i.number = entnum;
	//updated so set valid flag
	ent->i.valid = true;
	//link everything the first frame
	bool relink = defaultaasworld->numframes == 1;

	if ( ent->i.solid == Q3SOLID_BSP ) {
		//if the angles of the model changed
		if ( !VectorCompare( state->angles, ent->i.angles ) ) {
			VectorCopy( state->angles, ent->i.angles );
			relink = true;
		}
		//get the mins and maxs of the model
		//FIXME: rotate mins and maxs
		if ( GGameType & GAME_ET ) {
			// RF, this is broken, just use the state bounds
			VectorCopy( state->mins, ent->i.mins );
			VectorCopy( state->maxs, ent->i.maxs );
		} else {
			AAS_BSPModelMinsMaxs( ent->i.modelindex, ent->i.angles, ent->i.mins, ent->i.maxs );
		}
	} else if ( ent->i.solid == Q3SOLID_BBOX ) {
		//if the bounding box size changed
		if ( !VectorCompare( state->mins, ent->i.mins ) ||
			 !VectorCompare( state->maxs, ent->i.maxs ) ) {
			VectorCopy( state->mins, ent->i.mins );
			VectorCopy( state->maxs, ent->i.maxs );
			relink = true;
		}
		if ( GGameType & GAME_Quake3 ) {
			VectorCopy( state->angles, ent->i.angles );
		}
	}
	//if the origin changed
	if ( !VectorCompare( state->origin, ent->i.origin ) ) {
		VectorCopy( state->origin, ent->i.origin );
		relink = true;
	}
	//if the entity should be relinked
	if ( relink ) {
		//don't link the world model
		if ( entnum != Q3ENTITYNUM_WORLD ) {
			//absolute mins and maxs
			vec3_t absmins, absmaxs;
			VectorAdd( ent->i.mins, ent->i.origin, absmins );
			VectorAdd( ent->i.maxs, ent->i.origin, absmaxs );
			//unlink the entity
			AAS_UnlinkFromAreas( ent->areas );
			//relink the entity to the AAS areas (use the larges bbox)
			ent->areas = AAS_LinkEntityClientBBox( absmins, absmaxs, entnum, PRESENCE_NORMAL );
			//link the entity to the world BSP tree
			ent->leaves = NULL;
		}
	}
	return BLERR_NOERROR;
}

void AAS_UnlinkInvalidEntities() {
	for ( int i = 0; i < defaultaasworld->maxentities; i++ ) {
		aas_entity_t* ent = &defaultaasworld->entities[ i ];
		if ( !ent->i.valid ) {
			AAS_UnlinkFromAreas( ent->areas );
			ent->areas = NULL;
			ent->leaves = NULL;
		}
	}
}

int AAS_BestReachableEntityArea( int entnum ) {
	aas_entity_t* ent = &defaultaasworld->entities[ entnum ];
	return AAS_BestReachableLinkArea( ent->areas );
}

void AAS_SetAASBlockingEntity( const vec3_t absmin, const vec3_t absmax, int blocking ) {
	int maxWorlds = GGameType & GAME_ET ? 1 : MAX_AAS_WORLDS;
	int areas[ 1024 ];
	int numareas, i, w;
	//
	// check for resetting AAS blocking
	if ( VectorCompare( absmin, absmax ) && blocking < 0 ) {
		for ( w = 0; w < maxWorlds; w++ ) {
			AAS_SetCurrentWorld( w );
			//
			if ( !aasworld->loaded ) {
				continue;
			}
			// now clear blocking status
			for ( i = 1; i < aasworld->numareas; i++ ) {
				AAS_EnableRoutingArea( i, true );
			}
		}
		//
		return;
	}

	bool mover;
	if ( GGameType & GAME_ET && blocking & BLOCKINGFLAG_MOVER ) {
		mover = true;
		blocking &= ~BLOCKINGFLAG_MOVER;
	} else {
		mover = false;
	}

	bool changed = false;
areas_again:
	for ( w = 0; w < maxWorlds; w++ ) {
		AAS_SetCurrentWorld( w );
		//
		if ( !aasworld->loaded ) {
			continue;
		}
		// grab the list of areas
		numareas = AAS_BBoxAreas( absmin, absmax, areas, 1024 );
		// now set their blocking status
		for ( i = 0; i < numareas; i++ ) {
			if ( mover ) {
				if ( !( aasworld->areasettings[ areas[ i ] ].contents & WOLFAREACONTENTS_MOVER ) ) {
					continue;	// this isn't a mover area, so ignore it
				}
			}
			AAS_EnableRoutingArea( areas[ i ], GGameType & GAME_ET ? blocking ^ 1 : !blocking );
			changed = true;
		}
	}

	if ( mover && !changed ) {			// map must not be compiled with MOVER flags enabled, so redo the old way
		mover = false;
		goto areas_again;
	}
}
