// cl_ents.c -- entity parsing and management

#include "quakedef.h"

extern	QCvar*	cl_predict_players;
extern	QCvar*	cl_predict_players2;
extern	QCvar*	cl_solid_players;

extern qhandle_t	player_models[MAX_PLAYER_CLASS];

static struct predicted_player {
	int flags;
	qboolean active;
	vec3_t origin;	// predicted origin
} predicted_players[MAX_CLIENTS];

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

//============================================================

/*
===============
CL_AllocDlight

===============
*/
cdlight_t *CL_AllocDlight (int key)
{
	int		i;
	cdlight_t	*dl;

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
	cdlight_t	*dl;

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
	cdlight_t	*dl;

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;
		
		if (dl->radius > 0)
		{
			dl->radius -= host_frametime*dl->decay;
			if (dl->radius < 0)
				dl->radius = 0;
		}
		else
		{
			dl->radius += host_frametime*dl->decay;
			if (dl->radius > 0)
				dl->radius = 0;
		}
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
			Host_EndGame ("msg_badread in packetentities\n");
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


void HandleEffects(int effects, int number, refEntity_t *ent, vec3_t angles, vec3_t angleAdd, vec3_t oldOrg)
{
	cdlight_t			*dl;
	int					rotateSet = 0;

	// Effect Flags
	if (effects & EF_BRIGHTFIELD)
	{	// show that this guy is cool or something...
		//R_EntityParticles (ent);
		dl = CL_AllocDlight (number);
		//dl->color[0] = .5 + cos(cl.time*7)*.5;
		//dl->color[1] = .5 + cos(cl.time*7 + M_PI*2/3)*.5;
		//dl->color[2] = .5 + cos(cl.time*7 + M_PI*4/3)*.5;
		//dl->color[3] = 1.0;
		VectorCopy (ent->origin,  dl->origin);
		dl->radius = 200 + cos(cl.time*5)*100;
		dl->die = cl.time + 0.001;

		R_BrightFieldSource (ent->origin);
	}
	if (effects & EF_DARKFIELD)
		R_DarkFieldParticles (ent);
	if (effects & EF_MUZZLEFLASH)
	{
		dl = CL_AllocDlight (number);
		VectorCopy (ent->origin,  dl->origin);
		dl->origin[2] += 16;
		 
		VectorMA(dl->origin, 18, ent->axis[0], dl->origin);
		dl->radius = 200 + (rand()&31);
		dl->minlight = 32;
		dl->die = cl.time + 0.1;
	}
	if (effects & EF_BRIGHTLIGHT)
	{			
		dl = CL_AllocDlight (number);
		VectorCopy (ent->origin,  dl->origin);
		dl->origin[2] += 16;
		dl->radius = 400 + (rand()&31);
		dl->die = cl.time + 0.001;
	}
	if (effects & EF_DIMLIGHT)
	{			
		dl = CL_AllocDlight (number);
		VectorCopy (ent->origin,  dl->origin);
		dl->radius = 200 + (rand()&31);
		dl->die = cl.time + 0.001;
	}
/*	if (effects & EF_DARKLIGHT)
	{			
		dl = CL_AllocDlight (number);
		VectorCopy (ent->origin,  dl->origin);
		dl->radius = 200.0 + (rand()&31);
		dl->die = cl.time + 0.001;
//rjr		dl->dark = true;
	}*/
	if (effects & EF_LIGHT)
	{			
		dl = CL_AllocDlight (number);
		VectorCopy (ent->origin,  dl->origin);
		dl->radius = 200;
		dl->die = cl.time + 0.001;
	}


	if(effects & EF_POISON_GAS)
	{
		CL_UpdatePoisonGas(ent, angles, number);
	}
	if(effects & EF_ACIDBLOB)
	{
		angleAdd[0] = 0;
		angleAdd[1] = 0;
		angleAdd[2] = 200 * cl.time;

		rotateSet = 1;

		CL_UpdateAcidBlob(ent, angles, number);
	}
	if(effects & EF_ONFIRE)
	{
		CL_UpdateOnFire(ent, angles, number);
	}
	if(effects & EF_POWERFLAMEBURN)
	{
		CL_UpdatePowerFlameBurn(ent, number);
	}
	if(effects & EF_ICESTORM_EFFECT)
	{
		CL_UpdateIceStorm(ent, number);
	}
	if (effects & EF_HAMMER_EFFECTS)
	{
		angleAdd[0] = 200 * cl.time;
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
		R_SuccubusInvincibleParticles (ent);
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
	packet_entities_t	*pack;
	entity_state_t		*s1, *s2;
	float				f;
	qhandle_t			model;
	vec3_t				old_origin;
	float				autorotate;
	int					i;
	int					pnum;
	cdlight_t			*dl;

	pack = &cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities;
	packet_entities_t* PrevPack = &cl.frames[(cls.netchan.incoming_sequence - 1) & UPDATE_MASK].packet_entities;

	autorotate = AngleMod(100*cl.time);

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
			CL_SetRefEntAxis(&ent, angles, vec3_origin, s1->scale, s1->colormap, s1->abslight, drawflags);
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

//		cl.players[s1->number].invis=false;
		if(cl_siege)
			if((int)s1->effects & EF_NODRAW)
			{
				ent.skinNum = 101;//ice, but in siege will be invis skin for dwarf to see
				drawflags|=DRF_TRANSLUCENT;
				s1->effects &= ~EF_NODRAW;
//				cl.players[s1->number].invis=true;
			}

		vec3_t angleAdd;
		HandleEffects(s1->effects, s1->number, &ent, angles, angleAdd, old_origin);
		CL_SetRefEntAxis(&ent, angles, angleAdd, s1->scale, s1->colormap, s1->abslight, drawflags);
		R_AddRefEntityToScene(&ent);

		// add automatic particle trails
		int ModelFlags = R_ModelFlags(ent.hModel);
		if (!ModelFlags)
			continue;

		// Model Flags
		if (ModelFlags & H2MDLEF_GIB)
			R_RocketTrail (old_origin, ent.origin, 2);
		else if (ModelFlags & H2MDLEF_ZOMGIB)
			R_RocketTrail (old_origin, ent.origin, 4);
		else if (ModelFlags & H2MDLEF_TRACER)
			R_RocketTrail (old_origin, ent.origin, 3);
		else if (ModelFlags & H2MDLEF_TRACER2)
			R_RocketTrail (old_origin, ent.origin, 5);
		else if (ModelFlags & H2MDLEF_ROCKET)
		{
			R_RocketTrail (old_origin, ent.origin, 0);
/*			dl = CL_AllocDlight (i);
			VectorCopy (ent->origin, dl->origin);
			dl->radius = 200;
			dl->die = cl.time + 0.01;*/
		}
		else if (ModelFlags & H2MDLEF_FIREBALL)
		{
			R_RocketTrail (old_origin, ent.origin, rt_fireball);
			dl = CL_AllocDlight (i);
			VectorCopy (ent.origin, dl->origin);
			dl->radius = 120 - (rand() % 20);
			dl->die = cl.time + 0.01;
		}
		else if (ModelFlags & H2MDLEF_ICE)
		{
			R_RocketTrail (old_origin, ent.origin, rt_ice);
		}
		else if (ModelFlags & H2MDLEF_SPIT)
		{
			R_RocketTrail (old_origin, ent.origin, rt_spit);
			dl = CL_AllocDlight (i);
			VectorCopy (ent.origin, dl->origin);
			dl->radius = -120 - (rand() % 20);
			dl->die = cl.time + 0.05;
		}
		else if (ModelFlags & H2MDLEF_SPELL)
		{
			R_RocketTrail (old_origin, ent.origin, rt_spell);
		}
		else if (ModelFlags & H2MDLEF_GRENADE)
		{
//			R_RunParticleEffect4(old_origin,3,284,pt_slowgrav,3);
			R_RocketTrail (old_origin, ent.origin, rt_grensmoke);
		}
		else if (ModelFlags & H2MDLEF_TRACER3)
			R_RocketTrail (old_origin, ent.origin, 6);
		else if (ModelFlags & H2MDLEF_VORP_MISSILE)
		{
			R_RocketTrail (old_origin, ent.origin, rt_vorpal);
		}
		else if (ModelFlags & H2MDLEF_SET_STAFF)
		{
			R_RocketTrail (old_origin, ent.origin,rt_setstaff);
		}
		else if (ModelFlags & H2MDLEF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
				R_RocketTrail (old_origin, ent.origin, rt_magicmissile);
		}
		else if (ModelFlags & H2MDLEF_BONESHARD)
			R_RocketTrail (old_origin, ent.origin, rt_boneshard);
		else if (ModelFlags & H2MDLEF_SCARAB)
			R_RocketTrail (old_origin, ent.origin, rt_scarab);
		else if (ModelFlags & H2MDLEF_ACIDBALL)
			R_RocketTrail (old_origin, ent.origin, rt_acidball);
		else if (ModelFlags & H2MDLEF_BLOODSHOT)
			R_RocketTrail (old_origin, ent.origin, rt_bloodshot);
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
		CL_SetRefEntAxis(&ent, pr->angles, vec3_origin, 0, 0, 0, 0);
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
			CL_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 10, 0, 0, SCALE_ORIGIN_CENTER);
		}
		else
		{	//missilestar
			ent.hModel = cl.model_precache[cl_missilestarindex];
			CL_SetRefEntAxis(&ent, missilestar_angle, vec3_origin, 50, 0, 0, SCALE_ORIGIN_CENTER);
		}
		if(rand() % 10 < 3)		
		{
			R_RunParticleEffect4 (ent.origin, 7, 148 + rand() % 11, pt_grav, 10 + rand() % 10);
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
	player_info_t	*info;
	player_state_t	*state;

	num = net_message.ReadByte ();

	if (num > MAX_CLIENTS)
		Sys_Error ("CL_ParsePlayerinfo: bad num");

	info = &cl.players[num];
	state = &cl.frames[parsecountmod].playerstate[num];
	
	state->messagenum = cl.parsecount;
	state->state_time = parsecounttime;
}

void CL_ParsePlayerinfo (void)
{
	int			msec;
	int			flags;
	player_info_t	*info;
	player_state_t	*state;
	int			num;
	int			i;
	qboolean	playermodel = false;

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

void R_HandleCustomSkin(refEntity_t* Ent, int PlayerNum)
{
	if (Ent->skinNum >= 100)
	{
		if (Ent->skinNum > 255) 
		{
			Sys_Error("skinnum > 255");
		}

		if (!gl_extra_textures[Ent->skinNum - 100])  // Need to load it in
		{
			char temp[80];
			String::Sprintf(temp, sizeof(temp), "gfx/skin%d.lmp", Ent->skinNum);
			gl_extra_textures[Ent->skinNum - 100] = R_CachePic(temp);
		}

		Ent->customSkin = R_GetImageHandle(gl_extra_textures[Ent->skinNum - 100]);
	}
	else if (PlayerNum >= 0)
	{
		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
		//FIXME? What about Demoness and Dwarf?
		if (Ent->hModel == player_models[0] ||
			Ent->hModel == player_models[1] ||
			Ent->hModel == player_models[2] ||
			Ent->hModel == player_models[3])
		{
			if (!cl.players[PlayerNum].Translated)
			{
				R_TranslatePlayerSkin(PlayerNum);
			}

			Ent->customSkin = R_GetImageHandle(playertextures[PlayerNum]);
		}
	}
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
		info->shownames_off = true;
		if (state->messagenum != cl.parsecount)
			continue;	// not present this frame

		if (!state->modelindex)
			continue;

		cl.players[j].modelindex = state->modelindex;

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
		if (ent.hModel == player_models[0] ||
			ent.hModel == player_models[1] ||
			ent.hModel == player_models[2] ||
			ent.hModel == player_models[3] ||
			ent.hModel == player_models[4] ||//mg-siege
			ent.hModel == player_models[5])
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
				drawflags|=DRF_TRANSLUCENT;
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
			int my_team = cl.players[cl.playernum].siege_team;
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
				if (cl.players[cl.playernum].playerclass == CLASS_DWARF && ent.skinNum == 101)
				{
					colorshade = 141;
					info->shownames_off = false;
				}
				else if (cl.players[cl.playernum].playerclass == CLASS_DWARF && (ambientlight + shadelight) < 151)
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
				String::NCpy(client_team, Info_ValueForKey(cl.players[cl.playernum].userinfo, "team"), 16);
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

		CL_SetRefEntAxis(&ent, angles, angleAdd, state->scale, colorshade, state->abslight, drawflags);
		R_HandleCustomSkin(&ent, j);
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
			VectorCopy(cl.frames[cls.netchan.outgoing_sequence&UPDATE_MASK].playerstate[cl.playernum].origin,
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

	for (j=0, pplayer = predicted_players; j < MAX_CLIENTS;	j++, pplayer++) 
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
		if(!String::ICmp(cl.model_precache[cl.players[playernum].modelindex]->name,"models/yakman.mdl"))
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
		VectorCopy(pent->origin, rent.origin);
		rent.hModel = pent->model;
		rent.frame = pent->frame;
		rent.skinNum = pent->skinnum;
		rent.shaderTime = pent->syncbase;
		CL_SetRefEntAxis(&rent, pent->angles, pent->angleAdd, pent->scale, pent->colorshade, pent->abslight, pent->drawflags);
		R_HandleCustomSkin(&rent, -1);
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

