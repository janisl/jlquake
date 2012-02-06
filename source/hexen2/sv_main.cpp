// sv_main.c -- server main program

/*
 * $Header: /H2 Mission Pack/SV_MAIN.C 26    3/20/98 12:12a Jmonroe $
 */

#include "quakedef.h"
#include "../core/file_formats/bsp29.h"

server_t		sv;
server_static_t	svs;
char	localmodels[MAX_MODELS_H2][5];			// inline model names for precache

Cvar*	sv_sound_distance;

Cvar*	sv_update_player;
Cvar*	sv_update_monsters;
Cvar*	sv_update_missiles;
Cvar*	sv_update_misc;

Cvar*	sv_ce_scale;
Cvar*	sv_ce_max_size;

extern unsigned int	info_mask, info_mask2;
int		sv_kingofhill;
qboolean	intro_playing = false;
qboolean	skip_start = false;
int			num_intro_msg = 0;

//============================================================================

void Sv_Edicts_f(void);

/*
===============
SV_Init
===============
*/
void SV_Init (void)
{
	int		i;
	extern	Cvar*	sv_maxvelocity;
	extern	Cvar*	sv_gravity;
	extern	Cvar*	sv_nostep;
	extern	Cvar*	sv_friction;
	extern	Cvar*	sv_edgefriction;
	extern	Cvar*	sv_stopspeed;
	extern	Cvar*	sv_maxspeed;
	extern	Cvar*	sv_accelerate;
	extern	Cvar*	sv_idealpitchscale;
	extern	Cvar*	sv_idealrollscale;
	extern	Cvar*	sv_aim;
	extern	Cvar*	sv_walkpitch;
	extern	Cvar*	sv_flypitch;

	sv_maxvelocity = Cvar_Get("sv_maxvelocity", "2000", 0);
	sv_gravity = Cvar_Get("sv_gravity", "800", CVAR_SERVERINFO);
	sv_friction = Cvar_Get("sv_friction", "4", CVAR_SERVERINFO);
	sv_edgefriction = Cvar_Get("edgefriction", "2", 0);
	sv_stopspeed = Cvar_Get("sv_stopspeed", "100", 0);
	sv_maxspeed = Cvar_Get("sv_maxspeed", "640", CVAR_SERVERINFO);
	sv_accelerate = Cvar_Get("sv_accelerate", "10", 0);
	sv_idealpitchscale = Cvar_Get("sv_idealpitchscale","0.8", 0);
	sv_idealrollscale = Cvar_Get("sv_idealrollscale","0.8", 0);
	sv_aim = Cvar_Get("sv_aim", "0.93", 0);
	sv_nostep = Cvar_Get("sv_nostep", "0", 0);
	sv_walkpitch = Cvar_Get("sv_walkpitch", "0", 0);
	sv_flypitch = Cvar_Get("sv_flypitch", "20", 0);
	sv_sound_distance = Cvar_Get("sv_sound_distance","800", CVAR_ARCHIVE);
	sv_update_player	= Cvar_Get("sv_update_player","1", CVAR_ARCHIVE);
	sv_update_monsters	= Cvar_Get("sv_update_monsters","1", CVAR_ARCHIVE);
	sv_update_missiles	= Cvar_Get("sv_update_missiles","1", CVAR_ARCHIVE);
	sv_update_misc		= Cvar_Get("sv_update_misc","1", CVAR_ARCHIVE);
	sv_ce_scale			= Cvar_Get("sv_ce_scale","0", CVAR_ARCHIVE);
	sv_ce_max_size		= Cvar_Get("sv_ce_max_size","0", CVAR_ARCHIVE);

	Cmd_AddCommand ("sv_edicts", Sv_Edicts_f);	

	for (i=0 ; i<MAX_MODELS_H2 ; i++)
		sprintf (localmodels[i], "*%i", i);

	sv_kingofhill=0;//Initialize King of Hill to world
}

void SV_Edicts(const char *Name)
{
	fileHandle_t FH;
	int i;
	qhedict_t *e;

	FH = FS_FOpenFileWrite(Name);
	if (!FH)
	{
		Con_Printf("Could not open %s\n",Name);
		return;
	}

	FS_Printf(FH,"Number of Edicts: %d\n",sv.num_edicts);
	FS_Printf(FH,"Server Time: %f\n",sv.time);
	FS_Printf(FH,"\n");
	FS_Printf(FH,"Num.     Time Class Name                     Model                          Think                                    Touch                                    Use\n");
	FS_Printf(FH,"---- -------- ------------------------------ ------------------------------ ---------------------------------------- ---------------------------------------- ----------------------------------------\n");

	for ( i=1 ; i<sv.num_edicts ; i++)
	{
		e = EDICT_NUM(i);
		FS_Printf(FH,"%3d. %8.2f %-30s %-30s %-40s %-40s %-40s\n",
			i,e->GetNextThink(),PR_GetString(e->GetClassName()),PR_GetString(e->GetModel()),
			PR_GetString(pr_functions[e->GetThink()].s_name),PR_GetString(pr_functions[e->GetTouch()].s_name),
			PR_GetString(pr_functions[e->GetUse()].s_name));
	}
	FS_FCloseFile(FH);
}

