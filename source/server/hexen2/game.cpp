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

#include "../../common/Common.h"
#include "../public.h"
#include "../progsvm/progsvm.h"
#include "../quake_hexen/local.h"
#include "local.h"
#include "../../common/command_buffer.h"
#include "../../common/hexen2strings.h"
#include "../../client/public.h"
#include "../../common/Hexen2EffectsRandom.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"

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

static void PF_setpuzzlemodel()
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

static void PF_precache_sound2()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

static void PF_precache_sound3()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

static void PF_precache_sound4()
{
	//mission pack only
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

static void PF_precache_model2()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_model();
}

static void PF_precache_model3()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_model();
}

static void PF_precache_model4()
{
	if (!qh_registered->value)
	{
		return;
	}
	PF_precache_model();
}

//	stop ent's sound on this chan
static void PF_StopSound()
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
static void PF_UpdateSoundPos()
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
static void PF_tracearea()
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

static void PF_SpawnTemp()
{
	qhedict_t* ed = ED_Alloc_Temp();

	RETURN_EDICT(ed);
}

static void PF_precache_puzzle_model()
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

static void PFH2_lightstylevalue()
{
	int style = G_FLOAT(OFS_PARM0);
	if (style < 0 || style >= MAX_LIGHTSTYLES_Q1)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	G_FLOAT(OFS_RETURN) = CLH2_GetLightStyleValue(style);
}

static void PFHW_lightstylevalue()
{
	int style = G_FLOAT(OFS_PARM0);
	if (style < 0 || style >= MAX_LIGHTSTYLES_Q1)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	G_FLOAT(OFS_RETURN) = (int)pr_lightstylevalue[style];
}

static void PFH2_lightstylestatic()
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

static void PFHW_lightstylestatic()
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

static void PF_movestep()
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

static void PFH2_AdvanceFrame()
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

static void PFHW_AdvanceFrame()
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

static void PF_RewindFrame()
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

static void PF_advanceweaponframe()
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

static void PF_matchAngleToSlope()
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
	actor->GetAngles()[2] = (1 - idMath::Fabs(dot)) * pitch * mod;
}

static void PF_updateInfoPlaque()
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

static void PF_particle2()
{
	float* org = G_VECTOR(OFS_PARM0);
	float* dmin = G_VECTOR(OFS_PARM1);
	float* dmax = G_VECTOR(OFS_PARM2);
	float colour = G_FLOAT(OFS_PARM3);
	float effect = G_FLOAT(OFS_PARM4);
	float count = G_FLOAT(OFS_PARM5);
	SVH2_StartParticle2(org, dmin, dmax, colour, effect, count);
}

static void PF_particle3()
{
	float* org = G_VECTOR(OFS_PARM0);
	float* box = G_VECTOR(OFS_PARM1);
	float color = G_FLOAT(OFS_PARM2);
	float effect = G_FLOAT(OFS_PARM3);
	float count = G_FLOAT(OFS_PARM4);
	SVH2_StartParticle3(org, box, color, effect, count);
}

static void PF_particle4()
{
	float* org = G_VECTOR(OFS_PARM0);
	float radius = G_FLOAT(OFS_PARM1);
	float color = G_FLOAT(OFS_PARM2);
	float effect = G_FLOAT(OFS_PARM3);
	float count = G_FLOAT(OFS_PARM4);
	SVH2_StartParticle4(org, radius, color, effect, count);
}

static void PFH2_makestatic()
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

static void PFH2_changelevel()
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

static void PFHW_infokey()
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

static void PFH2_plaque_draw()
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

	Q1WriteDest()->WriteByte(h2svc_plaque);
	Q1WriteDest()->WriteShort(Index);
}

static void PFHW_plaque_draw()
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

static void PF_rain_go()
{
	float* min_org = G_VECTOR(OFS_PARM0);
	float* max_org = G_VECTOR(OFS_PARM1);
	float* e_size = G_VECTOR(OFS_PARM2);
	float* dir = G_VECTOR(OFS_PARM3);
	int color = G_FLOAT(OFS_PARM4);
	int count = G_FLOAT(OFS_PARM5);

	vec3_t org;
	org[0] = min_org[0];
	org[1] = min_org[1];
	org[2] = max_org[2];

	int x_dir = dir[0];
	int y_dir = dir[1];

	SVH2_StartRainEffect(org, e_size, x_dir, y_dir, color, count);
}

