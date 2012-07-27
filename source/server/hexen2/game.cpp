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

#include "../server.h"
#include "../progsvm/progsvm.h"
#include "../quake_hexen/local.h"
#include "local.h"

#define WF_NORMAL_ADVANCE 0
#define WF_CYCLE_STARTED 1
#define WF_CYCLE_WRAPPED 2
#define WF_LAST_FRAME 3

unsigned int info_mask, info_mask2;

static int pr_lightstylevalue[256];

static const char* styleDefs[] =
{
	"a", "b", "c", "d", "e", "f", "g",
	"h", "i", "j", "k", "l", "m", "n",
	"o", "p", "q", "r", "s", "t", "u",
	"v", "w", "x", "y", "z"
};

void PF_setpuzzlemodel()
{
	qhedict_t* e = G_EDICT(OFS_PARM0);
	const char* m = G_STRING(OFS_PARM1);

	char NewName[256];
	String::Sprintf(NewName, sizeof(NewName), "models/puzzle/%s.mdl", m);
	// check to see if model was properly precached
	int i;
	const char** check = sv.qh_model_precache;
	for (i = 0; *check; i++, check++)
	{
		if (!String::Cmp(*check, NewName))
		{
			break;
		}
	}

	e->SetModel(PR_SetString(ED_NewString(NewName)));

	if (!*check)
	{
		common->Printf("**** NO PRECACHE FOR PUZZLE PIECE:");
		common->Printf("**** %s\n",NewName);

		sv.qh_model_precache[i] = PR_GetString(e->GetModel());
		if (!(GGameType & GAME_HexenWorld))
		{
			sv.models[i] = CM_PrecacheModel(NewName);
		}
	}

	e->v.modelindex = i;

	clipHandle_t mod = sv.models[(int)e->v.modelindex];
	if (mod)
	{
		vec3_t mins;
		vec3_t maxs;
		CM_ModelBounds(mod, mins, maxs);
		SetMinMaxSize(e, mins, maxs);
	}
	else
	{
		SetMinMaxSize(e, vec3_origin, vec3_origin);
	}
}

void PF_precache_sound2()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

void PF_precache_sound3()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

void PF_precache_sound4()
{
	//mission pack only
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

void PF_precache_model2()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_model();
}

void PF_precache_model3()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_model();
}

void PF_precache_model4()
{
	if (!qh_registered->value)
	{
		return;
	}
	PF_precache_model();
}

//	stop ent's sound on this chan
void PF_StopSound()
{
	qhedict_t* entity = G_EDICT(OFS_PARM0);
	int channel = G_FLOAT(OFS_PARM1);

	if (channel < 0 || channel > 7)
	{
		common->Error("SVQH_StartSound: channel = %i", channel);
	}

	SVH2_StopSound(entity, channel);
}

//	sends cur pos to client to update this ent/chan pair
void PF_UpdateSoundPos()
{
	qhedict_t* entity = G_EDICT(OFS_PARM0);
	int channel = G_FLOAT(OFS_PARM1);

	if (channel < 0 || channel > 7)
	{
		common->Error("SVQH_StartSound: channel = %i", channel);
	}

	SVH2_UpdateSoundPos(entity, channel);
}

//	Used for use tracing and shot targeting
// Traces are blocked by bbox and exact bsp entityes, and also slide box entities
// if the tryents flag is set.
void PF_tracearea()
{
	float* v1 = G_VECTOR(OFS_PARM0);
	float* v2 = G_VECTOR(OFS_PARM1);
	float* mins = G_VECTOR(OFS_PARM2);
	float* maxs = G_VECTOR(OFS_PARM3);
	int type = G_FLOAT(OFS_PARM4);
	qhedict_t* ent = G_EDICT(OFS_PARM5);

	q1trace_t trace = SVQH_MoveHull0(v1, mins, maxs, v2, type, ent);

	SVQH_SetMoveTrace(trace);
}

void PF_SpawnTemp()
{
	qhedict_t* ed = ED_Alloc_Temp();

	RETURN_EDICT(ed);
}

