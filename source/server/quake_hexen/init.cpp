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
#include "../worldsector.h"
#include "../public.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"
#include "../../common/crc.h"

char qhw_localinfo[QHMAX_LOCALINFO_STRING + 1];	// local game info

func_t qhw_SpectatorConnect;
func_t qhw_SpectatorThink;
func_t qhw_SpectatorDisconnect;

char svqh_localmodels[BIGGEST_MAX_MODELS][5];				// inline model names for precache

//	Moves to the next signon buffer if needed
void SVQH_FlushSignon()
{
	if (sv.qh_signon.cursize < sv.qh_signon.maxsize - (GGameType & GAME_HexenWorld ? 100 : 512))
	{
		return;
	}

	if (sv.qh_num_signon_buffers == MAX_SIGNON_BUFFERS - 1)
	{
		common->Error("sv.qh_num_signon_buffers == MAX_SIGNON_BUFFERS-1");
	}

	sv.qh_signon_buffer_size[sv.qh_num_signon_buffers - 1] = sv.qh_signon.cursize;
	sv.qh_signon._data = sv.qh_signon_buffers[sv.qh_num_signon_buffers];
	sv.qh_num_signon_buffers++;
	sv.qh_signon.cursize = 0;
}

int SVQH_ModelIndex(const char* name)
{
	if (!name || !name[0])
	{
		return 0;
	}

	int i;
	for (i = 0; i < (GGameType & GAME_Hexen2 ? MAX_MODELS_H2 : MAX_MODELS_Q1) && sv.qh_model_precache[i]; i++)
	{
		if (!String::Cmp(sv.qh_model_precache[i], name))
		{
			return i;
		}
	}
	if (i == (GGameType & GAME_Hexen2 ? MAX_MODELS_H2 : MAX_MODELS_Q1) || !sv.qh_model_precache[i])
	{
		if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
		{
			common->Printf("SVQH_ModelIndex: model %s not precached\n", name);
			return 0;
		}
		common->Error("SVQH_ModelIndex: model %s not precached", name);
	}
	return i;
}

