/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_ents.c -- entity parsing and management

#include "quakedef.h"
#include "glquake.h"

extern	QCvar*	cl_predict_players;
extern	QCvar*	cl_predict_players2;
extern	QCvar*	cl_solid_players;

static struct predicted_player {
	int flags;
	qboolean active;
	vec3_t origin;	// predicted origin
} predicted_players[MAX_CLIENTS];

//============================================================

/*
===============
CL_AllocDlight

===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				Com_Memset(dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			Com_Memset(dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	Com_Memset(dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

/*
===============
CL_NewDlight
===============
*/
void CL_NewDlight (int key, float x, float y, float z, float radius, float time,
				   int type)
{
	dlight_t	*dl;

	dl = CL_AllocDlight (key);
	dl->origin[0] = x;
	dl->origin[1] = y;
	dl->origin[2] = z;
	dl->radius = radius;
	dl->die = cl.time + time;
	if (type == 0) {
		dl->color[0] = 0.2;
		dl->color[1] = 0.1;
		dl->color[2] = 0.05;
		dl->color[3] = 0.7;
	} else if (type == 1) {
		dl->color[0] = 0.05;
		dl->color[1] = 0.05;
		dl->color[2] = 0.3;
		dl->color[3] = 0.7;
	} else if (type == 2) {
		dl->color[0] = 0.5;
		dl->color[1] = 0.05;
		dl->color[2] = 0.05;
		dl->color[3] = 0.7;
	} else if (type == 3) {
		dl->color[0]=0.5;
		dl->color[1] = 0.05;
		dl->color[2] = 0.4;
		dl->color[3] = 0.7;
	}
}


/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;
		
		dl->radius -= host_frametime*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
=========================================================================

PACKET ENTITY PARSING / LINKING

=========================================================================
*/

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
int	bitcounts[32];	/// just for protocol profiling
void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int bits)
{
	int			i;

	// set everything to the state we are delta'ing from
	*to = *from;

	to->number = bits & 511;
	bits &= ~511;

	if (bits & U_MOREBITS)
	{	// read in the low order bits
		i = net_message.ReadByte ();
		bits |= i;
	}

	// count the bits for net profiling
	for (i=0 ; i<16 ; i++)
		if (bits&(1<<i))
			bitcounts[i]++;

	to->flags = bits;
	
	if (bits & U_MODEL)
		to->modelindex = net_message.ReadByte ();
		
	if (bits & U_FRAME)
		to->frame = net_message.ReadByte ();

	if (bits & U_COLORMAP)
		to->colormap = net_message.ReadByte();

	if (bits & U_SKIN)
		to->skinnum = net_message.ReadByte();

	if (bits & U_EFFECTS)
		to->effects = net_message.ReadByte();

	if (bits & U_ORIGIN1)
		to->origin[0] = net_message.ReadCoord ();
		
	if (bits & U_ANGLE1)
		to->angles[0] = net_message.ReadAngle();

	if (bits & U_ORIGIN2)
		to->origin[1] = net_message.ReadCoord ();
		
	if (bits & U_ANGLE2)
		to->angles[1] = net_message.ReadAngle();

	if (bits & U_ORIGIN3)
		to->origin[2] = net_message.ReadCoord ();
		
	if (bits & U_ANGLE3)
		to->angles[2] = net_message.ReadAngle();

	if (bits & U_SOLID)
	{
		// FIXME
	}
}