void PF_FindFloat()
{
	int e = G_EDICTNUM(OFS_PARM0);
	int f = G_INT(OFS_PARM1);
	float s = G_FLOAT(OFS_PARM2);
	if (!s)
	{
		PR_RunError("PF_Find: bad search string");
	}

	for (e++; e < sv.qh_num_edicts; e++)
	{
		qhedict_t* ed = QH_EDICT_NUM(e);
		if (ed->free)
		{
			continue;
		}
		float t = E_FLOAT(ed,f);
		if (t == s)
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv.qh_edicts);
}

void PF_precache_puzzle_model()
{
	if (sv.state != SS_LOADING)
	{
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
	}

	const char* m = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);

	char temp[MAX_QPATH];
	String::Sprintf(temp, MAX_QPATH, "models/puzzle/%s.mdl", m);
	const char* s = ED_NewString(temp);

	PR_CheckEmptyString(s);

	for (int i = 0; i < MAX_MODELS_H2; i++)
	{
		if (!sv.qh_model_precache[i])
		{
			sv.qh_model_precache[i] = s;
			if (!(GGameType & GAME_HexenWorld))
			{
				sv.models[i] = CM_PrecacheModel(s);
			}
			return;
		}
		if (!String::Cmp(sv.qh_model_precache[i], s))
		{
			return;
		}
	}
	PR_RunError("PF_precache_puzzle_model: overflow");
}

void PFHW_lightstylevalue()
{
	int style = G_FLOAT(OFS_PARM0);
	if (style < 0 || style >= MAX_LIGHTSTYLES_H2)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	G_FLOAT(OFS_RETURN) = (int)pr_lightstylevalue[style];
}

void PFH2_lightstylestatic()
{
	int styleNumber = G_FLOAT(OFS_PARM0);
	int value = G_FLOAT(OFS_PARM1);
	if (value < 0)
	{
		value = 0;
	}
	else if (value > 'z' - 'a')
	{
		value = 'z' - 'a';
	}
	const char* styleString = styleDefs[value];

	// Change the string in sv
	sv.qh_lightstyles[styleNumber] = styleString;

	if (sv.state != SS_GAME)
	{
		return;
	}

	// Send message to all clients on this server
	client_t* client = svs.clients;
	for (int i = 0; i < svs.qh_maxclients; i++, client++)
	{
		if (client->state >= CS_CONNECTED)
		{
			client->qh_message.WriteChar(h2svc_lightstyle);
			client->qh_message.WriteChar(styleNumber);
			client->qh_message.WriteString2(styleString);
		}
	}
}

void PFHW_lightstylestatic()
{
	int styleNumber = G_FLOAT(OFS_PARM0);
	int value = G_FLOAT(OFS_PARM1);
	if (value < 0)
	{
		value = 0;
	}
	else if (value > 'z' - 'a')
	{
		value = 'z' - 'a';
	}
	const char* styleString = styleDefs[value];

	// Change the string in sv
	sv.qh_lightstyles[styleNumber] = styleString;
	pr_lightstylevalue[styleNumber] = value;

	if (sv.state != SS_GAME)
	{
		return;
	}

	// Send message to all clients on this server
	client_t* client = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, client++)
	{
		if (client->state == CS_ACTIVE)
		{
			client->netchan.message.WriteChar(h2svc_lightstyle);
			client->netchan.message.WriteChar(styleNumber);
			client->netchan.message.WriteString2(styleString);
		}
	}
}

void PF_movestep()
{
	qhedict_t* ent = PROG_TO_EDICT(*pr_globalVars.self);

	vec3_t v;
	v[0] = G_FLOAT(OFS_PARM0);
	v[1] = G_FLOAT(OFS_PARM1);
	v[2] = G_FLOAT(OFS_PARM2);
	bool set_trace = G_FLOAT(OFS_PARM3);

	// save program state, because SVQH_movestep may call other progs
	dfunction_t* oldf = pr_xfunction;
	int oldself = *pr_globalVars.self;

	G_INT(OFS_RETURN) = SVQH_movestep(ent, v, false, true, set_trace);

	// restore program state
	pr_xfunction = oldf;
	*pr_globalVars.self = oldself;
}