//	Entity baselines are used to compress the update messages
// to the clients -- only the fields that differ from the
// baseline will be transmitted
static void SVQH_CreateBaseline()
{
	for (int entnum = 0; entnum < sv.qh_num_edicts; entnum++)
	{
		qhedict_t* svent = QH_EDICT_NUM(entnum);
		if (svent->free)
		{
			continue;
		}
		// create baselines for all player slots,
		// and any other edict that has a visible model
		if (entnum > (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ? MAX_CLIENTS_QHW : svs.qh_maxclients) && !svent->v.modelindex)
		{
			continue;
		}

		//
		// create entity baseline
		//
		if (GGameType & GAME_Quake)
		{
			VectorCopy(svent->GetOrigin(), svent->q1_baseline.origin);
			VectorCopy(svent->GetAngles(), svent->q1_baseline.angles);
			svent->q1_baseline.frame = svent->GetFrame();
			svent->q1_baseline.skinnum = svent->GetSkin();
			if (entnum > 0 && entnum <= (GGameType & GAME_QuakeWorld ? MAX_CLIENTS_QHW : svs.qh_maxclients))
			{
				svent->q1_baseline.colormap = entnum;
				svent->q1_baseline.modelindex = SVQH_ModelIndex("progs/player.mdl");
			}
			else
			{
				svent->q1_baseline.colormap = 0;
				svent->q1_baseline.modelindex =
					SVQH_ModelIndex(PR_GetString(svent->GetModel()));
			}
		}
		else
		{
			VectorCopy(svent->GetOrigin(), svent->h2_baseline.origin);
			VectorCopy(svent->GetAngles(), svent->h2_baseline.angles);
			svent->h2_baseline.frame = svent->GetFrame();
			svent->h2_baseline.skinnum = svent->GetSkin();
			svent->h2_baseline.scale = (int)(svent->GetScale() * 100.0) & 255;
			svent->h2_baseline.drawflags = svent->GetDrawFlags();
			svent->h2_baseline.abslight = (int)(svent->GetAbsLight() * 255.0) & 255;
			if (entnum > 0 && entnum <= (GGameType & GAME_HexenWorld ? MAX_CLIENTS_QHW : svs.qh_maxclients))
			{
				svent->h2_baseline.colormap = entnum;
				svent->h2_baseline.modelindex = GGameType & GAME_HexenWorld ? SVQH_ModelIndex("models/paladin.mdl") : 0;
			}
			else
			{
				svent->h2_baseline.colormap = 0;
				svent->h2_baseline.modelindex =
					SVQH_ModelIndex(PR_GetString(svent->GetModel()));
			}
			if (!(GGameType & GAME_HexenWorld))
			{
				Com_Memset(svent->h2_baseline.ClearCount, 99, sizeof(svent->h2_baseline.ClearCount));
			}
		}

		//
		// flush the signon message out to a seperate buffer if
		// nearly full
		//
		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
		{
			SVQH_FlushSignon();
		}

		//
		// add to the message
		//
		if (GGameType & GAME_Quake)
		{
			sv.qh_signon.WriteByte(q1svc_spawnbaseline);
			sv.qh_signon.WriteShort(entnum);

			sv.qh_signon.WriteByte(svent->q1_baseline.modelindex);
			sv.qh_signon.WriteByte(svent->q1_baseline.frame);
			sv.qh_signon.WriteByte(svent->q1_baseline.colormap);
			sv.qh_signon.WriteByte(svent->q1_baseline.skinnum);
			for (int i = 0; i < 3; i++)
			{
				sv.qh_signon.WriteCoord(svent->q1_baseline.origin[i]);
				sv.qh_signon.WriteAngle(svent->q1_baseline.angles[i]);
			}
		}
		else
		{
			sv.qh_signon.WriteByte(h2svc_spawnbaseline);
			sv.qh_signon.WriteShort(entnum);

			sv.qh_signon.WriteShort(svent->h2_baseline.modelindex);
			sv.qh_signon.WriteByte(svent->h2_baseline.frame);
			sv.qh_signon.WriteByte(svent->h2_baseline.colormap);
			sv.qh_signon.WriteByte(svent->h2_baseline.skinnum);
			sv.qh_signon.WriteByte(svent->h2_baseline.scale);
			sv.qh_signon.WriteByte(svent->h2_baseline.drawflags);
			sv.qh_signon.WriteByte(svent->h2_baseline.abslight);
			for (int i = 0; i < 3; i++)
			{
				sv.qh_signon.WriteCoord(svent->h2_baseline.origin[i]);
				sv.qh_signon.WriteAngle(svent->h2_baseline.angles[i]);
			}
		}
	}
}

//	Grabs the current state of the progs serverinfo flags
// and each client for saving across the
// transition to another level
void SVQH_SaveSpawnparms()
{
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) && !sv.state)
	{
		return;		// no progs loaded yet
	}

	// serverflags is the only game related thing maintained
	svs.qh_serverflags = *pr_globalVars.serverflags;

	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
	{
		return;
	}

	client_t* host_client = svs.clients;
	for (int i = 0; i < (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ? MAX_CLIENTS_QHW : svs.qh_maxclients); i++, host_client++)
	{
		if (host_client->state != CS_ACTIVE)
		{
			continue;
		}

		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
		{
			// needs to reconnect
			host_client->state = CS_CONNECTED;
		}

		// call the progs to get default spawn parms for the new client
		*pr_globalVars.self = EDICT_TO_PROG(host_client->qh_edict);
		PR_ExecuteProgram(*pr_globalVars.SetChangeParms);
		for (int j = 0; j < NUM_SPAWN_PARMS; j++)
		{
			host_client->qh_spawn_parms[j] = pr_globalVars.parm1[j];
		}
	}
}

static unsigned SVQW_CheckModel(const char* mdl)
{
	idList<byte> buffer;
	FS_ReadFile(mdl, buffer);
	return CRC_Block(buffer.Ptr(), buffer.Num());
}