static void PF_particleexplosion()
{
	float* org = G_VECTOR(OFS_PARM0);
	int color = G_FLOAT(OFS_PARM1);
	int radius = G_FLOAT(OFS_PARM2);
	int counter = G_FLOAT(OFS_PARM3);

	sv.qh_datagram.WriteByte(GGameType & GAME_HexenWorld ? hwsvc_particle_explosion : h2svc_particle_explosion);
	sv.qh_datagram.WriteCoord(org[0]);
	sv.qh_datagram.WriteCoord(org[1]);
	sv.qh_datagram.WriteCoord(org[2]);
	sv.qh_datagram.WriteShort(color);
	sv.qh_datagram.WriteShort(radius);
	sv.qh_datagram.WriteShort(counter);
}

static void PFH2_setclass()
{
	int entnum = G_EDICTNUM(OFS_PARM0);
	qhedict_t* e = G_EDICT(OFS_PARM0);
	float NewClass = G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > svs.qh_maxclients)
	{
		common->Printf("tried to sprint to a non-client\n");
		return;
	}

	client_t* client = &svs.clients[entnum - 1];

	SVQH_SendClientCommand(client, "playerclass %i\n", (int)NewClass);

	// These will get set again after the message has filtered its way
	// but it wouldn't take affect right away
	e->SetPlayerClass(NewClass);
	client->h2_playerclass = NewClass;
}

static void PFHW_setclass()
{
	int entnum = G_EDICTNUM(OFS_PARM0);
	qhedict_t* e = G_EDICT(OFS_PARM0);
	float NewClass = G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		common->Printf("tried to change class of a non-client\n");
		return;
	}

	client_t* client = &svs.clients[entnum - 1];

	if (NewClass > CLASS_DEMON && hw_dmMode->value != HWDM_SIEGE)
	{
		NewClass = CLASS_PALADIN;
	}

	e->SetPlayerClass(NewClass);
	client->h2_playerclass = NewClass;

	char temp[1024];
	sprintf(temp,"%d",(int)NewClass);
	Info_SetValueForKey(client->userinfo, "playerclass", temp, MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
	String::NCpy(client->name, Info_ValueForKey(client->userinfo, "name"), sizeof(client->name) - 1);
	client->qh_sendinfo = true;

	//update everyone else about playerclass change
	sv.qh_reliable_datagram.WriteByte(hwsvc_updatepclass);
	sv.qh_reliable_datagram.WriteByte(entnum - 1);
	sv.qh_reliable_datagram.WriteByte(((client->h2_playerclass << 5) | ((int)e->GetLevel() & 31)));
}

static void PFH2_starteffect()
{
	SVH2_ParseEffect(&sv.qh_reliable_datagram);
}

static void PFHW_starteffect()
{
	SVH2_ParseEffect(NULL);
}

static void PFH2_endeffect()
{
	int index = G_FLOAT(OFS_PARM1);

	if (!sv.h2_Effects[index].type)
	{
		return;
	}

	sv.h2_Effects[index].type = 0;
	sv.qh_reliable_datagram.WriteByte(h2svc_end_effect);
	sv.qh_reliable_datagram.WriteByte(index);
}

static void PFHW_endeffect()
{
	int index = G_FLOAT(OFS_PARM1);

	if (!sv.h2_Effects[index].type)
	{
		return;
	}

	sv.h2_Effects[index].type = 0;
	sv.multicast.WriteByte(hwsvc_end_effect);
	sv.multicast.WriteByte(index);
	SVQH_Multicast(vec3_origin, MULTICAST_ALL_R);
}

static void PF_turneffect()
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
	sv.multicast.WriteFloat(sv.qh_time * 0.001f);
	sv.multicast.WriteCoord(pos[0]);
	sv.multicast.WriteCoord(pos[1]);
	sv.multicast.WriteCoord(pos[2]);
	sv.multicast.WriteCoord(dir[0]);
	sv.multicast.WriteCoord(dir[1]);
	sv.multicast.WriteCoord(dir[2]);

	SVHW_MulticastSpecific(sv.h2_Effects[index].client_list, true);
}

//type-specific what this will send
static void PF_updateeffect()
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

	SVHW_MulticastSpecific(sv.h2_Effects[index].client_list, true);
}