/*
=================
FlushEntityPacket
=================
*/
void FlushEntityPacket (void)
{
	int			word;
	entity_state_t	olde, newe;

	Con_DPrintf ("FlushEntityPacket\n");

	Com_Memset(&olde, 0, sizeof(olde));

	cl.validsequence = 0;		// can't render a frame
	cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].invalid = true;

	// read it all, but ignore it
	while (1)
	{
		word = (unsigned short)net_message.ReadShort ();
		if (net_message.badread)
		{	// something didn't parse right...
			Host_EndGame ("msg_badread in packetentities");
			return;
		}

		if (!word)
			break;	// done

		CL_ParseDelta (&olde, &newe, word);
	}
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CL_ParsePacketEntities (qboolean delta)
{
	int			oldpacket, newpacket;
	packet_entities_t	*oldp, *newp, dummy;
	int			oldindex, newindex;
	int			word, newnum, oldnum;
	qboolean	full;
	byte		from;

	newpacket = cls.netchan.incoming_sequence&UPDATE_MASK;
	newp = &cl.frames[newpacket].packet_entities;
	cl.frames[newpacket].invalid = false;

	if (delta)
	{
		from = net_message.ReadByte ();

		oldpacket = cl.frames[newpacket].delta_sequence;

		if ( (from&UPDATE_MASK) != (oldpacket&UPDATE_MASK) )
			Con_DPrintf ("WARNING: from mismatch\n");
	}
	else
		oldpacket = -1;

	full = false;
	if (oldpacket != -1)
	{
		if (cls.netchan.outgoing_sequence - oldpacket >= UPDATE_BACKUP-1)
		{	// we can't use this, it is too old
			FlushEntityPacket ();
			return;
		}
		cl.validsequence = cls.netchan.incoming_sequence;
		oldp = &cl.frames[oldpacket&UPDATE_MASK].packet_entities;
	}
	else
	{	// this is a full update that we can start delta compressing from now
		oldp = &dummy;
		dummy.num_entities = 0;
		cl.validsequence = cls.netchan.incoming_sequence;
		full = true;
	}

	oldindex = 0;
	newindex = 0;
	newp->num_entities = 0;

	while (1)
	{
		word = (unsigned short)net_message.ReadShort ();
		if (net_message.badread)
		{	// something didn't parse right...
			Host_EndGame ("msg_badread in packetentities");
			return;
		}

		if (!word)
		{
			while (oldindex < oldp->num_entities)
			{	// copy all the rest of the entities from the old packet
//Con_Printf ("copy %i\n", oldp->entities[oldindex].number);
				if (newindex >= MAX_PACKET_ENTITIES)
					Host_EndGame ("CL_ParsePacketEntities: newindex == MAX_PACKET_ENTITIES");
				newp->entities[newindex] = oldp->entities[oldindex];
				newindex++;
				oldindex++;
			}
			break;
		}
		newnum = word&511;
		oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;

		while (newnum > oldnum)
		{
			if (full)
			{
				Con_Printf ("WARNING: oldcopy on full update");
				FlushEntityPacket ();
				return;
			}

//Con_Printf ("copy %i\n", oldnum);
			// copy one of the old entities over to the new packet unchanged
			if (newindex >= MAX_PACKET_ENTITIES)
				Host_EndGame ("CL_ParsePacketEntities: newindex == MAX_PACKET_ENTITIES");
			newp->entities[newindex] = oldp->entities[oldindex];
			newindex++;
			oldindex++;
			oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;
		}

		if (newnum < oldnum)
		{	// new from baseline
//Con_Printf ("baseline %i\n", newnum);
			if (word & U_REMOVE)
			{
				if (full)
				{
					cl.validsequence = 0;
					Con_Printf ("WARNING: U_REMOVE on full update\n");
					FlushEntityPacket ();
					return;
				}
				continue;
			}
			if (newindex >= MAX_PACKET_ENTITIES)
				Host_EndGame ("CL_ParsePacketEntities: newindex == MAX_PACKET_ENTITIES");
			CL_ParseDelta (&cl_baselines[newnum], &newp->entities[newindex], word);
			newindex++;
			continue;
		}

		if (newnum == oldnum)
		{	// delta from previous
			if (full)
			{
				cl.validsequence = 0;
				Con_Printf ("WARNING: delta on full update");
			}
			if (word & U_REMOVE)
			{
				oldindex++;
				continue;
			}
//Con_Printf ("delta %i\n",newnum);
			CL_ParseDelta (&oldp->entities[oldindex], &newp->entities[newindex], word);
			newindex++;
			oldindex++;
		}

	}

	newp->num_entities = newindex;
}


