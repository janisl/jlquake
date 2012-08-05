// sv_main.c -- server main program

/*
 * $Header: /H2 Mission Pack/SV_MAIN.C 26    3/20/98 12:12a Jmonroe $
 */

#include "quakedef.h"
#include "../common/file_formats/bsp29.h"

char localmodels[MAX_MODELS_H2][5];				// inline model names for precache

Cvar* sv_sound_distance;

Cvar* sv_update_player;
Cvar* sv_update_monsters;
Cvar* sv_update_missiles;
Cvar* sv_update_misc;
Cvar* sv_idealrollscale;

int sv_kingofhill;
qboolean skip_start = false;
int num_intro_msg = 0;

//============================================================================

void Sv_Edicts_f(void);

//JL Used in progs
Cvar* sv_walkpitch;

/*
===============
SV_Init
===============
*/
void SV_Init(void)
{
	int i;

	SVQH_RegisterPhysicsCvars();
	svqh_idealpitchscale = Cvar_Get("sv_idealpitchscale","0.8", 0);
	sv_idealrollscale = Cvar_Get("sv_idealrollscale","0.8", 0);
	svqh_aim = Cvar_Get("sv_aim", "0.93", 0);
	sv_walkpitch = Cvar_Get("sv_walkpitch", "0", 0);
	sv_sound_distance = Cvar_Get("sv_sound_distance","800", CVAR_ARCHIVE);
	sv_update_player    = Cvar_Get("sv_update_player","1", CVAR_ARCHIVE);
	sv_update_monsters  = Cvar_Get("sv_update_monsters","1", CVAR_ARCHIVE);
	sv_update_missiles  = Cvar_Get("sv_update_missiles","1", CVAR_ARCHIVE);
	sv_update_misc      = Cvar_Get("sv_update_misc","1", CVAR_ARCHIVE);
	sv_ce_scale         = Cvar_Get("sv_ce_scale","0", CVAR_ARCHIVE);
	sv_ce_max_size      = Cvar_Get("sv_ce_max_size","0", CVAR_ARCHIVE);

	Cmd_AddCommand("sv_edicts", Sv_Edicts_f);

	for (i = 0; i < MAX_MODELS_H2; i++)
		sprintf(localmodels[i], "*%i", i);

	sv_kingofhill = 0;	//Initialize King of Hill to world
}

void SV_Edicts(const char* Name)
{
	fileHandle_t FH;
	int i;
	qhedict_t* e;

	FH = FS_FOpenFileWrite(Name);
	if (!FH)
	{
		common->Printf("Could not open %s\n",Name);
		return;
	}

	FS_Printf(FH,"Number of Edicts: %d\n",sv.qh_num_edicts);
	FS_Printf(FH,"Server Time: %f\n",sv.qh_time);
	FS_Printf(FH,"\n");
	FS_Printf(FH,"Num.     Time Class Name                     Model                          Think                                    Touch                                    Use\n");
	FS_Printf(FH,"---- -------- ------------------------------ ------------------------------ ---------------------------------------- ---------------------------------------- ----------------------------------------\n");

	for (i = 1; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		FS_Printf(FH,"%3d. %8.2f %-30s %-30s %-40s %-40s %-40s\n",
			i,e->GetNextThink(),PR_GetString(e->GetClassName()),PR_GetString(e->GetModel()),
			PR_GetString(pr_functions[e->GetThink()].s_name),PR_GetString(pr_functions[e->GetTouch()].s_name),
			PR_GetString(pr_functions[e->GetUse()].s_name));
	}
	FS_FCloseFile(FH);
}

