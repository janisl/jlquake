
/*
 * $Header: /H2 Mission Pack/Pr_cmds.c 26    3/23/98 7:24p Jmonroe $
 */

#include "quakedef.h"
#ifdef _WIN32
#include <windows.h>
#endif

extern unsigned int info_mask, info_mask2;

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
	SV_BroadcastPrintf("%s", s);
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
		Con_Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	client->qh_message.WriteChar(h2svc_print);
	client->qh_message.WriteString2(s);
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
		Con_Printf("tried to sprint to a non-client\n");
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
PF_StopSound
    stop ent's sound on this chan
=================
*/
void PF_StopSound(void)
{
	int channel;
	qhedict_t* entity;

	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);

	if (channel < 0 || channel > 7)
	{
		Sys_Error("SVQH_StartSound: channel = %i", channel);
	}

	SVH2_StopSound(entity, channel);
}

/*
=================
PF_UpdateSoundPos
    sends cur pos to client to update this ent/chan pair
=================
*/
void PF_UpdateSoundPos(void)
{
	int channel;
	qhedict_t* entity;

	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);

	if (channel < 0 || channel > 7)
	{
		Sys_Error("SVQH_StartSound: channel = %i", channel);
	}

	SVH2_UpdateSoundPos(entity, channel);
}

/*
=================
PF_tracearea

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

tracearea (vector1, vector2, mins, maxs, tryents)
=================
*/
void PF_tracearea(void)
{
	float* v1, * v2, * mins, * maxs;
	q1trace_t trace;
	int nomonsters;
	qhedict_t* ent;
	float save_hull;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	mins = G_VECTOR(OFS_PARM2);
	maxs = G_VECTOR(OFS_PARM3);
	nomonsters = G_FLOAT(OFS_PARM4);
	ent = G_EDICT(OFS_PARM5);

	save_hull = ent->GetHull();
	ent->SetHull(0);
	trace = SVQH_Move(v1, mins, maxs, v2, nomonsters, ent);
	ent->SetHull(save_hull);

	SVQH_SetMoveTrace(trace);
}



struct PointInfo_t
{
	char Found,NumFound,MarkedWhen;
	struct PointInfo_t* FromPos, * Next;
};

#define MAX_POINT_X 21
#define MAX_POINT_Y 21
#define MAX_POINT_Z 11
#define MAX_POINT (MAX_POINT_X * MAX_POINT_Y * MAX_POINT_Z)

#define POINT_POS(x,y,z) ((z * ZOffset) + (y * YOffset) + (x))

#define POINT_X_SIZE 160
#define POINT_Y_SIZE 160
#define POINT_Z_SIZE 50

#define POINT_MAX_DEPTH 5

struct PointInfo_t PI[MAX_POINT];
int ZOffset,YOffset;

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
	Host_ClientCommands("%s", str);
	host_client = old;
}

void PF_SpawnTemp(void)
{
	qhedict_t* ed;

	ed = ED_Alloc_Temp();

	RETURN_EDICT(ed);
}

void PF_FindFloat(void)
{
	int e;
	int f;
	float s, t;
	qhedict_t* ed;

	e = G_EDICTNUM(OFS_PARM0);
	f = G_INT(OFS_PARM1);
	s = G_FLOAT(OFS_PARM2);
	if (!s)
	{
		PR_RunError("PF_Find: bad search string");
	}

	for (e++; e < sv.qh_num_edicts; e++)
	{
		ed = QH_EDICT_NUM(e);
		if (ed->free)
		{
			continue;
		}
		t = E_FLOAT(ed,f);
		if (t == s)
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv.qh_edicts);
}

void PF_precache_puzzle_model(void)
{
	int i;
	const char* s;
	char temp[256];
	const char* m;

	if (sv.state != SS_LOADING)
	{
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
	}

	m = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);

	sprintf(temp,"models/puzzle/%s.mdl",m);
	s = ED_NewString(temp);

	PR_CheckEmptyString(s);

	for (i = 0; i < MAX_MODELS_H2; i++)
	{
		if (!sv.qh_model_precache[i])
		{
			sv.qh_model_precache[i] = s;
			sv.models[i] = CM_PrecacheModel(s);
			return;
		}
		if (!String::Cmp(sv.qh_model_precache[i], s))
		{
			return;
		}
	}
	PR_RunError("PF_precache_puzzle_model: overflow");
}