/*
===============
CL_LinkPacketEntities

===============
*/
void CL_LinkPacketEntities (void)
{
	packet_entities_t	*pack;
	entity_state_t		*s1, *s2;
	float				f;
	model_t				*model;
	vec3_t				old_origin;
	float				autorotate;
	int					i;
	int					pnum;
	dlight_t			*dl;

	pack = &cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities;
	packet_entities_t* PrevPack = &cl.frames[(cls.netchan.incoming_sequence - 1) & UPDATE_MASK].packet_entities;

	autorotate = AngleMod(100*cl.time);

	f = 0;		// FIXME: no interpolation right now

	for (pnum=0 ; pnum<pack->num_entities ; pnum++)
	{
		s1 = &pack->entities[pnum];
		s2 = s1;	// FIXME: no interpolation right now

		// spawn light flashes, even ones coming from invisible objects
		if ((s1->effects & (EF_BLUE | EF_RED)) == (EF_BLUE | EF_RED))
			CL_NewDlight (s1->number, s1->origin[0], s1->origin[1], s1->origin[2], 200 + (rand()&31), 0.1, 3);
		else if (s1->effects & EF_BLUE)
			CL_NewDlight (s1->number, s1->origin[0], s1->origin[1], s1->origin[2], 200 + (rand()&31), 0.1, 1);
		else if (s1->effects & EF_RED)
			CL_NewDlight (s1->number, s1->origin[0], s1->origin[1], s1->origin[2], 200 + (rand()&31), 0.1, 2);
		else if (s1->effects & EF_BRIGHTLIGHT)
			CL_NewDlight (s1->number, s1->origin[0], s1->origin[1], s1->origin[2] + 16, 400 + (rand()&31), 0.1, 0);
		else if (s1->effects & EF_DIMLIGHT)
			CL_NewDlight (s1->number, s1->origin[0], s1->origin[1], s1->origin[2], 200 + (rand()&31), 0.1, 0);

		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		// create a new entity
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		model = cl.model_precache[s1->modelindex];
		ent.hModel = Mod_GetHandle(model);
	
		// set colormap
		if (s1->colormap && (s1->colormap < MAX_CLIENTS) && s1->modelindex == cl_playerindex)
		{
			R_HandlePlayerSkin(&ent, s1->colormap - 1);
		}

		// set skin
		ent.skinNum = s1->skinnum;
		
		// set frame
		ent.frame = s1->frame;

		// rotate binary objects locally
		vec3_t angles;
		if (model->flags & EF_ROTATE)
		{
			angles[0] = 0;
			angles[1] = autorotate;
			angles[2] = 0;
		}
		else
		{
			float	a1, a2;

			for (i=0 ; i<3 ; i++)
			{
				a1 = s1->angles[i];
				a2 = s2->angles[i];
				if (a1 - a2 > 180)
					a1 -= 360;
				if (a1 - a2 < -180)
					a1 += 360;
				angles[i] = a2 + f * (a1 - a2);
			}
		}
		CL_SetRefEntAxis(&ent, angles);

		// calculate origin
		for (i=0 ; i<3 ; i++)
			ent.origin[i] = s2->origin[i] + 
			f * (s1->origin[i] - s2->origin[i]);
		R_AddRefEntToScene(&ent);

		// add automatic particle trails
		if (!model->flags)
			continue;

		// scan the old entity display list for a matching
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
			continue;		// not in last message
		}

		for (i=0 ; i<3 ; i++)
			if ( abs(old_origin[i] - ent.origin[i]) > 128)
			{	// no trail if too far
				VectorCopy (ent.origin, old_origin);
				break;
			}
		if (model->flags & EF_ROCKET)
		{
			R_RocketTrail (old_origin, ent.origin, 0);
			dl = CL_AllocDlight (s1->number);
			VectorCopy (ent.origin, dl->origin);
			dl->radius = 200;
			dl->die = cl.time + 0.1;
		}
		else if (model->flags & EF_GRENADE)
			R_RocketTrail (old_origin, ent.origin, 1);
		else if (model->flags & EF_GIB)
			R_RocketTrail (old_origin, ent.origin, 2);
		else if (model->flags & EF_ZOMGIB)
			R_RocketTrail (old_origin, ent.origin, 4);
		else if (model->flags & EF_TRACER)
			R_RocketTrail (old_origin, ent.origin, 3);
		else if (model->flags & EF_TRACER2)
			R_RocketTrail (old_origin, ent.origin, 5);
		else if (model->flags & EF_TRACER3)
			R_RocketTrail (old_origin, ent.origin, 6);
	}
}


