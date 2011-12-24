// cl_ents.c -- entity parsing and management

#include "quakedef.h"

extern	Cvar*	cl_predict_players;
extern	Cvar*	cl_predict_players2;
extern	Cvar*	cl_solid_players;

static struct predicted_player {
	int flags;
	qboolean active;
	vec3_t origin;	// predicted origin
} predicted_players[HWMAX_CLIENTS];

#define	U_MODEL		(1<<16)
#define U_SOUND		(1<<17)
#define U_DRAWFLAGS (1<<18)
#define U_ABSLIGHT  (1<<19)

const char *parsedelta_strings[] =
{
	"U_ANGLE1",	//0
	"U_ANGLE3",	//1
	"U_SCALE",	//2
	"U_COLORMAP",//3
	"U_SKIN",	//4
	"U_EFFECTS",	//5
	"U_MODEL16",//6
	"U_MOREBITS2",			//7 
	"",			//8 is unused
	"U_ORIGIN1",	//9
	"U_ORIGIN2",	//10
	"U_ORIGIN3",	//11
	"U_ANGLE2",	//12
	"U_FRAME",	//13
	"U_REMOVE",	//14
	"U_MOREBITS",//15
	"U_MODEL",//16
	"U_SOUND",//17
	"U_DRAWFLAGS",//18
	"U_ABSLIGHT"//19
};

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

static void ShowNetParseDelta(int x)
{
	int i;
	char orstring[2];

	if (cl_shownet->value != 2)
		return;

	orstring[0]=0;
	orstring[1]=0;

	Con_Printf("bits: ");
	for (i=0;i<=19;i++)
	{
		if (x != 8)
		{
			if (x & (1 << i))
			{
				Con_Printf("%s%s",orstring,parsedelta_strings[i]);
				orstring[0]= '|';
			}
		}
	}
	Con_Printf("\n");
}

