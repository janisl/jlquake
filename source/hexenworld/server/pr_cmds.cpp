
#include "qwsvdef.h"
#ifdef _WIN32
#include <windows.h>
#endif

/*
===============================================================================

                        BUILT-IN FUNCTIONS

===============================================================================
*/

/*
=================
PF_name_print

print player's name

name_print(to, level, who)
=================
*/
void PF_name_print(void)
{
	int Index,Style;

//	plaquemessage = G_STRING(OFS_PARM0);

	Index = ((int)G_EDICTNUM(OFS_PARM2));
	Style = ((int)G_FLOAT(OFS_PARM1));


	if (hw_spartanPrint->value == 1 && Style < 2)
	{
		return;
	}

	if (Index <= 0)
	{
		PR_RunError("PF_name_print: index(%d) <= 0",Index);
	}

	if (Index > MAX_CLIENTS_QHW)
	{
		PR_RunError("PF_name_print: index(%d) > MAX_CLIENTS_QHW",Index);
	}


	if ((int)G_FLOAT(OFS_PARM0) == QHMSG_BROADCAST)	//broadcast message--send like bprint, print it out on server too.
	{
		client_t* cl;
		int i;

		common->Printf("%s",&svs.clients[Index - 1].name);

		for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
		{
			if (Style < cl->messagelevel)
			{
				continue;
			}
			if (cl->state != CS_ACTIVE)//should i be checking CS_CONNECTED too?
			{
				if (cl->state)	//not fully in so won't know name yet, explicitly say the name
				{
					SVQH_PrintToClient(cl, Style, svs.clients[Index - 1].name);
				}
				continue;
			}
			cl->netchan.message.WriteByte(hwsvc_name_print);
			cl->netchan.message.WriteByte(Style);
			cl->netchan.message.WriteByte(Index - 1);	//knows the name, send the index.
		}
		return;
	}

	if (G_FLOAT(OFS_PARM0) == QHMSG_ONE)
	{
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableWrite_Begin(cl, hwsvc_name_print, 3);
		SVQH_ClientReliableWrite_Byte(cl, Style);
		SVQH_ClientReliableWrite_Byte(cl, Index - 1);	//heh, don't need a short here.
	}
	else
	{
		QWWriteDest()->WriteByte(hwsvc_name_print);
		QWWriteDest()->WriteByte(Style);
		QWWriteDest()->WriteByte(Index - 1);	//heh, don't need a short here.
	}
}



/*
=================
PF_print_indexed

print string from strings.txt

print_indexed(to, level, index)
=================
*/
void PF_print_indexed(void)
{
	int Index,Style;

//	plaquemessage = G_STRING(OFS_PARM0);

	Index = ((int)G_FLOAT(OFS_PARM2));
	Style = ((int)G_FLOAT(OFS_PARM1));

	if (hw_spartanPrint->value == 1 && Style < 2)
	{
		return;
	}

	if (Index <= 0)
	{
		PR_RunError("PF_sprint_indexed: index(%d) < 1",Index);
	}

	if (Index > prh2_string_count)
	{
		PR_RunError("PF_sprint_indexed: index(%d) >= prh2_string_count(%d)",Index,prh2_string_count);
	}

	if ((int)G_FLOAT(OFS_PARM0) == QHMSG_BROADCAST)	//broadcast message--send like bprint, print it out on server too.
	{
		client_t* cl;
		int i;

		common->Printf("%s",&prh2_global_strings[prh2_string_index[Index - 1]]);

		for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
		{
			if (Style < cl->messagelevel)
			{
				continue;
			}
			if (!cl->state)
			{
				continue;
			}
			cl->netchan.message.WriteByte(hwsvc_indexed_print);
			cl->netchan.message.WriteByte(Style);
			cl->netchan.message.WriteShort(Index);
		}
		return;
	}

	if (G_FLOAT(OFS_PARM0) == QHMSG_ONE)
	{
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableWrite_Begin(cl, hwsvc_indexed_print, 4);
		SVQH_ClientReliableWrite_Byte(cl, Style);
		SVQH_ClientReliableWrite_Short(cl, Index);
	}
	else
	{
		QWWriteDest()->WriteByte(hwsvc_indexed_print);
		QWWriteDest()->WriteByte(Style);
		QWWriteDest()->WriteShort(Index);
	}
}



/*
=================
PF_bcenterprint2

single print to all

bcenterprint2(value, value)
=================
*/
void PF_bcenterprint2(void)
{
	const char* s;
	client_t* cl;
	int i;

	s = PF_VarString(0);

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		cl->netchan.message.WriteByte(h2svc_centerprint);
		cl->netchan.message.WriteString2(s);
	}
}