/*
=========================================================================

PROJECTILE PARSING / LINKING

=========================================================================
*/

typedef struct
{
	int		modelindex;
	vec3_t	origin;
	vec3_t	angles;
} projectile_t;

#define	MAX_PROJECTILES	32
projectile_t	cl_projectiles[MAX_PROJECTILES];
int				cl_num_projectiles;

extern int cl_spikeindex;

void CL_ClearProjectiles (void)
{
	cl_num_projectiles = 0;
}

/*
=====================
CL_ParseProjectiles

Nails are passed as efficient temporary entities
=====================
*/
void CL_ParseProjectiles (void)
{
	int		i, c, j;
	byte	bits[6];
	projectile_t	*pr;

	c = net_message.ReadByte ();
	for (i=0 ; i<c ; i++)
	{
		for (j=0 ; j<6 ; j++)
			bits[j] = net_message.ReadByte ();

		if (cl_num_projectiles == MAX_PROJECTILES)
			continue;

		pr = &cl_projectiles[cl_num_projectiles];
		cl_num_projectiles++;

		pr->modelindex = cl_spikeindex;
		pr->origin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		pr->origin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		pr->origin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		pr->angles[0] = 360*(bits[4]>>4)/16;
		pr->angles[1] = 360*bits[5]/256;
	}
}

/*
=============
CL_LinkProjectiles

=============
*/
void CL_LinkProjectiles (void)
{
	int		i;
	projectile_t	*pr;

	for (i=0, pr=cl_projectiles ; i<cl_num_projectiles ; i++, pr++)
	{
		// grab an entity to fill in
		if (pr->modelindex < 1)
			continue;
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;
		ent.hModel = Mod_GetHandle(cl.model_precache[pr->modelindex]);
		VectorCopy(pr->origin, ent.origin);
		CL_SetRefEntAxis(&ent, pr->angles);
		R_AddRefEntToScene(&ent);
	}
}

//========================================

/*
===================
CL_ParsePlayerinfo
===================
*/
extern int parsecountmod;
extern double parsecounttime;
void CL_ParsePlayerinfo (void)
{
	int			msec;
	int			flags;
	player_info_t	*info;
	player_state_t	*state;
	int			num;
	int			i;

	num = net_message.ReadByte ();
	if (num > MAX_CLIENTS)
		Sys_Error ("CL_ParsePlayerinfo: bad num");

	info = &cl.players[num];

	state = &cl.frames[parsecountmod].playerstate[num];

	flags = state->flags = net_message.ReadShort ();

	state->messagenum = cl.parsecount;
	state->origin[0] = net_message.ReadCoord ();
	state->origin[1] = net_message.ReadCoord ();
	state->origin[2] = net_message.ReadCoord ();

	state->frame = net_message.ReadByte ();

	// the other player's last move was likely some time
	// before the packet was sent out, so accurately track
	// the exact time it was valid at
	if (flags & PF_MSEC)
	{
		msec = net_message.ReadByte ();
		state->state_time = parsecounttime - msec*0.001;
	}
	else
		state->state_time = parsecounttime;

	if (flags & PF_COMMAND)
		MSG_ReadDeltaUsercmd (&nullcmd, &state->command);

	for (i=0 ; i<3 ; i++)
	{
		if (flags & (PF_VELOCITY1<<i) )
			state->velocity[i] = net_message.ReadShort();
		else
			state->velocity[i] = 0;
	}
	if (flags & PF_MODEL)
		state->modelindex = net_message.ReadByte ();
	else
		state->modelindex = cl_playerindex;

	if (flags & PF_SKINNUM)
		state->skinnum = net_message.ReadByte ();
	else
		state->skinnum = 0;

	if (flags & PF_EFFECTS)
		state->effects = net_message.ReadByte ();
	else
		state->effects = 0;

	if (flags & PF_WEAPONFRAME)
		state->weaponframe = net_message.ReadByte ();
	else
		state->weaponframe = 0;

	VectorCopy (state->command.angles, state->viewangles);
}


