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

#include "../progsvm/progsvm.h"
#include "local.h"
#include "../../common/file_formats/bsp29.h"
#include "../public.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"

Cvar* svqh_aim;

static byte checkpvs[ BSP29_MAX_MAP_LEAFS / 8 ];

//	Dumps out self, then an error message.  The program is aborted and self is
// removed, but the level can continue.
void PF_objerror() {
	const char* s = PF_VarString( 0 );
	common->Printf( "======OBJECT ERROR in %s:\n%s\n",
		PR_GetString( pr_xfunction->s_name ), s );
	qhedict_t* ed = PROG_TO_EDICT( *pr_globalVars.self );
	ED_Print( ed );
	ED_Free( ed );

	common->Error( "Program error" );
}

//	This was a major timewaster in progs, so it was converted to C
void PF_changeyaw() {
	qhedict_t* ent = PROG_TO_EDICT( *pr_globalVars.self );
	float current = AngleMod( ent->GetAngles()[ 1 ] );
	float ideal = ent->GetIdealYaw();
	float speed = ent->GetYawSpeed();

	if ( current == ideal ) {
		if ( GGameType & GAME_Hexen2 ) {
			G_FLOAT( OFS_RETURN ) = 0;
		}
		return;
	}
	float move = ideal - current;
	if ( ideal > current ) {
		if ( move >= 180 ) {
			move = move - 360;
		}
	} else   {
		if ( move <= -180 ) {
			move = move + 360;
		}
	}

	if ( GGameType & GAME_Hexen2 ) {
		G_FLOAT( OFS_RETURN ) = move;
	}

	if ( move > 0 ) {
		if ( move > speed ) {
			move = speed;
		}
	} else   {
		if ( move < -speed ) {
			move = -speed;
		}
	}

	ent->GetAngles()[ 1 ] = AngleMod( current + move );
}

//	This is the only valid way to move an object without using
// the physics of the world (setting velocity and waiting).
// Directly changing origin will not set internal links correctly,
// so clipping would be messed up. This should be called when an
// object is spawned, and then only if it is teleported.
void PF_setorigin() {
	qhedict_t* e = G_EDICT( OFS_PARM0 );
	float* org = G_VECTOR( OFS_PARM1 );

	VectorCopy( org, e->GetOrigin() );
	SVQH_LinkEdict( e, false );
}

void SetMinMaxSize( qhedict_t* e, const float* min, const float* max ) {
	for ( int i = 0; i < 3; i++ ) {
		if ( min[ i ] > max[ i ] ) {
			PR_RunError( "backwards mins/maxs" );
		}
	}

	e->SetMins( min );
	e->SetMaxs( max );
	VectorSubtract( max, min, e->GetSize() );

	SVQH_LinkEdict( e, false );
}

void PF_setsize() {
	qhedict_t* e = G_EDICT( OFS_PARM0 );
	float* min = G_VECTOR( OFS_PARM1 );
	float* max = G_VECTOR( OFS_PARM2 );

	SetMinMaxSize( e, min, max );
}

static void SetModelCommon( qhedict_t* e, const char* m ) {
	// check to see if model was properly precached
	int i;
	const char** check = sv.qh_model_precache;
	for ( i = 0; *check; i++, check++ ) {
		if ( !String::Cmp( *check, m ) ) {
			break;
		}
	}

	if ( !*check ) {
		PR_RunError( "no precache: %s\n", m );
	}

	e->SetModel( PR_SetString( m ) );
	e->v.modelindex = i;
}

//	Also sets size, mins, and maxs for inline bmodels
void PFQ1_setmodel() {
	qhedict_t* e = G_EDICT( OFS_PARM0 );
	const char* m = G_STRING( OFS_PARM1 );

	SetModelCommon( e, m );

	clipHandle_t mod = sv.models[ ( int )e->v.modelindex ];
	if ( mod ) {
		vec3_t mins;
		vec3_t maxs;
		CM_ModelBounds( mod, mins, maxs );
		SetMinMaxSize( e, mins, maxs );
	} else   {
		SetMinMaxSize( e, vec3_origin, vec3_origin );
	}
}

//	Also sets size, mins, and maxs for inline bmodels
void PFQW_setmodel() {
	qhedict_t* e = G_EDICT( OFS_PARM0 );
	const char* m = G_STRING( OFS_PARM1 );

	SetModelCommon( e, m );

	// if it is an inline model, get the size information for it
	if ( m[ 0 ] == '*' ) {
		clipHandle_t mod = CM_InlineModel( String::Atoi( m + 1 ) );
		vec3_t mins;
		vec3_t maxs;
		CM_ModelBounds( mod, mins, maxs );
		SetMinMaxSize( e, mins, maxs );
	}
}