int	bitcounts[32];	/// just for protocol profiling
void CL_ParseDelta (h2entity_state_t *from, h2entity_state_t *to, int bits)
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

	if(bits & U_MOREBITS2)
	{
		i =net_message.ReadByte ();
		bits |= (i << 16);
	}

	ShowNetParseDelta(bits);

	// count the bits for net profiling
	for (i=0 ; i<19 ; i++)
		if (bits&(1<<i))
			bitcounts[i]++;

	to->flags = bits;
	
	if (bits & U_MODEL)
	{
		if (bits & U_MODEL16)
		{
			to->modelindex = net_message.ReadShort ();
		}
		else
		{
			to->modelindex = net_message.ReadByte ();
		}
	}
		
	if (bits & U_FRAME)
		to->frame = net_message.ReadByte ();

	if (bits & U_COLORMAP)
		to->colormap = net_message.ReadByte();

	if (bits & U_SKIN)
	{
		to->skinnum = net_message.ReadByte();
	}

	if (bits & U_DRAWFLAGS)
		to->drawflags = net_message.ReadByte();

	if (bits & U_EFFECTS)
		to->effects = net_message.ReadLong();

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

	if (bits & U_SCALE)
	{
		to->scale = net_message.ReadByte();
	}
	if (bits & U_ABSLIGHT)
		to->abslight = net_message.ReadByte();
	if (bits & U_SOUND)
	{
		i = net_message.ReadShort();
		S_StartSound(to->origin, to->number, 1, cl.sound_precache[i], 1.0, 1.0);
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
	h2entity_state_t	olde, newe;

	Con_DPrintf ("FlushEntityPacket\n");

	Com_Memset(&olde, 0, sizeof(olde));

	cl.validsequence = 0;		// can't render a frame
	cl.frames[cls.netchan.incomingSequence&UPDATE_MASK].invalid = true;

	// read it all, but ignore it
	while (1)
	{
		word = (unsigned short)net_message.ReadShort ();
		if (net_message.badread)
		{	// something didn't parse right...
			Host_EndGame ("msg_badread in packetentities\n");
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
	hwpacket_entities_t	*oldp, *newp, dummy;
	int			oldindex, newindex;
	int			word, newnum, oldnum;
	qboolean	full;
	byte		from;

	newpacket = cls.netchan.incomingSequence&UPDATE_MASK;
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
		if (cls.netchan.outgoingSequence - oldpacket >= UPDATE_BACKUP-1)
		{	// we can't use this, it is too old
			FlushEntityPacket ();
			return;
		}
		cl.validsequence = cls.netchan.incomingSequence;
		oldp = &cl.frames[oldpacket&UPDATE_MASK].packet_entities;
	}
	else
	{	// this is a full update that we can start delta compressing from now
		oldp = &dummy;
		dummy.num_entities = 0;
		cl.validsequence = cls.netchan.incomingSequence;
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
			Host_EndGame ("msg_badread in packetentities\n");
			return;
		}

		if (!word)
		{
			while (oldindex < oldp->num_entities)
			{	// copy all the rest of the entities from the old packet
//Con_Printf ("copy %i\n", oldp->entities[oldindex].number);
				if (newindex >= HWMAX_PACKET_ENTITIES)
					Host_EndGame ("CL_ParsePacketEntities: newindex == HWMAX_PACKET_ENTITIES");
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
			if (newindex >= HWMAX_PACKET_ENTITIES)
				Host_EndGame ("CL_ParsePacketEntities: newindex == HWMAX_PACKET_ENTITIES");
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
			if (newindex >= HWMAX_PACKET_ENTITIES)
				Host_EndGame ("CL_ParsePacketEntities: newindex == HWMAX_PACKET_ENTITIES");
			CL_ParseDelta (&clh2_baselines[newnum], &newp->entities[newindex], word);
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


void HandleEffects(int effects, int number, refEntity_t *ent, vec3_t angles, vec3_t angleAdd, vec3_t oldOrg)
{
	int					rotateSet = 0;

	// Effect Flags
	if (effects & EF_BRIGHTFIELD)
	{
		// show that this guy is cool or something...
		CLH2_BrightFieldLight(number, ent->origin);
		CLHW_BrightFieldParticles(ent->origin);
	}
	if (effects & EF_DARKFIELD)
	{
		CLH2_DarkFieldParticles(ent->origin);
	}
	if (effects & EF_MUZZLEFLASH)
	{
		CLH2_MuzzleFlashLight(number, ent->origin, angles, true);
	}
	if (effects & EF_BRIGHTLIGHT)
	{			
		CLH2_BrightLight(number, ent->origin);
	}
	if (effects & EF_DIMLIGHT)
	{
		CLH2_DimLight(number, ent->origin);
	}
	if (effects & EF_LIGHT)
	{
		CLH2_Light(number, ent->origin);
	}


	if(effects & EF_POISON_GAS)
	{
		CLHW_UpdatePoisonGas(ent->origin, angles);
	}
	if(effects & EF_ACIDBLOB)
	{
		angleAdd[0] = 0;
		angleAdd[1] = 0;
		angleAdd[2] = 200 * cl.serverTimeFloat;

		rotateSet = 1;

		CLHW_UpdateAcidBlob(ent->origin, angles);
	}
	if(effects & EF_ONFIRE)
	{
		CLHW_UpdateOnFire(ent, angles, number);
	}
	if(effects & EF_POWERFLAMEBURN)
	{
		CLHW_UpdatePowerFlameBurn(ent, number);
	}
	if(effects & EF_ICESTORM_EFFECT)
	{
		CL_UpdateIceStorm(ent, number);
	}
	if (effects & EF_HAMMER_EFFECTS)
	{
		angleAdd[0] = 200 * cl.serverTimeFloat;
		angleAdd[1] = 0;
		angleAdd[2] = 0;

		rotateSet = 1;

		CL_UpdateHammer(ent, number);
	}
	if (effects & EF_BEETLE_EFFECTS)
	{
		CL_UpdateBug(ent);
	}
	if (effects & EF_DARKLIGHT)//EF_INVINC_CIRC)
	{
		CLHW_SuccubusInvincibleParticles(ent->origin);
	}

	if(effects & EF_UPDATESOUND)
	{
		S_UpdateSoundPos (number, 7, ent->origin);
	}


	if(!rotateSet)
	{
		angleAdd[0] = 0;
		angleAdd[1] = 0;
		angleAdd[2] = 0;
	}
}

/*
===============
CL_LinkPacketEntities

===============
*/
void CL_LinkPacketEntities (void)
{
	hwpacket_entities_t	*pack;
	h2entity_state_t		*s1, *s2;
	float				f;
	qhandle_t			model;
	vec3_t				old_origin;
	float				autorotate;
	int					i;
	int					pnum;

	pack = &cl.frames[cls.netchan.incomingSequence&UPDATE_MASK].packet_entities;
	hwpacket_entities_t* PrevPack = &cl.frames[(cls.netchan.incomingSequence - 1) & UPDATE_MASK].packet_entities;

	autorotate = AngleMod(100*cl.serverTimeFloat);

	f = 0;		// FIXME: no interpolation right now

	for (pnum=0 ; pnum<pack->num_entities ; pnum++)
	{
		s1 = &pack->entities[pnum];
		s2 = s1;	// FIXME: no interpolation right now

		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		// create a new entity
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));

		ent.reType = RT_MODEL;
		model = cl.model_precache[s1->modelindex];
		ent.hModel = model;
	
		// set skin
		ent.skinNum = s1->skinnum;
		
		// set frame
		ent.frame = s1->frame;

		int drawflags = s1->drawflags;

		vec3_t angles;
		// rotate binary objects locally
/*	rjr rotate them in renderer	if (model->flags & H2MDLEF_ROTATE)
		{
			ent->angles[0] = 0;
			ent->angles[1] = autorotate;
			ent->angles[2] = 0;
		}
		else*/
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

		// calculate origin
		for (i=0 ; i<3 ; i++)
			ent.origin[i] = s2->origin[i] + 
			f * (s1->origin[i] - s2->origin[i]);

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
			CLH2_SetRefEntAxis(&ent, angles, vec3_origin, s1->scale, s1->colormap, s1->abslight, drawflags);
			R_AddRefEntityToScene(&ent);
			continue;		// not in last message
		}

		for (i=0 ; i<3 ; i++)
			//if ( abs(old_origin[i] - ent->origin[i]) > 128)
			if ( abs(old_origin[i] - ent.origin[i]) > 512)	// this is an issue for laggy situations...
			{	// no trail if too far
				VectorCopy (ent.origin, old_origin);
				break;
			}

		// some of the effects need to know how far the thing has moved...

//		cl.h2_players[s1->number].invis=false;
		if(cl_siege)
			if((int)s1->effects & EF_NODRAW)
			{
				ent.skinNum = 101;//ice, but in siege will be invis skin for dwarf to see
				drawflags|=H2DRF_TRANSLUCENT;
				s1->effects &= ~EF_NODRAW;
//				cl.h2_players[s1->number].invis=true;
			}

		vec3_t angleAdd;
		HandleEffects(s1->effects, s1->number, &ent, angles, angleAdd, old_origin);
		CLH2_SetRefEntAxis(&ent, angles, angleAdd, s1->scale, s1->colormap, s1->abslight, drawflags);
		R_AddRefEntityToScene(&ent);

		// add automatic particle trails
		int ModelFlags = R_ModelFlags(ent.hModel);
		if (!ModelFlags)
			continue;

		// Model Flags
		if (ModelFlags & H2MDLEF_GIB)
			CLH2_TrailParticles (old_origin, ent.origin, rt_blood);
		else if (ModelFlags & H2MDLEF_ZOMGIB)
			CLH2_TrailParticles (old_origin, ent.origin, rt_slight_blood);
		else if (ModelFlags & H2MDLEF_TRACER)
			CLH2_TrailParticles (old_origin, ent.origin, rt_tracer);
		else if (ModelFlags & H2MDLEF_TRACER2)
			CLH2_TrailParticles (old_origin, ent.origin, rt_tracer2);
		else if (ModelFlags & H2MDLEF_ROCKET)
		{
			CLH2_TrailParticles (old_origin, ent.origin, rt_rocket_trail);
		}
		else if (ModelFlags & H2MDLEF_FIREBALL)
		{
			CLH2_TrailParticles (old_origin, ent.origin, rt_fireball);
			CLH2_FireBallLight(i, ent.origin);
		}
		else if (ModelFlags & H2MDLEF_ICE)
		{
			CLH2_TrailParticles (old_origin, ent.origin, rt_ice);
		}
		else if (ModelFlags & H2MDLEF_SPIT)
		{
			CLH2_TrailParticles (old_origin, ent.origin, rt_spit);
			CLH2_SpitLight(i, ent.origin);
		}
		else if (ModelFlags & H2MDLEF_SPELL)
		{
			CLH2_TrailParticles (old_origin, ent.origin, rt_spell);
		}
		else if (ModelFlags & H2MDLEF_GRENADE)
		{
			CLH2_TrailParticles (old_origin, ent.origin, rt_grensmoke);
		}
		else if (ModelFlags & H2MDLEF_TRACER3)
			CLH2_TrailParticles (old_origin, ent.origin, rt_voor_trail);
		else if (ModelFlags & H2MDLEF_VORP_MISSILE)
		{
			CLH2_TrailParticles (old_origin, ent.origin, rt_vorpal);
		}
		else if (ModelFlags & H2MDLEF_SET_STAFF)
		{
			CLH2_TrailParticles (old_origin, ent.origin,rt_setstaff);
		}
		else if (ModelFlags & H2MDLEF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
				CLH2_TrailParticles (old_origin, ent.origin, rt_magicmissile);
		}
		else if (ModelFlags & H2MDLEF_BONESHARD)
			CLH2_TrailParticles (old_origin, ent.origin, rt_boneshard);
		else if (ModelFlags & H2MDLEF_SCARAB)
			CLH2_TrailParticles (old_origin, ent.origin, rt_scarab);
		else if (ModelFlags & H2MDLEF_ACIDBALL)
			CLH2_TrailParticles (old_origin, ent.origin, rt_acidball);
		else if (ModelFlags & H2MDLEF_BLOODSHOT)
			CLH2_TrailParticles (old_origin, ent.origin, rt_bloodshot);
	}
}


/*
=========================================================================

PROJECTILE PARSING / LINKING
	changed for use with the powered ravens

=========================================================================
*/

typedef struct
{
	int		modelindex;
	vec3_t	origin;
	vec3_t	angles;
	int		frame;
} projectile_t;

#define	MAX_PROJECTILES	32
projectile_t	cl_projectiles[MAX_PROJECTILES];
int				cl_num_projectiles;

extern int cl_ravenindex, cl_raven2index;

void CL_ClearProjectiles (void)
{
	cl_num_projectiles = 0;
}

/*
=====================
CL_ParseProjectiles

Nails are passed as efficient temporary entities
Note: all references to these functions have been replaced with 
	calls to the Missile functions below
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

		pr->modelindex = cl_ravenindex;
		pr->origin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		pr->origin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		pr->origin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		pr->angles[0] = 360*(bits[4]>>4)/16;
		pr->angles[1] = 360*(bits[5]&31)/32;
		pr->frame = (bits[5]>>5) & 7;
	}

	c = net_message.ReadByte ();
	for (i=0 ; i<c ; i++)
	{
		for (j=0 ; j<6 ; j++)
			bits[j] = net_message.ReadByte ();

		if (cl_num_projectiles == MAX_PROJECTILES)
			continue;

		pr = &cl_projectiles[cl_num_projectiles];
		cl_num_projectiles++;

		pr->modelindex = cl_raven2index;
		pr->origin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		pr->origin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		pr->origin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		pr->angles[0] = 360*(bits[4]>>4)/16;
		pr->angles[1] = 360*bits[5]/256;
		pr->frame = 0;
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

		ent.hModel = cl.model_precache[pr->modelindex];
		ent.frame = pr->frame;
		VectorCopy(pr->origin, ent.origin);
		CLH2_SetRefEntAxis(&ent, pr->angles, vec3_origin, 0, 0, 0, 0);
		R_AddRefEntityToScene(&ent);
	}
}

/*
=========================================================================

MISSILE PROJECTILES

=========================================================================
*/

typedef struct
{
	int		modelindex;
	vec3_t	origin;
	int		type;
} missile_t;

#define	MAX_MISSILES 32
missile_t	cl_missiles[MAX_MISSILES];
int				cl_num_missiles;

extern int cl_ballindex, cl_missilestarindex;

void CL_ClearMissiles (void)
{
	cl_num_missiles = 0;
}

/*
=====================
CL_ParseProjectiles

Missiles are passed as efficient temporary entities
=====================
*/
void CL_ParsePackMissiles (void)
{
	int		i, c, j;
	byte	bits[5];
	missile_t	*pr;

	c = net_message.ReadByte ();
	for (i=0 ; i<c ; i++)
	{
		for (j=0 ; j<5 ; j++)
			bits[j] = net_message.ReadByte ();

		if (cl_num_missiles == MAX_MISSILES)
			continue;

		pr = &cl_missiles[cl_num_missiles];
		cl_num_missiles++;

		pr->origin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		pr->origin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		pr->origin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		pr->type = bits[4]>>4;
		//type may be used later to select models
	}
}

/*
=============
CL_LinkMissile

=============
*/

vec3_t missilestar_angle; 

void CL_LinkMissiles (void)
{
	int		i;
	missile_t	*pr;

	missilestar_angle[1] += host_frametime * 300; 
	missilestar_angle[2] += host_frametime * 400; 

	for (i=0, pr=cl_missiles ; i<cl_num_missiles ; i++, pr++)
	{
		// grab an entity to fill in for missile itself
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		VectorCopy (pr->origin, ent.origin);
		if(pr->type == 1)
		{	//ball
			ent.hModel = cl.model_precache[cl_ballindex];
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 10, 0, 0, H2SCALE_ORIGIN_CENTER);
		}
		else
		{	//missilestar
			ent.hModel = cl.model_precache[cl_missilestarindex];
			CLH2_SetRefEntAxis(&ent, missilestar_angle, vec3_origin, 50, 0, 0, H2SCALE_ORIGIN_CENTER);
		}
		if(rand() % 10 < 3)		
		{
			CLH2_RunParticleEffect4 (ent.origin, 7, 148 + rand() % 11, pt_h2grav, 10 + rand() % 10);
		}
		R_AddRefEntityToScene(&ent);
	}
}