void Sv_Edicts_f(void)
{
	const char* Name;

	if (sv.state == SS_DEAD)
	{
		common->Printf("This command can only be executed on a server running a map\n");
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
void SV_SendServerinfo(client_t* client)
{
	const char** s;

	SVQH_ClientPrintf(client, 0, "%c\nVERSION %4.2f SERVER (%i CRC)", 2, HEXEN2_VERSION, pr_crc);

	client->qh_message.WriteByte(h2svc_serverinfo);
	client->qh_message.WriteLong(PROTOCOL_VERSION);
	client->qh_message.WriteByte(svs.qh_maxclients);

	if (!svqh_coop->value && svqh_deathmatch->value)
	{
		client->qh_message.WriteByte(GAME_DEATHMATCH);
		client->qh_message.WriteShort(sv_kingofhill);
	}
	else
	{
		client->qh_message.WriteByte(GAME_COOP);
	}

	if (sv.qh_edicts->GetMessage() > 0 && sv.qh_edicts->GetMessage() <= prh2_string_count)
	{
		client->qh_message.WriteString2(&prh2_global_strings[prh2_string_index[(int)sv.qh_edicts->GetMessage() - 1]]);
	}
	else
	{
		client->qh_message.WriteString2(PR_GetString(sv.qh_edicts->GetNetName()));
	}

	for (s = sv.qh_model_precache + 1; *s; s++)
		client->qh_message.WriteString2(*s);
	client->qh_message.WriteByte(0);

	for (s = sv.qh_sound_precache + 1; *s; s++)
		client->qh_message.WriteString2(*s);
	client->qh_message.WriteByte(0);

// send music
	client->qh_message.WriteByte(h2svc_cdtrack);
//	client->message.WriteByte(sv.qh_edicts->v.soundtype);
//	client->message.WriteByte(sv.qh_edicts->v.soundtype);
	client->qh_message.WriteByte(sv.h2_cd_track);
	client->qh_message.WriteByte(sv.h2_cd_track);

	client->qh_message.WriteByte(h2svc_midi_name);
	client->qh_message.WriteString2(sv.h2_midi_name);

// set view
	client->qh_message.WriteByte(h2svc_setview);
	client->qh_message.WriteShort(QH_NUM_FOR_EDICT(client->qh_edict));

	client->qh_message.WriteByte(h2svc_signonnum);
	client->qh_message.WriteByte(1);

	client->qh_sendsignon = true;
	client->state = CS_CONNECTED;		// need prespawn, spawn, etc
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient(int clientnum)
{
	qhedict_t* ent;
	client_t* client;
	int edictnum;
	qsocket_t* netconnection;
	float spawn_parms[NUM_SPAWN_PARMS];
	int entnum;
	qhedict_t* svent;

	client = svs.clients + clientnum;

	common->DPrintf("Client %s connected\n", client->qh_netconnection->address);

	edictnum = clientnum + 1;

	ent = QH_EDICT_NUM(edictnum);

// set up the client_t
	netconnection = client->qh_netconnection;
	netchan_t netchan = client->netchan;

	if (sv.loadgame)
	{
		Com_Memcpy(spawn_parms, client->qh_spawn_parms, sizeof(spawn_parms));
	}
	Com_Memset(client, 0, sizeof(*client));
	client->h2_send_all_v = true;
	client->qh_netconnection = netconnection;
	client->netchan = netchan;

	String::Cpy(client->name, "unconnected");
	client->state = CS_CONNECTED;
	client->qh_edict = ent;

	client->qh_message.InitOOB(client->qh_messageBuffer, MAX_MSGLEN_H2);
	client->qh_message.allowoverflow = true;		// we can catch it

	client->datagram.InitOOB(client->datagramBuffer, MAX_MSGLEN_H2);

	for (entnum = 0; entnum < sv.qh_num_edicts; entnum++)
	{
		svent = QH_EDICT_NUM(entnum);
//		Com_Memcpy(&svent->baseline[clientnum],&svent->baseline[MAX_BASELINES-1],sizeof(h2entity_state_t));
	}
	Com_Memset(&sv.h2_states[clientnum],0,sizeof(h2client_state2_t));

	if (sv.loadgame)
	{
		Com_Memcpy(client->qh_spawn_parms, spawn_parms, sizeof(spawn_parms));
	}
//	else
//	{
	// call the progs to get default spawn parms for the new client
	//	PR_ExecuteProgram (pr_global_struct->SetNewParms);
	//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
	//		client->spawn_parms[i] = (&pr_global_struct->parm1)[i];
//	}

	SV_SendServerinfo(client);
}


/*
===================
SV_CheckForNewClients

===================
*/
void SV_CheckForNewClients(void)
{
	qsocket_t* ret;
	int i;

//
// check for new connections
//
	while (1)
	{
		netadr_t addr;
		ret = NET_CheckNewConnections(&addr);
		if (!ret)
		{
			break;
		}

		//
		// init a new client structure
		//
		for (i = 0; i < svs.qh_maxclients; i++)
			if (svs.clients[i].state < CS_CONNECTED)
			{
				break;
			}
		if (i == svs.qh_maxclients)
		{
			common->FatalError("Host_CheckForNewClients: no free clients");
		}

		Netchan_Setup(NS_SERVER, &svs.clients[i].netchan, addr, 0);
		svs.clients[i].netchan.lastReceived = net_time * 1000;
		svs.clients[i].qh_netconnection = ret;
		SV_ConnectClient(i);

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
void SV_ClearDatagram(void)
{
	sv.qh_datagram.Clear();
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

static byte fatpvs[BSP29_MAX_MAP_LEAFS / 8];

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte* SV_FatPVS(vec3_t org)
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
		common->FatalError("SV_FatPVS: count < 1");
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

#define CLIENT_FRAME_INIT   255
#define CLIENT_FRAME_RESET  254

void SV_PrepareClientEntities(client_t* client, qhedict_t* clent, QMsg* msg)
{
	int e, i;
	int bits;
	byte* pvs;
	vec3_t org;
	float miss;
	qhedict_t* ent;
	int temp_index;
	char NewName[MAX_QPATH];
	long flagtest;
	int position = 0;
	int client_num;
	unsigned long client_bit;
	h2client_frames_t* reference, * build;
	h2client_state2_t* state;
	h2entity_state_t* ref_ent,* set_ent,build_ent;
	qboolean FoundInList,DoRemove,DoPlayer,DoMonsters,DoMissiles,DoMisc,IgnoreEnt;
	short RemoveList[MAX_CLIENT_STATES_H2],NumToRemove;


	client_num = client - svs.clients;
	client_bit = 1 << client_num;
	state = &sv.h2_states[client_num];
	reference = &state->frames[0];

	if (client->h2_last_sequence != client->h2_current_sequence)
	{	// Old sequence
//		common->Printf("SV: Old sequence SV(%d,%d) CL(%d,%d)\n",client->current_sequence, client->current_frame, client->last_sequence, client->last_frame);
		client->h2_current_frame++;
		if (client->h2_current_frame > H2MAX_FRAMES + 1)
		{
			client->h2_current_frame = H2MAX_FRAMES + 1;
		}
	}
	else if (client->h2_last_frame == CLIENT_FRAME_INIT ||
			 client->h2_last_frame == 0 ||
			 client->h2_last_frame == H2MAX_FRAMES + 1)
	{	// Reference expired in current sequence
//		common->Printf("SV: Expired SV(%d,%d) CL(%d,%d)\n",client->current_sequence, client->current_frame, client->last_sequence, client->last_frame);
		client->h2_current_frame = 1;
		client->h2_current_sequence++;
	}
	else if (client->h2_last_frame >= 1 && client->h2_last_frame <= client->h2_current_frame)
	{	// Got a valid frame
//		common->Printf("SV: Valid SV(%d,%d) CL(%d,%d)\n",client->current_sequence, client->current_frame, client->last_sequence, client->last_frame);
		*reference = state->frames[client->h2_last_frame];

		for (i = 0; i < reference->count; i++)
			if (reference->states[i].flags & ENT_CLEARED)
			{
				e = reference->states[i].number;
				ent = QH_EDICT_NUM(e);
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
		client->h2_current_frame = 1;
		client->h2_current_sequence++;
	}
	else
	{	// Normal frame advance
//		common->Printf("SV: Normal SV(%d,%d) CL(%d,%d)\n",client->current_sequence, client->current_frame, client->last_sequence, client->last_frame);
		client->h2_current_frame++;
		if (client->h2_current_frame > H2MAX_FRAMES + 1)
		{
			client->h2_current_frame = H2MAX_FRAMES + 1;
		}
	}

	DoPlayer = DoMonsters = DoMissiles = DoMisc = false;

	if ((int)sv_update_player->value)
	{
		DoPlayer = (client->h2_current_sequence % ((int)sv_update_player->value)) == 0;
	}
	if ((int)sv_update_monsters->value)
	{
		DoMonsters = (client->h2_current_sequence % ((int)sv_update_monsters->value)) == 0;
	}
	if ((int)sv_update_missiles->value)
	{
		DoMissiles = (client->h2_current_sequence % ((int)sv_update_missiles->value)) == 0;
	}
	if ((int)sv_update_misc->value)
	{
		DoMisc = (client->h2_current_sequence % ((int)sv_update_misc->value)) == 0;
	}

	build = &state->frames[client->h2_current_frame];
	Com_Memset(build,0,sizeof(*build));
	client->h2_last_frame = CLIENT_FRAME_RESET;

	NumToRemove = 0;
	msg->WriteByte(h2svc_reference);
	msg->WriteByte(client->h2_current_frame);
	msg->WriteByte(client->h2_current_sequence);

	// find the client's PVS
	if (clent->GetCameraMode())
	{
		ent = PROG_TO_EDICT(clent->GetCameraMode());
		VectorCopy(ent->GetOrigin(), org);
	}
	else
	{
		VectorAdd(clent->GetOrigin(), clent->GetViewOfs(), org);
	}

	pvs = SV_FatPVS(org);

	// send over all entities (excpet the client) that touch the pvs
	ent = NEXT_EDICT(sv.qh_edicts);
	for (e = 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		DoRemove = false;
		// don't send if flagged for NODRAW and there are no lighting effects
		if (ent->GetEffects() == H2EF_NODRAW)
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

			for (i = 0; i < ent->num_leafs; i++)
			{
				int l = CM_LeafCluster(ent->LeafNums[i]);
				if (pvs[l >> 3] & (1 << (l & 7)))
				{
					break;
				}
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
			if (flagtest & QHFL_CLIENT)
			{
				if (!DoPlayer)
				{
					IgnoreEnt = true;
				}
			}
			else if (flagtest & QHFL_MONSTER)
			{
				if (!DoMonsters)
				{
					IgnoreEnt = true;
				}
			}
			else if (ent->GetMoveType() == QHMOVETYPE_FLYMISSILE ||
					 ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE ||
					 ent->GetMoveType() == QHMOVETYPE_BOUNCE)
			{
				if (!DoMissiles)
				{
					IgnoreEnt = true;
				}
			}
			else
			{
				if (!DoMisc)
				{
					IgnoreEnt = true;
				}
			}
		}

		bits = 0;

		while (position < reference->count&&
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
			{
				continue;
			}

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
		{
			continue;
		}

		// send an update
		for (i = 0; i < 3; i++)
		{
			miss = ent->GetOrigin()[i] - ref_ent->origin[i];
			if (miss < -0.1 || miss > 0.1)
			{
				bits |= H2U_ORIGIN1 << i;
				set_ent->origin[i] = ent->GetOrigin()[i];
			}
		}

		if (ent->GetAngles()[0] != ref_ent->angles[0])
		{
			bits |= H2U_ANGLE1;
			set_ent->angles[0] = ent->GetAngles()[0];
		}

		if (ent->GetAngles()[1] != ref_ent->angles[1])
		{
			bits |= H2U_ANGLE2;
			set_ent->angles[1] = ent->GetAngles()[1];
		}

		if (ent->GetAngles()[2] != ref_ent->angles[2])
		{
			bits |= H2U_ANGLE3;
			set_ent->angles[2] = ent->GetAngles()[2];
		}

		if (ent->GetMoveType() == QHMOVETYPE_STEP)
		{
			bits |= H2U_NOLERP;	// don't mess up the step animation

		}
		if (ref_ent->colormap != ent->GetColorMap())
		{
			bits |= H2U_COLORMAP;
			set_ent->colormap = ent->GetColorMap();
		}

		if (ref_ent->skinnum != ent->GetSkin() ||
			ref_ent->drawflags != ent->GetDrawFlags())
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
			common->Error("Invalid flags setting for class %s", PR_GetString(ent->GetClassName()));
			return;
		}

		temp_index = ent->v.modelindex;
		if (((int)ent->GetFlags() & H2FL_CLASS_DEPENDENT) && ent->GetModel())
		{
			String::Cpy(NewName, PR_GetString(ent->GetModel()));
			NewName[String::Length(NewName) - 5] = client->h2_playerclass + 48;
			temp_index = SVQH_ModelIndex(NewName);
		}

		if (ref_ent->modelindex != temp_index)
		{
			bits |= H2U_MODEL;
			set_ent->modelindex = temp_index;
		}

		if (ref_ent->scale != ((int)(ent->GetScale() * 100.0) & 255) ||
			ref_ent->abslight != ((int)(ent->GetAbsLight() * 255.0) & 255))
		{
			bits |= H2U_SCALE;
			set_ent->scale = ((int)(ent->GetScale() * 100.0) & 255);
			set_ent->abslight = (int)(ent->GetAbsLight() * 255.0) & 255;
		}

		if (ent->h2_baseline.ClearCount[client_num] < CLEAR_LIMIT)
		{
			bits |= H2U_CLEAR_ENT;
			set_ent->flags |= ENT_CLEARED;
		}

		if (!bits && FoundInList)
		{
			if (build->count >= MAX_CLIENT_STATES_H2)
			{
				break;
			}

			continue;
		}

		if (e >= 256)
		{
			bits |= H2U_LONGENTITY;
		}

		if (bits >= 256)
		{
			bits |= H2U_MOREBITS;
		}

		if (bits >= 65536)
		{
			bits |= H2U_MOREBITS2;
		}

		//
		// write the message
		//
		msg->WriteByte(bits | H2U_SIGNAL);

		if (bits & H2U_MOREBITS)
		{
			msg->WriteByte(bits >> 8);
		}
		if (bits & H2U_MOREBITS2)
		{
			msg->WriteByte(bits >> 16);
		}

		if (bits & H2U_LONGENTITY)
		{
			msg->WriteShort(e);
		}
		else
		{
			msg->WriteByte(e);
		}

		if (bits & H2U_MODEL)
		{
			msg->WriteShort(temp_index);
		}
		if (bits & H2U_FRAME)
		{
			msg->WriteByte(ent->GetFrame());
		}
		if (bits & H2U_COLORMAP)
		{
			msg->WriteByte(ent->GetColorMap());
		}
		if (bits & H2U_SKIN)
		{	// Used for skin and drawflags
			msg->WriteByte(ent->GetSkin());
			msg->WriteByte(ent->GetDrawFlags());
		}
		if (bits & H2U_EFFECTS)
		{
			msg->WriteByte(ent->GetEffects());
		}
		if (bits & H2U_ORIGIN1)
		{
			msg->WriteCoord(ent->GetOrigin()[0]);
		}
		if (bits & H2U_ANGLE1)
		{
			msg->WriteAngle(ent->GetAngles()[0]);
		}
		if (bits & H2U_ORIGIN2)
		{
			msg->WriteCoord(ent->GetOrigin()[1]);
		}
		if (bits & H2U_ANGLE2)
		{
			msg->WriteAngle(ent->GetAngles()[1]);
		}
		if (bits & H2U_ORIGIN3)
		{
			msg->WriteCoord(ent->GetOrigin()[2]);
		}
		if (bits & H2U_ANGLE3)
		{
			msg->WriteAngle(ent->GetAngles()[2]);
		}
		if (bits & H2U_SCALE)
		{	// Used for scale and abslight
			msg->WriteByte((int)(ent->GetScale() * 100.0) & 255);
			msg->WriteByte((int)(ent->GetAbsLight() * 255.0) & 255);
		}

		if (build->count >= MAX_CLIENT_STATES_H2)
		{
			break;
		}
	}

	msg->WriteByte(h2svc_clear_edicts);
	msg->WriteByte(NumToRemove);
	for (i = 0; i < NumToRemove; i++)
		msg->WriteShort(RemoveList[i]);
}

/*
=============
SV_CleanupEnts

=============
*/
void SV_CleanupEnts(void)
{
	int e;
	qhedict_t* ent;

	ent = NEXT_EDICT(sv.qh_edicts);
	for (e = 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		ent->SetEffects((int)ent->GetEffects() & ~H2EF_MUZZLEFLASH);
	}

}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram(client_t* client)
{
	byte buf[MAX_MSGLEN_H2];
	QMsg msg;

	msg.InitOOB(buf, sizeof(buf));

	msg.WriteByte(h2svc_time);
	msg.WriteFloat(sv.qh_time);

// add the client specific data to the datagram
	SVQH_WriteClientdataToMessage(client, &msg);

	SV_PrepareClientEntities(client, client->qh_edict, &msg);

/*	if ((rand() & 0xff) < 200)
    {
        return true;
    }*/

// copy the server datagram if there is space
	if (msg.cursize + sv.qh_datagram.cursize < msg.maxsize)
	{
		msg.WriteData(sv.qh_datagram._data, sv.qh_datagram.cursize);
	}

	if (msg.cursize + client->datagram.cursize < msg.maxsize)
	{
		msg.WriteData(client->datagram._data, client->datagram.cursize);
	}

	client->datagram.Clear();

	//if (msg.cursize > 300)
	//{
	//	common->DPrintf("WARNING: packet size is %i\n",msg.cursize);
	//}

// send the datagram
	if (NET_SendUnreliableMessage(client->qh_netconnection, &client->netchan, &msg) == -1)
	{
		SVQH_DropClient(host_client, true);// if the message couldn't send, kick off
		return false;
	}

	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages(void)
{
	int i, j;
	client_t* client;
	qhedict_t* ent;

// check for changes to be sent over the reliable streams
	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		ent = host_client->qh_edict;
		if (host_client->qh_old_frags != ent->GetFrags())
		{
			for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
			{
				if (client->state < CS_CONNECTED)
				{
					continue;
				}

				client->qh_message.WriteByte(h2svc_updatefrags);
				client->qh_message.WriteByte(i);
				client->qh_message.WriteShort(host_client->qh_edict->GetFrags());
			}

			host_client->qh_old_frags = ent->GetFrags();
		}
	}

	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		client->qh_message.WriteData(sv.qh_reliable_datagram._data, sv.qh_reliable_datagram.cursize);
	}

	sv.qh_reliable_datagram.Clear();
}


/*
=======================
SV_SendNop

Send a nop message without trashing or sending the accumulated client
message buffer
=======================
*/
void SV_SendNop(client_t* client)
{
	QMsg msg;
	byte buf[4];

	msg.InitOOB(buf, sizeof(buf));

	msg.WriteChar(h2svc_nop);

	if (NET_SendUnreliableMessage(client->qh_netconnection, &client->netchan, &msg) == -1)
	{
		SVQH_DropClient(host_client, true);	// if the message couldn't send, kick off
	}
	client->qh_last_message = realtime;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages(void)
{
	int i;

// update frags, names, etc
	SV_UpdateToReliableMessages();

// build individual updates
	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		if (host_client->state == CS_ACTIVE)
		{
			if (!SV_SendClientDatagram(host_client))
			{
				continue;
			}
		}
		else
		{
			// the player isn't totally in the game yet
			// send small keepalive messages if too much time has passed
			// send a full message when the next signon stage has been requested
			// some other message data (name changes, etc) may accumulate
			// between signon stages
			if (!host_client->qh_sendsignon)
			{
				if (realtime - host_client->qh_last_message > 5)
				{
					SV_SendNop(host_client);
				}
				continue;	// don't send out non-signon messages
			}
		}

		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->qh_message.overflowed)
		{
			SVQH_DropClient(host_client, true);
			host_client->qh_message.overflowed = false;
			continue;
		}

		if (host_client->qh_message.cursize || host_client->qh_dropasap)
		{
			if (!NET_CanSendMessage(host_client->qh_netconnection, &host_client->netchan))
			{
//				I_Printf ("can't write\n");
				continue;
			}

			if (host_client->qh_dropasap)
			{
				SVQH_DropClient(host_client, false);	// went to another level
			}
			else
			{
				if (NET_SendMessage(host_client->qh_netconnection,
						&host_client->netchan, &host_client->qh_message) == -1)
				{
					SVQH_DropClient(host_client, true);	// if the message couldn't send, kick off
				}
				host_client->qh_message.Clear();
				host_client->qh_last_message = realtime;
				host_client->qh_sendsignon = false;
			}
		}
	}


// clear muzzle flashes
	SV_CleanupEnts();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline(void)
{
	int i;
	qhedict_t* svent;
	int entnum;
//	int client_num = 1<<(MAX_BASELINES-1);

	for (entnum = 0; entnum < sv.qh_num_edicts; entnum++)
	{
		// get the current server version
		svent = QH_EDICT_NUM(entnum);
		if (svent->free)
		{
			continue;
		}
		if (entnum > svs.qh_maxclients && !svent->v.modelindex)
		{
			continue;
		}

		//
		// create entity baseline
		//
		VectorCopy(svent->GetOrigin(), svent->h2_baseline.origin);
		VectorCopy(svent->GetAngles(), svent->h2_baseline.angles);
		svent->h2_baseline.frame = svent->GetFrame();
		svent->h2_baseline.skinnum = svent->GetSkin();
		svent->h2_baseline.scale = (int)(svent->GetScale() * 100.0) & 255;
		svent->h2_baseline.drawflags = svent->GetDrawFlags();
		svent->h2_baseline.abslight = (int)(svent->GetAbsLight() * 255.0) & 255;
		if (entnum > 0  && entnum <= svs.qh_maxclients)
		{
			svent->h2_baseline.colormap = entnum;
			svent->h2_baseline.modelindex = 0;	//SVQH_ModelIndex("models/paladin.mdl");
		}
		else
		{
			svent->h2_baseline.colormap = 0;
			svent->h2_baseline.modelindex =
				SVQH_ModelIndex(PR_GetString(svent->GetModel()));
		}
		Com_Memset(svent->h2_baseline.ClearCount,99,sizeof(svent->h2_baseline.ClearCount));

		//
		// add to the message
		//
		sv.qh_signon.WriteByte(h2svc_spawnbaseline);
		sv.qh_signon.WriteShort(entnum);

		sv.qh_signon.WriteShort(svent->h2_baseline.modelindex);
		sv.qh_signon.WriteByte(svent->h2_baseline.frame);
		sv.qh_signon.WriteByte(svent->h2_baseline.colormap);
		sv.qh_signon.WriteByte(svent->h2_baseline.skinnum);
		sv.qh_signon.WriteByte(svent->h2_baseline.scale);
		sv.qh_signon.WriteByte(svent->h2_baseline.drawflags);
		sv.qh_signon.WriteByte(svent->h2_baseline.abslight);
		for (i = 0; i < 3; i++)
		{
			sv.qh_signon.WriteCoord(svent->h2_baseline.origin[i]);
			sv.qh_signon.WriteAngle(svent->h2_baseline.angles[i]);
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect(void)
{
	byte data[128];
	QMsg msg;

	msg.InitOOB(data, sizeof(data));

	msg.WriteChar(h2svc_stufftext);
	msg.WriteString2("reconnect\n");
	NET_SendToAll(&msg, 5);

#ifndef DEDICATED
	if (cls.state != CA_DEDICATED)
	{
		Cmd_ExecuteString("reconnect\n", src_command);
	}
#endif
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms(void)
{
	int i;

	svs.qh_serverflags = *pr_globalVars.serverflags;

	for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
	{
		if (host_client->state < CS_CONNECTED)
		{
			continue;
		}

		// call the progs to get default spawn parms for the new client
//		*pr_globalVars.self = EDICT_TO_PROG(host_client->edict);
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
extern float scr_centertime_off;

void SV_SpawnServer(char* server, char* startspot)
{
	qhedict_t* ent;
	int i;
	qboolean stats_restored;

	// let's not have any servers with no name
	if (sv_hostname->string[0] == 0)
	{
		Cvar_Set("hostname", "UNNAMED");
	}
#ifndef DEDICATED
	scr_centertime_off = 0;
#endif

	common->DPrintf("SpawnServer: %s\n",server);
	if (svs.qh_changelevel_issued)
	{
		stats_restored = true;
		SaveGamestate(true);
	}
	else
	{
		stats_restored = false;
	}

//
// tell all connected clients that we are going to a new level
//
	if (sv.state != SS_DEAD)
	{
		SV_SendReconnect();
	}

//
// make cvars consistant
//
	if (svqh_coop->value)
	{
		Cvar_SetValue("deathmatch", 0);
	}

	svqh_current_skill = (int)(qh_skill->value + 0.1);
	if (svqh_current_skill < 0)
	{
		svqh_current_skill = 0;
	}
	if (svqh_current_skill > 4)
	{
		svqh_current_skill = 4;
	}

	Cvar_SetValue("skill", (float)svqh_current_skill);

//
// set up the new server
//
	Host_ClearMemory();

	//Com_Memset(&sv, 0, sizeof(sv));

	String::Cpy(sv.name, server);
	if (startspot)
	{
		String::Cpy(sv.h2_startspot, startspot);
	}

// load progs to get entity field count

#ifndef DEDICATED
	total_loading_size = 100;
	current_loading_size = 0;
	loading_stage = 1;
#endif
	PR_LoadProgs();
#ifndef DEDICATED
	current_loading_size += 60;
	SCR_UpdateScreen();
#endif

// allocate server memory
	Com_Memset(sv.h2_Effects,0,sizeof(sv.h2_Effects));

	sv.h2_states = (h2client_state2_t*)Hunk_AllocName(svs.qh_maxclients * sizeof(h2client_state2_t), "states");
	Com_Memset(sv.h2_states,0,svs.qh_maxclients * sizeof(h2client_state2_t));

	sv.qh_edicts = (qhedict_t*)Hunk_AllocName(MAX_EDICTS_QH * pr_edict_size, "edicts");

	//JL WTF????
	sv.qh_datagram.InitOOB(sv.qh_datagramBuffer, MAX_MSGLEN_H2);

	sv.qh_reliable_datagram.InitOOB(sv.qh_reliable_datagramBuffer, MAX_MSGLEN_H2);

	sv.qh_signon.InitOOB(sv.qh_signonBuffer, MAX_MSGLEN_H2);

// leave slots at start for clients only
	sv.qh_num_edicts = svs.qh_maxclients + 1 + max_temp_edicts->value;
	for (i = 0; i < svs.qh_maxclients; i++)
	{
		ent = QH_EDICT_NUM(i + 1);
		svs.clients[i].qh_edict = ent;
		svs.clients[i].h2_send_all_v = true;
	}

	for (i = 0; i < max_temp_edicts->value; i++)
	{
		ent = QH_EDICT_NUM(i + svs.qh_maxclients + 1);
		ED_ClearEdict(ent);

		ent->free = true;
		ent->freetime = -999;
	}

	sv.state = SS_LOADING;
	sv.qh_paused = false;

	sv.qh_time = 1.0;

	String::Cpy(sv.name, server);
	sprintf(sv.qh_modelname,"maps/%s.bsp", server);

	CM_LoadMap(sv.qh_modelname, false, NULL);
	sv.models[1] = 0;

//
// clear world interaction links
//
	SV_ClearWorld();

	sv.qh_sound_precache[0] = PR_GetString(0);

	sv.qh_model_precache[0] = PR_GetString(0);
	sv.qh_model_precache[1] = sv.qh_modelname;
	for (i = 1; i < CM_NumInlineModels(); i++)
	{
		sv.qh_model_precache[1 + i] = localmodels[i];
		sv.models[i + 1] = CM_InlineModel(i);
	}

//
// load the rest of the entities
//
	ent = QH_EDICT_NUM(0);
	Com_Memset(&ent->v, 0, progs->entityfields * 4);
	ent->free = false;
	ent->SetModel(PR_SetString(sv.qh_modelname));
	ent->v.modelindex = 1;		// world model
	ent->SetSolid(QHSOLID_BSP);
	ent->SetMoveType(QHMOVETYPE_PUSH);

	if (svqh_coop->value)
	{
		*pr_globalVars.coop = svqh_coop->value;
	}
	else
	{
		*pr_globalVars.deathmatch = svqh_deathmatch->value;
	}

	*pr_globalVars.randomclass = randomclass->value;

	*pr_globalVars.mapname = PR_SetString(sv.name);
	*pr_globalVars.startspot = PR_SetString(sv.h2_startspot);

	// serverflags are for cross level information (sigils)
	*pr_globalVars.serverflags = svs.qh_serverflags;

#ifndef DEDICATED
	current_loading_size += 20;
	SCR_UpdateScreen();
#endif
	ED_LoadFromFile(CM_EntityString());

// all setup is completed, any further precache statements are errors
	sv.state = SS_GAME;

	SVQH_SetMoveVars();

	// run two frames to allow everything to settle
	host_frametime = 0.1;
	SVQH_RunPhysicsAndUpdateTime(host_frametime, realtime);
	SVQH_RunPhysicsAndUpdateTime(host_frametime, realtime);

#ifndef DEDICATED
	current_loading_size += 20;
	SCR_UpdateScreen();
#endif
// create a baseline for more efficient communications
	SV_CreateBaseline();

// send serverinfo to all connected clients
	for (i = 0,host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		if (host_client->state >= CS_CONNECTED)
		{
			SV_SendServerinfo(host_client);
		}

	svs.qh_changelevel_issued = false;		// now safe to issue another

	common->DPrintf("Server spawned.\n");

#ifndef DEDICATED
	total_loading_size = 0;
	loading_stage = 0;
#endif
}