void PF_ambientsound() {
	float* pos = G_VECTOR( OFS_PARM0 );
	const char* samp = G_STRING( OFS_PARM1 );
	float vol = G_FLOAT( OFS_PARM2 );
	float attenuation = G_FLOAT( OFS_PARM3 );

	// check to see if samp was properly precached
	int soundnum;
	const char** check;
	for ( soundnum = 0, check = sv.qh_sound_precache; *check; check++, soundnum++ ) {
		if ( !String::Cmp( *check,samp ) ) {
			break;
		}
	}

	if ( !*check ) {
		common->Printf( "no precache: %s\n", samp );
		return;
	}

	// add an svc_spawnambient command to the level signon packet
	sv.qh_signon.WriteByte( GGameType & GAME_Hexen2 ? h2svc_spawnstaticsound : q1svc_spawnstaticsound );
	sv.qh_signon.WritePos( pos );
	if ( GGameType & GAME_Hexen2 && !( GGameType & GAME_HexenWorld ) ) {
		sv.qh_signon.WriteShort( soundnum );
	} else   {
		sv.qh_signon.WriteByte( soundnum );
	}
	sv.qh_signon.WriteByte( vol * 255 );
	sv.qh_signon.WriteByte( attenuation * 64 );

}

void PF_sound() {
	qhedict_t* entity = G_EDICT( OFS_PARM0 );
	int channel = G_FLOAT( OFS_PARM1 );
	const char* sample = G_STRING( OFS_PARM2 );
	int volume = G_FLOAT( OFS_PARM3 ) * 255;
	float attenuation = G_FLOAT( OFS_PARM4 );

	SVQH_StartSound( entity, channel, sample, volume, attenuation );
}

//	Used for use tracing and shot targeting
// Traces are blocked by bbox and exact bsp entityes, and also slide box entities
// if the tryents flag is set.
void PF_traceline() {
	float* v1 = G_VECTOR( OFS_PARM0 );
	float* v2 = G_VECTOR( OFS_PARM1 );
	int type = G_FLOAT( OFS_PARM2 );
	qhedict_t* ent = G_EDICT( OFS_PARM3 );

	q1trace_t trace = SVQH_MoveHull0( v1, vec3_origin, vec3_origin, v2, type, ent );

	SVQH_SetMoveTrace( trace );
}

static int PF_newcheckclient( int check ) {
	// cycle to the next one
	int maxclients = GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ? MAX_CLIENTS_QHW : svs.qh_maxclients;
	if ( check < 1 ) {
		check = 1;
	}
	if ( check > maxclients ) {
		check = maxclients;
	}

	int i;
	if ( check == maxclients ) {
		i = 1;
	} else   {
		i = check + 1;
	}

	qhedict_t* ent;
	for (;; i++ ) {
		if ( i == maxclients + 1 ) {
			i = 1;
		}

		ent = QH_EDICT_NUM( i );

		if ( i == check ) {
			break;	// didn't find anything else

		}
		if ( ent->free ) {
			continue;
		}
		if ( ent->GetHealth() <= 0 ) {
			continue;
		}
		if ( ( int )ent->GetFlags() & QHFL_NOTARGET ) {
			continue;
		}

		// anything that is a client, or has a client as an enemy
		break;
	}

	// get the PVS for the entity
	vec3_t org;
	VectorAdd( ent->GetOrigin(), ent->GetViewOfs(), org );
	int leaf = CM_PointLeafnum( org );
	byte* pvs = CM_ClusterPVS( CM_LeafCluster( leaf ) );
	Com_Memcpy( checkpvs, pvs, ( CM_NumClusters() + 7 ) >> 3 );

	return i;
}

