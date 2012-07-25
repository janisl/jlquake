
/*
 * $Header: /H2 Mission Pack/Pr_cmds.c 26    3/23/98 7:24p Jmonroe $
 */

#include "quakedef.h"
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
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint(void)
{
	const char* s;

	s = PF_VarString(0);
	SVQH_BroadcastPrintf(0, "%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF_sprint(void)
{
	const char* s;
	client_t* client;
	int entnum;

	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);

	if (entnum < 1 || entnum > svs.qh_maxclients)
	{
		common->Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	SVQH_ClientPrintf(client, 0, "%s", s);
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF_centerprint(void)
{
	const char* s;
	client_t* client;
	int entnum;

	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);

	if (entnum < 1 || entnum > svs.qh_maxclients)
	{
		common->Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	client->qh_message.WriteChar(h2svc_centerprint);
	client->qh_message.WriteString2(s);
}


/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void PF_particle(void)
{
	float* org, * dir;
	float color;
	float count;

	org = G_VECTOR(OFS_PARM0);
	dir = G_VECTOR(OFS_PARM1);
	color = G_FLOAT(OFS_PARM2);
	count = G_FLOAT(OFS_PARM3);
	SV_StartParticle(org, dir, color, count);
}


/*
=================
PF_particle2

particle(origin, dmin, dmax, color, effect, count)
=================
*/
void PF_particle2(void)
{
	float* org, * dmin, * dmax;
	float color;
	float count;
	float effect;

	org = G_VECTOR(OFS_PARM0);
	dmin = G_VECTOR(OFS_PARM1);
	dmax = G_VECTOR(OFS_PARM2);
	color = G_FLOAT(OFS_PARM3);
	effect = G_FLOAT(OFS_PARM4);
	count = G_FLOAT(OFS_PARM5);
	SV_StartParticle2(org, dmin, dmax, color, effect, count);
}


/*
=================
PF_particle3

particle(origin, box, color, effect, count)
=================
*/
void PF_particle3(void)
{
	float* org, * box;
	float color;
	float count;
	float effect;

	org = G_VECTOR(OFS_PARM0);
	box = G_VECTOR(OFS_PARM1);
	color = G_FLOAT(OFS_PARM2);
	effect = G_FLOAT(OFS_PARM3);
	count = G_FLOAT(OFS_PARM4);
	SV_StartParticle3(org, box, color, effect, count);
}

/*
=================
PF_particle4

particle(origin, radius, color, effect, count)
=================
*/
void PF_particle4(void)
{
	float* org;
	float radius;
	float color;
	float count;
	float effect;

	org = G_VECTOR(OFS_PARM0);
	radius = G_FLOAT(OFS_PARM1);
	color = G_FLOAT(OFS_PARM2);
	effect = G_FLOAT(OFS_PARM3);
	count = G_FLOAT(OFS_PARM4);
	SV_StartParticle4(org, radius, color, effect, count);
}

/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd(void)
{
	int entnum;
	const char* str;
	client_t* old;

	entnum = G_EDICTNUM(OFS_PARM0);
	if (entnum < 1 || entnum > svs.qh_maxclients)
	{
		PR_RunError("Parm 0 not a client");
	}
	str = G_STRING(OFS_PARM1);

	old = host_client;
	host_client = &svs.clients[entnum - 1];
	SVQH_SendClientCommand(host_client, "%s", str);
	host_client = old;
}

//==========================================================================
//
// PFH2_lightstylevalue
//
// void lightstylevalue(float style);
//
//==========================================================================

void PFH2_lightstylevalue(void)
{
	int style;

	style = G_FLOAT(OFS_PARM0);
	if (style < 0 || style >= MAX_LIGHTSTYLES_H2)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

#ifdef DEDICATED
	G_FLOAT(OFS_RETURN) = 0;
#else
	G_FLOAT(OFS_RETURN) = (int)(cl_lightstyle[style].value[0] / 22.0 * 256.0);
#endif
}


void PF_makestatic(void)
{
	qhedict_t* ent;
	int i;

	ent = G_EDICT(OFS_PARM0);

	sv.qh_signon.WriteByte(h2svc_spawnstatic);

	sv.qh_signon.WriteShort(SV_ModelIndex(PR_GetString(ent->GetModel())));

	sv.qh_signon.WriteByte(ent->GetFrame());
	sv.qh_signon.WriteByte(ent->GetColorMap());
	sv.qh_signon.WriteByte(ent->GetSkin());
	sv.qh_signon.WriteByte((int)(ent->GetScale() * 100.0) & 255);
	sv.qh_signon.WriteByte(ent->GetDrawFlags());
	sv.qh_signon.WriteByte((int)(ent->GetAbsLight() * 255.0) & 255);

	for (i = 0; i < 3; i++)
	{
		sv.qh_signon.WriteCoord(ent->GetOrigin()[i]);
		sv.qh_signon.WriteAngle(ent->GetAngles()[i]);
	}

// throw the entity away now
	ED_Free(ent);
}

//=============================================================================

/*
==============
PF_changelevel
==============
*/
void PF_changelevel(void)
{
	const char* s1, * s2;

	if (svs.qh_changelevel_issued)
	{
		return;
	}
	svs.qh_changelevel_issued = true;

	s1 = G_STRING(OFS_PARM0);
	s2 = G_STRING(OFS_PARM1);

	if ((int)*pr_globalVars.serverflags & (SFL_NEW_UNIT | SFL_NEW_EPISODE))
	{
		Cbuf_AddText(va("changelevel %s %s\n",s1, s2));
	}
	else
	{
		Cbuf_AddText(va("changelevel2 %s %s\n",s1, s2));
	}
}

void PF_plaque_draw(void)
{
	int Index = ((int)G_FLOAT(OFS_PARM1));

	if (Index < 0)
	{
		PR_RunError("PF_plaque_draw: index(%d) < 1",Index);
	}

	if (Index > pr_string_count)
	{
		PR_RunError("PF_plaque_draw: index(%d) >= pr_string_count(%d)",Index,pr_string_count);
	}

	Q1WriteDest()->WriteByte(h2svc_plaque);
	Q1WriteDest()->WriteShort(Index);
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

//void SV_StartRainEffect (vec3_t org, vec3_t e_size, int x_dir, int y_dir, int color, int count)
	{
		sv.qh_datagram.WriteByte(h2svc_raineffect);
		sv.qh_datagram.WriteCoord(org[0]);
		sv.qh_datagram.WriteCoord(org[1]);
		sv.qh_datagram.WriteCoord(org[2]);
		sv.qh_datagram.WriteCoord(e_size[0]);
		sv.qh_datagram.WriteCoord(e_size[1]);
		sv.qh_datagram.WriteCoord(e_size[2]);
		sv.qh_datagram.WriteAngle(x_dir);
		sv.qh_datagram.WriteAngle(y_dir);
		sv.qh_datagram.WriteShort(color);
		sv.qh_datagram.WriteShort(count);

//	SV_Multicast (org, MULTICAST_PVS);
	}

}

void PF_particleexplosion(void)
{
	float* org;
	int color,radius,counter;

	org = G_VECTOR(OFS_PARM0);
	color = G_FLOAT(OFS_PARM1);
	radius = G_FLOAT(OFS_PARM2);
	counter = G_FLOAT(OFS_PARM3);

	sv.qh_datagram.WriteByte(h2svc_particle_explosion);
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

	entnum = G_EDICTNUM(OFS_PARM0);
	e = G_EDICT(OFS_PARM0);
	NewClass = G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > svs.qh_maxclients)
	{
		common->Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	old = host_client;
	host_client = client;
	SVQH_SendClientCommand(host_client, "playerclass %i\n", (int)NewClass);
	host_client = old;

	// These will get set again after the message has filtered its way
	// but it wouldn't take affect right away
	e->SetPlayerClass(NewClass);
	client->h2_playerclass = NewClass;
}

void PF_starteffect(void)
{
	SV_ParseEffect(&sv.qh_reliable_datagram);
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
	sv.qh_reliable_datagram.WriteByte(h2svc_end_effect);
	sv.qh_reliable_datagram.WriteByte(index);
}

void PF_GetString(void)
{
	int Index;

	Index = (int)G_FLOAT(OFS_PARM0) - 1;

	if (Index < 0)
	{
		PR_RunError("PF_GetString: index(%d) < 1",Index + 1);
	}

	if (Index >= pr_string_count)
	{
		PR_RunError("PF_GetString: index(%d) >= pr_string_count(%d)",Index,pr_string_count);
	}

	G_INT(OFS_RETURN) = PR_SetString(&pr_global_strings[pr_string_index[Index]]);
}

extern void V_WhiteFlash_f(void);

void PF_doWhiteFlash(void)
{
#ifndef DEDICATED
	V_WhiteFlash_f();
#endif
}

builtin_t pr_builtin[] =
{
	PF_Fixme,

	PF_makevectors,		// 1
	PF_setorigin,		// 2
	PFQ1_setmodel,		// 3
	PF_setsize,			// 4
	PFH2_lightstylestatic,// 5

	PF_break,								// void() break														= #6
	PF_random,								// float() random														= #7
	PF_sound,								// void(entity e, float chan, string samp) sound			= #8
	PF_normalize,							// vector(vector v) normalize										= #9
	PF_error,								// void(string e) error												= #10
	PF_objerror,							// void(string e) objerror											= #11
	PF_vlen,									// float(vector v) vlen												= #12
	PF_vectoyaw,							// float(vector v) vectoyaw										= #13
	PF_Spawn,								// entity() spawn														= #14
	PFH2_Remove,								// void(entity e) remove											= #15
	PF_traceline,							// float(vector v1, vector v2, float tryents) traceline	= #16
	PF_checkclient,						// entity() clientlist												= #17
	PF_Find,									// entity(entity start, .string fld, string match) find	= #18
	PF_precache_sound,					// void(string s) precache_sound									= #19
	PF_precache_model,					// void(string s) precache_model									= #20
	PF_stuffcmd,							// void(entity client, string s)stuffcmd						= #21
	PF_findradius,							// entity(vector org, float rad) findradius					= #22
	PF_bprint,								// void(string s) bprint											= #23
	PF_sprint,								// void(entity client, string s) sprint						= #24
	PF_dprint,								// void(string s) dprint											= #25
	PF_ftos,									// void(string s) ftos												= #26
	PF_vtos,									// void(string s) vto				s								= #27
	PF_coredump,							//	PF_coredump															= #28
	PF_traceon,								// PF_traceon															= #29
	PF_traceoff,							// PF_traceoff															= #30
	PF_eprint,								// void(entity e) debug print an entire entity				= #31
	PFH2_walkmove,							// float(float yaw, float dist) walkmove						= #32
	PF_tracearea,							// float(vector v1, vector v2, vector mins, vector maxs,
	//       float tryents) traceline								= #33
	PF_droptofloor,						// PF_droptofloor														= #34
	PFQ1_lightstyle,							//																			= #35
	PFH2_rint,									//																			= #36
	PF_floor,								//																			= #37
	PF_ceil,									//																			= #38
	PF_Fixme,
	PF_checkbottom,						//																			= #40
	PF_pointcontents,						//																			= #41
	PF_particle2,
	PF_fabs,									//																			= #43
	PFH2_aim,									//																			= #44
	PF_cvar,									//																			= #45
	PF_localcmd,							//																			= #46
	PF_nextent,								//																			= #47
	PF_particle,							//																			= #48
	PF_changeyaw,							//																			= #49
	PF_vhlen,								// float(vector v) vhlen											= #50
	PF_vectoangles,						//																			= #51

	PFQ1_WriteByte,							//																			= #52
	PFQ1_WriteChar,							//																			= #53
	PFQ1_WriteShort,							//																			= #54
	PFQ1_WriteLong,							//																			= #55
	PFQ1_WriteCoord,							//																			= #56
	PFQ1_WriteAngle,							//																			= #57
	PFQ1_WriteString,						//																			= #58
	PFQ1_WriteEntity,						//																			= #59

//PF_FindPath,								//																			= #60
	PF_dprintf,								// void(string s1, string s2) dprint										= #60
	PF_Cos,									//																			= #61
	PF_Sin,									//																			= #62
	PFH2_AdvanceFrame,						//																			= #63
	PF_dprintv,								// void(string s1, string s2) dprint										= #64
	PF_RewindFrame,							//																			= #65
	PF_setclass,

	SVQH_MoveToGoal,
	PF_precache_file,
	PF_makestatic,

	PF_changelevel,

	PFH2_lightstylevalue,	// 71

	PF_cvar_set,
	PF_centerprint,

	PF_ambientsound,

	PF_precache_model2,
	PF_precache_sound2,						// precache_sound2 is different only for qcc
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
	PF_matchAngleToSlope,					// 99
	PF_updateInfoPlaque,					//100

	PF_precache_sound4,						// 101
	PF_precache_model4,						// 102
	PF_precache_file,						// 103

	PF_doWhiteFlash,						//104
	PF_UpdateSoundPos,						//105
	PF_StopSound,							//106

	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
};

void PR_InitBuiltins()
{
	pr_builtins = pr_builtin;
	pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);
}