/*
=================
PF_centerprint2

single print to a specific client

centerprint(clientent, value, value)
=================
*/
void PF_centerprint2(void)
{
	const char* s;
	client_t* client;
	int entnum;

	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);

	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		common->Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	client->netchan.message.WriteChar(h2svc_centerprint);
	client->netchan.message.WriteString2(s);
}

void PF_plaque_draw(void)
{
	int Index = ((int)G_FLOAT(OFS_PARM1));

	if (Index < 0)
	{
		PR_RunError("PF_plaque_draw: index(%d) < 1",Index);
	}

	if (Index > prh2_string_count)
	{
		PR_RunError("PF_plaque_draw: index(%d) >= prh2_string_count(%d)",Index,prh2_string_count);
	}

	if (G_FLOAT(OFS_PARM0) == QHMSG_ONE)
	{
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableWrite_Begin(cl, hwsvc_plaque, 3);
		SVQH_ClientReliableWrite_Short(cl, Index);
	}
	else
	{
		QWWriteDest()->WriteByte(hwsvc_plaque);
		QWWriteDest()->WriteShort(Index);
	}
}

void PF_rain_go(void)
{
	float* min_org,* max_org,* e_size;
	float* dir;
	vec3_t org,org2;
	int color,count,x_dir,y_dir;

	min_org = G_VECTOR(OFS_PARM0);
	max_org = G_VECTOR(OFS_PARM1);
	e_size  = G_VECTOR(OFS_PARM2);
	dir     = G_VECTOR(OFS_PARM3);
	color   = G_FLOAT(OFS_PARM4);
	count = G_FLOAT(OFS_PARM5);

	org[0] = min_org[0];
	org[1] = min_org[1];
	org[2] = max_org[2];

	org2[0] = e_size[0];
	org2[1] = e_size[1];
	org2[2] = e_size[2];

	x_dir = dir[0];
	y_dir = dir[1];

	SV_StartRainEffect(org,org2,x_dir,y_dir,color,count);
}

void PF_particleexplosion(void)
{
	float* org;
	int color,radius,counter;

	org = G_VECTOR(OFS_PARM0);
	color = G_FLOAT(OFS_PARM1);
	radius = G_FLOAT(OFS_PARM2);
	counter = G_FLOAT(OFS_PARM3);

	sv.qh_datagram.WriteByte(hwsvc_particle_explosion);
	sv.qh_datagram.WriteCoord(org[0]);
	sv.qh_datagram.WriteCoord(org[1]);
	sv.qh_datagram.WriteCoord(org[2]);
	sv.qh_datagram.WriteShort(color);
	sv.qh_datagram.WriteShort(radius);
	sv.qh_datagram.WriteShort(counter);
}

