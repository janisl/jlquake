
#include "qwsvdef.h"

char localmodels[MAX_MODELS_H2][5];		// inline model names for precache

char localinfo[MAX_LOCALINFO_STRING + 1];	// local game info

func_t SpectatorConnect;
func_t SpectatorThink;
func_t SpectatorDisconnect;

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex(const char* name)
{
	int i;

	if (!name || !name[0])
	{
		return 0;
	}

	for (i = 0; i < MAX_MODELS_H2 && sv.qh_model_precache[i]; i++)
		if (!String::Cmp(sv.qh_model_precache[i], name))
		{
			return i;
		}
	if (i == MAX_MODELS_H2 || !sv.qh_model_precache[i])
	{
		SV_Error("SV_ModelIndex: model %s not precached", name);
	}
	return i;
}

/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline(void)
{
	int i;
	qhedict_t* svent;
	int entnum;

	for (entnum = 0; entnum < sv.qh_num_edicts; entnum++)
	{
		svent = QH_EDICT_NUM(entnum);
		if (svent->free)
		{
			continue;
		}
		// create baselines for all player slots,
		// and any other edict that has a visible model
		if (entnum > MAX_CLIENTS_QHW && !svent->v.modelindex)
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
		if (entnum > 0 && entnum <= MAX_CLIENTS_QHW)
		{
			svent->h2_baseline.colormap = entnum;
//			svent->baseline.modelindex = SV_ModelIndex("progs/player.mdl");
			svent->h2_baseline.modelindex = SV_ModelIndex("models/paladin.mdl");
		}
		else
		{
			svent->h2_baseline.colormap = 0;
			svent->h2_baseline.modelindex =
				SV_ModelIndex(PR_GetString(svent->GetModel()));
		}

		svent->h2_baseline.scale = (int)(svent->GetScale() * 100.0) & 255;
		svent->h2_baseline.drawflags = svent->GetDrawFlags();
		svent->h2_baseline.abslight = (int)(svent->GetAbsLight() * 255.0) & 255;
		//
		// flush the signon message out to a seperate buffer if
		// nearly full
		//
		SVQH_FlushSignon();

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
SV_SaveSpawnparms

Grabs the current state of the progs serverinfo flags
and each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms(void)
{
	int i, j;

	if (!sv.state)
	{
		return;		// no progs loaded yet

	}
	// serverflags is the only game related thing maintained
	svs.qh_serverflags = *pr_globalVars.serverflags;

	for (i = 0, host_client = svs.clients; i < MAX_CLIENTS_QHW; i++, host_client++)
	{
		if (host_client->state != CS_ACTIVE)
		{
			continue;
		}

		// needs to reconnect
		host_client->state = CS_CONNECTED;

		// call the progs to get default spawn parms for the new client
		*pr_globalVars.self = EDICT_TO_PROG(host_client->qh_edict);
		PR_ExecuteProgram(*pr_globalVars.SetChangeParms);
		for (j = 0; j < NUM_SPAWN_PARMS; j++)
			host_client->qh_spawn_parms[j] = pr_globalVars.parm1[j];
	}
}

void SV_AddProgCrcTotheServerInfo()
{
	char num[32];
	sprintf(num, "%i", pr_crc);
	Info_SetValueForKey(svs.qh_info, "*progs", num, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}

void SV_FindSpectatorFunctions()
{
	SpectatorConnect = SpectatorThink = SpectatorDisconnect = 0;

	dfunction_t* f;
	if ((f = ED_FindFunction("SpectatorConnect")) != NULL)
	{
		SpectatorConnect = (func_t)(f - pr_functions);
	}
	if ((f = ED_FindFunction("SpectatorThink")) != NULL)
	{
		SpectatorThink = (func_t)(f - pr_functions);
	}
	if ((f = ED_FindFunction("SpectatorDisconnect")) != NULL)
	{
		SpectatorDisconnect = (func_t)(f - pr_functions);
	}
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.

This is only called from the SV_Map_f() function.
================
*/
void SV_SpawnServer(char* server, char* startspot)
{
	qhedict_t* ent;
	int i;

	Con_DPrintf("SpawnServer: %s\n",server);

	SV_SaveSpawnparms();

	svs.spawncount++;		// any partially connected client will be
							// restarted

	sv.state = SS_DEAD;

	Hunk_FreeToLowMark(host_hunklevel);

	// wipe the entire per-level structure
	Com_Memset(&sv, 0, sizeof(sv));

	sv.qh_datagram.InitOOB(sv.qh_datagramBuffer, MAX_DATAGRAM_HW);
	sv.qh_datagram.allowoverflow = true;

	sv.qh_reliable_datagram.InitOOB(sv.qh_reliable_datagramBuffer, MAX_MSGLEN_HW);

	sv.multicast.InitOOB(sv.multicastBuffer, MAX_MSGLEN_HW);

	sv.qh_signon.InitOOB(sv.qh_signon_buffers[0], MAX_DATAGRAM_HW);
	sv.qh_num_signon_buffers = 1;

	String::Cpy(sv.name, server);
	if (startspot)
	{
		String::Cpy(sv.h2_startspot, startspot);
	}

	// load progs to get entity field count
	// which determines how big each edict is
	PR_InitBuiltins();
	PR_LoadProgs();

	SV_AddProgCrcTotheServerInfo();

	SV_FindSpectatorFunctions();

	PR_LoadStrings();

	// allocate edicts
	sv.qh_edicts = (qhedict_t*)Hunk_AllocName(MAX_EDICTS_QH * pr_edict_size, "edicts");

	// leave slots at start for clients only
	sv.qh_num_edicts = MAX_CLIENTS_QHW + 1 + max_temp_edicts->value;
	for (i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		ent = QH_EDICT_NUM(i + 1);
		svs.clients[i].qh_edict = ent;
//ZOID - make sure we update frags right
		svs.clients[i].qh_old_frags = 0;
	}

	sv.qh_time = 1.0;

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
		sv.qh_model_precache[1 + i] = localmodels[i];
		sv.models[i + 1] = CM_InlineModel(i);
	}

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = SS_LOADING;

	ent = QH_EDICT_NUM(0);
	ent->free = false;
	ent->SetModel(PR_SetString(sv.qh_modelname));
	ent->v.modelindex = 1;		// world model
	ent->SetSolid(QHSOLID_BSP);
	ent->SetMoveType(QHMOVETYPE_PUSH);

	if (svqh_coop->value)
	{
		Cvar_SetValue("deathmatch", 0);
	}

	*pr_globalVars.coop = svqh_coop->value;
	*pr_globalVars.deathmatch = svqh_deathmatch->value;
	*pr_globalVars.randomclass = randomclass->value;
	*pr_globalVars.damageScale = damageScale->value;
	*pr_globalVars.shyRespawn = shyRespawn->value;
	*pr_globalVars.spartanPrint = spartanPrint->value;
	*pr_globalVars.meleeDamScale = meleeDamScale->value;
	*pr_globalVars.manaScale = manaScale->value;
	*pr_globalVars.tomeMode = tomeMode->value;
	*pr_globalVars.tomeRespawn = tomeRespawn->value;
	*pr_globalVars.w2Respawn = w2Respawn->value;
	*pr_globalVars.altRespawn = altRespawn->value;
	*pr_globalVars.fixedLevel = fixedLevel->value;
	*pr_globalVars.autoItems = autoItems->value;
	*pr_globalVars.dmMode = dmMode->value;
	*pr_globalVars.easyFourth = easyFourth->value;
	*pr_globalVars.patternRunner = patternRunner->value;
	*pr_globalVars.max_players = maxclients->value;

	*pr_globalVars.startspot = PR_SetString(sv.h2_startspot);

	sv.hw_current_skill = (int)(skill->value + 0.5);
	if (sv.hw_current_skill < 0)
	{
		sv.hw_current_skill = 0;
	}
	if (sv.hw_current_skill > 3)
	{
		sv.hw_current_skill = 3;
	}

	Cvar_SetValue("skill", (float)sv.hw_current_skill);

	*pr_globalVars.mapname = PR_SetString(sv.name);
	// serverflags are for cross level information (sigils)
	*pr_globalVars.serverflags = svs.qh_serverflags;

	// run the frame start qc function to let progs check cvars
	SVQH_ProgStartFrame();

	// load and spawn all other entities
	ED_LoadFromFile(CM_EntityString());

	// look up some model indexes for specialized message compression
	SV_FindModelNumbers();

	// all spawning is completed, any further precache statements
	// or prog writes to the signon message are errors
	sv.state = SS_GAME;

	// run two frames to allow everything to settle
	host_frametime = HX_FRAME_TIME;
	SVQH_RunPhysicsForTime(realtime);
	SVQH_RunPhysicsForTime(realtime);

	// save movement vars
	SVQH_SetMoveVars();

	// create a baseline for more efficient communications
	SV_CreateBaseline();
	sv.qh_signon_buffer_size[sv.qh_num_signon_buffers - 1] = sv.qh_signon.cursize;

	Info_SetValueForKey(svs.qh_info, "map", sv.name, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	Con_DPrintf("Server spawned.\n");

	svs.qh_changelevel_issued = false;		// now safe to issue another
}