void PFH2_AdvanceFrame()
{
	qhedict_t* ent = PROG_TO_EDICT(*pr_globalVars.self);
	float start = G_FLOAT(OFS_PARM0);
	float end = G_FLOAT(OFS_PARM1);

	float result;
	if (ent->GetFrame() < start || ent->GetFrame() > end)
	{
		// Didn't start in the range
		ent->SetFrame(start);
		result = 0;
	}
	else if (ent->GetFrame() == end)
	{
		// Wrapping
		ent->SetFrame(start);
		result = 1;
	}
	else
	{
		// Regular Advance
		ent->SetFrame(ent->GetFrame() + 1);
		if (ent->GetFrame() == end)
		{
			result = 2;
		}
		else
		{
			result = 0;
		}
	}

	G_FLOAT(OFS_RETURN) = result;
}

void PFHW_AdvanceFrame()
{
	qhedict_t* ent = PROG_TO_EDICT(*pr_globalVars.self);
	float start = G_FLOAT(OFS_PARM0);
	float end = G_FLOAT(OFS_PARM1);

	float result;
	if ((start < end && (ent->GetFrame() < start || ent->GetFrame() > end)) ||
		(start > end && (ent->GetFrame() > start || ent->GetFrame() < end)))
	{
		// Didn't start in the range
		ent->SetFrame(start);
		result = 0;
	}
	else if (ent->GetFrame() == end)
	{
		// Wrapping
		ent->SetFrame(start);
		result = 1;
	}
	else if (end > start)
	{
		// Regular Advance
		ent->SetFrame(ent->GetFrame() + 1);
		if (ent->GetFrame() == end)
		{
			result = 2;
		}
		else
		{
			result = 0;
		}
	}
	else if (end < start)
	{
		// Reverse Advance
		ent->SetFrame(ent->GetFrame() - 1);
		if (ent->GetFrame() == end)
		{
			result = 2;
		}
		else
		{
			result = 0;
		}
	}
	else
	{
		ent->SetFrame(end);
		result = 1;
	}

	G_FLOAT(OFS_RETURN) = result;
}

