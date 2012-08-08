
#include "qwsvdef.h"

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

	common->DPrintf("SpawnServer: %s\n",server);

	SVQH_SaveSpawnparms();

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
	PR_LoadProgs();

	SVQHW_AddProgCrcTotheServerInfo();

	SVQHW_FindSpectatorFunctions();

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
		sv.qh_model_precache[1 + i] = svqh_localmodels[i];
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
	*pr_globalVars.spartanPrint = hw_spartanPrint->value;
	*pr_globalVars.meleeDamScale = meleeDamScale->value;
	*pr_globalVars.manaScale = manaScale->value;
	*pr_globalVars.tomeMode = tomeMode->value;
	*pr_globalVars.tomeRespawn = tomeRespawn->value;
	*pr_globalVars.w2Respawn = w2Respawn->value;
	*pr_globalVars.altRespawn = altRespawn->value;
	*pr_globalVars.fixedLevel = fixedLevel->value;
	*pr_globalVars.autoItems = autoItems->value;
	*pr_globalVars.dmMode = hw_dmMode->value;
	*pr_globalVars.easyFourth = easyFourth->value;
	*pr_globalVars.patternRunner = patternRunner->value;
	*pr_globalVars.max_players = sv_maxclients->value;

	*pr_globalVars.startspot = PR_SetString(sv.h2_startspot);

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

	*pr_globalVars.mapname = PR_SetString(sv.name);
	// serverflags are for cross level information (sigils)
	*pr_globalVars.serverflags = svs.qh_serverflags;

	// run the frame start qc function to let progs check cvars
	SVQH_ProgStartFrame();

	// load and spawn all other entities
	ED_LoadFromFile(CM_EntityString());

	// look up some model indexes for specialized message compression
	SVHW_FindModelNumbers();

	// all spawning is completed, any further precache statements
	// or prog writes to the signon message are errors
	sv.state = SS_GAME;

	// run two frames to allow everything to settle
	SVQH_RunPhysicsForTime(realtime);
	SVQH_RunPhysicsForTime(realtime);

	// save movement vars
	SVQH_SetMoveVars();

	// create a baseline for more efficient communications
	SVQH_CreateBaseline();
	sv.qh_signon_buffer_size[sv.qh_num_signon_buffers - 1] = sv.qh_signon.cursize;

	Info_SetValueForKey(svs.qh_info, "map", sv.name, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	common->DPrintf("Server spawned.\n");

	svs.qh_changelevel_issued = false;		// now safe to issue another
}
