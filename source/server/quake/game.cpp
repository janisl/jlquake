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
#include "../quake_hexen/local.h"
#include "local.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"

static void PFQ1_makestatic()
{
	qhedict_t* ent = G_EDICT(OFS_PARM0);

	sv.qh_signon.WriteByte(q1svc_spawnstatic);

	sv.qh_signon.WriteByte(SVQH_ModelIndex(PR_GetString(ent->GetModel())));

	sv.qh_signon.WriteByte(ent->GetFrame());
	sv.qh_signon.WriteByte(ent->GetColorMap());
	sv.qh_signon.WriteByte(ent->GetSkin());
	for (int i = 0; i < 3; i++)
	{
		sv.qh_signon.WriteCoord(ent->GetOrigin()[i]);
		sv.qh_signon.WriteAngle(ent->GetAngles()[i]);
	}

	// throw the entity away now
	ED_Free(ent);
}

static void PFQ1_changelevel()
{
	// make sure we don't issue two changelevels
	if (svs.qh_changelevel_issued)
	{
		return;
	}
	svs.qh_changelevel_issued = true;

	const char* s = G_STRING(OFS_PARM0);
	Cbuf_AddText(va("changelevel %s\n",s));
}

static void PFQW_changelevel()
{
	static int last_spawncount;

	// make sure we don't issue two changelevels
	if (svs.spawncount == last_spawncount)
	{
		return;
	}
	last_spawncount = svs.spawncount;

	const char* s = G_STRING(OFS_PARM0);
	Cbuf_AddText(va("map %s\n",s));
}

static void PFQW_infokey()
{
	qhedict_t* e;
	int e1;
	const char* value;
	const char* key;
	static char ov[256];

	e = G_EDICT(OFS_PARM0);
	e1 = QH_NUM_FOR_EDICT(e);
	key = G_STRING(OFS_PARM1);

	if (e1 == 0)
	{
		if ((value = Info_ValueForKey(svs.qh_info, key)) == NULL ||
			!*value)
		{
			value = Info_ValueForKey(qhw_localinfo, key);
		}
	}
	else if (e1 <= MAX_CLIENTS_QHW)
	{
		if (!String::Cmp(key, "ip"))
		{
			String::Cpy(ov, SOCK_BaseAdrToString(svs.clients[e1 - 1].netchan.remoteAddress));
			value = ov;
		}
		else if (!String::Cmp(key, "ping"))
		{
			int ping = SVQH_CalcPing(&svs.clients[e1 - 1]);
			sprintf(ov, "%d", ping);
			value = ov;
		}
		else
		{
			value = Info_ValueForKey(svs.clients[e1 - 1].userinfo, key);
		}
	}
	else
	{
		value = "";
	}

	RETURN_STRING(value);
}