void PF_setclass(void)
{
	float NewClass;
	int entnum;
	qhedict_t* e;
	client_t* client,* old;
	char temp[1024];

	entnum = G_EDICTNUM(OFS_PARM0);
	e = G_EDICT(OFS_PARM0);
	NewClass = G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		common->Printf("tried to change class of a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	old = host_client;
	host_client = client;

	if (NewClass > CLASS_DEMON && dmMode->value != DM_SIEGE)
	{
		NewClass = CLASS_PALADIN;
	}

	e->SetPlayerClass(NewClass);
	host_client->h2_playerclass = NewClass;

	sprintf(temp,"%d",(int)NewClass);
	Info_SetValueForKey(host_client->userinfo, "playerclass", temp, HWMAX_INFO_STRING, 64, 64, !svqh_highchars->value);
	String::NCpy(host_client->name, Info_ValueForKey(host_client->userinfo, "name"),
		sizeof(host_client->name) - 1);
	host_client->qh_sendinfo = true;

	// process any changed values
//	SV_ExtractFromUserinfo (host_client);

	//update everyone else about playerclass change
	sv.qh_reliable_datagram.WriteByte(hwsvc_updatepclass);
	sv.qh_reliable_datagram.WriteByte(entnum - 1);
	sv.qh_reliable_datagram.WriteByte(((host_client->h2_playerclass << 5) | ((int)e->GetLevel() & 31)));
	host_client = old;
}

void PF_setsiegeteam(void)
{
	float NewTeam;
	int entnum;
	qhedict_t* e;
	client_t* client,* old;

	entnum = G_EDICTNUM(OFS_PARM0);
	e = G_EDICT(OFS_PARM0);
	NewTeam = G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		common->Printf("tried to change siege_team of a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	old = host_client;
	host_client = client;

	e->SetSiegeTeam(NewTeam);
	host_client->hw_siege_team = NewTeam;

//???
//	sprintf(temp,"%d",(int)NewTeam);
//	Info_SetValueForKey (host_client->userinfo, "playerclass", temp, HWMAX_INFO_STRING);
//	String::NCpy(host_client->name, Info_ValueForKey (host_client->userinfo, "name")
//		, sizeof(host_client->name)-1);
//	host_client->sendinfo = true;

	//update everyone else about playerclass change
	sv.qh_reliable_datagram.WriteByte(hwsvc_updatesiegeteam);
	sv.qh_reliable_datagram.WriteByte(entnum - 1);
	sv.qh_reliable_datagram.WriteByte(host_client->hw_siege_team);
	host_client = old;
}

void PF_updateSiegeInfo(void)
{
	int j;
	client_t* client;

	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		client->netchan.message.WriteByte(hwsvc_updatesiegeinfo);
		client->netchan.message.WriteByte((int)ceil(timelimit->value));
		client->netchan.message.WriteByte((int)ceil(fraglimit->value));
	}
}

void PF_starteffect(void)
{
	SVH2_ParseEffect(NULL);
}

void PF_endeffect(void)
{
	int index;

	index = G_FLOAT(OFS_PARM0);
	index = G_FLOAT(OFS_PARM1);

	if (!sv.h2_Effects[index].type)
	{
		return;
	}

	sv.h2_Effects[index].type = 0;
	sv.multicast.WriteByte(hwsvc_end_effect);
	sv.multicast.WriteByte(index);
	SVQH_Multicast(vec3_origin, MULTICAST_ALL_R);
}

void PF_turneffect(void)
{
	float* dir, * pos;
	int index;

	index = G_FLOAT(OFS_PARM0);
	pos = G_VECTOR(OFS_PARM1);
	dir = G_VECTOR(OFS_PARM2);

	if (!sv.h2_Effects[index].type)
	{
		return;
	}
	VectorCopy(pos, sv.h2_Effects[index].Missile.origin);
	VectorCopy(dir, sv.h2_Effects[index].Missile.velocity);

	sv.multicast.WriteByte(hwsvc_turn_effect);
	sv.multicast.WriteByte(index);
	sv.multicast.WriteFloat(sv.qh_time);
	sv.multicast.WriteCoord(pos[0]);
	sv.multicast.WriteCoord(pos[1]);
	sv.multicast.WriteCoord(pos[2]);
	sv.multicast.WriteCoord(dir[0]);
	sv.multicast.WriteCoord(dir[1]);
	sv.multicast.WriteCoord(dir[2]);

	SV_MulticastSpecific(sv.h2_Effects[index].client_list, true);
}

void PF_updateeffect(void)	//type-specific what this will send
{
	int index,type,cmd;
	vec3_t tvec;

	index = G_FLOAT(OFS_PARM0);	// the effect we're lookin to change is parm 0
	type = G_FLOAT(OFS_PARM1);	// the type of effect that it had better be is parm 1

	if (!sv.h2_Effects[index].type)
	{
		return;
	}

	if (sv.h2_Effects[index].type != type)
	{
		return;
	}

	//common writing--PLEASE use sent type when determining how much and what to read, so it's safe
	sv.multicast.WriteByte(hwsvc_update_effect);
	sv.multicast.WriteByte(index);	//
	sv.multicast.WriteByte(type);	//paranoia alert--make sure client reads the correct number of bytes

	switch (type)
	{
	case HWCE_SCARABCHAIN:	//new ent to be attached to--pass in 0 for chain retract
		sv.h2_Effects[index].Chain.owner = G_INT(OFS_PARM2) & 0x0fff;
		sv.h2_Effects[index].Chain.material = G_INT(OFS_PARM2) >> 12;

		if (sv.h2_Effects[index].Chain.owner)
		{
			sv.h2_Effects[index].Chain.state = 1;
		}
		else
		{
			sv.h2_Effects[index].Chain.state = 2;
		}

		sv.multicast.WriteShort(G_EDICTNUM(OFS_PARM2));
		break;
	case HWCE_HWSHEEPINATOR:
	case HWCE_HWXBOWSHOOT:
		cmd = G_FLOAT(OFS_PARM2);
		sv.multicast.WriteByte(cmd);
		if (cmd & 1)
		{
			sv.h2_Effects[index].Xbow.activebolts &= ~(1 << ((cmd >> 4) & 7));
			sv.multicast.WriteCoord(G_FLOAT(OFS_PARM3));
		}
		else
		{
			sv.h2_Effects[index].Xbow.vel[(cmd >> 4) & 7][0] = G_FLOAT(OFS_PARM3);
			sv.h2_Effects[index].Xbow.vel[(cmd >> 4) & 7][1] = G_FLOAT(OFS_PARM4);
			sv.h2_Effects[index].Xbow.vel[(cmd >> 4) & 7][2] = 0;

			sv.multicast.WriteAngle(G_FLOAT(OFS_PARM3));
			sv.multicast.WriteAngle(G_FLOAT(OFS_PARM4));
			if (cmd & 128)	//send origin too
			{
				sv.h2_Effects[index].Xbow.turnedbolts |= 1 << ((cmd >> 4) & 7);
				VectorCopy(G_VECTOR(OFS_PARM5), sv.h2_Effects[index].Xbow.origin[(cmd >> 4) & 7]);
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[(cmd >> 4) & 7][0]);
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[(cmd >> 4) & 7][1]);
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[(cmd >> 4) & 7][2]);
			}
		}
		break;
	case HWCE_HWDRILLA:
		cmd = G_FLOAT(OFS_PARM2);
		sv.multicast.WriteByte(cmd);
		if (cmd == 0)
		{
			VectorCopy(G_VECTOR(OFS_PARM3), tvec);
			sv.multicast.WriteCoord(tvec[0]);
			sv.multicast.WriteCoord(tvec[1]);
			sv.multicast.WriteCoord(tvec[2]);
			sv.multicast.WriteByte(G_FLOAT(OFS_PARM4));
		}
		else
		{
			sv.h2_Effects[index].Missile.angle[0] = G_FLOAT(OFS_PARM3);
			sv.multicast.WriteAngle(G_FLOAT(OFS_PARM3));
			sv.h2_Effects[index].Missile.angle[1] = G_FLOAT(OFS_PARM4);
			sv.multicast.WriteAngle(G_FLOAT(OFS_PARM4));

			VectorCopy(G_VECTOR(OFS_PARM5), sv.h2_Effects[index].Missile.origin);
			sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[0]);
			sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[1]);
			sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[2]);
		}
		break;
	}

	SV_MulticastSpecific(sv.h2_Effects[index].client_list, true);
}