/*
================
CL_AddFlagModels

Called when the CTF flags are set
================
*/
void CL_AddFlagModels (refEntity_t *ent, int team, vec3_t angles)
{
	int		i;
	float	f;
	vec3_t	v_forward, v_right, v_up;

	if (cl_flagindex == -1)
		return;

	f = 14;
	if (ent->frame >= 29 && ent->frame <= 40) {
		if (ent->frame >= 29 && ent->frame <= 34) { //axpain
			if      (ent->frame == 29) f = f + 2; 
			else if (ent->frame == 30) f = f + 8;
			else if (ent->frame == 31) f = f + 12;
			else if (ent->frame == 32) f = f + 11;
			else if (ent->frame == 33) f = f + 10;
			else if (ent->frame == 34) f = f + 4;
		} else if (ent->frame >= 35 && ent->frame <= 40) { // pain
			if      (ent->frame == 35) f = f + 2; 
			else if (ent->frame == 36) f = f + 10;
			else if (ent->frame == 37) f = f + 10;
			else if (ent->frame == 38) f = f + 8;
			else if (ent->frame == 39) f = f + 4;
			else if (ent->frame == 40) f = f + 2;
		}
	} else if (ent->frame >= 103 && ent->frame <= 118) {
		if      (ent->frame >= 103 && ent->frame <= 104) f = f + 6;  //nailattack
		else if (ent->frame >= 105 && ent->frame <= 106) f = f + 6;  //light 
		else if (ent->frame >= 107 && ent->frame <= 112) f = f + 7;  //rocketattack
		else if (ent->frame >= 112 && ent->frame <= 118) f = f + 7;  //shotattack
	}

	refEntity_t newent;
	Com_Memset(&newent, 0, sizeof(newent));

	newent.reType = RT_MODEL;
	newent.hModel = Mod_GetHandle(cl.model_precache[cl_flagindex]);
	newent.skinNum = team;

	AngleVectors(angles, v_forward, v_right, v_up);
	v_forward[2] = -v_forward[2]; // reverse z component
	for (i=0 ; i<3 ; i++)
		newent.origin[i] = ent->origin[i] - f*v_forward[i] + 22*v_right[i];
	newent.origin[2] -= 16;

	vec3_t flag_angles;
	VectorCopy(angles, flag_angles);
	flag_angles[2] -= 45;
	CL_SetRefEntAxis(&newent, flag_angles);
	R_AddRefEntToScene(&newent);
}

