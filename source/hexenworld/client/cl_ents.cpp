// cl_ents.c -- entity parsing and management

#include "quakedef.h"

void HandleEffects(int effects, int number, refEntity_t* ent, vec3_t angles, vec3_t angleAdd, vec3_t oldOrg)
{
	int rotateSet = 0;

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


	if (effects & EF_POISON_GAS)
	{
		CLHW_UpdatePoisonGas(ent->origin, angles);
	}
	if (effects & EF_ACIDBLOB)
	{
		angleAdd[0] = 0;
		angleAdd[1] = 0;
		angleAdd[2] = 200 * cl.qh_serverTimeFloat;

		rotateSet = 1;

		CLHW_UpdateAcidBlob(ent->origin, angles);
	}
	if (effects & EF_ONFIRE)
	{
		CLHW_UpdateOnFire(ent, angles, number);
	}
	if (effects & EF_POWERFLAMEBURN)
	{
		CLHW_UpdatePowerFlameBurn(ent, number);
	}
	if (effects & EF_ICESTORM_EFFECT)
	{
		CLHW_UpdateIceStorm(ent, number);
	}
	if (effects & EF_HAMMER_EFFECTS)
	{
		angleAdd[0] = 200 * cl.qh_serverTimeFloat;
		angleAdd[1] = 0;
		angleAdd[2] = 0;

		rotateSet = 1;

		CLHW_UpdateHammer(ent, number);
	}
	if (effects & EF_BEETLE_EFFECTS)
	{
		CLHW_UpdateBug(ent);
	}
	if (effects & EF_DARKLIGHT)	//EF_INVINC_CIRC)
	{
		CLHW_SuccubusInvincibleParticles(ent->origin);
	}

	if (effects & EF_UPDATESOUND)
	{
		S_UpdateSoundPos(number, 7, ent->origin);
	}


	if (!rotateSet)
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
void CL_LinkPacketEntities(void)
{
	hwpacket_entities_t* pack;
	h2entity_state_t* s1, * s2;
	float f;
	qhandle_t model;
	vec3_t old_origin;
	float autorotate;
	int i;
	int pnum;

	pack = &cl.hw_frames[clc.netchan.incomingSequence & UPDATE_MASK_HW].packet_entities;
	hwpacket_entities_t* PrevPack = &cl.hw_frames[(clc.netchan.incomingSequence - 1) & UPDATE_MASK_HW].packet_entities;

	autorotate = AngleMod(100 * cl.qh_serverTimeFloat);

	f = 0;		// FIXME: no interpolation right now

	for (pnum = 0; pnum < pack->num_entities; pnum++)
	{
		s1 = &pack->entities[pnum];
		s2 = s1;	// FIXME: no interpolation right now

		// if set to invisible, skip
		if (!s1->modelindex)
		{
			continue;
		}

		// create a new entity
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));

		ent.reType = RT_MODEL;
		model = cl.model_draw[s1->modelindex];
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
			float a1, a2;

			for (i = 0; i < 3; i++)
			{
				a1 = s1->angles[i];
				a2 = s2->angles[i];
				if (a1 - a2 > 180)
				{
					a1 -= 360;
				}
				if (a1 - a2 < -180)
				{
					a1 += 360;
				}
				angles[i] = a2 + f * (a1 - a2);
			}
		}

		// calculate origin
		for (i = 0; i < 3; i++)
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

		for (i = 0; i < 3; i++)
			//if ( abs(old_origin[i] - ent->origin[i]) > 128)
			if (abs(old_origin[i] - ent.origin[i]) > 512)	// this is an issue for laggy situations...
			{	// no trail if too far
				VectorCopy(ent.origin, old_origin);
				break;
			}

		// some of the effects need to know how far the thing has moved...

//		cl.h2_players[s1->number].invis=false;
		if (cl_siege)
		{
			if ((int)s1->effects & EF_NODRAW)
			{
				ent.skinNum = 101;	//ice, but in siege will be invis skin for dwarf to see
				drawflags |= H2DRF_TRANSLUCENT;
				s1->effects &= ~EF_NODRAW;
//				cl.h2_players[s1->number].invis=true;
			}
		}

		vec3_t angleAdd;
		HandleEffects(s1->effects, s1->number, &ent, angles, angleAdd, old_origin);
		CLH2_SetRefEntAxis(&ent, angles, angleAdd, s1->scale, s1->colormap, s1->abslight, drawflags);
		R_AddRefEntityToScene(&ent);

		// add automatic particle trails
		int ModelFlags = R_ModelFlags(ent.hModel);
		if (!ModelFlags)
		{
			continue;
		}

		// Model Flags
		if (ModelFlags & H2MDLEF_GIB)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_blood);
		}
		else if (ModelFlags & H2MDLEF_ZOMGIB)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_slight_blood);
		}
		else if (ModelFlags & H2MDLEF_TRACER)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_tracer);
		}
		else if (ModelFlags & H2MDLEF_TRACER2)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_tracer2);
		}
		else if (ModelFlags & H2MDLEF_ROCKET)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_rocket_trail);
		}
		else if (ModelFlags & H2MDLEF_FIREBALL)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_fireball);
			CLH2_FireBallLight(i, ent.origin);
		}
		else if (ModelFlags & H2MDLEF_ICE)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_ice);
		}
		else if (ModelFlags & H2MDLEF_SPIT)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_spit);
			CLH2_SpitLight(i, ent.origin);
		}
		else if (ModelFlags & H2MDLEF_SPELL)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_spell);
		}
		else if (ModelFlags & H2MDLEF_GRENADE)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_grensmoke);
		}
		else if (ModelFlags & H2MDLEF_TRACER3)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_voor_trail);
		}
		else if (ModelFlags & H2MDLEF_VORP_MISSILE)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_vorpal);
		}
		else if (ModelFlags & H2MDLEF_SET_STAFF)
		{
			CLH2_TrailParticles(old_origin, ent.origin,rt_setstaff);
		}
		else if (ModelFlags & H2MDLEF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
			{
				CLH2_TrailParticles(old_origin, ent.origin, rt_magicmissile);
			}
		}
		else if (ModelFlags & H2MDLEF_BONESHARD)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_boneshard);
		}
		else if (ModelFlags & H2MDLEF_SCARAB)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_scarab);
		}
		else if (ModelFlags & H2MDLEF_ACIDBALL)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_acidball);
		}
		else if (ModelFlags & H2MDLEF_BLOODSHOT)
		{
			CLH2_TrailParticles(old_origin, ent.origin, rt_bloodshot);
		}
	}
}

//========================================

extern int cl_spikeindex, cl_flagindex;

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
CL_LinkPlayers

Create visible entities in the correct position
for all current players
=============
*/
void CL_LinkPlayers(void)
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
		angles[ROLL] = V_CalcRoll(angles, state->velocity) * 4;

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

		if (cl_siege)
		{
			if ((int)state->effects & EF_NODRAW)
			{
				ent.skinNum = 101;	//ice, but in siege will be invis skin for dwarf to see
				drawflags |= H2DRF_TRANSLUCENT;
				state->effects &= ~EF_NODRAW;
			}
		}

		vec3_t angleAdd;
		HandleEffects(state->effects, j + 1, &ent, angles, angleAdd, NULL);

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

	CL_LinkPlayers();
	CL_LinkPacketEntities();
	CLH2_LinkProjectiles();
	CLH2_LinkMissiles();
	CLH2_UpdateTEnts();
	CL_LinkStaticEntities();
}
