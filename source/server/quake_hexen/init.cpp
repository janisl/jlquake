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

#include "../server.h"
#include "../progsvm/progsvm.h"
#include "local.h"

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
void SVQH_CreateBaseline()
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

unsigned SVQW_CheckModel(const char* mdl)
{
	Array<byte> buffer;
	FS_ReadFile(mdl, buffer);
	return CRC_Block(buffer.Ptr(), buffer.Num());
}

void SVQHW_AddProgCrcTotheServerInfo()
{
	char num[32];
	sprintf(num, "%i", pr_crc);
	Info_SetValueForKey(svs.qh_info, "*progs", num, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}

void SVQHW_FindSpectatorFunctions()
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
void SVQH_SendReconnect()
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