/*
=============
CL_LinkPlayers

Create visible entities in the correct position
for all current players
=============
*/
void CL_LinkPlayers (void)
{
	int				j;
	player_info_t	*info;
	player_state_t	*state;
	player_state_t	exact;
	double			playertime;
	int				msec;
	frame_t			*frame;
	int				oldphysent;

	playertime = realtime - cls.latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];

	for (j=0, info=cl.players, state=frame->playerstate ; j < MAX_CLIENTS 
		; j++, info++, state++)
	{
		if (state->messagenum != cl.parsecount)
			continue;	// not present this frame

		// spawn light flashes, even ones coming from invisible objects
		if (!gl_flashblend->value || j != cl.playernum) {
			if ((state->effects & (EF_BLUE | EF_RED)) == (EF_BLUE | EF_RED))
				CL_NewDlight (j, state->origin[0], state->origin[1], state->origin[2], 200 + (rand()&31), 0.1, 3);
			else if (state->effects & EF_BLUE)
				CL_NewDlight (j, state->origin[0], state->origin[1], state->origin[2], 200 + (rand()&31), 0.1, 1);
			else if (state->effects & EF_RED)
				CL_NewDlight (j, state->origin[0], state->origin[1], state->origin[2], 200 + (rand()&31), 0.1, 2);
			else if (state->effects & EF_BRIGHTLIGHT)
				CL_NewDlight (j, state->origin[0], state->origin[1], state->origin[2] + 16, 400 + (rand()&31), 0.1, 0);
			else if (state->effects & EF_DIMLIGHT)
				CL_NewDlight (j, state->origin[0], state->origin[1], state->origin[2], 200 + (rand()&31), 0.1, 0);
		}

		// the player object never gets added
		if (j == cl.playernum)
			continue;

		if (!state->modelindex)
			continue;

		if (!Cam_DrawPlayer(j))
			continue;

		// grab an entity to fill in
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		ent.hModel = Mod_GetHandle(cl.model_precache[state->modelindex]);
		ent.skinNum = state->skinnum;
		ent.frame = state->frame;
		if (state->modelindex == cl_playerindex)
		{
			// use custom skin
			R_HandlePlayerSkin(&ent, j);
		}

		//
		// angles
		//
		vec3_t angles;
		angles[PITCH] = -state->viewangles[PITCH]/3;
		angles[YAW] = state->viewangles[YAW];
		angles[ROLL] = 0;
		angles[ROLL] = V_CalcRoll(angles, state->velocity)*4;
		CL_SetRefEntAxis(&ent, angles);

		// only predict half the move to minimize overruns
		msec = 500*(playertime - state->state_time);
		if (msec <= 0 || (!cl_predict_players->value && !cl_predict_players2->value))
		{
			VectorCopy (state->origin, ent.origin);
//Con_DPrintf ("nopredict\n");
		}
		else
		{
			// predict players movement
			if (msec > 255)
				msec = 255;
			state->command.msec = msec;
//Con_DPrintf ("predict: %i\n", msec);

			oldphysent = pmove.numphysent;
			CL_SetSolidPlayers (j);
			CL_PredictUsercmd (state, &exact, &state->command, false);
			pmove.numphysent = oldphysent;
			VectorCopy (exact.origin, ent.origin);
		}
		R_AddRefEntToScene(&ent);

		if (state->effects & EF_FLAG1)
			CL_AddFlagModels(&ent, 0, angles);
		else if (state->effects & EF_FLAG2)
			CL_AddFlagModels(&ent, 1, angles);
	}
}

//======================================================================

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
===============
*/
void CL_SetSolidEntities (void)
{
	int		i;
	frame_t	*frame;
	packet_entities_t	*pak;
	entity_state_t		*state;

	pmove.physents[0].model = 0;
	VectorCopy (vec3_origin, pmove.physents[0].origin);
	pmove.physents[0].info = 0;
	pmove.numphysent = 1;

	frame = &cl.frames[parsecountmod];
	pak = &frame->packet_entities;

	for (i=0 ; i<pak->num_entities ; i++)
	{
		state = &pak->entities[i];

		if (state->modelindex < 2)
			continue;
		if (!cl.clip_models[state->modelindex])
			continue;
		pmove.physents[pmove.numphysent].model = cl.clip_models[state->modelindex];
		VectorCopy(state->origin, pmove.physents[pmove.numphysent].origin);
		pmove.numphysent++;
	}

}