static void PF_GetString()
{
	int index = (int)G_FLOAT(OFS_PARM0) - 1;

	if (index < 0)
	{
		PR_RunError("PF_GetString: index(%d) < 1",index + 1);
	}

	if (index >= prh2_string_count)
	{
		PR_RunError("PF_GetString: index(%d) >= prh2_string_count(%d)",index,prh2_string_count);
	}

	G_INT(OFS_RETURN) = PR_SetString(&prh2_global_strings[prh2_string_index[index]]);
}

static void PF_doWhiteFlash()
{
	Cmd_ExecuteString("wf");
}

//	print player's name
static void PF_name_print()
{
	int index = ((int)G_EDICTNUM(OFS_PARM2));
	int style = ((int)G_FLOAT(OFS_PARM1));

	if (hw_spartanPrint->value == 1 && style < 2)
	{
		return;
	}

	if (index <= 0)
	{
		PR_RunError("PF_name_print: index(%d) <= 0",index);
	}

	if (index > MAX_CLIENTS_QHW)
	{
		PR_RunError("PF_name_print: index(%d) > MAX_CLIENTS_QHW",index);
	}


	if ((int)G_FLOAT(OFS_PARM0) == QHMSG_BROADCAST)	//broadcast message--send like bprint, print it out on server too.
	{
		common->Printf("%s", svs.clients[index - 1].name);

		client_t* cl = svs.clients;
		for (int i = 0; i < MAX_CLIENTS_QHW; i++, cl++)
		{
			if (style < cl->messagelevel)
			{
				continue;
			}
			if (cl->state != CS_ACTIVE)//should i be checking CS_CONNECTED too?
			{
				if (cl->state)	//not fully in so won't know name yet, explicitly say the name
				{
					SVQH_PrintToClient(cl, style, svs.clients[index - 1].name);
				}
				continue;
			}
			cl->netchan.message.WriteByte(hwsvc_name_print);
			cl->netchan.message.WriteByte(style);
			cl->netchan.message.WriteByte(index - 1);	//knows the name, send the index.
		}
		return;
	}

	if (G_FLOAT(OFS_PARM0) == QHMSG_ONE)
	{
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableWrite_Begin(cl, hwsvc_name_print, 3);
		SVQH_ClientReliableWrite_Byte(cl, style);
		SVQH_ClientReliableWrite_Byte(cl, index - 1);	//heh, don't need a short here.
	}
	else
	{
		QWWriteDest()->WriteByte(hwsvc_name_print);
		QWWriteDest()->WriteByte(style);
		QWWriteDest()->WriteByte(index - 1);	//heh, don't need a short here.
	}
}

//	print string from strings.txt
static void PF_print_indexed()
{
	int index = ((int)G_FLOAT(OFS_PARM2));
	int style = ((int)G_FLOAT(OFS_PARM1));

	if (hw_spartanPrint->value == 1 && style < 2)
	{
		return;
	}

	if (index <= 0)
	{
		PR_RunError("PF_sprint_indexed: index(%d) < 1",index);
	}

	if (index > prh2_string_count)
	{
		PR_RunError("PF_sprint_indexed: index(%d) >= prh2_string_count(%d)",index,prh2_string_count);
	}

	if ((int)G_FLOAT(OFS_PARM0) == QHMSG_BROADCAST)	//broadcast message--send like bprint, print it out on server too.
	{
		common->Printf("%s",&prh2_global_strings[prh2_string_index[index - 1]]);

		client_t* cl = svs.clients;
		for (int i = 0; i < MAX_CLIENTS_QHW; i++, cl++)
		{
			if (style < cl->messagelevel)
			{
				continue;
			}
			if (!cl->state)
			{
				continue;
			}
			cl->netchan.message.WriteByte(hwsvc_indexed_print);
			cl->netchan.message.WriteByte(style);
			cl->netchan.message.WriteShort(index);
		}
		return;
	}

	if (G_FLOAT(OFS_PARM0) == QHMSG_ONE)
	{
		client_t* cl = Write_GetClient();
		SVQH_ClientReliableWrite_Begin(cl, hwsvc_indexed_print, 4);
		SVQH_ClientReliableWrite_Byte(cl, style);
		SVQH_ClientReliableWrite_Short(cl, index);
	}
	else
	{
		QWWriteDest()->WriteByte(hwsvc_indexed_print);
		QWWriteDest()->WriteByte(style);
		QWWriteDest()->WriteShort(index);
	}
}