void Sv_Edicts_f(void)
{
	const char *Name;

	if (!sv.active)
	{
		Con_Printf("This command can only be executed on a server running a map\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Name = "edicts.txt";
	}
	else 
	{
		Name = Cmd_Argv(1);
	}

	SV_Edicts(Name);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*  
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count)
{
	int		i, v;

	if (sv.datagram.cursize > MAX_DATAGRAM_H2-16)
		return;	
	sv.datagram.WriteByte(h2svc_particle);
	sv.datagram.WriteCoord(org[0]);
	sv.datagram.WriteCoord(org[1]);
	sv.datagram.WriteCoord(org[2]);
	for (i=0 ; i<3 ; i++)
	{
		v = dir[i]*16;
		if (v > 127)
			v = 127;
		else if (v < -128)
			v = -128;
		sv.datagram.WriteChar(v);
	}
	sv.datagram.WriteByte(count);
	sv.datagram.WriteByte(color);
}           

/*  
==================
SV_StartParticle2

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count)
{
	if (sv.datagram.cursize > MAX_DATAGRAM_H2-36)
		return;	
	sv.datagram.WriteByte(h2svc_particle2);
	sv.datagram.WriteCoord(org[0]);
	sv.datagram.WriteCoord(org[1]);
	sv.datagram.WriteCoord(org[2]);
	sv.datagram.WriteFloat(dmin[0]);
	sv.datagram.WriteFloat(dmin[1]);
	sv.datagram.WriteFloat(dmin[2]);
	sv.datagram.WriteFloat(dmax[0]);
	sv.datagram.WriteFloat(dmax[1]);
	sv.datagram.WriteFloat(dmax[2]);

	sv.datagram.WriteShort(color);
	sv.datagram.WriteByte(count);
	sv.datagram.WriteByte(effect);
}           

/*  
==================
SV_StartParticle3

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle3 (vec3_t org, vec3_t box, int color, int effect, int count)
{
	if (sv.datagram.cursize > MAX_DATAGRAM_H2-15)
		return;	
	sv.datagram.WriteByte(h2svc_particle3);
	sv.datagram.WriteCoord(org[0]);
	sv.datagram.WriteCoord(org[1]);
	sv.datagram.WriteCoord(org[2]);
	sv.datagram.WriteByte(box[0]);
	sv.datagram.WriteByte(box[1]);
	sv.datagram.WriteByte(box[2]);

	sv.datagram.WriteShort(color);
	sv.datagram.WriteByte(count);
	sv.datagram.WriteByte(effect);
}           

/*  
==================
SV_StartParticle4

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle4 (vec3_t org, float radius, int color, int effect, int count)
{
	if (sv.datagram.cursize > MAX_DATAGRAM_H2-13)
		return;	
	sv.datagram.WriteByte(h2svc_particle4);
	sv.datagram.WriteCoord(org[0]);
	sv.datagram.WriteCoord(org[1]);
	sv.datagram.WriteCoord(org[2]);
	sv.datagram.WriteByte(radius);

	sv.datagram.WriteShort(color);
	sv.datagram.WriteByte(count);
	sv.datagram.WriteByte(effect);
}           

/*  
==================
SV_StopSound
==================
*/
void SV_StopSound (qhedict_t *entity, int channel)
{
	int			ent;

	if (sv.datagram.cursize > MAX_DATAGRAM_H2-4)
		return;	

	ent = NUM_FOR_EDICT(entity);
	channel = (ent<<3) | channel;

	sv.datagram.WriteByte(h2svc_stopsound);
	sv.datagram.WriteShort(channel);
}

/*  
==================
SV_UpdateSoundPos
==================
*/
void SV_UpdateSoundPos (qhedict_t *entity, int channel)
{
	int			ent;
    int			i;

	if (sv.datagram.cursize > MAX_DATAGRAM_H2-4)
		return;	

	ent = NUM_FOR_EDICT(entity);
	channel = (ent<<3) | channel;

	sv.datagram.WriteByte(h2svc_sound_update_pos);
	sv.datagram.WriteShort(channel);
	for (i=0 ; i<3 ; i++)
		sv.datagram.WriteCoord(entity->GetOrigin()[i]+0.5*(entity->GetMins()[i]+entity->GetMaxs()[i]));
}

/*  
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/  
void SV_StartSound (qhedict_t *entity, int channel, const char *sample, int volume,
    float attenuation)
{       
    int         sound_num;
    int			field_mask;
    int			i;
	int			ent;
	QMsg	   cm;
	byte		datagram_buf[MAX_DATAGRAM_H2];

	cm.InitOOB(datagram_buf, sizeof(datagram_buf));
	
	if (String::ICmp(sample,"misc/null.wav") == 0)
	{
		SV_StopSound(entity,channel);
		return;
	}

	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

	if (sv.datagram.cursize > MAX_DATAGRAM_H2-16)
		return;	

// find precache number for sound
    for (sound_num=1 ; sound_num<MAX_SOUNDS_H2
        && sv.sound_precache[sound_num] ; sound_num++)
        if (!String::Cmp(sample, sv.sound_precache[sound_num]))
            break;
    
    if ( sound_num == MAX_SOUNDS_H2 || !sv.sound_precache[sound_num] )
    {
        Con_Printf ("SV_StartSound: %s not precached\n", sample);
        return;
    }
    
	ent = NUM_FOR_EDICT(entity);

	channel = (ent<<3) | channel;

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;
	if (sound_num>255)
	{
		field_mask |= SND_ATTENUATION;
		sound_num -= 255;
	}

// directed messages go only to the entity the are targeted on
	sv.datagram.WriteByte(h2svc_sound);
	sv.datagram.WriteByte(field_mask);
	if (field_mask & SND_VOLUME)
		sv.datagram.WriteByte(volume);
	if (field_mask & SND_ATTENUATION)
		sv.datagram.WriteByte(attenuation*64);
	sv.datagram.WriteShort(channel);
	sv.datagram.WriteByte(sound_num);
	for (i=0 ; i<3 ; i++)
		sv.datagram.WriteCoord(entity->GetOrigin()[i]+0.5*(entity->GetMins()[i]+entity->GetMaxs()[i]));
}           

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo (client_t *client)
{
	const char			**s;
	char			message[2048];

	client->message.WriteByte(h2svc_print);
	sprintf (message, "%c\nVERSION %4.2f SERVER (%i CRC)", 2, HEXEN2_VERSION, pr_crc);
	client->message.WriteString2(message);

	client->message.WriteByte(h2svc_serverinfo);
	client->message.WriteLong(PROTOCOL_VERSION);
	client->message.WriteByte(svs.maxclients);

	if (!coop->value && deathmatch->value)
	{
		client->message.WriteByte(GAME_DEATHMATCH);
		client->message.WriteShort(sv_kingofhill);
	}
	else
		client->message.WriteByte(GAME_COOP);

	if (sv.edicts->GetMessage() > 0 && sv.edicts->GetMessage() <= pr_string_count)
	{
		client->message.WriteString2(&pr_global_strings[pr_string_index[(int)sv.edicts->GetMessage() - 1]]);
	}
	else
	{
//		client->message.WriteString2("");
		client->message.WriteString2(PR_GetString(sv.edicts->GetNetName()));
	}

	for (s = sv.model_precache+1 ; *s ; s++)
		client->message.WriteString2(*s);
	client->message.WriteByte(0);

	for (s = sv.sound_precache+1 ; *s ; s++)
		client->message.WriteString2(*s);
	client->message.WriteByte(0);

// send music
	client->message.WriteByte(h2svc_cdtrack);
//	client->message.WriteByte(sv.edicts->v.soundtype);
//	client->message.WriteByte(sv.edicts->v.soundtype);
	client->message.WriteByte(sv.cd_track);
	client->message.WriteByte(sv.cd_track);

	client->message.WriteByte(h2svc_midi_name);
	client->message.WriteString2(sv.midi_name);

// set view	
	client->message.WriteByte(h2svc_setview);
	client->message.WriteShort(NUM_FOR_EDICT(client->edict));

	client->message.WriteByte(h2svc_signonnum);
	client->message.WriteByte(1);

	client->sendsignon = true;
	client->spawned = false;		// need prespawn, spawn, etc
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient (int clientnum)
{
	qhedict_t			*ent;
	client_t		*client;
	int				edictnum;
	qsocket_t* netconnection;
	float			spawn_parms[NUM_SPAWN_PARMS];
	int				entnum;
	qhedict_t			*svent;

	client = svs.clients + clientnum;

	Con_DPrintf ("Client %s connected\n", client->netconnection->address);

	edictnum = clientnum+1;

	ent = EDICT_NUM(edictnum);
	
// set up the client_t
	netconnection = client->netconnection;
	netchan_t netchan = client->netchan;

	if (sv.loadgame)
		Com_Memcpy(spawn_parms, client->spawn_parms, sizeof(spawn_parms));
	Com_Memset(client, 0, sizeof(*client));
	client->send_all_v = true;
	client->netconnection = netconnection;
	client->netchan = netchan;

	String::Cpy(client->name, "unconnected");
	client->active = true;
	client->spawned = false;
	client->edict = ent;

	client->message.InitOOB(client->msgbuf, sizeof(client->msgbuf));
	client->message.allowoverflow = true;		// we can catch it

	client->datagram.InitOOB(client->datagram_buf, sizeof(client->datagram_buf));

	for (entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
		svent = EDICT_NUM(entnum);
//		Com_Memcpy(&svent->baseline[clientnum],&svent->baseline[MAX_BASELINES-1],sizeof(h2entity_state_t));
	}
	Com_Memset(&sv.states[clientnum],0,sizeof(client_state2_t ));

#ifdef IDGODS
	client->privileged = IsID(&client->netconnection->addr);
#else	
	client->privileged = false;				
#endif

	if (sv.loadgame)
		Com_Memcpy(client->spawn_parms, spawn_parms, sizeof(spawn_parms));
//	else
//	{
	// call the progs to get default spawn parms for the new client
	//	PR_ExecuteProgram (pr_global_struct->SetNewParms);
	//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
	//		client->spawn_parms[i] = (&pr_global_struct->parm1)[i];
//	}

	SV_SendServerinfo (client);
}


/*
===================
SV_CheckForNewClients

===================
*/
void SV_CheckForNewClients (void)
{
	qsocket_t* ret;
	int				i;
		
//
// check for new connections
//
	while (1)
	{
		netadr_t addr;
		ret = NET_CheckNewConnections (&addr);
		if (!ret)
			break;

	// 
	// init a new client structure
	//	
		for (i=0 ; i<svs.maxclients ; i++)
			if (!svs.clients[i].active)
				break;
		if (i == svs.maxclients)
			Sys_Error ("Host_CheckForNewClients: no free clients");
		
		Com_Memset(&svs.clients[i].netchan, 0, sizeof(svs.clients[i].netchan));
		svs.clients[i].netchan.sock = NS_SERVER;
		svs.clients[i].netchan.remoteAddress = addr;
		svs.clients[i].netchan.lastReceived = net_time * 1000;
		svs.clients[i].netconnection = ret;
		SV_ConnectClient (i);	
	
		net_activeconnections++;
	}
}



/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
==================
SV_ClearDatagram

==================
*/
void SV_ClearDatagram (void)
{
	sv.datagram.Clear();
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

static byte		fatpvs[BSP29_MAX_MAP_LEAFS/8];

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte *SV_FatPVS (vec3_t org)
{
	vec3_t mins, maxs;
	for (int i = 0; i < 3; i++)
	{
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	int leafs[64];
	int count = CM_BoxLeafnums(mins, maxs, leafs, 64);
	if (count < 1)
	{
		throw Exception("SV_FatPVS: count < 1");
	}

	// convert leafs to clusters
	for (int i = 0; i < count; i++)
	{
		leafs[i] = CM_LeafCluster(leafs[i]);
	}

	int fatbytes = (CM_NumClusters() + 31) >> 3;
	Com_Memcpy(fatpvs, CM_ClusterPVS(leafs[0]), fatbytes);
	// or in all the other leaf bits
	for (int i = 1; i < count; i++)
	{
		byte* pvs = CM_ClusterPVS(leafs[i]);
		for (int j = 0; j < fatbytes; j++)
		{
			fatpvs[j] |= pvs[j];
		}
	}
	return fatpvs;
}

//=============================================================================


/*
=============
SV_WriteEntitiesToClient

=============
*/

#define CLIENT_FRAME_INIT	255
#define CLIENT_FRAME_RESET	254

void SV_PrepareClientEntities (client_t *client, qhedict_t	*clent, QMsg *msg)
{
	int		e, i;
	int		bits;
	byte	*pvs;
	vec3_t	org;
	float	miss;
	qhedict_t	*ent;
	int		temp_index;
	char	NewName[MAX_QPATH];
	long	flagtest;
	int				position = 0;
	int				client_num;
	unsigned long   client_bit;
	client_frames_t *reference, *build;
	client_state2_t  *state;
	h2entity_state_t *ref_ent,*set_ent,build_ent;
	qboolean		FoundInList,DoRemove,DoPlayer,DoMonsters,DoMissiles,DoMisc,IgnoreEnt;
	short			RemoveList[MAX_CLIENT_STATES_H2],NumToRemove;


	client_num = client-svs.clients;
	client_bit = 1<<client_num;
	state = &sv.states[client_num];
	reference = &state->frames[0];

	if (client->last_sequence != client->current_sequence)
	{	// Old sequence
//		Con_Printf("SV: Old sequence SV(%d,%d) CL(%d,%d)\n",client->current_sequence, client->current_frame, client->last_sequence, client->last_frame);
		client->current_frame++;
		if (client->current_frame > MAX_FRAMES+1)
			client->current_frame = MAX_FRAMES+1;
	}
	else if (client->last_frame == CLIENT_FRAME_INIT ||
			 client->last_frame == 0 ||
			 client->last_frame == MAX_FRAMES+1)
	{	// Reference expired in current sequence
//		Con_Printf("SV: Expired SV(%d,%d) CL(%d,%d)\n",client->current_sequence, client->current_frame, client->last_sequence, client->last_frame);
		client->current_frame = 1;
		client->current_sequence++;
	}
	else if (client->last_frame >= 1 && client->last_frame <= client->current_frame)
	{	// Got a valid frame
//		Con_Printf("SV: Valid SV(%d,%d) CL(%d,%d)\n",client->current_sequence, client->current_frame, client->last_sequence, client->last_frame);
		*reference = state->frames[client->last_frame];

		for(i=0;i<reference->count;i++)
			if (reference->states[i].flags & ENT_CLEARED)
			{
				e = reference->states[i].number;
				ent = EDICT_NUM(e);
				if (ent->h2_baseline.ClearCount[client_num] < CLEAR_LIMIT)
				{
					ent->h2_baseline.ClearCount[client_num]++;
				}
				else if (ent->h2_baseline.ClearCount[client_num] == CLEAR_LIMIT)
				{
					ent->h2_baseline.ClearCount[client_num] = 3;
					reference->states[i].flags &= ~ENT_CLEARED;
				}
			}
		client->current_frame = 1;
		client->current_sequence++;
	}
	else
	{	// Normal frame advance
//		Con_Printf("SV: Normal SV(%d,%d) CL(%d,%d)\n",client->current_sequence, client->current_frame, client->last_sequence, client->last_frame);
		client->current_frame++;
		if (client->current_frame > MAX_FRAMES+1)
			client->current_frame = MAX_FRAMES+1;
	}

	DoPlayer = DoMonsters = DoMissiles = DoMisc = false;

	if ((int)sv_update_player->value)
		DoPlayer = (client->current_sequence % ((int)sv_update_player->value)) == 0;
	if ((int)sv_update_monsters->value)
		DoMonsters = (client->current_sequence % ((int)sv_update_monsters->value)) == 0;
	if ((int)sv_update_missiles->value)
		DoMissiles = (client->current_sequence % ((int)sv_update_missiles->value)) == 0;
	if ((int)sv_update_misc->value)
		DoMisc = (client->current_sequence % ((int)sv_update_misc->value)) == 0;

	build = &state->frames[client->current_frame];
	Com_Memset(build,0,sizeof(*build));
	client->last_frame = CLIENT_FRAME_RESET;

	NumToRemove = 0;
	msg->WriteByte(h2svc_reference);
	msg->WriteByte(client->current_frame);
	msg->WriteByte(client->current_sequence);

	// find the client's PVS
	if (clent->GetCameraMode())
	{
		ent = PROG_TO_EDICT(clent->GetCameraMode());
		VectorCopy(ent->GetOrigin(), org);
	}
	else
		VectorAdd (clent->GetOrigin(), clent->GetViewOfs(), org);

	pvs = SV_FatPVS (org);

	// send over all entities (excpet the client) that touch the pvs
	ent = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		DoRemove = false;
		// don't send if flagged for NODRAW and there are no lighting effects
		if (ent->GetEffects() == EF_NODRAW)
		{
			DoRemove = true;
			goto skipA;
		}

		// ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALWAYS sent
		{	// ignore ents without visible models
			if (!ent->v.modelindex || !*PR_GetString(ent->GetModel()))
			{
				DoRemove = true;
				goto skipA;
			}

			for (i=0 ; i < ent->num_leafs ; i++)
			{
				int l = CM_LeafCluster(ent->LeafNums[i]);
				if (pvs[l >> 3] & (1 << (l & 7)))
					break;
			}
				
			if (i == ent->num_leafs)
			{
				DoRemove = true;
				goto skipA;
			}
		}

skipA:
		IgnoreEnt = false;
		flagtest = (long)ent->GetFlags();
		if (!DoRemove)
		{
			if (flagtest & FL_CLIENT)
			{
				if (!DoPlayer)
					IgnoreEnt = true;
			}
			else if (flagtest & FL_MONSTER)
			{
				if (!DoMonsters)
					IgnoreEnt = true;
			}
			else if (ent->GetMoveType() == QHMOVETYPE_FLYMISSILE ||
					 ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE ||
					 ent->GetMoveType() == QHMOVETYPE_BOUNCE)
			{
				if (!DoMissiles)
					IgnoreEnt = true;
			}
			else
			{
				if (!DoMisc)
					IgnoreEnt = true;
			}
		}

		bits = 0;

		while (position < reference->count && 
			   e > reference->states[position].number)
			position++;

		if (position < reference->count && reference->states[position].number == e)
		{
			FoundInList = true;
			if (DoRemove)
			{
				RemoveList[NumToRemove] = e;
				NumToRemove++;
				continue;
			}
			else
			{
				ref_ent = &reference->states[position];
			}
		}
		else
		{
			if (DoRemove || IgnoreEnt) 
				continue;

			ref_ent = &build_ent;

			build_ent.number = e;
			build_ent.origin[0] = ent->h2_baseline.origin[0];
			build_ent.origin[1] = ent->h2_baseline.origin[1];
			build_ent.origin[2] = ent->h2_baseline.origin[2];
			build_ent.angles[0] = ent->h2_baseline.angles[0];
			build_ent.angles[1] = ent->h2_baseline.angles[1];
			build_ent.angles[2] = ent->h2_baseline.angles[2];
			build_ent.modelindex = ent->h2_baseline.modelindex;
			build_ent.frame = ent->h2_baseline.frame;
			build_ent.colormap = ent->h2_baseline.colormap;
			build_ent.skinnum = ent->h2_baseline.skinnum;
			build_ent.effects = ent->h2_baseline.effects;
			build_ent.scale = ent->h2_baseline.scale;
			build_ent.drawflags = ent->h2_baseline.drawflags;
			build_ent.abslight = ent->h2_baseline.abslight;
			build_ent.flags = 0;

			FoundInList = false;
		}

		set_ent = &build->states[build->count];
		build->count++;
		if (ent->h2_baseline.ClearCount[client_num] < CLEAR_LIMIT)
		{
			Com_Memset(ref_ent,0,sizeof(*ref_ent));
			ref_ent->number = e;
		}
		*set_ent = *ref_ent;

		if (IgnoreEnt)
			continue;

		// send an update
		for (i=0 ; i<3 ; i++)
		{
			miss = ent->GetOrigin()[i] - ref_ent->origin[i];
			if ( miss < -0.1 || miss > 0.1 )
			{
				bits |= H2U_ORIGIN1<<i;
				set_ent->origin[i] = ent->GetOrigin()[i];
			}
		}

		if ( ent->GetAngles()[0] != ref_ent->angles[0] )
		{
			bits |= H2U_ANGLE1;
			set_ent->angles[0] = ent->GetAngles()[0];
		}
			
		if ( ent->GetAngles()[1] != ref_ent->angles[1] )
		{
			bits |= H2U_ANGLE2;
			set_ent->angles[1] = ent->GetAngles()[1];
		}
			
		if ( ent->GetAngles()[2] != ref_ent->angles[2] )
		{
			bits |= H2U_ANGLE3;
			set_ent->angles[2] = ent->GetAngles()[2];
		}
			
		if (ent->GetMoveType() == QHMOVETYPE_STEP)
			bits |= H2U_NOLERP;	// don't mess up the step animation
	
		if (ref_ent->colormap != ent->GetColorMap())
		{
			bits |= H2U_COLORMAP;
			set_ent->colormap = ent->GetColorMap();
		}
			
		if (ref_ent->skinnum != ent->GetSkin()
			|| ref_ent->drawflags != ent->GetDrawFlags())
		{
			bits |= H2U_SKIN;
			set_ent->skinnum = ent->GetSkin();
			set_ent->drawflags = ent->GetDrawFlags();
		}

		if (ref_ent->frame != ent->GetFrame())
		{
			bits |= H2U_FRAME;
			set_ent->frame = ent->GetFrame();
		}
	
		if (ref_ent->effects != ent->GetEffects())
		{
			bits |= H2U_EFFECTS;
			set_ent->effects = ent->GetEffects();
		}

//		flagtest = (long)ent->v.flags;
		if (flagtest & 0xff000000)
		{
			Host_Error("Invalid flags setting for class %s", PR_GetString(ent->GetClassName()));
			return;
		}

		temp_index = ent->v.modelindex;
		if (((int)ent->GetFlags() & FL_CLASS_DEPENDENT) && ent->GetModel())
		{
			String::Cpy(NewName, PR_GetString(ent->GetModel()));
			NewName[String::Length(NewName)-5] = client->playerclass + 48;
			temp_index = SV_ModelIndex (NewName);
		}

		if (ref_ent->modelindex != temp_index)
		{
			bits |= H2U_MODEL;
			set_ent->modelindex = temp_index;
		}

		if (ref_ent->scale != ((int)(ent->GetScale()*100.0)&255)
			|| ref_ent->abslight != ((int)(ent->GetAbsLight()*255.0)&255))
		{
			bits |= H2U_SCALE;
			set_ent->scale = ((int)(ent->GetScale()*100.0)&255);
			set_ent->abslight = (int)(ent->GetAbsLight()*255.0)&255;
		}

		if (ent->h2_baseline.ClearCount[client_num] < CLEAR_LIMIT)
		{
			bits |= H2U_CLEAR_ENT;
			set_ent->flags |= ENT_CLEARED;
		}

		if (!bits && FoundInList)
		{
			if (build->count >= MAX_CLIENT_STATES_H2)
				break;

			continue;
		}

		if (e >= 256)
			bits |= H2U_LONGENTITY;

		if (bits >= 256)
			bits |= H2U_MOREBITS;

		if (bits >= 65536)
			bits |= H2U_MOREBITS2;

	//
	// write the message
	//
		msg->WriteByte(bits | H2U_SIGNAL);
		
		if (bits & H2U_MOREBITS)
			msg->WriteByte(bits>>8);
		if (bits & H2U_MOREBITS2)
			msg->WriteByte(bits>>16);

		if (bits & H2U_LONGENTITY)
			msg->WriteShort(e);
		else
			msg->WriteByte(e);

		if (bits & H2U_MODEL)
			msg->WriteShort(temp_index);
		if (bits & H2U_FRAME)
			msg->WriteByte(ent->GetFrame());
		if (bits & H2U_COLORMAP)
			msg->WriteByte(ent->GetColorMap());
		if(bits & H2U_SKIN)
		{ // Used for skin and drawflags
			msg->WriteByte(ent->GetSkin());
			msg->WriteByte(ent->GetDrawFlags());
		}
		if (bits & H2U_EFFECTS)
			msg->WriteByte(ent->GetEffects());
		if (bits & H2U_ORIGIN1)
			msg->WriteCoord(ent->GetOrigin()[0]);		
		if (bits & H2U_ANGLE1)
			msg->WriteAngle(ent->GetAngles()[0]);
		if (bits & H2U_ORIGIN2)
			msg->WriteCoord(ent->GetOrigin()[1]);
		if (bits & H2U_ANGLE2)
			msg->WriteAngle(ent->GetAngles()[1]);
		if (bits & H2U_ORIGIN3)
			msg->WriteCoord(ent->GetOrigin()[2]);
		if (bits & H2U_ANGLE3)
			msg->WriteAngle(ent->GetAngles()[2]);
		if(bits & H2U_SCALE)
		{ // Used for scale and abslight
			msg->WriteByte((int)(ent->GetScale()*100.0)&255);
			msg->WriteByte((int)(ent->GetAbsLight()*255.0)&255);
		}

		if (build->count >= MAX_CLIENT_STATES_H2)
			break;
	}

	msg->WriteByte(h2svc_clear_edicts);
	msg->WriteByte(NumToRemove);
	for(i=0;i<NumToRemove;i++)
		msg->WriteShort(RemoveList[i]);
}

/*
=============
SV_CleanupEnts

=============
*/
void SV_CleanupEnts (void)
{
	int		e;
	qhedict_t	*ent;
	
	ent = NEXT_EDICT(sv.edicts);
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		ent->SetEffects((int)ent->GetEffects() & ~EF_MUZZLEFLASH);
	}

}

/*
==================
SV_WriteClientdataToMessage

==================
*/ 
void SV_WriteClientdataToMessage (client_t *client, qhedict_t *ent, QMsg *msg)
{
	int		bits,sc1,sc2;
	byte	test;
	int		i;
	qhedict_t	*other;
	static  int next_update = 0;
	static	int next_count = 0;

//
// send a damage message
//
	if (ent->GetDmgTake() || ent->GetDmgSave())
	{
		other = PROG_TO_EDICT(ent->GetDmgInflictor());
		msg->WriteByte(h2svc_damage);
		msg->WriteByte(ent->GetDmgSave());
		msg->WriteByte(ent->GetDmgTake());
		for (i=0 ; i<3 ; i++)
			msg->WriteCoord(other->GetOrigin()[i] + 0.5*(other->GetMins()[i] + other->GetMaxs()[i]));
	
		ent->SetDmgTake(0);
		ent->SetDmgSave(0);
	}

//
// send the current viewpos offset from the view entity
//
	SV_SetIdealPitch ();		// how much to look up / down ideally

// a fixangle might get lost in a dropped packet.  Oh well.
	if ( ent->GetFixAngle() )
	{
		msg->WriteByte(h2svc_setangle);
		for (i=0 ; i < 3 ; i++)
			msg->WriteAngle(ent->GetAngles()[i] );
		ent->SetFixAngle(0);
	}

	bits = 0;
	
	if (client->send_all_v)
	{
		bits = SU_VIEWHEIGHT | SU_IDEALPITCH | SU_IDEALROLL | 
			   SU_VELOCITY1 | (SU_VELOCITY1<<1) | (SU_VELOCITY1<<2) | 
			   SU_PUNCH1 | (SU_PUNCH1<<1) | (SU_PUNCH1<<2) | SU_WEAPONFRAME |
			   SU_ARMOR | SU_WEAPON;
	}
	else
	{
		if (ent->GetViewOfs()[2] != client->old_v.view_ofs[2])
			bits |= SU_VIEWHEIGHT;
		
		if (ent->GetIdealPitch() != client->old_v.idealpitch)
			bits |= SU_IDEALPITCH;
		
		if (ent->GetIdealRoll() != client->old_v.idealroll)
			bits |= SU_IDEALROLL;
		
		for (i=0 ; i<3 ; i++)
		{
			if (ent->GetPunchAngle()[i] != client->old_v.punchangle[i])
				bits |= (SU_PUNCH1<<i);
			if (ent->GetVelocity()[i] != client->old_v.velocity[i])
				bits |= (SU_VELOCITY1<<i);
		}
		
		if (ent->GetWeaponFrame() != client->old_v.weaponframe)
			bits |= SU_WEAPONFRAME;
		
		if (ent->GetArmorValue() != client->old_v.armorvalue)
			bits |= SU_ARMOR;
		
		if (ent->GetWeaponModel() != client->old_v.weaponmodel)
			bits |= SU_WEAPON;
	}

// send the data
 

	//fjm: this wasn't in here b4, and the centerview command requires it.
	if ( (int)ent->GetFlags() & FL_ONGROUND) 
		bits |= SU_ONGROUND;

	next_count++;
	if (next_count >= 3)
	{
		next_count = 0;
		next_update++;
		if (next_update > 11)
			next_update = 0;

		switch(next_update)
		{
			case 0:bits |= SU_VIEWHEIGHT;
				break;
			case 1:bits |= SU_IDEALPITCH;
				break;
			case 2:bits |= SU_IDEALROLL;
				break;
			case 3:bits |= SU_VELOCITY1;
				break;
			case 4:bits |= (SU_VELOCITY1<<1);
				break;
			case 5:bits |= (SU_VELOCITY1<<2);
				break;
			case 6:bits |= SU_PUNCH1;
				break;
			case 7:bits |= (SU_PUNCH1<<1);
				break;
			case 8:bits |= (SU_PUNCH1<<2);
				break;
			case 9:bits |= SU_WEAPONFRAME;
				break;
			case 10:bits |= SU_ARMOR;
				break;
			case 11:bits |= SU_WEAPON;
				break;
		}
	}

	msg->WriteByte(h2svc_clientdata);
	msg->WriteShort(bits);
	
	if (bits & SU_VIEWHEIGHT)
		msg->WriteChar(ent->GetViewOfs()[2]);

	if (bits & SU_IDEALPITCH)
		msg->WriteChar(ent->GetIdealPitch());

	if (bits & SU_IDEALROLL)
		msg->WriteChar(ent->GetIdealRoll());

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
			msg->WriteChar(ent->GetPunchAngle()[i]);
		if (bits & (SU_VELOCITY1<<i))
			msg->WriteChar(ent->GetVelocity()[i]/16);
	}

	if (bits & SU_WEAPONFRAME)
		msg->WriteByte(ent->GetWeaponFrame());
	if (bits & SU_ARMOR)
		msg->WriteByte(ent->GetArmorValue());
	if (bits & SU_WEAPON)
		msg->WriteShort(SV_ModelIndex(PR_GetString(ent->GetWeaponModel())));

	if (host_client->send_all_v) 
	{
		sc1 = sc2 = 0xffffffff;
		host_client->send_all_v = false;
	}
	else
	{
		sc1 = sc2 = 0;

		if (ent->GetHealth() != host_client->old_v.health)
			sc1 |= SC1_HEALTH;
		if(ent->GetLevel() != host_client->old_v.level)
			sc1 |= SC1_LEVEL;
		if(ent->GetIntelligence() != host_client->old_v.intelligence)
			sc1 |= SC1_INTELLIGENCE;
		if(ent->GetWisdom() != host_client->old_v.wisdom)
			sc1 |= SC1_WISDOM;
		if(ent->GetStrength() != host_client->old_v.strength)
			sc1 |= SC1_STRENGTH;
		if(ent->GetDexterity() != host_client->old_v.dexterity)
			sc1 |= SC1_DEXTERITY;
		if (ent->GetWeapon() != host_client->old_v.weapon)
			sc1 |= SC1_WEAPON;
		if (ent->GetBlueMana() != host_client->old_v.bluemana)
			sc1 |= SC1_BLUEMANA;
		if (ent->GetGreenMana() != host_client->old_v.greenmana)
			sc1 |= SC1_GREENMANA;
		if (ent->GetExperience() != host_client->old_v.experience)
			sc1 |= SC1_EXPERIENCE;
		if (ent->GetCntTorch() != host_client->old_v.cnt_torch)
			sc1 |= SC1_CNT_TORCH;
		if (ent->GetCntHBoost() != host_client->old_v.cnt_h_boost)
			sc1 |= SC1_CNT_H_BOOST;
		if (ent->GetCntSHBoost() != host_client->old_v.cnt_sh_boost)
			sc1 |= SC1_CNT_SH_BOOST;
		if (ent->GetCntManaBoost() != host_client->old_v.cnt_mana_boost)
			sc1 |= SC1_CNT_MANA_BOOST;
		if (ent->GetCntTeleport() != host_client->old_v.cnt_teleport)
			sc1 |= SC1_CNT_TELEPORT;
		if (ent->GetCntTome() != host_client->old_v.cnt_tome)
			sc1 |= SC1_CNT_TOME;
		if (ent->GetCntSummon() != host_client->old_v.cnt_summon)
			sc1 |= SC1_CNT_SUMMON;
		if (ent->GetCntInvisibility() != host_client->old_v.cnt_invisibility)
			sc1 |= SC1_CNT_INVISIBILITY;
		if (ent->GetCntGlyph() != host_client->old_v.cnt_glyph)
			sc1 |= SC1_CNT_GLYPH;
		if (ent->GetCntHaste() != host_client->old_v.cnt_haste)
			sc1 |= SC1_CNT_HASTE;
		if (ent->GetCntBlast() != host_client->old_v.cnt_blast)
			sc1 |= SC1_CNT_BLAST;
		if (ent->GetCntPolyMorph() != host_client->old_v.cnt_polymorph)
			sc1 |= SC1_CNT_POLYMORPH;
		if (ent->GetCntFlight() != host_client->old_v.cnt_flight)
			sc1 |= SC1_CNT_FLIGHT;
		if (ent->GetCntCubeOfForce() != host_client->old_v.cnt_cubeofforce)
			sc1 |= SC1_CNT_CUBEOFFORCE;
		if (ent->GetCntInvincibility() != host_client->old_v.cnt_invincibility)
			sc1 |= SC1_CNT_INVINCIBILITY;
		if (ent->GetArtifactActive() != host_client->old_v.artifact_active)
			sc1 |= SC1_ARTIFACT_ACTIVE;
		if (ent->GetArtifactLow() != host_client->old_v.artifact_low)
			sc1 |= SC1_ARTIFACT_LOW;
		if (ent->GetMoveType() != host_client->old_v.movetype)
			sc1 |= SC1_MOVETYPE;
		if (ent->GetCameraMode() != host_client->old_v.cameramode)
			sc1 |= SC1_CAMERAMODE;
		if (ent->GetHasted() != host_client->old_v.hasted)
			sc1 |= SC1_HASTED;
		if (ent->GetInventory() != host_client->old_v.inventory)
			sc1 |= SC1_INVENTORY;
		if (ent->GetRingsActive() != host_client->old_v.rings_active)
			sc1 |= SC1_RINGS_ACTIVE;

		if (ent->GetRingsLow() != host_client->old_v.rings_low)
			sc2 |= SC2_RINGS_LOW;
		if (ent->GetArmorAmulet() != host_client->old_v.armor_amulet)
			sc2 |= SC2_AMULET;
		if (ent->GetArmorBracer() != host_client->old_v.armor_bracer)
			sc2 |= SC2_BRACER;
		if (ent->GetArmorBreastPlate() != host_client->old_v.armor_breastplate)
			sc2 |= SC2_BREASTPLATE;
		if (ent->GetArmorHelmet() != host_client->old_v.armor_helmet)
			sc2 |= SC2_HELMET;
		if (ent->GetRingFlight() != host_client->old_v.ring_flight)
			sc2 |= SC2_FLIGHT_T;
		if (ent->GetRingWater() != host_client->old_v.ring_water)
			sc2 |= SC2_WATER_T;
		if (ent->GetRingTurning() != host_client->old_v.ring_turning)
			sc2 |= SC2_TURNING_T;
		if (ent->GetRingRegeneration() != host_client->old_v.ring_regeneration)
			sc2 |= SC2_REGEN_T;
		if (ent->GetHasteTime() != host_client->old_v.haste_time)
			sc2 |= SC2_HASTE_T;
		if (ent->GetTomeTime() != host_client->old_v.tome_time)
			sc2 |= SC2_TOME_T;
		if (ent->GetPuzzleInv1() != host_client->old_v.puzzle_inv1)
			sc2 |= SC2_PUZZLE1;
		if (ent->GetPuzzleInv2() != host_client->old_v.puzzle_inv2)
			sc2 |= SC2_PUZZLE2;
		if (ent->GetPuzzleInv3() != host_client->old_v.puzzle_inv3)
			sc2 |= SC2_PUZZLE3;
		if (ent->GetPuzzleInv4() != host_client->old_v.puzzle_inv4)
			sc2 |= SC2_PUZZLE4;
		if (ent->GetPuzzleInv5() != host_client->old_v.puzzle_inv5)
			sc2 |= SC2_PUZZLE5;
		if (ent->GetPuzzleInv6() != host_client->old_v.puzzle_inv6)
			sc2 |= SC2_PUZZLE6;
		if (ent->GetPuzzleInv7() != host_client->old_v.puzzle_inv7)
			sc2 |= SC2_PUZZLE7;
		if (ent->GetPuzzleInv8() != host_client->old_v.puzzle_inv8)
			sc2 |= SC2_PUZZLE8;
		if (ent->GetMaxHealth() != host_client->old_v.max_health)
			sc2 |= SC2_MAXHEALTH;
		if (ent->GetMaxMana() != host_client->old_v.max_mana)
			sc2 |= SC2_MAXMANA;
		if (ent->GetFlags() != host_client->old_v.flags)
			sc2 |= SC2_FLAGS;
		if (info_mask != client->info_mask)
			sc2 |= SC2_OBJ;
		if (info_mask2 != client->info_mask2)
			sc2 |= SC2_OBJ2;
	}

	if (!sc1 && !sc2)
		goto end;

	host_client->message.WriteByte(h2svc_update_inv);
	test = 0;
	if (sc1 & 0x000000ff)
		test |= 1;
	if (sc1 & 0x0000ff00)
		test |= 2;
	if (sc1 & 0x00ff0000)
		test |= 4;
	if (sc1 & 0xff000000)
		test |= 8;
	if (sc2 & 0x000000ff)
		test |= 16;
	if (sc2 & 0x0000ff00)
		test |= 32;
	if (sc2 & 0x00ff0000)
		test |= 64;
	if (sc2 & 0xff000000)
		test |= 128;

	host_client->message.WriteByte(test);

	if (test & 1)
		host_client->message.WriteByte(sc1 & 0xff);
	if (test & 2)
		host_client->message.WriteByte((sc1 >> 8) & 0xff);
	if (test & 4)
		host_client->message.WriteByte((sc1 >> 16) & 0xff);
	if (test & 8)
		host_client->message.WriteByte((sc1 >> 24) & 0xff);
	if (test & 16)
		host_client->message.WriteByte(sc2 & 0xff);
	if (test & 32)
		host_client->message.WriteByte((sc2 >> 8) & 0xff);
	if (test & 64)
		host_client->message.WriteByte((sc2 >> 16) & 0xff);
	if (test & 128)
		host_client->message.WriteByte((sc2 >> 24) & 0xff);

	if (sc1 & SC1_HEALTH)
		host_client->message.WriteShort(ent->GetHealth());
	if (sc1 & SC1_LEVEL)
		host_client->message.WriteByte(ent->GetLevel());
	if (sc1 & SC1_INTELLIGENCE)
		host_client->message.WriteByte(ent->GetIntelligence());
	if (sc1 & SC1_WISDOM)
		host_client->message.WriteByte(ent->GetWisdom());
	if (sc1 & SC1_STRENGTH)
		host_client->message.WriteByte(ent->GetStrength());
	if (sc1 & SC1_DEXTERITY)
		host_client->message.WriteByte(ent->GetDexterity());
	if (sc1 & SC1_WEAPON)
		host_client->message.WriteByte(ent->GetWeapon());
	if (sc1 & SC1_BLUEMANA)
		host_client->message.WriteByte(ent->GetBlueMana());
	if (sc1 & SC1_GREENMANA)
		host_client->message.WriteByte(ent->GetGreenMana());
	if (sc1 & SC1_EXPERIENCE)
		host_client->message.WriteLong(ent->GetExperience());
	if (sc1 & SC1_CNT_TORCH)
		host_client->message.WriteByte(ent->GetCntTorch());
	if (sc1 & SC1_CNT_H_BOOST)
		host_client->message.WriteByte(ent->GetCntHBoost());
	if (sc1 & SC1_CNT_SH_BOOST)
		host_client->message.WriteByte(ent->GetCntSHBoost());
	if (sc1 & SC1_CNT_MANA_BOOST)
		host_client->message.WriteByte(ent->GetCntManaBoost());
	if (sc1 & SC1_CNT_TELEPORT)
		host_client->message.WriteByte(ent->GetCntTeleport());
	if (sc1 & SC1_CNT_TOME)
		host_client->message.WriteByte(ent->GetCntTome());
	if (sc1 & SC1_CNT_SUMMON)
		host_client->message.WriteByte(ent->GetCntSummon());
	if (sc1 & SC1_CNT_INVISIBILITY)
		host_client->message.WriteByte(ent->GetCntInvisibility());
	if (sc1 & SC1_CNT_GLYPH)
		host_client->message.WriteByte(ent->GetCntGlyph());
	if (sc1 & SC1_CNT_HASTE)
		host_client->message.WriteByte(ent->GetCntHaste());
	if (sc1 & SC1_CNT_BLAST)
		host_client->message.WriteByte(ent->GetCntBlast());
	if (sc1 & SC1_CNT_POLYMORPH)
		host_client->message.WriteByte(ent->GetCntPolyMorph());
	if (sc1 & SC1_CNT_FLIGHT)
		host_client->message.WriteByte(ent->GetCntFlight());
	if (sc1 & SC1_CNT_CUBEOFFORCE)
		host_client->message.WriteByte(ent->GetCntCubeOfForce());
	if (sc1 & SC1_CNT_INVINCIBILITY)
		host_client->message.WriteByte(ent->GetCntInvincibility());
	if (sc1 & SC1_ARTIFACT_ACTIVE)
		host_client->message.WriteFloat(ent->GetArtifactActive());
	if (sc1 & SC1_ARTIFACT_LOW)
		host_client->message.WriteFloat(ent->GetArtifactLow());
	if (sc1 & SC1_MOVETYPE)
		host_client->message.WriteByte(ent->GetMoveType());
	if (sc1 & SC1_CAMERAMODE)
		host_client->message.WriteByte(ent->GetCameraMode());
	if (sc1 & SC1_HASTED)
		host_client->message.WriteFloat(ent->GetHasted());
	if (sc1 & SC1_INVENTORY)
		host_client->message.WriteByte(ent->GetInventory());
	if (sc1 & SC1_RINGS_ACTIVE)
		host_client->message.WriteFloat(ent->GetRingsActive());

	if (sc2 & SC2_RINGS_LOW)
		host_client->message.WriteFloat(ent->GetRingsLow());
	if (sc2 & SC2_AMULET)
		host_client->message.WriteByte(ent->GetArmorAmulet());
	if (sc2 & SC2_BRACER)
		host_client->message.WriteByte(ent->GetArmorBracer());
	if (sc2 & SC2_BREASTPLATE)
		host_client->message.WriteByte(ent->GetArmorBreastPlate());
	if (sc2 & SC2_HELMET)
		host_client->message.WriteByte(ent->GetArmorHelmet());
	if (sc2 & SC2_FLIGHT_T)
		host_client->message.WriteByte(ent->GetRingFlight());
	if (sc2 & SC2_WATER_T)
		host_client->message.WriteByte(ent->GetRingWater());
	if (sc2 & SC2_TURNING_T)
		host_client->message.WriteByte(ent->GetRingTurning());
	if (sc2 & SC2_REGEN_T)
		host_client->message.WriteByte(ent->GetRingRegeneration());
	if (sc2 & SC2_HASTE_T)
		host_client->message.WriteFloat(ent->GetHasteTime());
	if (sc2 & SC2_TOME_T)
		host_client->message.WriteFloat(ent->GetTomeTime());
	if (sc2 & SC2_PUZZLE1)
		host_client->message.WriteString2(PR_GetString(ent->GetPuzzleInv1()));
	if (sc2 & SC2_PUZZLE2)
		host_client->message.WriteString2(PR_GetString(ent->GetPuzzleInv2()));
	if (sc2 & SC2_PUZZLE3)
		host_client->message.WriteString2(PR_GetString(ent->GetPuzzleInv3()));
	if (sc2 & SC2_PUZZLE4)
		host_client->message.WriteString2(PR_GetString(ent->GetPuzzleInv4()));
	if (sc2 & SC2_PUZZLE5)
		host_client->message.WriteString2(PR_GetString(ent->GetPuzzleInv5()));
	if (sc2 & SC2_PUZZLE6)
		host_client->message.WriteString2(PR_GetString(ent->GetPuzzleInv6()));
	if (sc2 & SC2_PUZZLE7)
		host_client->message.WriteString2(PR_GetString(ent->GetPuzzleInv7()));
	if (sc2 & SC2_PUZZLE8)
		host_client->message.WriteString2(PR_GetString(ent->GetPuzzleInv8()));
	if (sc2 & SC2_MAXHEALTH)
		host_client->message.WriteShort(ent->GetMaxHealth());
	if (sc2 & SC2_MAXMANA)
		host_client->message.WriteByte(ent->GetMaxMana());
	if (sc2 & SC2_FLAGS)
		host_client->message.WriteFloat(ent->GetFlags());
	if (sc2 & SC2_OBJ)
	{
		host_client->message.WriteLong(info_mask);
		client->info_mask = info_mask;
	}
	if (sc2 & SC2_OBJ2)
	{
		host_client->message.WriteLong(info_mask2);
		client->info_mask2 = info_mask2;
	}

end:
	client->old_v.movetype = ent->GetMoveType();
	VectorCopy(ent->GetVelocity(), client->old_v.velocity);
	VectorCopy(ent->GetPunchAngle(), client->old_v.punchangle);
	client->old_v.weapon = ent->GetWeapon();
	client->old_v.weaponmodel = ent->GetWeaponModel();
	client->old_v.weaponframe = ent->GetWeaponFrame();
	client->old_v.health = ent->GetHealth();
	client->old_v.max_health = ent->GetMaxHealth();
	client->old_v.bluemana = ent->GetBlueMana();
	client->old_v.greenmana = ent->GetGreenMana();
	client->old_v.max_mana = ent->GetMaxMana();
	client->old_v.armor_amulet = ent->GetArmorAmulet();
	client->old_v.armor_bracer = ent->GetArmorBracer();
	client->old_v.armor_breastplate = ent->GetArmorBreastPlate();
	client->old_v.armor_helmet = ent->GetArmorHelmet();
	client->old_v.level = ent->GetLevel();
	client->old_v.intelligence = ent->GetIntelligence();
	client->old_v.wisdom = ent->GetWisdom();
	client->old_v.dexterity = ent->GetDexterity();
	client->old_v.strength = ent->GetStrength();
	client->old_v.experience = ent->GetExperience();
	client->old_v.ring_flight = ent->GetRingFlight();
	client->old_v.ring_water = ent->GetRingWater();
	client->old_v.ring_turning = ent->GetRingTurning();
	client->old_v.ring_regeneration = ent->GetRingRegeneration();
	client->old_v.haste_time = ent->GetHasteTime();
	client->old_v.tome_time = ent->GetTomeTime();
	client->old_v.puzzle_inv1 = ent->GetPuzzleInv1();
	client->old_v.puzzle_inv2 = ent->GetPuzzleInv2();
	client->old_v.puzzle_inv3 = ent->GetPuzzleInv3();
	client->old_v.puzzle_inv4 = ent->GetPuzzleInv4();
	client->old_v.puzzle_inv5 = ent->GetPuzzleInv5();
	client->old_v.puzzle_inv6 = ent->GetPuzzleInv6();
	client->old_v.puzzle_inv7 = ent->GetPuzzleInv7();
	client->old_v.puzzle_inv8 = ent->GetPuzzleInv8();
	VectorCopy(ent->GetViewOfs(), client->old_v.view_ofs);
	client->old_v.idealpitch = ent->GetIdealPitch();
	client->old_v.idealroll = ent->GetIdealRoll();
	client->old_v.flags = ent->GetFlags();
	client->old_v.armorvalue = ent->GetArmorValue();
	client->old_v.rings_active = ent->GetRingsActive();
	client->old_v.rings_low = ent->GetRingsLow();
	client->old_v.artifact_active = ent->GetArtifactActive();
	client->old_v.artifact_low = ent->GetArtifactLow();
	client->old_v.hasted = ent->GetHasted();
	client->old_v.inventory = ent->GetInventory();
	client->old_v.cnt_torch = ent->GetCntTorch();
	client->old_v.cnt_h_boost = ent->GetCntHBoost();
	client->old_v.cnt_sh_boost = ent->GetCntSHBoost();
	client->old_v.cnt_mana_boost = ent->GetCntManaBoost();
	client->old_v.cnt_teleport = ent->GetCntTeleport();
	client->old_v.cnt_tome = ent->GetCntTome();
	client->old_v.cnt_summon = ent->GetCntSummon();
	client->old_v.cnt_invisibility = ent->GetCntInvisibility();
	client->old_v.cnt_glyph = ent->GetCntGlyph();
	client->old_v.cnt_haste = ent->GetCntHaste();
	client->old_v.cnt_blast = ent->GetCntBlast();
	client->old_v.cnt_polymorph = ent->GetCntPolyMorph();
	client->old_v.cnt_flight = ent->GetCntFlight();
	client->old_v.cnt_cubeofforce = ent->GetCntCubeOfForce();
	client->old_v.cnt_invincibility = ent->GetCntInvincibility();
	client->old_v.cameramode = ent->GetCameraMode();
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	byte		buf[MAX_MSGLEN_H2];
	QMsg		msg;
	
	msg.InitOOB(buf, sizeof(buf));

	msg.WriteByte(h2svc_time);
	msg.WriteFloat(sv.time);

// add the client specific data to the datagram
	SV_WriteClientdataToMessage (client, client->edict, &msg);

	SV_PrepareClientEntities (client, client->edict, &msg);

/*	if ((rand() & 0xff) < 200)
	{
		return true;
	}*/

// copy the server datagram if there is space
	if (msg.cursize + sv.datagram.cursize < msg.maxsize)
		msg.WriteData(sv.datagram._data, sv.datagram.cursize);

	if (msg.cursize + client->datagram.cursize < msg.maxsize)
		msg.WriteData(client->datagram._data, client->datagram.cursize);

	client->datagram.Clear();

	//if (msg.cursize > 300)
	//{
	//	Con_DPrintf("WARNING: packet size is %i\n",msg.cursize);
	//}

// send the datagram
	if (NET_SendUnreliableMessage (client->netconnection, &client->netchan, &msg) == -1)
	{
		SV_DropClient (true);// if the message couldn't send, kick off
		return false;
	}
	
	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages (void)
{
	int			i, j;
	client_t *client;
	qhedict_t *ent;

// check for changes to be sent over the reliable streams
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		ent = host_client->edict;
		if (host_client->old_frags != ent->GetFrags())
		{
			for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
			{
				if (!client->active)
					continue;

				client->message.WriteByte(h2svc_updatefrags);
				client->message.WriteByte(i);
				client->message.WriteShort(host_client->edict->GetFrags());
			}

			host_client->old_frags = ent->GetFrags();
		}
	}

	for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
	{
		if (!client->active)
			continue;
		client->message.WriteData(sv.reliable_datagram._data, sv.reliable_datagram.cursize);
	}

	sv.reliable_datagram.Clear();
}