//	Returns a client (or object that has a client enemy) that would be a
// valid target.
//	If there are more than one valid options, they are cycled each frame
//	If (self.origin + self.viewofs) is not in the PVS of the current target,
// it is not returned at all.
void PF_checkclient() {
	// find a new check if on a new frame
	if ( sv.qh_time - sv.qh_lastchecktime >= ( GGameType & GAME_HexenWorld ? HX_FRAME_MSTIME : 100 ) ) {
		sv.qh_lastcheck = PF_newcheckclient( sv.qh_lastcheck );
		sv.qh_lastchecktime = sv.qh_time;
	}

	// return check if it might be visible
	qhedict_t* ent = QH_EDICT_NUM( sv.qh_lastcheck );
	if ( ent->free || ent->GetHealth() <= 0 ) {
		RETURN_EDICT( sv.qh_edicts );
		return;
	}

	// if current entity can't possibly see the check entity, return 0
	qhedict_t* self = PROG_TO_EDICT( *pr_globalVars.self );
	vec3_t view;
	VectorAdd( self->GetOrigin(), self->GetViewOfs(), view );
	int leaf = CM_PointLeafnum( view );
	int l = CM_LeafCluster( leaf );
	if ( ( l < 0 ) || !( checkpvs[ l >> 3 ] & ( 1 << ( l & 7 ) ) ) ) {
		RETURN_EDICT( sv.qh_edicts );
		return;
	}

	// might be able to see it
	RETURN_EDICT( ent );
}

//	Returns a chain of entities that have origins within a spherical area
void PF_findradius() {
	float* org = G_VECTOR( OFS_PARM0 );
	float rad = G_FLOAT( OFS_PARM1 );

	qhedict_t* chain = ( qhedict_t* )sv.qh_edicts;

	qhedict_t* ent = NEXT_EDICT( sv.qh_edicts );
	for ( int i = 1; i < sv.qh_num_edicts; i++, ent = NEXT_EDICT( ent ) ) {
		if ( ent->free ) {
			continue;
		}
		if ( ent->GetSolid() == QHSOLID_NOT ) {
			continue;
		}
		vec3_t eorg;
		for ( int j = 0; j < 3; j++ ) {
			eorg[ j ] = org[ j ] - ( ent->GetOrigin()[ j ] + ( ent->GetMins()[ j ] + ent->GetMaxs()[ j ] ) * 0.5 );
		}
		if ( VectorLength( eorg ) > rad ) {
			continue;
		}

		ent->SetChain( EDICT_TO_PROG( chain ) );
		chain = ent;
	}

	RETURN_EDICT( chain );
}

void PF_Spawn() {
	qhedict_t* ed = ED_Alloc();
	RETURN_EDICT( ed );
}

void PFQ1_Remove() {
	qhedict_t* ed = G_EDICT( OFS_PARM0 );
	ED_Free( ed );
}

void PFH2_Remove() {
	qhedict_t* ed = G_EDICT( OFS_PARM0 );
	if ( ed == sv.qh_edicts ) {
		common->DPrintf( "Tried to remove the world at %s in %s!\n",
			PR_GetString( pr_xfunction->s_name ), PR_GetString( pr_xfunction->s_file ) );
		return;
	}

	int i = QH_NUM_FOR_EDICT( ed );
	if ( i <= svs.qh_maxclients ) {
		common->DPrintf( "Tried to remove a client at %s in %s!\n",
			PR_GetString( pr_xfunction->s_name ), PR_GetString( pr_xfunction->s_file ) );
		return;
	}
	ED_Free( ed );
}

void PFHW_Remove() {
	qhedict_t* ed = G_EDICT( OFS_PARM0 );
	if ( ed == sv.qh_edicts ) {
		common->Printf( "Tried to remove the world at %s in %s!\n",
			PR_GetString( pr_xfunction->s_name ), PR_GetString( pr_xfunction->s_file ) );
		return;
	}

	int i = QH_NUM_FOR_EDICT( ed );
	if ( i <= MAX_CLIENTS_QHW ) {
		common->Printf( "Tried to remove a client at %s in %s!\n",
			PR_GetString( pr_xfunction->s_name ), PR_GetString( pr_xfunction->s_file ) );
		return;
	}
	ED_Free( ed );
}

void PF_Find() {
	int e = G_EDICTNUM( OFS_PARM0 );
	int f = G_INT( OFS_PARM1 );
	const char* s = G_STRING( OFS_PARM2 );
	if ( !s ) {
		PR_RunError( "PF_Find: bad search string" );
	}

	for ( e++; e < sv.qh_num_edicts; e++ ) {
		qhedict_t* ed = QH_EDICT_NUM( e );
		if ( ed->free ) {
			continue;
		}
		const char* t = E_STRING( ed, f );
		if ( !t ) {
			continue;
		}
		if ( !String::Cmp( t, s ) ) {
			RETURN_EDICT( ed );
			return;
		}
	}

	RETURN_EDICT( sv.qh_edicts );
}

void PR_CheckEmptyString( const char* s ) {
	if ( s[ 0 ] <= ' ' ) {
		PR_RunError( "Bad string" );
	}
}

void PF_precache_file() {
	// precache_file is only used to copy files with qcc, it does nothing
	G_INT( OFS_RETURN ) = G_INT( OFS_PARM0 );
}

