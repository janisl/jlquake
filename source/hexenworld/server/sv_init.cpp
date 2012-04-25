
#include "qwsvdef.h"

server_static_t svs;				// persistant server info
server_t sv;						// local server

char localmodels[MAX_MODELS_H2][5];		// inline model names for precache

char localinfo[MAX_LOCALINFO_STRING + 1];	// local game info

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

	for (i = 0; i < MAX_MODELS_H2 && sv.model_precache[i]; i++)
		if (!String::Cmp(sv.model_precache[i], name))
		{
			return i;
		}
	if (i == MAX_MODELS_H2 || !sv.model_precache[i])
	{
		SV_Error("SV_ModelIndex: model %s not precached", name);
	}
	return i;
}

/*
================
SV_FlushSignon

Moves to the next signon buffer if needed
================
*/
void SV_FlushSignon(void)
{
	if (sv.signon.cursize < sv.signon.maxsize - 100)
	{
		return;
	}

	if (sv.num_signon_buffers == MAX_SIGNON_BUFFERS - 1)
	{
		SV_Error("sv.num_signon_buffers == MAX_SIGNON_BUFFERS-1");
	}

	sv.signon_buffer_size[sv.num_signon_buffers - 1] = sv.signon.cursize;
	sv.signon._data = sv.signon_buffers[sv.num_signon_buffers];
	sv.num_signon_buffers++;
	sv.signon.cursize = 0;
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

	for (entnum = 0; entnum < sv.num_edicts; entnum++)
	{
		svent = EDICT_NUM(entnum);
		if (svent->free)
		{
			continue;
		}
		// create baselines for all player slots,
		// and any other edict that has a visible model
		if (entnum > HWMAX_CLIENTS && !svent->v.modelindex)
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
		if (entnum > 0 && entnum <= HWMAX_CLIENTS)
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
		SV_FlushSignon();

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
		for (i = 0; i < 3; i++)
		{
			sv.signon.WriteCoord(svent->h2_baseline.origin[i]);
			sv.signon.WriteAngle(svent->h2_baseline.angles[i]);
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
	svs.serverflags = pr_global_struct->serverflags;

	for (i = 0, host_client = svs.clients; i < HWMAX_CLIENTS; i++, host_client++)
	{
		if (host_client->state != cs_spawned)
		{
			continue;
		}

		// needs to reconnect
		host_client->state = cs_connected;

		// call the progs to get default spawn parms for the new client
		pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
		PR_ExecuteProgram(pr_global_struct->SetChangeParms);
		for (j = 0; j < NUM_SPAWN_PARMS; j++)
			host_client->spawn_parms[j] = (&pr_global_struct->parm1)[j];
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

	sv.state = ss_dead;

	Hunk_FreeToLowMark(host_hunklevel);

	// wipe the entire per-level structure
	Com_Memset(&sv, 0, sizeof(sv));

	sv.datagram.InitOOB(sv.datagram_buf, sizeof(sv.datagram_buf));
	sv.datagram.allowoverflow = true;

	sv.reliable_datagram.InitOOB(sv.reliable_datagram_buf, sizeof(sv.reliable_datagram_buf));

	sv.multicast.InitOOB(sv.multicast_buf, sizeof(sv.multicast_buf));

	sv.master.InitOOB(sv.master_buf, sizeof(sv.master_buf));

	sv.signon.InitOOB(sv.signon_buffers[0], sizeof(sv.signon_buffers[0]));
	sv.num_signon_buffers = 1;

	String::Cpy(sv.name, server);
	if (startspot)
	{
		String::Cpy(sv.startspot, startspot);
	}

	// load progs to get entity field count
	// which determines how big each edict is
	PR_LoadProgs();
	PR_LoadStrings();

	// allocate edicts
	sv.edicts = (qhedict_t*)Hunk_AllocName(MAX_EDICTS_H2 * pr_edict_size, "edicts");

	// leave slots at start for clients only
	sv.num_edicts = HWMAX_CLIENTS + 1 + max_temp_edicts->value;
	for (i = 0; i < HWMAX_CLIENTS; i++)
	{
		ent = EDICT_NUM(i + 1);
		svs.clients[i].edict = ent;
//ZOID - make sure we update frags right
		svs.clients[i].old_frags = 0;
	}

	sv.time = 1.0;

	String::Cpy(sv.name, server);
	sprintf(sv.modelname,"maps/%s.bsp", server);
	CM_LoadMap(sv.modelname, false, NULL);

	//
	// clear physics interaction links
	//
	SV_ClearWorld();

	sv.sound_precache[0] = PR_GetString(0);

	sv.model_precache[0] = PR_GetString(0);
	sv.model_precache[1] = sv.modelname;
	sv.models[1] = 0;
	for (i = 1; i < CM_NumInlineModels(); i++)
	{
		sv.model_precache[1 + i] = localmodels[i];
		sv.models[i + 1] = CM_InlineModel(i);
	}

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;

	ent = EDICT_NUM(0);
	ent->free = false;
	ent->SetModel(PR_SetString(sv.modelname));
	ent->v.modelindex = 1;		// world model
	ent->SetSolid(SOLID_BSP);
	ent->SetMoveType(QHMOVETYPE_PUSH);

	if (coop->value)
	{
		Cvar_SetValue("deathmatch", 0);
	}

	pr_global_struct->coop = coop->value;
	pr_global_struct->deathmatch = deathmatch->value;
	pr_global_struct->randomclass = randomclass->value;
	pr_global_struct->damageScale = damageScale->value;
	pr_global_struct->shyRespawn = shyRespawn->value;
	pr_global_struct->spartanPrint = spartanPrint->value;
	pr_global_struct->meleeDamScale = meleeDamScale->value;
	pr_global_struct->manaScale = manaScale->value;
	pr_global_struct->tomeMode = tomeMode->value;
	pr_global_struct->tomeRespawn = tomeRespawn->value;
	pr_global_struct->w2Respawn = w2Respawn->value;
	pr_global_struct->altRespawn = altRespawn->value;
	pr_global_struct->fixedLevel = fixedLevel->value;
	pr_global_struct->autoItems = autoItems->value;
	pr_global_struct->dmMode = dmMode->value;
	pr_global_struct->easyFourth = easyFourth->value;
	pr_global_struct->patternRunner = patternRunner->value;
	pr_global_struct->max_players = maxclients->value;

	pr_global_struct->startspot = PR_SetString(sv.startspot);

	sv.current_skill = (int)(skill->value + 0.5);
	if (sv.current_skill < 0)
	{
		sv.current_skill = 0;
	}
	if (sv.current_skill > 3)
	{
		sv.current_skill = 3;
	}

	Cvar_SetValue("skill", (float)sv.current_skill);

	pr_global_struct->mapname = PR_SetString(sv.name);
	// serverflags are for cross level information (sigils)
	pr_global_struct->serverflags = svs.serverflags;

	// run the frame start qc function to let progs check cvars
	SV_ProgStartFrame();

	// load and spawn all other entities
	ED_LoadFromFile(CM_EntityString());

	// look up some model indexes for specialized message compression
	SV_FindModelNumbers();

	// all spawning is completed, any further precache statements
	// or prog writes to the signon message are errors
	sv.state = ss_active;

	// run two frames to allow everything to settle
	host_frametime = HX_FRAME_TIME;
	SV_Physics();
	SV_Physics();

	// save movement vars
	SV_SetMoveVars();

	// create a baseline for more efficient communications
	SV_CreateBaseline();
	sv.signon_buffer_size[sv.num_signon_buffers - 1] = sv.signon.cursize;

	Info_SetValueForKey(svs.info, "map", sv.name, MAX_SERVERINFO_STRING, 64, 64, !sv_highchars->value);
	Con_DPrintf("Server spawned.\n");

	svs.changelevel_issued = false;		// now safe to issue another
}