/*
===
Calculate the new position of players, without other player clipping

We do this to set up real player prediction.
Players are predicted twice, first without clipping other players,
then with clipping against them.
This sets up the first phase.
===
*/
void CL_SetUpPlayerPrediction(qboolean dopred)
{
	int				j;
	player_state_t	*state;
	player_state_t	exact;
	double			playertime;
	int				msec;
	frame_t			*frame;
	struct predicted_player *pplayer;

	playertime = realtime - cls.latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];

	for (j=0, pplayer = predicted_players, state=frame->playerstate; 
		j < MAX_CLIENTS;
		j++, pplayer++, state++) {

		pplayer->active = false;

		if (state->messagenum != cl.parsecount)
			continue;	// not present this frame

		if (!state->modelindex)
			continue;

		pplayer->active = true;
		pplayer->flags = state->flags;

		// note that the local player is special, since he moves locally
		// we use his last predicted postition
		if (j == cl.playernum) {
			VectorCopy(cl.frames[cls.netchan.outgoing_sequence&UPDATE_MASK].playerstate[cl.playernum].origin,
				pplayer->origin);
		} else {
			// only predict half the move to minimize overruns
			msec = 500*(playertime - state->state_time);
			if (msec <= 0 ||
				(!cl_predict_players->value && !cl_predict_players2->value) ||
				!dopred)
			{
				VectorCopy (state->origin, pplayer->origin);
	//Con_DPrintf ("nopredict\n");
			}
			else
			{
				// predict players movement
				if (msec > 255)
					msec = 255;
				state->command.msec = msec;
	//Con_DPrintf ("predict: %i\n", msec);

				CL_PredictUsercmd (state, &exact, &state->command, false);
				VectorCopy (exact.origin, pplayer->origin);
			}
		}
	}
}

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
Note that CL_SetUpPlayerPrediction() must be called first!
pmove must be setup with world and solid entity hulls before calling
(via CL_PredictMove)
===============
*/
void CL_SetSolidPlayers (int playernum)
{
	int		j;
	extern	vec3_t	player_mins;
	extern	vec3_t	player_maxs;
	struct predicted_player *pplayer;
	physent_t *pent;

	if (!cl_solid_players->value)
		return;

	pent = pmove.physents + pmove.numphysent;

	for (j=0, pplayer = predicted_players; j < MAX_CLIENTS;	j++, pplayer++) {

		if (!pplayer->active)
			continue;	// not present this frame

		// the player object never gets added
		if (j == playernum)
			continue;

		if (pplayer->flags & PF_DEAD)
			continue; // dead players aren't solid

		pent->model = -1;
		VectorCopy(pplayer->origin, pent->origin);
		VectorCopy(player_mins, pent->mins);
		VectorCopy(player_maxs, pent->maxs);
		pmove.numphysent++;
		pent++;
	}
}

static void CL_LinkStaticEntities()
{
	entity_t* pent = cl_static_entities;
	for (int i = 0; i < cl.num_statics; i++, pent++)
	{
		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(pent->origin, rent.origin);
		rent.hModel = Mod_GetHandle(pent->model);
		CL_SetRefEntAxis(&rent, pent->angles);
		rent.frame = pent->frame;
		rent.skinNum = pent->skinnum;
		rent.shaderTime = pent->syncbase;
		R_AddRefEntToScene(&rent);
	}
}

/*
===============
CL_EmitEntities

Builds the visedicts array for cl.time

Made up of: clients, packet_entities, nails, and tents
===============
*/
void CL_EmitEntities (void)
{
	if (cls.state != ca_active)
		return;
	if (!cl.validsequence)
		return;

	R_ClearScene();

	CL_LinkPlayers ();
	CL_LinkPacketEntities ();
	CL_LinkProjectiles ();
	CL_UpdateTEnts ();
	CL_LinkStaticEntities();
}