//========================================

extern	int		cl_spikeindex, cl_playerindex[MAX_PLAYER_CLASS], cl_flagindex;

/*
===================
CL_ParsePlayerinfo
===================
*/
extern int parsecountmod;
extern double parsecounttime;
void CL_SavePlayer (void)
{
	int			num;
	h2player_info_t	*info;
	hwplayer_state_t	*state;

	num = net_message.ReadByte ();

	if (num > HWMAX_CLIENTS)
		Sys_Error ("CL_ParsePlayerinfo: bad num");

	info = &cl.h2_players[num];
	state = &cl.frames[parsecountmod].playerstate[num];
	
	state->messagenum = cl.parsecount;
	state->state_time = parsecounttime;
}

void CL_ParsePlayerinfo (void)
{
	int			msec;
	int			flags;
	h2player_info_t	*info;
	hwplayer_state_t	*state;
	int			num;
	int			i;
	qboolean	playermodel = false;

	num = net_message.ReadByte ();
	if (num > HWMAX_CLIENTS)
		Sys_Error ("CL_ParsePlayerinfo: bad num");

	info = &cl.h2_players[num];

	state = &cl.frames[parsecountmod].playerstate[num];

	flags = state->flags = net_message.ReadShort ();

	state->messagenum = cl.parsecount;
	state->origin[0] = net_message.ReadCoord ();
	state->origin[1] = net_message.ReadCoord ();
	state->origin[2] = net_message.ReadCoord ();
	VectorCopy(state->origin, info->origin);

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
		MSG_ReadUsercmd (&state->command, false);

	for (i=0 ; i<3 ; i++)
	{
		if (flags & (PF_VELOCITY1<<i) )
			state->velocity[i] = net_message.ReadShort();
		else
			state->velocity[i] = 0;
	}

	if (flags & PF_MODEL)
		state->modelindex = net_message.ReadShort ();
	else
	{
		playermodel = true;
		i = info->playerclass;
		if (i >= 1 && i <= MAX_PLAYER_CLASS)
		{
			state->modelindex = cl_playerindex[i-1];
		}
		else
		{
			state->modelindex = cl_playerindex[0];
		}
	}

	if (flags & PF_SKINNUM)
		state->skinnum = net_message.ReadByte ();
	else
	{
		if(info->siege_team==ST_ATTACKER&&playermodel)
			state->skinnum = 1;//using a playermodel and attacker - skin is set to 1
		else
			state->skinnum = 0;
	}

	if (flags & PF_EFFECTS)
		state->effects = net_message.ReadByte ();
	else
		state->effects = 0;

	if (flags & PF_EFFECTS2)
		state->effects |= (net_message.ReadByte() << 8);
	else
		state->effects &= 0xff;

	if (flags & PF_WEAPONFRAME)
		state->weaponframe = net_message.ReadByte ();
	else
		state->weaponframe = 0;

	if (flags & PF_DRAWFLAGS)
	{
		state->drawflags = net_message.ReadByte();
	}
	else
	{
		state->drawflags = 0;
	}

	if (flags & PF_SCALE)
	{
		state->scale = net_message.ReadByte();
	}
	else
	{
		state->scale = 0;
	}

	if (flags & PF_ABSLIGHT)
	{
		state->abslight = net_message.ReadByte();
	}
	else
	{
		state->abslight = 0;
	}
	
	if(flags & PF_SOUND)
	{
		i = net_message.ReadShort ();
		S_StartSound(state->origin, num, 1, cl.sound_precache[i], 1.0, 1.0);
	}
	
	VectorCopy (state->command.angles, state->viewangles);
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
	h2player_info_t	*info;
	hwplayer_state_t	*state;
	hwplayer_state_t	exact;
	double			playertime;
	int				msec;
	frame_t			*frame;
	int				oldphysent;

	playertime = realtime - cls.latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];

	for (j=0, info=cl.h2_players, state=frame->playerstate ; j < HWMAX_CLIENTS 
		; j++, info++, state++)
	{
		info->shownames_off = true;
		if (state->messagenum != cl.parsecount)
			continue;	// not present this frame

		if (!state->modelindex)
			continue;

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

		ent.hModel = cl.model_precache[state->modelindex];
		ent.skinNum = state->skinnum;
		ent.frame = state->frame;

		int drawflags = state->drawflags;
		if (ent.hModel == clh2_player_models[0] ||
			ent.hModel == clh2_player_models[1] ||
			ent.hModel == clh2_player_models[2] ||
			ent.hModel == clh2_player_models[3] ||
			ent.hModel == clh2_player_models[4] ||//mg-siege
			ent.hModel == clh2_player_models[5])
		{
			// use custom skin
			info->shownames_off = false;
		}

		//
		// angles
		//
		vec3_t angles;
		angles[PITCH] = -state->viewangles[PITCH]/3;
		angles[YAW] = state->viewangles[YAW];
		angles[ROLL] = 0;
		angles[ROLL] = V_CalcRoll(angles, state->velocity)*4;

		// only predict half the move to minimize overruns
		msec = 500*(playertime - state->state_time);
		if (msec <= 0 || (!cl_predict_players->value && !cl_predict_players2->value) || j == cl.playernum)
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

		if(cl_siege)
			if((int)state->effects & EF_NODRAW)
			{
				ent.skinNum = 101;//ice, but in siege will be invis skin for dwarf to see
				drawflags|=H2DRF_TRANSLUCENT;
				state->effects &= ~EF_NODRAW;
			}

		vec3_t angleAdd;
		HandleEffects(state->effects, j+1, &ent, angles, angleAdd, NULL);

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
			if ((ambientlight + shadelight) > 75 || (cl_siege && my_team == ve_team))
			{
				info->shownames_off = false;
			}
			else
			{
				info->shownames_off = true;
			}
			if (cl_siege)
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
				else if (ve_team == ST_DEFENDER)
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
						colorshade = cl_teamcolor->value;
					}
				}
			}
		}

		CLH2_SetRefEntAxis(&ent, angles, angleAdd, state->scale, colorshade, state->abslight, drawflags);
		CLH2_HandleCustomSkin(&ent, j);
		R_AddRefEntityToScene(&ent);
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
	hwpacket_entities_t	*pak;
	h2entity_state_t		*state;

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
		VectorCopy(state->angles, pmove.physents[pmove.numphysent].angles);
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

	hwplayer_state_t	*state;
	hwplayer_state_t	exact;
	double			playertime;
	int				msec;
	frame_t			*frame;
	struct predicted_player *pplayer;

	playertime = realtime - cls.latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];

	for (j=0, pplayer = predicted_players, state=frame->playerstate; 
		j < HWMAX_CLIENTS;
		j++, pplayer++, state++) 
	{

		pplayer->active = false;

		if (state->messagenum != cl.parsecount)
			continue;	// not present this frame

		if (!state->modelindex)
			continue;

		pplayer->active = true;
		pplayer->flags = state->flags;

		// note that the local player is special, since he moves locally
		// we use his last predicted postition
		if (j == cl.playernum) 
		{
			VectorCopy(cl.frames[cls.netchan.outgoingSequence&UPDATE_MASK].playerstate[cl.playernum].origin,
				pplayer->origin);
		} 
		else 
		{
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
	extern	vec3_t	player_maxs_crouch;
	struct predicted_player *pplayer;
	physent_t *pent;

	if (!cl_solid_players->value)
		return;

	pent = pmove.physents + pmove.numphysent;

	for (j=0, pplayer = predicted_players; j < HWMAX_CLIENTS;	j++, pplayer++) 
	{

		if (!pplayer->active)
			continue;	// not present this frame

		// the player object never gets added
		if (j == playernum)
			continue;

		if (pplayer->flags & PF_DEAD)
			continue; // dead players aren't solid

		pent->model = -1;
		VectorCopy(pplayer->origin, pent->origin);
/*shitbox
		if(!String::ICmp(cl.model_precache[cl.h2_players[playernum].modelindex]->name,"models/yakman.mdl"))
		{//use golem hull
			Sys_Error("Using beast model");
			VectorCopy(beast_mins, pent->mins);
			VectorCopy(beast_maxs, pent->mins);
		}
		else
		{*/
			VectorCopy(player_mins, pent->mins);
			if(pplayer->flags & PF_CROUCH)
			{
				VectorCopy(player_maxs_crouch, pent->maxs);
			}
			else
			{
				VectorCopy(player_maxs, pent->maxs);
			}
//		}
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
		VectorCopy(pent->state.origin, rent.origin);
		rent.hModel = cl.model_precache[pent->state.modelindex];
		rent.frame = pent->state.frame;
		rent.skinNum = pent->state.skinnum;
		rent.shaderTime = pent->syncbase;
		CLH2_SetRefEntAxis(&rent, pent->state.angles, vec3_origin, pent->state.scale, 0, pent->state.abslight, pent->state.drawflags);
		CLH2_HandleCustomSkin(&rent, -1);
		R_AddRefEntityToScene(&rent);
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
	CL_LinkMissiles();
	CL_UpdateTEnts ();
	CL_LinkStaticEntities();
}

