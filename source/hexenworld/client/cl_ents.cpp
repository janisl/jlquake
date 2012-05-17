// cl_ents.c -- entity parsing and management

#include "quakedef.h"

/*
===================
CLHW_ParsePlayerinfo
===================
*/
void CL_SavePlayer(void)
{
	int num;
	h2player_info_t* info;
	hwplayer_state_t* state;

	num = net_message.ReadByte();

	if (num > HWMAX_CLIENTS)
	{
		Sys_Error("CLHW_ParsePlayerinfo: bad num");
	}

	info = &cl.h2_players[num];
	state = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].playerstate[num];

	state->messagenum = cl.qh_parsecount;
	state->state_time = cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW].senttime;
}

/*
=============
CLHW_LinkPlayers

Create visible entities in the correct position
for all current players
=============
*/
void CLHW_LinkPlayers(void)
{
	int j;
	h2player_info_t* info;
	hwplayer_state_t* state;
	hwplayer_state_t exact;
	double playertime;
	int msec;
	hwframe_t* frame;
	int oldphysent;

	playertime = realtime - cls.qh_latency + 0.02;
	if (playertime > realtime)
	{
		playertime = realtime;
	}

	frame = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW];

	for (j = 0, info = cl.h2_players, state = frame->playerstate; j < HWMAX_CLIENTS
		 ; j++, info++, state++)
	{
		info->shownames_off = true;
		if (state->messagenum != cl.qh_parsecount)
		{
			continue;	// not present this frame

		}
		if (!state->modelindex)
		{
			continue;
		}

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

		ent.hModel = cl.model_draw[state->modelindex];
		ent.skinNum = state->skinnum;
		ent.frame = state->frame;

		int drawflags = state->drawflags;
		if (ent.hModel == clh2_player_models[0] ||
			ent.hModel == clh2_player_models[1] ||
			ent.hModel == clh2_player_models[2] ||
			ent.hModel == clh2_player_models[3] ||
			ent.hModel == clh2_player_models[4] ||	//mg-siege
			ent.hModel == clh2_player_models[5])
		{
			// use custom skin
			info->shownames_off = false;
		}

		//
		// angles
		//
		vec3_t angles;
		angles[PITCH] = -state->viewangles[PITCH] / 3;
		angles[YAW] = state->viewangles[YAW];
		angles[ROLL] = 0;
		angles[ROLL] = VQH_CalcRoll(angles, state->velocity) * 4;

		// only predict half the move to minimize overruns
		msec = 500 * (playertime - state->state_time);
		if (msec <= 0 || (!cl_predict_players->value && !cl_predict_players2->value) || j == cl.playernum)
		{
			VectorCopy(state->origin, ent.origin);
			//Con_DPrintf ("nopredict\n");
		}
		else
		{
			// predict players movement
			if (msec > 255)
			{
				msec = 255;
			}
			state->command.msec = msec;
			//Con_DPrintf ("predict: %i\n", msec);

			oldphysent = qh_pmove.numphysent;
			CLQHW_SetSolidPlayers(j);
			CLHW_PredictUsercmd(state, &exact, &state->command, false);
			qh_pmove.numphysent = oldphysent;
			VectorCopy(exact.origin, ent.origin);
		}

		if (clhw_siege)
		{
			if ((int)state->effects & H2EF_NODRAW)
			{
				ent.skinNum = 101;	//ice, but in siege will be invis skin for dwarf to see
				drawflags |= H2DRF_TRANSLUCENT;
				state->effects &= ~H2EF_NODRAW;
			}
		}

		vec3_t angleAdd;
		HandleEffects(state->effects, j + 1, &ent, angles, angleAdd);

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
			if ((ambientlight + shadelight) > 75 || (clhw_siege && my_team == ve_team))
			{
				info->shownames_off = false;
			}
			else
			{
				info->shownames_off = true;
			}
			if (clhw_siege)
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
				else if (ve_team == HWST_DEFENDER)
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

Builds all the qh_pmove physents for the current frame
===============
*/
void CL_SetSolidEntities(void)
{
	int i;
	hwframe_t* frame;
	hwpacket_entities_t* pak;
	h2entity_state_t* state;

	qh_pmove.physents[0].model = 0;
	VectorCopy(vec3_origin, qh_pmove.physents[0].origin);
	qh_pmove.physents[0].info = 0;
	qh_pmove.numphysent = 1;

	frame = &cl.hw_frames[cl.qh_parsecount & UPDATE_MASK_HW];
	pak = &frame->packet_entities;

	for (i = 0; i < pak->num_entities; i++)
	{
		state = &pak->entities[i];

		if (state->modelindex < 2)
		{
			continue;
		}
		if (!cl.model_clip[state->modelindex])
		{
			continue;
		}
		qh_pmove.physents[qh_pmove.numphysent].model = cl.model_clip[state->modelindex];
		VectorCopy(state->origin, qh_pmove.physents[qh_pmove.numphysent].origin);
		VectorCopy(state->angles, qh_pmove.physents[qh_pmove.numphysent].angles);
		qh_pmove.numphysent++;
	}

}

static void CL_LinkStaticEntities()
{
	h2entity_t* pent = h2cl_static_entities;
	for (int i = 0; i < cl.qh_num_statics; i++, pent++)
	{
		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(pent->state.origin, rent.origin);
		rent.hModel = cl.model_draw[pent->state.modelindex];
		rent.frame = pent->state.frame;
		rent.skinNum = pent->state.skinnum;
		rent.syncBase = pent->syncbase;
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
void CL_EmitEntities(void)
{
	if (cls.state != CA_ACTIVE)
	{
		return;
	}
	if (!cl.qh_validsequence)
	{
		return;
	}

	R_ClearScene();

	CLHW_LinkPlayers();
	CLHW_LinkPacketEntities();
	CLH2_LinkProjectiles();
	CLH2_LinkMissiles();
	CLH2_UpdateTEnts();
	CL_LinkStaticEntities();
}