/*
=======================
SV_SendNop

Send a nop message without trashing or sending the accumulated client
message buffer
=======================
*/
void SV_SendNop (client_t *client)
{
	QMsg		msg;
	byte		buf[4];
	
	msg.InitOOB(buf, sizeof(buf));

	msg.WriteChar(h2svc_nop);

	if (NET_SendUnreliableMessage (client->netconnection, &client->netchan, &msg) == -1)
		SV_DropClient (true);	// if the message couldn't send, kick off
	client->last_message = realtime;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages (void)
{
	int			i;
	
// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

		if (host_client->spawned)
		{
			if (!SV_SendClientDatagram (host_client))
				continue;
		}
		else
		{
		// the player isn't totally in the game yet
		// send small keepalive messages if too much time has passed
		// send a full message when the next signon stage has been requested
		// some other message data (name changes, etc) may accumulate 
		// between signon stages
			if (!host_client->sendsignon)
			{
				if (realtime - host_client->last_message > 5)
					SV_SendNop (host_client);
				continue;	// don't send out non-signon messages
			}
		}

		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->message.overflowed)
		{
			SV_DropClient (true);
			host_client->message.overflowed = false;
			continue;
		}
			
		if (host_client->message.cursize || host_client->dropasap)
		{
			if (!NET_CanSendMessage (host_client->netconnection, &host_client->netchan))
			{
//				I_Printf ("can't write\n");
				continue;
			}

			if (host_client->dropasap)
				SV_DropClient (false);	// went to another level
			else
			{
				if (NET_SendMessage (host_client->netconnection
				, &host_client->netchan, &host_client->message) == -1)
					SV_DropClient (true);	// if the message couldn't send, kick off
				host_client->message.Clear();
				host_client->last_message = realtime;
				host_client->sendsignon = false;
			}
		}
	}
	
	