static void PF_bcenterprint2()
{
	const char* s = PF_VarString(0);

	client_t* cl = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		cl->netchan.message.WriteByte(h2svc_centerprint);
		cl->netchan.message.WriteString2(s);
	}
}

static void PF_centerprint2()
{
	int entnum = G_EDICTNUM(OFS_PARM0);
	const char* s = PF_VarString(1);

	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		common->Printf("tried to sprint to a non-client\n");
		return;
	}

	client_t* client = &svs.clients[entnum - 1];

	client->netchan.message.WriteChar(h2svc_centerprint);
	client->netchan.message.WriteString2(s);
}

static void PF_setsiegeteam()
{
	int entnum = G_EDICTNUM(OFS_PARM0);
	qhedict_t* e = G_EDICT(OFS_PARM0);
	float NewTeam = G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > MAX_CLIENTS_QHW)
	{
		common->Printf("tried to change siege_team of a non-client\n");
		return;
	}

	client_t* client = &svs.clients[entnum - 1];

	e->SetSiegeTeam(NewTeam);
	client->hw_siege_team = NewTeam;

	//update everyone else about playerclass change
	sv.qh_reliable_datagram.WriteByte(hwsvc_updatesiegeteam);
	sv.qh_reliable_datagram.WriteByte(entnum - 1);
	sv.qh_reliable_datagram.WriteByte(client->hw_siege_team);
}

static void PF_updateSiegeInfo()
{
	client_t* client = svs.clients;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		client->netchan.message.WriteByte(hwsvc_updatesiegeinfo);
		client->netchan.message.WriteByte((int)ceil(qh_timelimit->value));
		client->netchan.message.WriteByte((int)ceil(qh_fraglimit->value));
	}
}

static idHexen2EffectsRandom svh2_random;

static void PF_setseed()
{
	svh2_random.SetSeed(G_FLOAT(OFS_PARM0));
}

static void PF_seedrand()
{
	G_FLOAT(OFS_RETURN) = svh2_random.SeedRand();
}

static void PF_multieffect()
{
	SVHW_ParseMultiEffect(&sv.qh_reliable_datagram);

}

static void PF_getmeid()
{
	G_FLOAT(OFS_RETURN) = SVHW_GetMultiEffectId();
}

static void PF_weapon_sound()
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

static builtin_t prh2_builtin[] =
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
	PFQ1_bprint,								// void(string s) bprint											= #23
	PFQ1_sprint,								// void(entity client, string s) sprint						= #24
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
	PFH2_setclass,

	SVQH_MoveToGoal,
	PF_precache_file,
	PFH2_makestatic,

	PFH2_changelevel,

	PFH2_lightstylevalue,	// 71

	PF_cvar_set,
	PF_centerprint,

	PF_ambientsound,

	PF_precache_model2,
	PF_precache_sound2,						// precache_sound2 is different only for qcc
	PF_precache_file,

	PF_setspawnparms,
	PFH2_plaque_draw,
	PF_rain_go,									//																				=	#80
	PF_particleexplosion,						//																				=	#81
	PF_movestep,
	PF_advanceweaponframe,
	PF_sqrt,

	PF_particle3,							// 85
	PF_particle4,							// 86
	PF_setpuzzlemodel,						// 87

	PFH2_starteffect,							// 88
	PFH2_endeffect,							// 89

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

void PRH2_InitBuiltins()
{
	pr_builtins = prh2_builtin;
	pr_numbuiltins = sizeof(prh2_builtin) / sizeof(prh2_builtin[0]);
}

static builtin_t prhw_builtin[] =
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
	PFHW_setclass,

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

	PFHW_plaque_draw,
	PF_rain_go,									//																				=	#80
	PF_particleexplosion,						//																				=	#81
	PF_movestep,
	PF_advanceweaponframe,
	PF_sqrt,

	PF_particle3,							// 85
	PF_particle4,							// 86
	PF_setpuzzlemodel,						// 87

	PFHW_starteffect,							// 88
	PFHW_endeffect,							// 89

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

void PRHW_InitBuiltins()
{
	pr_builtins = prhw_builtin;
	pr_numbuiltins = sizeof(prhw_builtin) / sizeof(prhw_builtin[0]);
}