void PF_GetString(void)
{
	int Index;

	Index = (int)G_FLOAT(OFS_PARM0) - 1;

	if (Index < 0)
	{
		PR_RunError("PF_GetString: index(%d) < 1",Index + 1);
	}

	if (Index >= prh2_string_count)
	{
		PR_RunError("PF_GetString: index(%d) >= prh2_string_count(%d)",Index,prh2_string_count);
	}

	G_INT(OFS_RETURN) = PR_SetString(&prh2_global_strings[prh2_string_index[Index]]);
}


void PF_setseed(void)
{
	SVHW_setseed(G_FLOAT(OFS_PARM0));
}

void PF_seedrand(void)
{
	G_FLOAT(OFS_RETURN) = SVHW_seedrand();
}

void PF_multieffect(void)
{
	SVHW_ParseMultiEffect(&sv.qh_reliable_datagram);

}

void PF_getmeid(void)
{
	G_FLOAT(OFS_RETURN) = SVHW_GetMultiEffectId();
}

void PF_weapon_sound(void)
{
	qhedict_t* entity;
	int sound_num;
	const char* sample;

	entity = G_EDICT(OFS_PARM0);
	sample = G_STRING(OFS_PARM1);

	for (sound_num = 1; sound_num < MAX_SOUNDS_HW &&
		 sv.qh_sound_precache[sound_num]; sound_num++)
		if (!String::Cmp(sample, sv.qh_sound_precache[sound_num]))
		{
			break;
		}

	if (sound_num == MAX_SOUNDS_HW || !sv.qh_sound_precache[sound_num])
	{
		common->Printf("SVQH_StartSound: %s not precacheed\n", sample);
		return;
	}
	entity->SetWpnSound(sound_num);
}