// clear muzzle flashes
	SV_CleanupEnts ();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex (const char *name)
{
	int		i;
	
	if (!name || !name[0])
		return 0;

	for (i=0 ; i<MAX_MODELS_H2 && sv.model_precache[i] ; i++)
		if (!String::Cmp(sv.model_precache[i], name))
			return i;
	if (i==MAX_MODELS_H2 || !sv.model_precache[i])
	{
		Con_Printf("SV_ModelIndex: model %s not precached\n", name);
		return 0;
	}

	return i;
}

/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline (void)
{
	int			i;
	qhedict_t			*svent;
	int				entnum;	
//	int client_num = 1<<(MAX_BASELINES-1);
		
	for (entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
	// get the current server version
		svent = EDICT_NUM(entnum);
		if (svent->free)
			continue;
		if (entnum > svs.maxclients && !svent->v.modelindex)
			continue;

	//
	// create entity baseline
	//
		VectorCopy (svent->GetOrigin(), svent->h2_baseline.origin);
		VectorCopy (svent->GetAngles(), svent->h2_baseline.angles);
		svent->h2_baseline.frame = svent->GetFrame();
		svent->h2_baseline.skinnum = svent->GetSkin();
		svent->h2_baseline.scale = (int)(svent->GetScale()*100.0)&255;
		svent->h2_baseline.drawflags = svent->GetDrawFlags();
		svent->h2_baseline.abslight = (int)(svent->GetAbsLight()*255.0)&255;
		if (entnum > 0	&& entnum <= svs.maxclients)
		{
			svent->h2_baseline.colormap = entnum;
			svent->h2_baseline.modelindex = 0;//SV_ModelIndex("models/paladin.mdl");
		}
		else
		{
			svent->h2_baseline.colormap = 0;
			svent->h2_baseline.modelindex =
				SV_ModelIndex(PR_GetString(svent->GetModel()));
		}
		Com_Memset(svent->h2_baseline.ClearCount,99,sizeof(svent->h2_baseline.ClearCount));
		
	//
	// add to the message
	//
		sv.signon.WriteByte(h2svc_spawnbaseline);
		sv.signon.WriteShort(entnum);

		sv.signon.WriteShort(svent->h2_baseline.modelindex);
		sv.signon.WriteByte(svent->h2_baseline.frame);
		sv.signon.WriteByte(svent->h2_baseline.colormap);
		sv.signon.WriteByte(svent->h2_baseline.skinnum);
		sv.signon.WriteByte(svent->h2_baseline.scale);
		sv.signon.WriteByte(svent->h2_baseline.drawflags);
		sv.signon.WriteByte(svent->h2_baseline.abslight);
		for (i=0 ; i<3 ; i++)
		{
			sv.signon.WriteCoord(svent->h2_baseline.origin[i]);
			sv.signon.WriteAngle(svent->h2_baseline.angles[i]);
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect (void)
{
	byte	data[128];
	QMsg	msg;

	msg.InitOOB(data, sizeof(data));

	msg.WriteChar(h2svc_stufftext);
	msg.WriteString2("reconnect\n");
	NET_SendToAll (&msg, 5);
	
	if (cls.state != CA_DEDICATED)
		Cmd_ExecuteString ("reconnect\n", src_command);
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms (void)
{
	int		i;

	svs.serverflags = pr_global_struct->serverflags;

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

	// call the progs to get default spawn parms for the new client
//		pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
//		PR_ExecuteProgram (pr_global_struct->SetChangeParms);
//		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
//			host_client->spawn_parms[j] = (&pr_global_struct->parm1)[j];
	}
}


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
extern float		scr_centertime_off;

void SV_SpawnServer (char *server, char *startspot)
{
	qhedict_t		*ent;
	int			i;
	qboolean	stats_restored;

	// let's not have any servers with no name
	if (hostname->string[0] == 0)
		Cvar_Set ("hostname", "UNNAMED");
	scr_centertime_off = 0;

	Con_DPrintf ("SpawnServer: %s\n",server);
	if (svs.changelevel_issued)
	{
		stats_restored = true;
		SaveGamestate(true);
	}
	else 
		stats_restored = false;

//
// tell all connected clients that we are going to a new level
//
	if (sv.active)
	{
		SV_SendReconnect ();
	}

//
// make cvars consistant
//
	if (coop->value)
		Cvar_SetValue ("deathmatch", 0);

	current_skill = (int)(skill->value + 0.1);
	if (current_skill < 0)
		current_skill = 0;
	if (current_skill > 4)
		current_skill = 4;

	Cvar_SetValue ("skill", (float)current_skill);
	
//
// set up the new server
//
	Host_ClearMemory ();

	//Com_Memset(&sv, 0, sizeof(sv));

	String::Cpy(sv.name, server);
	if (startspot)
		String::Cpy(sv.startspot, startspot);

// load progs to get entity field count

	total_loading_size = 100;
	current_loading_size = 0;
	loading_stage = 1;
	PR_LoadProgs ();
	current_loading_size += 10;
	SCR_UpdateScreen();
//	PR_LoadStrings();
//	PR_LoadInfoStrings();
	current_loading_size += 5;
	SCR_UpdateScreen();

// allocate server memory
	Com_Memset(sv.Effects,0,sizeof(sv.Effects));

	sv.states = (client_state2_t*)Hunk_AllocName (svs.maxclients * sizeof(client_state2_t), "states");
	Com_Memset(sv.states,0,svs.maxclients * sizeof(client_state2_t));

	sv.max_edicts = MAX_EDICTS_H2;
	
	sv.edicts = (qhedict_t*)Hunk_AllocName (sv.max_edicts*pr_edict_size, "edicts");

	sv.datagram.InitOOB(sv.datagram_buf, sizeof(sv.datagram_buf));
	
	sv.reliable_datagram.InitOOB(sv.reliable_datagram_buf, sizeof(sv.reliable_datagram_buf));
	
	sv.signon.InitOOB(sv.signon_buf, sizeof(sv.signon_buf));
	
// leave slots at start for clients only
	sv.num_edicts = svs.maxclients+1+max_temp_edicts->value;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		ent = EDICT_NUM(i+1);
		svs.clients[i].edict = ent;
		svs.clients[i].send_all_v = true;
	}

	for (i=0 ; i < max_temp_edicts->value ; i++)
	{
		ent = EDICT_NUM(i+svs.maxclients+1);
		ED_ClearEdict(ent);
		
		ent->free = true;
		ent->freetime = -999;
	}
	
	sv.state = ss_loading;
	sv.paused = false;

	sv.time = 1.0;
	
	String::Cpy(sv.name, server);
	sprintf (sv.modelname,"maps/%s.bsp", server);

	CM_LoadMap(sv.modelname, false, NULL);
	sv.models[1] = 0;
	
//
// clear world interaction links
//
	SV_ClearWorld ();

	sv.sound_precache[0] = PR_GetString(0);

	sv.model_precache[0] = PR_GetString(0);
	sv.model_precache[1] = sv.modelname;
	for (i = 1; i < CM_NumInlineModels(); i++)
	{
		sv.model_precache[1+i] = localmodels[i];
		sv.models[i+1] = CM_InlineModel(i);
	}

//
// load the rest of the entities
//	
	ent = EDICT_NUM(0);
	Com_Memset(&ent->v, 0, progs->entityfields * 4);
	ent->free = false;
	ent->SetModel(PR_SetString(sv.modelname));
	ent->v.modelindex = 1;		// world model
	ent->SetSolid(SOLID_BSP);
	ent->SetMoveType(QHMOVETYPE_PUSH);

	if (coop->value)
		pr_global_struct->coop = coop->value;
	else
		pr_global_struct->deathmatch = deathmatch->value;

	pr_global_struct->randomclass = randomclass->value;

	pr_global_struct->mapname = PR_SetString(sv.name);
	pr_global_struct->startspot = PR_SetString(sv.startspot);

	// serverflags are for cross level information (sigils)
	pr_global_struct->serverflags = svs.serverflags;
	
	current_loading_size += 5;
	SCR_UpdateScreen();
	ED_LoadFromFile(CM_EntityString());

	sv.active = true;

// all setup is completed, any further precache statements are errors
	sv.state = ss_active;

// run two frames to allow everything to settle
	host_frametime = 0.1;
	SV_Physics ();
	SV_Physics ();

// create a baseline for more efficient communications
	SV_CreateBaseline ();

// send serverinfo to all connected clients
	for (i=0,host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_SendServerinfo (host_client);

	svs.changelevel_issued = false;		// now safe to issue another
	
	Con_DPrintf ("Server spawned.\n");

	total_loading_size = 0;
	loading_stage = 0;
}
