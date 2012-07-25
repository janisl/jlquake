/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qwsvdef.h"

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
	int level;

	level = G_FLOAT(OFS_PARM0);

	s = PF_VarString(1);
	SV_BroadcastPrintf(level, "%s", s);
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
	int level;

	entnum = G_EDICTNUM(OFS_PARM0);
	level = G_FLOAT(OFS_PARM1);

	s = PF_VarString(2);

	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		common->Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	SV_ClientPrintf(client, level, "%s", s);
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
	int entnum;
	client_t* cl;

	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);

	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		common->Printf("tried to sprint to a non-client\n");
		return;
	}

	cl = &svs.clients[entnum - 1];

	SVQH_ClientReliableWrite_Begin(cl, q1svc_centerprint, 2 + String::Length(s));
	SVQH_ClientReliableWrite_String(cl, s);
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
	client_t* cl;

	entnum = G_EDICTNUM(OFS_PARM0);
	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		PR_RunError("Parm 0 not a client");
	}
	str = G_STRING(OFS_PARM1);

	cl = &svs.clients[entnum - 1];

	if (String::Cmp(str, "disconnect\n") == 0)
	{
		// so long and thanks for all the fish
		cl->qw_drop = true;
		return;
	}

	SVQH_ClientReliableWrite_Begin(cl, q1svc_stufftext, 2 + String::Length(str));
	SVQH_ClientReliableWrite_String(cl, str);
}

void PF_makestatic(void)
{
	qhedict_t* ent;
	int i;

	ent = G_EDICT(OFS_PARM0);

	sv.qh_signon.WriteByte(q1svc_spawnstatic);

	sv.qh_signon.WriteByte(SV_ModelIndex(PR_GetString(ent->GetModel())));

	sv.qh_signon.WriteByte(ent->GetFrame());
	sv.qh_signon.WriteByte(ent->GetColorMap());
	sv.qh_signon.WriteByte(ent->GetSkin());
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
	const char* s;
	static int last_spawncount;

// make sure we don't issue two changelevels
	if (svs.spawncount == last_spawncount)
	{
		return;
	}
	last_spawncount = svs.spawncount;

	s = G_STRING(OFS_PARM0);
	Cbuf_AddText(va("map %s\n",s));
}



/*
==============
PF_infokey

string(entity e, string key) infokey
==============
*/
void PF_infokey(void)
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
			value = Info_ValueForKey(localinfo, key);
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
			int ping = SV_CalcPing(&svs.clients[e1 - 1]);
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

builtin_t pr_builtin[] =
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
	PF_bprint,	// void(string s) bprint				= #23;
	PF_sprint,	// void(entity client, string s) sprint = #24;
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
	PF_makestatic,

	PF_changelevel,
	PF_Fixme,

	PF_cvar_set,
	PF_centerprint,

	PF_ambientsound,

	PF_precache_model,
	PF_precache_sound,	// precache_sound2 is different only for qcc
	PF_precache_file,

	PF_setspawnparms,

	PF_logfrag,

	PF_infokey,
	PF_stof,
	PF_multicast
};

void PR_InitBuiltins()
{
	pr_builtins = pr_builtin;
	pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);
}