void PF_precache_sound() {
	if ( sv.state != SS_LOADING ) {
		PR_RunError( "PF_Precache_*: Precache can only be done in spawn functions" );
	}

	const char* s = G_STRING( OFS_PARM0 );
	G_INT( OFS_RETURN ) = G_INT( OFS_PARM0 );
	PR_CheckEmptyString( s );

	int maxSounds = GGameType & GAME_Hexen2 ? GGameType & GAME_HexenWorld ? MAX_SOUNDS_HW : MAX_SOUNDS_H2 : MAX_SOUNDS_Q1;
	for ( int i = 0; i < maxSounds; i++ ) {
		if ( !sv.qh_sound_precache[ i ] ) {
			sv.qh_sound_precache[ i ] = s;
			return;
		}
		if ( !String::Cmp( sv.qh_sound_precache[ i ], s ) ) {
			return;
		}
	}
	PR_RunError( "PF_precache_sound: overflow" );
}

void PF_precache_model() {
	if ( sv.state != SS_LOADING ) {
		PR_RunError( "PF_Precache_*: Precache can only be done in spawn functions" );
	}

	const char* s = G_STRING( OFS_PARM0 );
	G_INT( OFS_RETURN ) = G_INT( OFS_PARM0 );
	PR_CheckEmptyString( s );

	for ( int i = 0; i < ( GGameType & GAME_Hexen2 ? MAX_MODELS_H2 : MAX_MODELS_Q1 ); i++ ) {
		if ( !sv.qh_model_precache[ i ] ) {
			sv.qh_model_precache[ i ] = s;
			if ( !( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) ) {
				sv.models[ i ] = CM_PrecacheModel( s );
			}
			return;
		}
		if ( !String::Cmp( sv.qh_model_precache[ i ], s ) ) {
			return;
		}
	}
	PR_RunError( "PF_precache_model: overflow" );
}