static void SVQHW_AddProgCrcTotheServerInfo()
{
	char num[32];
	sprintf(num, "%i", pr_crc);
	Info_SetValueForKey(svs.qh_info, "*progs", num, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}

static void SVQHW_FindSpectatorFunctions()
{
	qhw_SpectatorConnect = qhw_SpectatorThink = qhw_SpectatorDisconnect = 0;

	dfunction_t* f;
	if ((f = ED_FindFunction("SpectatorConnect")) != NULL)
	{
		qhw_SpectatorConnect = (func_t)(f - pr_functions);
	}
	if ((f = ED_FindFunction("SpectatorThink")) != NULL)
	{
		qhw_SpectatorThink = (func_t)(f - pr_functions);
	}
	if ((f = ED_FindFunction("SpectatorDisconnect")) != NULL)
	{
		qhw_SpectatorDisconnect = (func_t)(f - pr_functions);
	}
}

//	Tell all the clients that the server is changing levels
static void SVQH_SendReconnect()
{
	byte data[128];
	QMsg msg;
	msg.InitOOB(data, sizeof(data));

	msg.WriteChar(GGameType & GAME_Hexen2 ? h2svc_stufftext : q1svc_stufftext);
	msg.WriteString2("reconnect\n");
	NET_SendToAll(&msg, 5);

	if (!com_dedicated->integer)
	{
		Cmd_ExecuteString("reconnect\n");
	}
}

//	Change the server to a new map, taking all connected
// clients along with it.
//	This is called at the start of each level
void SVQH_SpawnServer(const char* server, const char* startspot)
{
	qhedict_t* ent;
	int i;

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		// let's not have any servers with no name
		if (!sv_hostname || sv_hostname->string[0] == 0)
		{
			Cvar_Set("hostname", "UNNAMED");
		}
	}

	common->DPrintf("SpawnServer: %s\n",server);

	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld) && svs.qh_changelevel_issued)
	{
		SVH2_SaveGamestate(true);
	}

	if (GGameType & GAME_Quake && !(GGameType & GAME_QuakeWorld))
	{
		svs.qh_changelevel_issued = false;		// now safe to issue another
	}

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		//
		// tell all connected clients that we are going to a new level
		//
		if (sv.state != SS_DEAD)
		{
			SVQH_SendReconnect();
		}

		//
		// make cvars consistant
		//
		if (svqh_coop->value)
		{
			Cvar_SetValue("deathmatch", 0);
		}

		svqh_current_skill = (int)(qh_skill->value + 0.5);
		if (svqh_current_skill < 0)
		{
			svqh_current_skill = 0;
		}
		if (GGameType & GAME_Quake && svqh_current_skill > 3)
		{
			svqh_current_skill = 3;
		}
		if (GGameType & GAME_Hexen2 && svqh_current_skill > 4)
		{
			svqh_current_skill = 4;
		}

		Cvar_SetValue("skill", (float)svqh_current_skill);
	}
	else
	{
		SVQH_SaveSpawnparms();

		svs.spawncount++;		// any partially connected client will be
								// restarted

		sv.state = SS_DEAD;
	}

	SVQH_FreeMemory();

	// wipe the entire per-level structure
	Com_Memset(&sv, 0, sizeof(sv));

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		sv.qh_datagram.InitOOB(sv.qh_datagramBuffer, GGameType & GAME_HexenWorld ? MAX_DATAGRAM_HW : MAX_DATAGRAM_QW);
		sv.qh_datagram.allowoverflow = true;

		sv.qh_reliable_datagram.InitOOB(sv.qh_reliable_datagramBuffer, GGameType & GAME_HexenWorld ? MAX_MSGLEN_HW : MAX_MSGLEN_QW);

		sv.multicast.InitOOB(sv.multicastBuffer, GGameType & GAME_HexenWorld ? MAX_MSGLEN_HW : MAX_MSGLEN_QW);

		sv.qh_signon.InitOOB(sv.qh_signon_buffers[0], GGameType & GAME_HexenWorld ? MAX_DATAGRAM_HW : MAX_DATAGRAM_QW);
		sv.qh_num_signon_buffers = 1;
	}

	String::Cpy(sv.name, server);
	if (GGameType & GAME_Hexen2 && startspot)
	{
		String::Cpy(sv.h2_startspot, startspot);
	}

	// load progs to get entity field count
	// which determines how big each edict is
	PR_LoadProgs();

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		SVQHW_AddProgCrcTotheServerInfo();

		SVQHW_FindSpectatorFunctions();
	}

	// allocate edicts
	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
	{
		Com_Memset(sv.h2_Effects, 0, sizeof(sv.h2_Effects));

		sv.h2_states = (h2client_state2_t*)Mem_ClearedAlloc(svs.qh_maxclients * sizeof(h2client_state2_t));
	}

	sv.qh_edicts = (qhedict_t*)Mem_ClearedAlloc(MAX_EDICTS_QH * pr_edict_size);

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		//JL WTF????
		sv.qh_datagram.InitOOB(sv.qh_datagramBuffer, GGameType & GAME_Hexen2 ? MAX_MSGLEN_H2 : MAX_DATAGRAM_QH);

		sv.qh_reliable_datagram.InitOOB(sv.qh_reliable_datagramBuffer, GGameType & GAME_Hexen2 ? MAX_MSGLEN_H2 : MAX_DATAGRAM_QH);

		sv.qh_signon.InitOOB(sv.qh_signonBuffer, GGameType & GAME_Hexen2 ? MAX_MSGLEN_H2 : MAX_MSGLEN_Q1);
	}

	// leave slots at start for clients only
	sv.qh_num_edicts = (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ? MAX_CLIENTS_QHW : svs.qh_maxclients) + 1 + (GGameType & GAME_Hexen2 ? max_temp_edicts->integer : 0);
	for (i = 0; i < (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ? MAX_CLIENTS_QHW : svs.qh_maxclients); i++)
	{
		ent = QH_EDICT_NUM(i + 1);
		svs.clients[i].qh_edict = ent;
		if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
		{
			svs.clients[i].h2_send_all_v = true;
		}
		if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
		{
			//ZOID - make sure we update frags right
			svs.clients[i].qh_old_frags = 0;
		}
	}

	if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
	{
		for (i = 0; i < max_temp_edicts->value; i++)
		{
			ent = QH_EDICT_NUM(i + svs.qh_maxclients + 1);
			ED_ClearEdict(ent);

			ent->free = true;
			ent->freetime = -999;
		}
	}

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		sv.qh_paused = false;
	}

	sv.qh_time = 1000;

	String::Cpy(sv.name, server);
	sprintf(sv.qh_modelname,"maps/%s.bsp", server);
	CM_LoadMap(sv.qh_modelname, false, NULL);

	//
	// clear physics interaction links
	//
	SV_ClearWorld();

	sv.qh_sound_precache[0] = PR_GetString(0);

	sv.qh_model_precache[0] = PR_GetString(0);
	sv.qh_model_precache[1] = sv.qh_modelname;
	sv.models[1] = 0;
	for (i = 1; i < CM_NumInlineModels(); i++)
	{
		sv.qh_model_precache[1 + i] = svqh_localmodels[i];
		sv.models[i + 1] = CM_InlineModel(i);
	}

	if (GGameType & GAME_QuakeWorld)
	{
		//check player/eyes models for hacks
		sv.qw_model_player_checksum = SVQW_CheckModel("progs/player.mdl");
		sv.qw_eyes_player_checksum = SVQW_CheckModel("progs/eyes.mdl");
	}

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = SS_LOADING;

	ent = QH_EDICT_NUM(0);
	Com_Memset(&ent->v, 0, progs->entityfields * 4);
	ent->free = false;
	ent->SetModel(PR_SetString(sv.qh_modelname));
	ent->v.modelindex = 1;		// world model
	ent->SetSolid(QHSOLID_BSP);
	ent->SetMoveType(QHMOVETYPE_PUSH);

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		if (svqh_coop->value)
		{
			*pr_globalVars.coop = svqh_coop->value;
		}
		else
		{
			*pr_globalVars.deathmatch = svqh_deathmatch->value;
		}
	}

	*pr_globalVars.mapname = PR_SetString(sv.name);
	// serverflags are for cross level information (sigils)
	*pr_globalVars.serverflags = svs.qh_serverflags;

	if (GGameType & GAME_Hexen2)
	{
		*pr_globalVars.startspot = PR_SetString(sv.h2_startspot);

		*pr_globalVars.randomclass = h2_randomclass->value;
	}

	if (GGameType & GAME_HexenWorld)
	{
		if (svqh_coop->value)
		{
			Cvar_SetValue("deathmatch", 0);
		}

		*pr_globalVars.coop = svqh_coop->value;
		*pr_globalVars.deathmatch = svqh_deathmatch->value;
		*pr_globalVars.damageScale = hw_damageScale->value;
		*pr_globalVars.shyRespawn = hw_shyRespawn->value;
		*pr_globalVars.spartanPrint = hw_spartanPrint->value;
		*pr_globalVars.meleeDamScale = hw_meleeDamScale->value;
		*pr_globalVars.manaScale = hw_manaScale->value;
		*pr_globalVars.tomeMode = hw_tomeMode->value;
		*pr_globalVars.tomeRespawn = hw_tomeRespawn->value;
		*pr_globalVars.w2Respawn = hw_w2Respawn->value;
		*pr_globalVars.altRespawn = hw_altRespawn->value;
		*pr_globalVars.fixedLevel = hw_fixedLevel->value;
		*pr_globalVars.autoItems = hw_autoItems->value;
		*pr_globalVars.dmMode = hw_dmMode->value;
		*pr_globalVars.easyFourth = hw_easyFourth->value;
		*pr_globalVars.patternRunner = hw_patternRunner->value;
		*pr_globalVars.max_players = sv_maxclients->value;

		sv.hw_current_skill = (int)(qh_skill->value + 0.5);
		if (sv.hw_current_skill < 0)
		{
			sv.hw_current_skill = 0;
		}
		if (sv.hw_current_skill > 3)
		{
			sv.hw_current_skill = 3;
		}

		Cvar_SetValue("skill", (float)sv.hw_current_skill);
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		// run the frame start qc function to let progs check cvars
		SVQH_ProgStartFrame();
	}

	// load and spawn all other entities
	ED_LoadFromFile(CM_EntityString());

	// look up some model indexes for specialized message compression
	if (GGameType & GAME_QuakeWorld)
	{
		SVQW_FindModelNumbers();
	}
	if (GGameType & GAME_HexenWorld)
	{
		SVHW_FindModelNumbers();
	}

	// all spawning is completed, any further precache statements
	// or prog writes to the signon message are errors
	sv.state = SS_GAME;

	// save movement vars
	SVQH_SetMoveVars();

	// run two frames to allow everything to settle
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		SVQH_RunPhysicsForTime(svs.realtime * 0.001);
		SVQH_RunPhysicsForTime(svs.realtime * 0.001);
	}
	else
	{
		SVQH_RunPhysicsAndUpdateTime(0.1, svs.realtime * 0.001);
		SVQH_RunPhysicsAndUpdateTime(0.1, svs.realtime * 0.001);
	}

	// create a baseline for more efficient communications
	SVQH_CreateBaseline();
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		sv.qh_signon_buffer_size[sv.qh_num_signon_buffers - 1] = sv.qh_signon.cursize;

		Info_SetValueForKey(svs.qh_info, "map", sv.name, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	}

	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		// send serverinfo to all connected clients
		client_t* client = svs.clients;
		for (i = 0; i < svs.qh_maxclients; i++, client++)
		{
			if (client->state >= CS_CONNECTED)
			{
				SVQH_SendServerinfo(client);
			}
		}
	}

	if (GGameType & GAME_Hexen2)
	{
		svs.qh_changelevel_issued = false;		// now safe to issue another
	}

	common->DPrintf("Server spawned.\n");
}