builtin_t pr_builtin[] =
{
	PF_Fixme,
	PF_makevectors,	// void(entity e)	makevectors         = #1;
	PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
	PFQ1_setmodel,// void(entity e, string m) setmodel	= #3;
	PF_setsize,	// void(entity e, vector min, vector max) setsize = #4;
	PFHW_lightstylestatic,// 5
	PF_break,	// void() break						= #6;
	PF_random,	// float() random						= #7;
	PF_sound,	// void(entity e, float chan, string samp) sound = #8;
	PF_normalize,	// vector(vector v) normalize			= #9;
	PF_error,	// void(string e) error				= #10;
	PF_objerror,// void(string e) objerror				= #11;
	PF_vlen,// float(vector v) vlen				= #12;
	PF_vectoyaw,// float(vector v) vectoyaw		= #13;
	PF_Spawn,	// entity() spawn						= #14;
	PFHW_Remove,	// void(entity e) remove				= #15;
	PF_traceline,	// float(vector v1, vector v2, float tryents) traceline = #16;
	PF_checkclient,	// entity() clientlist					= #17;
	PF_Find,// entity(entity start, .string fld, string match) find = #18;
	PF_precache_sound,	// void(string s) precache_sound		= #19;
	PF_precache_model,	// void(string s) precache_model		= #20;
	PF_stuffcmd,// void(entity client, string s)stuffcmd = #21;
	PF_findradius,	// entity(vector org, float rad) findradius = #22;
	PFQW_bprint,	// void(string s) bprint				= #23;
	PFQW_sprint,	// void(entity client, string s) sprint = #24;
	PF_dprint,	// void(string s) dprint				= #25;
	PF_ftos,// void(string s) ftos				= #26;
	PF_vtos,// void(string s) vtos				= #27;
	PF_coredump,
	PF_traceon,
	PF_traceoff,
	PF_eprint,	// void(entity e) debug print an entire entity
	PFH2_walkmove,// float(float yaw, float dist) walkmove
	PF_tracearea,							// float(vector v1, vector v2, vector mins, vector maxs,
	PF_droptofloor,
	PFQW_lightstyle,
	PF_rint,
	PF_floor,
	PF_ceil,
	PF_Fixme,
	PF_checkbottom,
	PF_pointcontents,
	PF_particle2,
	PF_fabs,
	PFH2_aim,
	PF_cvar,
	PF_localcmd,
	PF_nextent,
	PF_particle,							//																			= #48
	PF_changeyaw,
	PF_vhlen,								// float(vector v) vhlen											= #50
	PF_vectoangles,

	PFQW_WriteByte,
	PFQW_WriteChar,
	PFQW_WriteShort,
	PFQW_WriteLong,
	PFQW_WriteCoord,
	PFQW_WriteAngle,
	PFQW_WriteString,
	PFQW_WriteEntity,

	PF_dprintf,								// void(string s1, string s2) dprint										= #60
	PF_Cos,									//																			= #61
	PF_Sin,									//																			= #62
	PFHW_AdvanceFrame,						//																			= #63
	PF_dprintv,								// void(string s1, string s2) dprint										= #64
	PF_RewindFrame,							//																			= #65
	PF_setclass,

	SVQH_MoveToGoal,
	PF_precache_file,
	PFH2_makestatic,

	PFH2_changelevel,
	PFHW_lightstylevalue,	// 71

	PF_cvar_set,
	PF_centerprint,

	PF_ambientsound,

	PF_precache_model2,
	PF_precache_sound2,	// precache_sound2 is different only for qcc
	PF_precache_file,

	PF_setspawnparms,

	PF_plaque_draw,
	PF_rain_go,									//																				=	#80
	PF_particleexplosion,						//																				=	#81
	PF_movestep,
	PF_advanceweaponframe,
	PF_sqrt,

	PF_particle3,							// 85
	PF_particle4,							// 86
	PF_setpuzzlemodel,						// 87

	PF_starteffect,							// 88
	PF_endeffect,							// 89

	PF_precache_puzzle_model,				// 90
	PF_concatv,								// 91
	PF_GetString,							// 92
	PF_SpawnTemp,							// 93
	PF_v_factor,							// 94
	PF_v_factorrange,						// 95

	PF_precache_sound3,						// 96
	PF_precache_model3,						// 97
	PF_precache_file,						// 98

	PF_logfrag,	//99

	PFHW_infokey,	//100
	PF_stof,	//101
	PF_multicast,	//102
	PF_turneffect,	//103
	PF_updateeffect,//104
	PF_setseed,	//105
	PF_seedrand,//106
	PF_multieffect,	//107
	PF_getmeid,	//108
	PF_weapon_sound,//109
	PF_bcenterprint2,	//110
	PF_print_indexed,	//111
	PF_centerprint2,//112
	PF_name_print,	//113
	PF_StopSound,	//114
	PF_UpdateSoundPos,	//115

	PF_precache_sound,			// 116
	PF_precache_model,			// 117
	PF_precache_file,			// 118
	PF_setsiegeteam,			// 119
	PF_updateSiegeInfo,			// 120
};

void PR_InitBuiltins()
{
	pr_builtins = pr_builtin;
	pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);
}