static builtin_t prq1_builtin[] =
{
	PF_Fixme,
	PF_makevectors,	// void(entity e)	makevectors         = #1;
	PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
	PFQ1_setmodel,// void(entity e, string m) setmodel	= #3;
	PF_setsize,	// void(entity e, vector min, vector max) setsize = #4;
	PF_Fixme,	// void(entity e, vector min, vector max) setabssize = #5;
	PF_break,	// void() break						= #6;
	PF_random,	// float() random						= #7;
	PF_sound,	// void(entity e, float chan, string samp) sound = #8;
	PF_normalize,	// vector(vector v) normalize			= #9;
	PF_error,	// void(string e) error				= #10;
	PF_objerror,// void(string e) objerror				= #11;
	PF_vlen,// float(vector v) vlen				= #12;
	PF_vectoyaw,// float(vector v) vectoyaw		= #13;
	PF_Spawn,	// entity() spawn						= #14;
	PFQ1_Remove,	// void(entity e) remove				= #15;
	PF_traceline,	// float(vector v1, vector v2, float tryents) traceline = #16;
	PF_checkclient,	// entity() clientlist					= #17;
	PF_Find,// entity(entity start, .string fld, string match) find = #18;
	PF_precache_sound,	// void(string s) precache_sound		= #19;
	PF_precache_model,	// void(string s) precache_model		= #20;
	PF_stuffcmd,// void(entity client, string s)stuffcmd = #21;
	PF_findradius,	// entity(vector org, float rad) findradius = #22;
	PFQ1_bprint,	// void(string s) bprint				= #23;
	PFQ1_sprint,	// void(entity client, string s) sprint = #24;
	PF_dprint,	// void(string s) dprint				= #25;
	PF_ftos,// void(string s) ftos				= #26;
	PF_vtos,// void(string s) vtos				= #27;
	PF_coredump,
	PF_traceon,
	PF_traceoff,
	PF_eprint,	// void(entity e) debug print an entire entity
	PFQ1_walkmove,// float(float yaw, float dist) walkmove
	PF_Fixme,	// float(float yaw, float dist) walkmove
	PF_droptofloor,
	PFQ1_lightstyle,
	PF_rint,
	PF_floor,
	PF_ceil,
	PF_Fixme,
	PF_checkbottom,
	PF_pointcontents,
	PF_Fixme,
	PF_fabs,
	PFQ1_aim,
	PF_cvar,
	PF_localcmd,
	PF_nextent,
	PF_particle,
	PF_changeyaw,
	PF_Fixme,
	PF_vectoangles,

	PFQ1_WriteByte,
	PFQ1_WriteChar,
	PFQ1_WriteShort,
	PFQ1_WriteLong,
	PFQ1_WriteCoord,
	PFQ1_WriteAngle,
	PFQ1_WriteString,
	PFQ1_WriteEntity,

	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,

	SVQH_MoveToGoal,
	PF_precache_file,
	PFQ1_makestatic,

	PFQ1_changelevel,
	PF_Fixme,

	PF_cvar_set,
	PF_centerprint,

	PF_ambientsound,

	PF_precache_model,
	PF_precache_sound,	// precache_sound2 is different only for qcc
	PF_precache_file,

	PF_setspawnparms
};

void PRQ1_InitBuiltins()
{
	pr_builtins = prq1_builtin;
	pr_numbuiltins = sizeof(prq1_builtin) / sizeof(prq1_builtin[0]);
}

static builtin_t prqw_builtin[] =
{
	PF_Fixme,
	PF_makevectors,	// void(entity e)	makevectors         = #1;
	PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
	PFQW_setmodel,// void(entity e, string m) setmodel	= #3;
	PF_setsize,	// void(entity e, vector min, vector max) setsize = #4;
	PF_Fixme,	// void(entity e, vector min, vector max) setabssize = #5;
	PF_break,	// void() break						= #6;
	PF_random,	// float() random						= #7;
	PF_sound,	// void(entity e, float chan, string samp) sound = #8;
	PF_normalize,	// vector(vector v) normalize			= #9;
	PF_error,	// void(string e) error				= #10;
	PF_objerror,// void(string e) objerror				= #11;
	PF_vlen,// float(vector v) vlen				= #12;
	PF_vectoyaw,// float(vector v) vectoyaw		= #13;
	PF_Spawn,	// entity() spawn						= #14;
	PFQ1_Remove,	// void(entity e) remove				= #15;
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
	PFQ1_walkmove,// float(float yaw, float dist) walkmove
	PF_Fixme,	// float(float yaw, float dist) walkmove
	PF_droptofloor,
	PFQW_lightstyle,
	PF_rint,
	PF_floor,
	PF_ceil,
	PF_Fixme,
	PF_checkbottom,
	PF_pointcontents,
	PF_Fixme,
	PF_fabs,
	PFQW_aim,
	PF_cvar,
	PF_localcmd,
	PF_nextent,
	PF_Fixme,
	PF_changeyaw,
	PF_Fixme,
	PF_vectoangles,

	PFQW_WriteByte,
	PFQW_WriteChar,
	PFQW_WriteShort,
	PFQW_WriteLong,
	PFQW_WriteCoord,
	PFQW_WriteAngle,
	PFQW_WriteString,
	PFQW_WriteEntity,

	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,

	SVQH_MoveToGoal,
	PF_precache_file,
	PFQ1_makestatic,

	PFQW_changelevel,
	PF_Fixme,

	PF_cvar_set,
	PF_centerprint,

	PF_ambientsound,

	PF_precache_model,
	PF_precache_sound,	// precache_sound2 is different only for qcc
	PF_precache_file,

	PF_setspawnparms,

	PF_logfrag,

	PFQW_infokey,
	PF_stof,
	PF_multicast
};

void PRQW_InitBuiltins()
{
	pr_builtins = prqw_builtin;
	pr_numbuiltins = sizeof(prqw_builtin) / sizeof(prqw_builtin[0]);
}