void PF_RewindFrame()
{
	qhedict_t* ent = PROG_TO_EDICT(*pr_globalVars.self);
	float start = G_FLOAT(OFS_PARM0);
	float end = G_FLOAT(OFS_PARM1);

	float Result;
	if (ent->GetFrame() > start || ent->GetFrame() < end)
	{
		// Didn't start in the range
		ent->SetFrame(start);
		Result = 0;
	}
	else if (ent->GetFrame() == end)
	{
		// Wrapping
		ent->SetFrame(start);
		Result = 1;
	}
	else
	{
		// Regular Advance
		ent->SetFrame(ent->GetFrame() - 1);
		if (ent->GetFrame() == end)
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

void PF_advanceweaponframe()
{
	qhedict_t* ent = PROG_TO_EDICT(*pr_globalVars.self);
	float startframe = G_FLOAT(OFS_PARM0);
	float endframe = G_FLOAT(OFS_PARM1);

	float state;
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

void PF_matchAngleToSlope()
{
	// OFS_PARM0 is used by PF_vectoangles below
	qhedict_t* actor = G_EDICT(OFS_PARM1);

	vec3_t old_forward, old_right;
	AngleVectors(actor->GetAngles(), old_forward, old_right, pr_globalVars.v_up);

	PF_vectoangles();

	float pitch = G_FLOAT(OFS_RETURN) - 90;

	vec3_t new_angles2 = { 0, 0, 0 };
	new_angles2[1] = G_FLOAT(OFS_RETURN + 1);

	vec3_t v_forward;
	AngleVectors(new_angles2, v_forward, pr_globalVars.v_right, pr_globalVars.v_up);

	float mod = DotProduct(v_forward, old_right);

	if (mod < 0)
	{
		mod = 1;
	}
	else
	{
		mod = -1;
	}

	float dot = DotProduct(v_forward, old_forward);

	actor->GetAngles()[0] = dot * pitch;
	actor->GetAngles()[2] = (1 - Q_fabs(dot)) * pitch * mod;
}

void PF_updateInfoPlaque()
{
	unsigned int index = G_FLOAT(OFS_PARM0);
	unsigned int mode = G_FLOAT(OFS_PARM1);

	unsigned int* use;
	int ofs = 0;
	if (index > 31)
	{
		use = &info_mask2;
		ofs = 32;
	}
	else
	{
		use = &info_mask;
	}

	unsigned int check = (long)(1 << (index - ofs));

	if (((mode & 1) && ((*use) & check)) || ((mode & 2) && !((*use) & check)))
	{
		;
	}
	else
	{
		(*use) ^= check;
	}
}

void PF_particle2()
{
	float* org = G_VECTOR(OFS_PARM0);
	float* dmin = G_VECTOR(OFS_PARM1);
	float* dmax = G_VECTOR(OFS_PARM2);
	float colour = G_FLOAT(OFS_PARM3);
	float effect = G_FLOAT(OFS_PARM4);
	float count = G_FLOAT(OFS_PARM5);
	SVH2_StartParticle2(org, dmin, dmax, colour, effect, count);
}

void PF_particle3()
{
	float* org = G_VECTOR(OFS_PARM0);
	float* box = G_VECTOR(OFS_PARM1);
	float color = G_FLOAT(OFS_PARM2);
	float effect = G_FLOAT(OFS_PARM3);
	float count = G_FLOAT(OFS_PARM4);
	SVH2_StartParticle3(org, box, color, effect, count);
}

void PF_particle4()
{
	float* org = G_VECTOR(OFS_PARM0);
	float radius = G_FLOAT(OFS_PARM1);
	float color = G_FLOAT(OFS_PARM2);
	float effect = G_FLOAT(OFS_PARM3);
	float count = G_FLOAT(OFS_PARM4);
	SVH2_StartParticle4(org, radius, color, effect, count);
}

void PFH2_makestatic()
{
	qhedict_t* ent = G_EDICT(OFS_PARM0);

	sv.qh_signon.WriteByte(h2svc_spawnstatic);

	sv.qh_signon.WriteShort(SVQH_ModelIndex(PR_GetString(ent->GetModel())));

	sv.qh_signon.WriteByte(ent->GetFrame());
	sv.qh_signon.WriteByte(ent->GetColorMap());
	sv.qh_signon.WriteByte(ent->GetSkin());
	sv.qh_signon.WriteByte((int)(ent->GetScale() * 100.0) & 255);
	sv.qh_signon.WriteByte(ent->GetDrawFlags());
	sv.qh_signon.WriteByte((int)(ent->GetAbsLight() * 255.0) & 255);

	for (int i = 0; i < 3; i++)
	{
		sv.qh_signon.WriteCoord(ent->GetOrigin()[i]);
		sv.qh_signon.WriteAngle(ent->GetAngles()[i]);
	}

	// throw the entity away now
	ED_Free(ent);
}

void PFH2_changelevel()
{
	if (svs.qh_changelevel_issued)
	{
		return;
	}
	svs.qh_changelevel_issued = true;

	const char* s1 = G_STRING(OFS_PARM0);
	const char* s2 = G_STRING(OFS_PARM1);

	if ((int)*pr_globalVars.serverflags & (H2SFL_NEW_UNIT | H2SFL_NEW_EPISODE))
	{
		if (GGameType & GAME_HexenWorld)
		{
			Cbuf_AddText(va("map %s %s\n",s1, s2));
		}
		else
		{
			Cbuf_AddText(va("changelevel %s %s\n",s1, s2));
		}
	}
	else
	{
		Cbuf_AddText(va("changelevel2 %s %s\n",s1, s2));
	}
}

void PFHW_infokey()
{
	qhedict_t* e = G_EDICT(OFS_PARM0);
	int e1 = QH_NUM_FOR_EDICT(e);
	const char* key = G_STRING(OFS_PARM1);

	const char* value;
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
		value = Info_ValueForKey(svs.clients[e1 - 1].userinfo, key);
	}
	else
	{
		value = "";
	}

	RETURN_STRING(value);
}