static void WalkMoveCommon( bool noEnemy, bool setTrace ) {
	qhedict_t* ent = PROG_TO_EDICT( *pr_globalVars.self );
	float yaw = G_FLOAT( OFS_PARM0 );
	float dist = G_FLOAT( OFS_PARM1 );

	if ( !( ( int )ent->GetFlags() & ( QHFL_ONGROUND | QHFL_FLY | QHFL_SWIM ) ) ) {
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	yaw = DEG2RAD( yaw );

	vec3_t move;
	move[ 0 ] = cos( yaw ) * dist;
	move[ 1 ] = sin( yaw ) * dist;
	move[ 2 ] = 0;

	// save program state, because SVQH_movestep may call other progs
	dfunction_t* oldf = pr_xfunction;
	int oldself = *pr_globalVars.self;

	G_FLOAT( OFS_RETURN ) = SVQH_movestep( ent, move, true, noEnemy, setTrace );

	// restore program state
	pr_xfunction = oldf;
	*pr_globalVars.self = oldself;
}

void PFQ1_walkmove() {
	WalkMoveCommon( false, false );
}

void PFH2_walkmove() {
	bool set_trace = G_FLOAT( OFS_PARM2 );

	WalkMoveCommon( true, set_trace );
}

void PF_droptofloor() {
	qhedict_t* ent = PROG_TO_EDICT( *pr_globalVars.self );

	vec3_t end;
	VectorCopy( ent->GetOrigin(), end );
	end[ 2 ] -= 256;

	q1trace_t trace = SVQH_Move( ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, false, ent );

	if ( trace.fraction == 1 || trace.allsolid ) {
		G_FLOAT( OFS_RETURN ) = 0;
	} else   {
		VectorCopy( trace.endpos, ent->GetOrigin() );
		SVQH_LinkEdict( ent, false );
		ent->SetFlags( ( int )ent->GetFlags() | QHFL_ONGROUND );
		ent->SetGroundEntity( EDICT_TO_PROG( QH_EDICT_NUM( trace.entityNum ) ) );
		G_FLOAT( OFS_RETURN ) = 1;
	}
}

void PFQ1_lightstyle() {
	int style = G_FLOAT( OFS_PARM0 );
	const char* val = G_STRING( OFS_PARM1 );

	// change the string in sv
	sv.qh_lightstyles[ style ] = val;

	// send message to all clients on this server
	if ( sv.state != SS_GAME ) {
		return;
	}

	client_t* client = svs.clients;
	for ( int j = 0; j < svs.qh_maxclients; j++, client++ ) {
		if ( client->state >= CS_CONNECTED ) {
			client->qh_message.WriteChar( GGameType & GAME_Hexen2 ? h2svc_lightstyle : q1svc_lightstyle );
			client->qh_message.WriteChar( style );
			client->qh_message.WriteString2( val );
		}
	}
}

void PFQW_lightstyle() {
	int style = G_FLOAT( OFS_PARM0 );
	const char* val = G_STRING( OFS_PARM1 );

	// change the string in sv
	sv.qh_lightstyles[ style ] = val;

	// send message to all clients on this server
	if ( sv.state != SS_GAME ) {
		return;
	}

	client_t* client = svs.clients;
	for ( int j = 0; j < MAX_CLIENTS_QHW; j++, client++ ) {
		if ( client->state == CS_ACTIVE ) {
			SVQH_ClientReliableWrite_Begin( client, GGameType & GAME_Hexen2 ?
				h2svc_lightstyle : q1svc_lightstyle, String::Length( val ) + 3 );
			SVQH_ClientReliableWrite_Char( client, style );
			SVQH_ClientReliableWrite_String( client, val );
		}
	}
}

void PF_checkbottom() {
	qhedict_t* ent = G_EDICT( OFS_PARM0 );

	G_FLOAT( OFS_RETURN ) = SVQH_CheckBottom( ent );
}

void PF_pointcontents() {
	float* v = G_VECTOR( OFS_PARM0 );

	G_FLOAT( OFS_RETURN ) = SVQH_PointContents( v );
}

void PF_nextent() {
	int i = G_EDICTNUM( OFS_PARM0 );
	while ( 1 ) {
		i++;
		if ( i == sv.qh_num_edicts ) {
			RETURN_EDICT( sv.qh_edicts );
			return;
		}
		qhedict_t* ent = QH_EDICT_NUM( i );
		if ( !ent->free ) {
			RETURN_EDICT( ent );
			return;
		}
	}
}

//	Pick a vector for the player to shoot along
static void SVQH_Aim( qhedict_t* ent, const vec3_t shot_org ) {
	vec3_t start;
	VectorCopy( shot_org, start );
	start[ 2 ] += 20;

	// try sending a trace straight
	vec3_t dir;
	VectorCopy( pr_globalVars.v_forward, dir );
	vec3_t end;
	VectorMA( start, 2048, dir, end );
	q1trace_t tr = SVQH_MoveHull0( start, vec3_origin, vec3_origin, end, false, ent );
	if ( tr.entityNum >= 0 &&
		 QH_EDICT_NUM( tr.entityNum )->GetTakeDamage() == ( GGameType & GAME_Hexen2 ? QHDAMAGE_YES : Q1DAMAGE_AIM ) &&
		 ( !svqh_teamplay->value || ent->GetTeam() <= 0 || ent->GetTeam() != QH_EDICT_NUM( tr.entityNum )->GetTeam() ) ) {
		VectorCopy( pr_globalVars.v_forward, G_VECTOR( OFS_RETURN ) );
		return;
	}

	// try all possible entities
	vec3_t bestdir;
	VectorCopy( dir, bestdir );
	float bestdist = svqh_aim->value;
	qhedict_t* bestent = NULL;

	qhedict_t* check = NEXT_EDICT( sv.qh_edicts );
	for ( int i = 1; i < sv.qh_num_edicts; i++, check = NEXT_EDICT( check ) ) {
		if ( check->GetTakeDamage() != ( GGameType & GAME_Hexen2 ? QHDAMAGE_YES : Q1DAMAGE_AIM ) ) {
			continue;
		}
		if ( check == ent ) {
			continue;
		}
		if ( svqh_teamplay->value && ent->GetTeam() > 0 && ent->GetTeam() == check->GetTeam() ) {
			continue;	// don't aim at teammate
		}
		for ( int j = 0; j < 3; j++ ) {
			end[ j ] = check->GetOrigin()[ j ]
					   + 0.5 * ( check->GetMins()[ j ] + check->GetMaxs()[ j ] );
		}
		VectorSubtract( end, start, dir );
		VectorNormalize( dir );
		float dist = DotProduct( dir, pr_globalVars.v_forward );
		if ( dist < bestdist ) {
			continue;	// to far to turn
		}
		q1trace_t tr = SVQH_MoveHull0( start, vec3_origin, vec3_origin, end, false, ent );
		if ( QH_EDICT_NUM( tr.entityNum ) == check ) {	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}

	if ( bestent ) {
		if ( GGameType & GAME_Hexen2 ) {
			// Since all origins are at the base, move the point to the middle of the victim model
			vec3_t hold_org;
			hold_org[ 0 ] = bestent->GetOrigin()[ 0 ];
			hold_org[ 1 ] = bestent->GetOrigin()[ 1 ];
			hold_org[ 2 ] = bestent->GetOrigin()[ 2 ] + ( 0.5 * bestent->GetMaxs()[ 2 ] );

			VectorSubtract( hold_org,shot_org,dir );
		} else   {
			VectorSubtract( bestent->GetOrigin(), shot_org, dir );
		}
		float dist = DotProduct( dir, pr_globalVars.v_forward );
		VectorScale( pr_globalVars.v_forward, dist, end );
		end[ 2 ] = dir[ 2 ];
		VectorNormalize( end );
		VectorCopy( end, G_VECTOR( OFS_RETURN ) );
	} else   {
		VectorCopy( bestdir, G_VECTOR( OFS_RETURN ) );
	}
}

void PFQ1_aim() {
	qhedict_t* ent = G_EDICT( OFS_PARM0 );

	SVQH_Aim( ent, ent->GetOrigin() );
}

void PFQW_aim() {
	qhedict_t* ent = G_EDICT( OFS_PARM0 );

	// noaim option
	int i = QH_NUM_FOR_EDICT( ent );
	if ( i > 0 && i < MAX_CLIENTS_QHW ) {
		const char* noaim = Info_ValueForKey( svs.clients[ i - 1 ].userinfo, "noaim" );
		if ( String::Atoi( noaim ) > 0 ) {
			VectorCopy( pr_globalVars.v_forward, G_VECTOR( OFS_RETURN ) );
			return;
		}
	}

	SVQH_Aim( ent, ent->GetOrigin() );
}

void PFH2_aim() {
	qhedict_t* ent = G_EDICT( OFS_PARM0 );
	float* shot_org = G_VECTOR( OFS_PARM1 );

	SVQH_Aim( ent, shot_org );
}

QMsg* Q1WriteDest() {
	int entnum;
	int dest;
	qhedict_t* ent;

	dest = G_FLOAT( OFS_PARM0 );
	switch ( dest ) {
	case QHMSG_BROADCAST:
		return &sv.qh_datagram;

	case QHMSG_ONE:
		ent = PROG_TO_EDICT( *pr_globalVars.msg_entity );
		entnum = QH_NUM_FOR_EDICT( ent );
		if ( entnum < 1 || entnum > svs.qh_maxclients ) {
			PR_RunError( "WriteDest: not a client" );
		}
		return &svs.clients[ entnum - 1 ].qh_message;

	case QHMSG_ALL:
		return &sv.qh_reliable_datagram;

	case QHMSG_INIT:
		return &sv.qh_signon;

	default:
		PR_RunError( "WriteDest: bad destination" );
		break;
	}

	return NULL;
}

QMsg* QWWriteDest() {
	int dest = G_FLOAT( OFS_PARM0 );
	switch ( dest ) {
	case QHMSG_BROADCAST:
		return &sv.qh_datagram;

	case QHMSG_ONE:
		common->FatalError( "Shouldn't be at QHMSG_ONE" );

	case QHMSG_ALL:
		return &sv.qh_reliable_datagram;

	case QHMSG_INIT:
		if ( sv.state != SS_LOADING ) {
			PR_RunError( "PF_Write_*: QHMSG_INIT can only be written in spawn functions" );
		}
		return &sv.qh_signon;

	case QHMSG_MULTICAST:
		return &sv.multicast;

	default:
		PR_RunError( "WriteDest: bad destination" );
		break;
	}

	return NULL;
}

client_t* Write_GetClient() {
	qhedict_t* ent = PROG_TO_EDICT( *pr_globalVars.msg_entity );
	int entnum = QH_NUM_FOR_EDICT( ent );
	if ( entnum < 1 || entnum > MAX_CLIENTS_QHW ) {
		PR_RunError( "WriteDest: not a client" );
	}
	return &svs.clients[ entnum - 1 ];
}

void PFQ1_WriteByte() {
	Q1WriteDest()->WriteByte( G_FLOAT( OFS_PARM1 ) );
}

void PFQW_WriteByte() {
	if ( G_FLOAT( OFS_PARM0 ) == QHMSG_ONE ) {
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableCheckBlock( cl, 1 );
		SVQH_ClientReliableWrite_Byte( cl, G_FLOAT( OFS_PARM1 ) );
	} else   {
		QWWriteDest()->WriteByte( G_FLOAT( OFS_PARM1 ) );
	}
}

void PFQ1_WriteChar() {
	Q1WriteDest()->WriteChar( G_FLOAT( OFS_PARM1 ) );
}

void PFQW_WriteChar() {
	if ( G_FLOAT( OFS_PARM0 ) == QHMSG_ONE ) {
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableCheckBlock( cl, 1 );
		SVQH_ClientReliableWrite_Char( cl, G_FLOAT( OFS_PARM1 ) );
	} else   {
		QWWriteDest()->WriteChar( G_FLOAT( OFS_PARM1 ) );
	}
}

void PFQ1_WriteShort() {
	Q1WriteDest()->WriteShort( G_FLOAT( OFS_PARM1 ) );
}

void PFQW_WriteShort() {
	if ( G_FLOAT( OFS_PARM0 ) == QHMSG_ONE ) {
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableCheckBlock( cl, 2 );
		SVQH_ClientReliableWrite_Short( cl, G_FLOAT( OFS_PARM1 ) );
	} else   {
		QWWriteDest()->WriteShort( G_FLOAT( OFS_PARM1 ) );
	}
}

void PFQ1_WriteLong() {
	Q1WriteDest()->WriteLong( G_FLOAT( OFS_PARM1 ) );
}

void PFQW_WriteLong() {
	if ( G_FLOAT( OFS_PARM0 ) == QHMSG_ONE ) {
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableCheckBlock( cl, 4 );
		SVQH_ClientReliableWrite_Long( cl, G_FLOAT( OFS_PARM1 ) );
	} else   {
		QWWriteDest()->WriteLong( G_FLOAT( OFS_PARM1 ) );
	}
}

void PFQ1_WriteAngle() {
	Q1WriteDest()->WriteAngle( G_FLOAT( OFS_PARM1 ) );
}

void PFQW_WriteAngle() {
	if ( G_FLOAT( OFS_PARM0 ) == QHMSG_ONE ) {
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableCheckBlock( cl, 1 );
		SVQH_ClientReliableWrite_Angle( cl, G_FLOAT( OFS_PARM1 ) );
	} else   {
		QWWriteDest()->WriteAngle( G_FLOAT( OFS_PARM1 ) );
	}
}

void PFQ1_WriteCoord() {
	Q1WriteDest()->WriteCoord( G_FLOAT( OFS_PARM1 ) );
}

void PFQW_WriteCoord() {
	if ( G_FLOAT( OFS_PARM0 ) == QHMSG_ONE ) {
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableCheckBlock( cl, 2 );
		SVQH_ClientReliableWrite_Coord( cl, G_FLOAT( OFS_PARM1 ) );
	} else   {
		QWWriteDest()->WriteCoord( G_FLOAT( OFS_PARM1 ) );
	}
}

void PFQ1_WriteString() {
	Q1WriteDest()->WriteString2( G_STRING( OFS_PARM1 ) );
}

void PFQW_WriteString() {
	if ( G_FLOAT( OFS_PARM0 ) == QHMSG_ONE ) {
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableCheckBlock( cl, 1 + String::Length( G_STRING( OFS_PARM1 ) ) );
		SVQH_ClientReliableWrite_String( cl, G_STRING( OFS_PARM1 ) );
	} else   {
		QWWriteDest()->WriteString2( G_STRING( OFS_PARM1 ) );
	}
}

void PFQ1_WriteEntity() {
	Q1WriteDest()->WriteShort( G_EDICTNUM( OFS_PARM1 ) );
}

void PFQW_WriteEntity() {
	if ( G_FLOAT( OFS_PARM0 ) == QHMSG_ONE ) {
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableCheckBlock( cl, 2 );
		SVQH_ClientReliableWrite_Short( cl, G_EDICTNUM( OFS_PARM1 ) );
	} else   {
		QWWriteDest()->WriteShort( G_EDICTNUM( OFS_PARM1 ) );
	}
}

void PF_setspawnparms() {
	qhedict_t* ent = G_EDICT( OFS_PARM0 );
	int i = QH_NUM_FOR_EDICT( ent );
	if ( i < 1 || i > ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ? MAX_CLIENTS_QHW : svs.qh_maxclients ) ) {
		PR_RunError( "Entity is not a client" );
	}

	// copy spawn parms out of the client_t
	client_t* client = svs.clients + ( i - 1 );

	for ( i = 0; i < NUM_SPAWN_PARMS; i++ ) {
		pr_globalVars.parm1[ i ] = client->qh_spawn_parms[ i ];
	}
}

void PF_logfrag() {
	qhedict_t* ent1 = G_EDICT( OFS_PARM0 );
	qhedict_t* ent2 = G_EDICT( OFS_PARM1 );

	int e1 = QH_NUM_FOR_EDICT( ent1 );
	int e2 = QH_NUM_FOR_EDICT( ent2 );

	if ( e1 < 1 || e1 > MAX_CLIENTS_QHW ||
		 e2 < 1 || e2 > MAX_CLIENTS_QHW ) {
		return;
	}

	char* s = va( "\\%s\\%s\\\n",svs.clients[ e1 - 1 ].name, svs.clients[ e2 - 1 ].name );

	svs.qh_log[ svs.qh_logsequence & 1 ].Print( s );
	if ( svqhw_fraglogfile ) {
		FS_Printf( svqhw_fraglogfile, s );
		FS_Flush( svqhw_fraglogfile );
	}
}

void PF_multicast() {
	float* o = G_VECTOR( OFS_PARM0 );
	int to = G_FLOAT( OFS_PARM1 );

	SVQH_Multicast( o, to );
}

//	broadcast print to everyone on server
void PFQ1_bprint() {
	const char* s = PF_VarString( 0 );
	SVQH_BroadcastPrintf( 0, "%s", s );
}

void PFQW_bprint() {
	int level = G_FLOAT( OFS_PARM0 );
	const char* s = PF_VarString( 1 );

	if ( GGameType & GAME_HexenWorld && hw_spartanPrint->value == 1 && level < 2 ) {
		return;
	}

	SVQH_BroadcastPrintf( level, "%s", s );
}

//	single print to a specific client
void PFQ1_sprint() {
	int entnum = G_EDICTNUM( OFS_PARM0 );
	const char* s = PF_VarString( 1 );

	if ( entnum < 1 || entnum > svs.qh_maxclients ) {
		common->Printf( "tried to sprint to a non-client\n" );
		return;
	}

	client_t* client = &svs.clients[ entnum - 1 ];

	SVQH_ClientPrintf( client, 0, "%s", s );
}

void PFQW_sprint() {
	int entnum = G_EDICTNUM( OFS_PARM0 );
	int level = G_FLOAT( OFS_PARM1 );

	const char* s = PF_VarString( 2 );

	if ( entnum < 1 || entnum > MAX_CLIENTS_QHW ) {
		common->Printf( "tried to sprint to a non-client\n" );
		return;
	}

	client_t* client = &svs.clients[ entnum - 1 ];

	if ( GGameType & GAME_HexenWorld && hw_spartanPrint->value == 1 && level < 2 ) {
		return;
	}

	SVQH_ClientPrintf( client, level, "%s", s );
}

void PF_centerprint() {
	int entnum = G_EDICTNUM( OFS_PARM0 );
	const char* s = PF_VarString( 1 );

	if ( entnum < 1 || entnum > ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ? MAX_CLIENTS_QHW : svs.qh_maxclients ) ) {
		common->Printf( "tried to sprint to a non-client\n" );
		return;
	}

	client_t* client = &svs.clients[ entnum - 1 ];

	if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) {
		SVQH_ClientReliableWrite_Begin( client, GGameType & GAME_HexenWorld ? h2svc_centerprint : q1svc_centerprint, 2 + String::Length( s ) );
		SVQH_ClientReliableWrite_String( client, s );
	} else   {
		client->qh_message.WriteChar( GGameType & GAME_Hexen2 ? h2svc_centerprint : q1svc_centerprint );
		client->qh_message.WriteString2( s );
	}
}

void PF_particle() {
	float* org = G_VECTOR( OFS_PARM0 );
	float* dir = G_VECTOR( OFS_PARM1 );
	float color = G_FLOAT( OFS_PARM2 );
	float count = G_FLOAT( OFS_PARM3 );
	SVQH_StartParticle( org, dir, color, count );
}

//	Sends text over to the client's execution buffer
void PF_stuffcmd() {
	int entnum = G_EDICTNUM( OFS_PARM0 );
	if ( entnum < 1 || entnum > ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ? MAX_CLIENTS_QHW : svs.qh_maxclients ) ) {
		PR_RunError( "Parm 0 not a client" );
	}
	const char* str = G_STRING( OFS_PARM1 );

	client_t* cl = &svs.clients[ entnum - 1 ];

	if ( GGameType & GAME_QuakeWorld && String::Cmp( str, "disconnect\n" ) == 0 ) {
		// so long and thanks for all the fish
		cl->qw_drop = true;
		return;
	}

	SVQH_SendClientCommand( cl, "%s", str );
}