//==========================================================================
//
// PF_lightstylevalue
//
// void lightstylevalue(float style);
//
//==========================================================================

void PF_lightstylevalue(void)
{
	int style;

	style = G_FLOAT(OFS_PARM0);
	if (style < 0 || style >= MAX_LIGHTSTYLES_H2)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	G_FLOAT(OFS_RETURN) = (int)(cl_lightstyle[style].value[0] / 22.0 * 256.0);
}

//==========================================================================
//
// PF_lightstylestatic
//
// void lightstylestatic(float style, float value);
//
//==========================================================================

void PF_lightstylestatic(void)
{
	int i;
	int value;
	int styleNumber;
	const char* styleString;
	client_t* client;
	static const char* styleDefs[] =
	{
		"a", "b", "c", "d", "e", "f", "g",
		"h", "i", "j", "k", "l", "m", "n",
		"o", "p", "q", "r", "s", "t", "u",
		"v", "w", "x", "y", "z"
	};

	styleNumber = G_FLOAT(OFS_PARM0);
	value = G_FLOAT(OFS_PARM1);
	if (value < 0)
	{
		value = 0;
	}
	else if (value > 'z' - 'a')
	{
		value = 'z' - 'a';
	}
	styleString = styleDefs[value];

	// Change the string in sv
	sv.qh_lightstyles[styleNumber] = styleString;

	if (sv.state != SS_GAME)
	{
		return;
	}

	// Send message to all clients on this server
	for (i = 0, client = svs.clients; i < svs.qh_maxclients; i++, client++)
	{
		if (client->state >= CS_CONNECTED)
		{
			client->qh_message.WriteChar(h2svc_lightstyle);
			client->qh_message.WriteChar(styleNumber);
			client->qh_message.WriteString2(styleString);
		}
	}
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
	int Index;

//	plaquemessage = G_STRING(OFS_PARM0);

	Index = ((int)G_FLOAT(OFS_PARM1));

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

void PF_movestep(void)
{
	vec3_t v;
	qhedict_t* ent;
	dfunction_t* oldf;
	int oldself;
	qboolean set_trace;

	ent = PROG_TO_EDICT(*pr_globalVars.self);

	v[0] = G_FLOAT(OFS_PARM0);
	v[1] = G_FLOAT(OFS_PARM1);
	v[2] = G_FLOAT(OFS_PARM2);
	set_trace = G_FLOAT(OFS_PARM3);

// save program state, because SVQH_movestep may call other progs
	oldf = pr_xfunction;
	oldself = *pr_globalVars.self;

	G_INT(OFS_RETURN) = SVQH_movestep(ent, v, false, true, set_trace);

// restore program state
	pr_xfunction = oldf;
	*pr_globalVars.self = oldself;
}

void PF_AdvanceFrame(void)
{
	qhedict_t* Ent;
	float Start,End,Result;

	Ent = PROG_TO_EDICT(*pr_globalVars.self);
	Start = G_FLOAT(OFS_PARM0);
	End = G_FLOAT(OFS_PARM1);

	if (Ent->GetFrame() < Start || Ent->GetFrame() > End)
	{	// Didn't start in the range
		Ent->SetFrame(Start);
		Result = 0;
	}
	else if (Ent->GetFrame() == End)
	{	// Wrapping
		Ent->SetFrame(Start);
		Result = 1;
	}
	else
	{	// Regular Advance
		Ent->SetFrame(Ent->GetFrame() + 1);
		if (Ent->GetFrame() == End)
		{
			Result = 2;
		}
		else
		{
			Result = 0;
		}
	}

	G_FLOAT(OFS_RETURN) = Result;
}

void PF_RewindFrame(void)
{
	qhedict_t* Ent;
	float Start,End,Result;

	Ent = PROG_TO_EDICT(*pr_globalVars.self);
	Start = G_FLOAT(OFS_PARM0);
	End = G_FLOAT(OFS_PARM1);

	if (Ent->GetFrame() > Start || Ent->GetFrame() < End)
	{	// Didn't start in the range
		Ent->SetFrame(Start);
		Result = 0;
	}
	else if (Ent->GetFrame() == End)
	{	// Wrapping
		Ent->SetFrame(Start);
		Result = 1;
	}
	else
	{	// Regular Advance
		Ent->SetFrame(Ent->GetFrame() - 1);
		if (Ent->GetFrame() == End)
		{
			Result = 2;
		}
		else
		{
			Result = 0;
		}
	}

	G_FLOAT(OFS_RETURN) = Result;
}

#define WF_NORMAL_ADVANCE 0
#define WF_CYCLE_STARTED 1
#define WF_CYCLE_WRAPPED 2
#define WF_LAST_FRAME 3

void PF_advanceweaponframe(void)
{
	qhedict_t* ent;
	float startframe,endframe;
	float state;

	ent = PROG_TO_EDICT(*pr_globalVars.self);
	startframe = G_FLOAT(OFS_PARM0);
	endframe = G_FLOAT(OFS_PARM1);

	if ((endframe > startframe && (ent->GetWeaponFrame() > endframe || ent->GetWeaponFrame() < startframe)) ||
		(endframe < startframe && (ent->GetWeaponFrame() < endframe || ent->GetWeaponFrame() > startframe)))
	{
		ent->SetWeaponFrame(startframe);
		state = WF_CYCLE_STARTED;
	}
	else if (ent->GetWeaponFrame() == endframe)
	{
		ent->SetWeaponFrame(startframe);
		state = WF_CYCLE_WRAPPED;
	}
	else
	{
		if (startframe > endframe)
		{
			ent->SetWeaponFrame(ent->GetWeaponFrame() - 1);
		}
		else if (startframe < endframe)
		{
			ent->SetWeaponFrame(ent->GetWeaponFrame() + 1);
		}

		if (ent->GetWeaponFrame() == endframe)
		{
			state = WF_LAST_FRAME;
		}
		else
		{
			state = WF_NORMAL_ADVANCE;
		}
	}

	G_FLOAT(OFS_RETURN) = state;
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
		Con_Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	old = host_client;
	host_client = client;
	Host_ClientCommands("playerclass %i\n", (int)NewClass);
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


void PF_matchAngleToSlope(void)
{
	qhedict_t* actor;
	vec3_t v_forward, old_forward, old_right, new_angles2 = { 0, 0, 0 };
	float pitch, mod, dot;

	// OFS_PARM0 is used by PF_vectoangles below
	actor = G_EDICT(OFS_PARM1);

	AngleVectors(actor->GetAngles(), old_forward, old_right, pr_globalVars.v_up);

	PF_vectoangles();

	pitch = G_FLOAT(OFS_RETURN) - 90;

	new_angles2[1] = G_FLOAT(OFS_RETURN + 1);

	AngleVectors(new_angles2, v_forward, pr_globalVars.v_right, pr_globalVars.v_up);

	mod = DotProduct(v_forward, old_right);

	if (mod < 0)
	{
		mod = 1;
	}
	else
	{
		mod = -1;
	}

	dot = DotProduct(v_forward, old_forward);

	actor->GetAngles()[0] = dot * pitch;
	actor->GetAngles()[2] = (1 - Q_fabs(dot)) * pitch * mod;
}

void PF_updateInfoPlaque(void)
{
	unsigned int check;
	unsigned int index, mode;
	unsigned int* use;
	int ofs = 0;

	index = G_FLOAT(OFS_PARM0);
	mode = G_FLOAT(OFS_PARM1);

	if (index > 31)
	{
		use = &info_mask2;
		ofs = 32;
	}
	else
	{
		use = &info_mask;
	}

	check = (long)(1 << (index - ofs));

	if (((mode & 1) && ((*use) & check)) || ((mode & 2) && !((*use) & check)))
	{
		;
	}
	else
	{
		(*use) ^= check;
	}
}

extern void V_WhiteFlash_f(void);

void PF_doWhiteFlash(void)
{
	V_WhiteFlash_f();
}

builtin_t pr_builtin[] =
{
	PF_Fixme,

	PF_makevectors,		// 1
	PF_setorigin,		// 2
	PFQ1_setmodel,		// 3
	PF_setsize,			// 4
	PF_lightstylestatic,// 5

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
	PF_AdvanceFrame,						//																			= #63
	PF_dprintv,								// void(string s1, string s2) dprint										= #64
	PF_RewindFrame,							//																			= #65
	PF_setclass,

	SVQH_MoveToGoal,
	PF_precache_file,
	PF_makestatic,

	PF_changelevel,

	PF_lightstylevalue,	// 71

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
