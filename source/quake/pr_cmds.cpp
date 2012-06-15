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

#include "quakedef.h"
#include "../common/file_formats/bsp29.h"

#define RETURN_EDICT(e) (((int*)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))

/*
===============================================================================

                        BUILT-IN FUNCTIONS

===============================================================================
*/

char* PF_VarString(int first)
{
	int i;
	static char out[256];

	out[0] = 0;
	for (i = first; i < pr_argc; i++)
	{
		String::Cat(out, sizeof(out), G_STRING((OFS_PARM0 + i * 3)));
	}
	return out;
}


/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/
void PF_error(void)
{
	char* s;
	qhedict_t* ed;

	s = PF_VarString(0);
	Con_Printf("======SERVER ERROR in %s:\n%s\n",
		PR_GetString(pr_xfunction->s_name),s);
	ed = PROG_TO_EDICT(pr_global_struct->self);
	ED_Print(ed);

	Host_Error("Program error");
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void PF_objerror(void)
{
	char* s;
	qhedict_t* ed;

	s = PF_VarString(0);
	Con_Printf("======OBJECT ERROR in %s:\n%s\n",
		PR_GetString(pr_xfunction->s_name),s);
	ed = PROG_TO_EDICT(pr_global_struct->self);
	ED_Print(ed);
	ED_Free(ed);

	Host_Error("Program error");
}



/*
==============
PF_makevectors

Writes len values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors(void)
{
	AngleVectors(G_VECTOR(OFS_PARM0), pr_global_struct->v_forward, pr_global_struct->v_right, pr_global_struct->v_up);
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void PF_setorigin(void)
{
	qhedict_t* e;
	float* org;

	e = G_EDICT(OFS_PARM0);
	org = G_VECTOR(OFS_PARM1);
	VectorCopy(org, e->GetOrigin());
	SV_LinkEdict(e, false);
}


void SetMinMaxSize(qhedict_t* e, float* min, float* max, qboolean rotate)
{
	float* angles;
	vec3_t rmin, rmax;
	float bounds[2][3];
	float xvector[2], yvector[2];
	float a;
	vec3_t base, transformed;
	int i, j, k, l;

	for (i = 0; i < 3; i++)
		if (min[i] > max[i])
		{
			PR_RunError("backwards mins/maxs");
		}

	rotate = false;		// FIXME: implement rotation properly again

	if (!rotate)
	{
		VectorCopy(min, rmin);
		VectorCopy(max, rmax);
	}
	else
	{
		// find min / max for rotations
		angles = e->GetAngles();

		a = angles[1] / 180 * M_PI;

		xvector[0] = cos(a);
		xvector[1] = sin(a);
		yvector[0] = -sin(a);
		yvector[1] = cos(a);

		VectorCopy(min, bounds[0]);
		VectorCopy(max, bounds[1]);

		rmin[0] = rmin[1] = rmin[2] = 9999;
		rmax[0] = rmax[1] = rmax[2] = -9999;

		for (i = 0; i <= 1; i++)
		{
			base[0] = bounds[i][0];
			for (j = 0; j <= 1; j++)
			{
				base[1] = bounds[j][1];
				for (k = 0; k <= 1; k++)
				{
					base[2] = bounds[k][2];

					// transform the point
					transformed[0] = xvector[0] * base[0] + yvector[0] * base[1];
					transformed[1] = xvector[1] * base[0] + yvector[1] * base[1];
					transformed[2] = base[2];

					for (l = 0; l < 3; l++)
					{
						if (transformed[l] < rmin[l])
						{
							rmin[l] = transformed[l];
						}
						if (transformed[l] > rmax[l])
						{
							rmax[l] = transformed[l];
						}
					}
				}
			}
		}
	}

// set derived values
	e->SetMins(rmin);
	e->SetMaxs(rmax);
	VectorSubtract(max, min, e->GetSize());

	SV_LinkEdict(e, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF_setsize(void)
{
	qhedict_t* e;
	float* min, * max;

	e = G_EDICT(OFS_PARM0);
	min = G_VECTOR(OFS_PARM1);
	max = G_VECTOR(OFS_PARM2);
	SetMinMaxSize(e, min, max, false);
}


/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
void PF_setmodel(void)
{
	qhedict_t* e;
	const char* m, ** check;
	clipHandle_t mod;
	int i;

	e = G_EDICT(OFS_PARM0);
	m = G_STRING(OFS_PARM1);

// check to see if model was properly precached
	for (i = 0, check = sv.qh_model_precache; *check; i++, check++)
		if (!String::Cmp(*check, m))
		{
			break;
		}

	if (!*check)
	{
		PR_RunError("no precache: %s\n", m);
	}

	e->SetModel(PR_SetString(m));
	e->v.modelindex = i;

	mod = sv.models[(int)e->v.modelindex];

	if (mod)
	{
		vec3_t mins;
		vec3_t maxs;
		CM_ModelBounds(mod, mins, maxs);
		SetMinMaxSize(e, mins, maxs, true);
	}
	else
	{
		SetMinMaxSize(e, vec3_origin, vec3_origin, true);
	}
}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint(void)
{
	char* s;

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
	char* s;
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

	client->qh_message.WriteChar(q1svc_print);
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
	char* s;
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

	client->qh_message.WriteChar(q1svc_centerprint);
	client->qh_message.WriteString2(s);
}


/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize(void)
{
	float* value1;
	vec3_t newvalue;
	float len;

	value1 = G_VECTOR(OFS_PARM0);

	len = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	len = sqrt(len);

	if (len == 0)
	{
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	}
	else
	{
		len = 1 / len;
		newvalue[0] = value1[0] * len;
		newvalue[1] = value1[1] * len;
		newvalue[2] = value1[2] * len;
	}

	VectorCopy(newvalue, G_VECTOR(OFS_RETURN));
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
void PF_vlen(void)
{
	float* value1;
	float len;

	value1 = G_VECTOR(OFS_PARM0);

	len = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	len = sqrt(len);

	G_FLOAT(OFS_RETURN) = len;
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw()
{
	float* value1 = G_VECTOR(OFS_PARM0);

	float yaw = VecToYaw(value1);

	G_FLOAT(OFS_RETURN) = (int)yaw;
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void PF_vectoangles()
{
	float* value1 = G_VECTOR(OFS_PARM0);

	vec3_t angles;
	VecToAnglesBuggy(value1, angles);

	G_FLOAT(OFS_RETURN + 0) = (int)angles[0];
	G_FLOAT(OFS_RETURN + 1) = (int)angles[1];
	G_FLOAT(OFS_RETURN + 2) = (int)angles[2];
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void PF_random(void)
{
	float num;

	num = (rand() & 0x7fff) / ((float)0x7fff);

	G_FLOAT(OFS_RETURN) = num;
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
PF_ambientsound

=================
*/
void PF_ambientsound(void)
{
	const char** check;
	const char* samp;
	float* pos;
	float vol, attenuation;
	int i, soundnum;

	pos = G_VECTOR(OFS_PARM0);
	samp = G_STRING(OFS_PARM1);
	vol = G_FLOAT(OFS_PARM2);
	attenuation = G_FLOAT(OFS_PARM3);

// check to see if samp was properly precached
	for (soundnum = 0, check = sv.qh_sound_precache; *check; check++, soundnum++)
		if (!String::Cmp(*check,samp))
		{
			break;
		}

	if (!*check)
	{
		Con_Printf("no precache: %s\n", samp);
		return;
	}

// add an svc_spawnambient command to the level signon packet

	sv.qh_signon.WriteByte(q1svc_spawnstaticsound);
	for (i = 0; i < 3; i++)
		sv.qh_signon.WriteCoord(pos[i]);

	sv.qh_signon.WriteByte(soundnum);

	sv.qh_signon.WriteByte(vol * 255);
	sv.qh_signon.WriteByte(attenuation * 64);

}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound(void)
{
	const char* sample;
	int channel;
	qhedict_t* entity;
	int volume;
	float attenuation;

	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);
	sample = G_STRING(OFS_PARM2);
	volume = G_FLOAT(OFS_PARM3) * 255;
	attenuation = G_FLOAT(OFS_PARM4);

	if (volume < 0 || volume > 255)
	{
		Sys_Error("SV_StartSound: volume = %i", volume);
	}

	if (attenuation < 0 || attenuation > 4)
	{
		Sys_Error("SV_StartSound: attenuation = %f", attenuation);
	}

	if (channel < 0 || channel > 7)
	{
		Sys_Error("SV_StartSound: channel = %i", channel);
	}

	SV_StartSound(entity, channel, sample, volume, attenuation);
}

/*
=================
PF_break

break()
=================
*/
void PF_break(void)
{
	Con_Printf("break statement\n");
	*(int*)-4 = 0;	// dump to debugger
//	PR_RunError ("break statement");
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF_traceline(void)
{
	float* v1, * v2;
	q1trace_t trace;
	int nomonsters;
	qhedict_t* ent;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	nomonsters = G_FLOAT(OFS_PARM2);
	ent = G_EDICT(OFS_PARM3);

	trace = SV_Move(v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

	pr_global_struct->trace_allsolid = trace.allsolid;
	pr_global_struct->trace_startsolid = trace.startsolid;
	pr_global_struct->trace_fraction = trace.fraction;
	pr_global_struct->trace_inwater = trace.inwater;
	pr_global_struct->trace_inopen = trace.inopen;
	VectorCopy(trace.endpos, pr_global_struct->trace_endpos);
	VectorCopy(trace.plane.normal, pr_global_struct->trace_plane_normal);
	pr_global_struct->trace_plane_dist =  trace.plane.dist;
	if (trace.entityNum >= 0)
	{
		pr_global_struct->trace_ent = EDICT_TO_PROG(QH_EDICT_NUM(trace.entityNum));
	}
	else
	{
		pr_global_struct->trace_ent = EDICT_TO_PROG(sv.qh_edicts);
	}
}


/*
=================
PF_checkpos

Returns true if the given entity can move to the given position from it's
current position by walking or rolling.
FIXME: make work...
scalar checkpos (entity, vector)
=================
*/
void PF_checkpos(void)
{
}

//============================================================================

byte checkpvs[BSP29_MAX_MAP_LEAFS / 8];

int PF_newcheckclient(int check)
{
	int i;
	byte* pvs;
	qhedict_t* ent;
	vec3_t org;

// cycle to the next one

	if (check < 1)
	{
		check = 1;
	}
	if (check > svs.qh_maxclients)
	{
		check = svs.qh_maxclients;
	}

	if (check == svs.qh_maxclients)
	{
		i = 1;
	}
	else
	{
		i = check + 1;
	}

	for (;; i++)
	{
		if (i == svs.qh_maxclients + 1)
		{
			i = 1;
		}

		ent = QH_EDICT_NUM(i);

		if (i == check)
		{
			break;	// didn't find anything else

		}
		if (ent->free)
		{
			continue;
		}
		if (ent->GetHealth() <= 0)
		{
			continue;
		}
		if ((int)ent->GetFlags() & FL_NOTARGET)
		{
			continue;
		}

		// anything that is a client, or has a client as an enemy
		break;
	}

	// get the PVS for the entity
	VectorAdd(ent->GetOrigin(), ent->GetViewOfs(), org);
	int leaf = CM_PointLeafnum(org);
	pvs = CM_ClusterPVS(CM_LeafCluster(leaf));
	Com_Memcpy(checkpvs, pvs, (CM_NumClusters() + 7) >> 3);

	return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define MAX_CHECK   16
int c_invis, c_notvis;
void PF_checkclient(void)
{
	qhedict_t* ent, * self;
	vec3_t view;

// find a len check if on a len frame
	if (sv.qh_time - sv.qh_lastchecktime >= 0.1)
	{
		sv.qh_lastcheck = PF_newcheckclient(sv.qh_lastcheck);
		sv.qh_lastchecktime = sv.qh_time;
	}

// return check if it might be visible
	ent = QH_EDICT_NUM(sv.qh_lastcheck);
	if (ent->free || ent->GetHealth() <= 0)
	{
		RETURN_EDICT(sv.qh_edicts);
		return;
	}

// if current entity can't possibly see the check entity, return 0
	self = PROG_TO_EDICT(pr_global_struct->self);
	VectorAdd(self->GetOrigin(), self->GetViewOfs(), view);
	int leaf = CM_PointLeafnum(view);
	int l = CM_LeafCluster(leaf);
	if ((l < 0) || !(checkpvs[l >> 3] & (1 << (l & 7))))
	{
		c_notvis++;
		RETURN_EDICT(sv.qh_edicts);
		return;
	}

// might be able to see it
	c_invis++;
	RETURN_EDICT(ent);
}

//============================================================================


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

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
void PF_localcmd(void)
{
	const char* str;

	str = G_STRING(OFS_PARM0);
	Cbuf_AddText(str);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
void PF_cvar(void)
{
	const char* str;

	str = G_STRING(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = Cvar_VariableValue(str);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
void PF_cvar_set(void)
{
	const char* var, * val;

	var = G_STRING(OFS_PARM0);
	val = G_STRING(OFS_PARM1);

	Cvar_Set(var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void PF_findradius(void)
{
	qhedict_t* ent, * chain;
	float rad;
	float* org;
	vec3_t eorg;
	int i, j;

	chain = (qhedict_t*)sv.qh_edicts;

	org = G_VECTOR(OFS_PARM0);
	rad = G_FLOAT(OFS_PARM1);

	ent = NEXT_EDICT(sv.qh_edicts);
	for (i = 1; i < sv.qh_num_edicts; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
		{
			continue;
		}
		if (ent->GetSolid() == SOLID_NOT)
		{
			continue;
		}
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (ent->GetOrigin()[j] + (ent->GetMins()[j] + ent->GetMaxs()[j]) * 0.5);
		if (VectorLength(eorg) > rad)
		{
			continue;
		}

		ent->SetChain(EDICT_TO_PROG(chain));
		chain = ent;
	}

	RETURN_EDICT(chain);
}


/*
=========
PF_dprint
=========
*/
void PF_dprint(void)
{
	Con_DPrintf("%s",PF_VarString(0));
}

char pr_string_temp[128];

void PF_ftos(void)
{
	float v;
	v = G_FLOAT(OFS_PARM0);

	if (v == (int)v)
	{
		sprintf(pr_string_temp, "%d",(int)v);
	}
	else
	{
		sprintf(pr_string_temp, "%5.1f",v);
	}
	G_INT(OFS_RETURN) = PR_SetString(pr_string_temp);
}

void PF_fabs(void)
{
	float v;
	v = G_FLOAT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = fabs(v);
}

void PF_vtos(void)
{
	sprintf(pr_string_temp, "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
	G_INT(OFS_RETURN) = PR_SetString(pr_string_temp);
}

void PF_Spawn(void)
{
	qhedict_t* ed;
	ed = ED_Alloc();
	RETURN_EDICT(ed);
}

void PF_Remove(void)
{
	qhedict_t* ed;

	ed = G_EDICT(OFS_PARM0);
	ED_Free(ed);
}


// entity (entity start, .string field, string match) find = #5;
void PF_Find(void)
{
	int e;
	int f;
	const char* s, * t;
	qhedict_t* ed;

	e = G_EDICTNUM(OFS_PARM0);
	f = G_INT(OFS_PARM1);
	s = G_STRING(OFS_PARM2);
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
		t = E_STRING(ed,f);
		if (!t)
		{
			continue;
		}
		if (!String::Cmp(t,s))
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv.qh_edicts);
}

void PR_CheckEmptyString(const char* s)
{
	if (s[0] <= ' ')
	{
		PR_RunError("Bad string");
	}
}

void PF_precache_file(void)
{	// precache_file is only used to copy files with qcc, it does nothing
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

void PF_precache_sound(void)
{
	const char* s;
	int i;

	if (sv.state != SS_LOADING)
	{
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
	}

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (i = 0; i < MAX_SOUNDS_Q1; i++)
	{
		if (!sv.qh_sound_precache[i])
		{
			sv.qh_sound_precache[i] = s;
			return;
		}
		if (!String::Cmp(sv.qh_sound_precache[i], s))
		{
			return;
		}
	}
	PR_RunError("PF_precache_sound: overflow");
}

void PF_precache_model(void)
{
	const char* s;
	int i;

	if (sv.state != SS_LOADING)
	{
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
	}

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (i = 0; i < MAX_MODELS_Q1; i++)
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
	PR_RunError("PF_precache_model: overflow");
}


void PF_coredump(void)
{
	ED_PrintEdicts();
}

void PF_traceon(void)
{
	pr_trace = true;
}

void PF_traceoff(void)
{
	pr_trace = false;
}

void PF_eprint(void)
{
	ED_PrintNum(G_EDICTNUM(OFS_PARM0));
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void PF_walkmove(void)
{
	qhedict_t* ent;
	float yaw, dist;
	vec3_t move;
	dfunction_t* oldf;
	int oldself;

	ent = PROG_TO_EDICT(pr_global_struct->self);
	yaw = G_FLOAT(OFS_PARM0);
	dist = G_FLOAT(OFS_PARM1);

	if (!((int)ent->GetFlags() & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	yaw = yaw * M_PI * 2 / 360;

	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

// save program state, because SV_movestep may call other progs
	oldf = pr_xfunction;
	oldself = pr_global_struct->self;

	G_FLOAT(OFS_RETURN) = SV_movestep(ent, move, true);


// restore program state
	pr_xfunction = oldf;
	pr_global_struct->self = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void PF_droptofloor(void)
{
	qhedict_t* ent;
	vec3_t end;
	q1trace_t trace;

	ent = PROG_TO_EDICT(pr_global_struct->self);

	VectorCopy(ent->GetOrigin(), end);
	end[2] -= 256;

	trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, false, ent);

	if (trace.fraction == 1 || trace.allsolid)
	{
		G_FLOAT(OFS_RETURN) = 0;
	}
	else
	{
		VectorCopy(trace.endpos, ent->GetOrigin());
		SV_LinkEdict(ent, false);
		ent->SetFlags((int)ent->GetFlags() | FL_ONGROUND);
		ent->SetGroundEntity(EDICT_TO_PROG(QH_EDICT_NUM(trace.entityNum)));
		G_FLOAT(OFS_RETURN) = 1;
	}
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle(void)
{
	int style;
	const char* val;
	client_t* client;
	int j;

	style = G_FLOAT(OFS_PARM0);
	val = G_STRING(OFS_PARM1);

// change the string in sv
	sv.qh_lightstyles[style] = val;

// send message to all clients on this server
	if (sv.state != SS_GAME)
	{
		return;
	}

	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
		if (client->state >= CS_CONNECTED)
		{
			client->qh_message.WriteChar(q1svc_lightstyle);
			client->qh_message.WriteChar(style);
			client->qh_message.WriteString2(val);
		}
}

void PF_rint(void)
{
	float f;
	f = G_FLOAT(OFS_PARM0);
	if (f > 0)
	{
		G_FLOAT(OFS_RETURN) = (int)(f + 0.5);
	}
	else
	{
		G_FLOAT(OFS_RETURN) = (int)(f - 0.5);
	}
}
void PF_floor(void)
{
	G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}
void PF_ceil(void)
{
	G_FLOAT(OFS_RETURN) = ceil(G_FLOAT(OFS_PARM0));
}


/*
=============
PF_checkbottom
=============
*/
void PF_checkbottom(void)
{
	qhedict_t* ent;

	ent = G_EDICT(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = SV_CheckBottom(ent);
}

/*
=============
PF_pointcontents
=============
*/
void PF_pointcontents(void)
{
	float* v;

	v = G_VECTOR(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = SV_PointContents(v);
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void PF_nextent(void)
{
	int i;
	qhedict_t* ent;

	i = G_EDICTNUM(OFS_PARM0);
	while (1)
	{
		i++;
		if (i == sv.qh_num_edicts)
		{
			RETURN_EDICT(sv.qh_edicts);
			return;
		}
		ent = QH_EDICT_NUM(i);
		if (!ent->free)
		{
			RETURN_EDICT(ent);
			return;
		}
	}
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
Cvar* sv_aim;
void PF_aim(void)
{
	qhedict_t* ent, * check, * bestent;
	vec3_t start, dir, end, bestdir;
	int i, j;
	q1trace_t tr;
	float dist, bestdist;
	float speed;

	ent = G_EDICT(OFS_PARM0);
	speed = G_FLOAT(OFS_PARM1);

	VectorCopy(ent->GetOrigin(), start);
	start[2] += 20;

// try sending a trace straight
	VectorCopy(pr_global_struct->v_forward, dir);
	VectorMA(start, 2048, dir, end);
	tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
	if (tr.entityNum >= 0 && QH_EDICT_NUM(tr.entityNum)->GetTakeDamage() == DAMAGE_AIM &&
		(!teamplay->value || ent->GetTeam() <= 0 || ent->GetTeam() != QH_EDICT_NUM(tr.entityNum)->GetTeam()))
	{
		VectorCopy(pr_global_struct->v_forward, G_VECTOR(OFS_RETURN));
		return;
	}


// try all possible entities
	VectorCopy(dir, bestdir);
	bestdist = sv_aim->value;
	bestent = NULL;

	check = NEXT_EDICT(sv.qh_edicts);
	for (i = 1; i < sv.qh_num_edicts; i++, check = NEXT_EDICT(check))
	{
		if (check->GetTakeDamage() != DAMAGE_AIM)
		{
			continue;
		}
		if (check == ent)
		{
			continue;
		}
		if (teamplay->value && ent->GetTeam() > 0 && ent->GetTeam() == check->GetTeam())
		{
			continue;	// don't aim at teammate
		}
		for (j = 0; j < 3; j++)
			end[j] = check->GetOrigin()[j]
					 + 0.5 * (check->GetMins()[j] + check->GetMaxs()[j]);
		VectorSubtract(end, start, dir);
		VectorNormalize(dir);
		dist = DotProduct(dir, pr_global_struct->v_forward);
		if (dist < bestdist)
		{
			continue;	// to far to turn
		}
		tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
		if (QH_EDICT_NUM(tr.entityNum) == check)
		{	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}

	if (bestent)
	{
		VectorSubtract(bestent->GetOrigin(), ent->GetOrigin(), dir);
		dist = DotProduct(dir, pr_global_struct->v_forward);
		VectorScale(pr_global_struct->v_forward, dist, end);
		end[2] = dir[2];
		VectorNormalize(end);
		VectorCopy(end, G_VECTOR(OFS_RETURN));
	}
	else
	{
		VectorCopy(bestdir, G_VECTOR(OFS_RETURN));
	}
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw(void)
{
	qhedict_t* ent;
	float ideal, current, move, speed;

	ent = PROG_TO_EDICT(pr_global_struct->self);
	current = AngleMod(ent->GetAngles()[1]);
	ideal = ent->GetIdealYaw();
	speed = ent->GetYawSpeed();

	if (current == ideal)
	{
		return;
	}
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
		{
			move = move - 360;
		}
	}
	else
	{
		if (move <= -180)
		{
			move = move + 360;
		}
	}
	if (move > 0)
	{
		if (move > speed)
		{
			move = speed;
		}
	}
	else
	{
		if (move < -speed)
		{
			move = -speed;
		}
	}

	ent->GetAngles()[1] = AngleMod(current + move);
}

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define MSG_BROADCAST   0		// unreliable to all
#define MSG_ONE         1		// reliable to one (msg_entity)
#define MSG_ALL         2		// reliable to all
#define MSG_INIT        3		// write to the init string

QMsg* WriteDest(void)
{
	int entnum;
	int dest;
	qhedict_t* ent;

	dest = G_FLOAT(OFS_PARM0);
	switch (dest)
	{
	case MSG_BROADCAST:
		return &sv.qh_datagram;

	case MSG_ONE:
		ent = PROG_TO_EDICT(pr_global_struct->msg_entity);
		entnum = QH_NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > svs.qh_maxclients)
		{
			PR_RunError("WriteDest: not a client");
		}
		return &svs.clients[entnum - 1].qh_message;

	case MSG_ALL:
		return &sv.qh_reliable_datagram;

	case MSG_INIT:
		return &sv.qh_signon;

	default:
		PR_RunError("WriteDest: bad destination");
		break;
	}

	return NULL;
}

void PF_WriteByte(void)
{
	WriteDest()->WriteByte(G_FLOAT(OFS_PARM1));
}

void PF_WriteChar()
{
	WriteDest()->WriteChar(G_FLOAT(OFS_PARM1));
}

void PF_WriteShort(void)
{
	WriteDest()->WriteShort(G_FLOAT(OFS_PARM1));
}

void PF_WriteLong(void)
{
	WriteDest()->WriteLong(G_FLOAT(OFS_PARM1));
}

void PF_WriteAngle(void)
{
	WriteDest()->WriteAngle(G_FLOAT(OFS_PARM1));
}

void PF_WriteCoord(void)
{
	WriteDest()->WriteCoord(G_FLOAT(OFS_PARM1));
}

void PF_WriteString(void)
{
	WriteDest()->WriteString2(G_STRING(OFS_PARM1));
}


void PF_WriteEntity(void)
{
	WriteDest()->WriteShort(G_EDICTNUM(OFS_PARM1));
}

//=============================================================================

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
PF_setspawnparms
==============
*/
void PF_setspawnparms(void)
{
	qhedict_t* ent;
	int i;
	client_t* client;

	ent = G_EDICT(OFS_PARM0);
	i = QH_NUM_FOR_EDICT(ent);
	if (i < 1 || i > svs.qh_maxclients)
	{
		PR_RunError("Entity is not a client");
	}

	// copy spawn parms out of the client_t
	client = svs.clients + (i - 1);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		(&pr_global_struct->parm1)[i] = client->qh_spawn_parms[i];
}

/*
==============
PF_changelevel
==============
*/
void PF_changelevel(void)
{
	const char* s;

// make sure we don't issue two changelevels
	if (svs.qh_changelevel_issued)
	{
		return;
	}
	svs.qh_changelevel_issued = true;

	s = G_STRING(OFS_PARM0);
	Cbuf_AddText(va("changelevel %s\n",s));
}


void PF_Fixme(void)
{
	PR_RunError("unimplemented bulitin");
}



builtin_t pr_builtin[] =
{
	PF_Fixme,
	PF_makevectors,	// void(entity e)	makevectors         = #1;
	PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
	PF_setmodel,// void(entity e, string m) setmodel	= #3;
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
	PF_Remove,	// void(entity e) remove				= #15;
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
	PF_walkmove,// float(float yaw, float dist) walkmove
	PF_Fixme,	// float(float yaw, float dist) walkmove
	PF_droptofloor,
	PF_lightstyle,
	PF_rint,
	PF_floor,
	PF_ceil,
	PF_Fixme,
	PF_checkbottom,
	PF_pointcontents,
	PF_Fixme,
	PF_fabs,
	PF_aim,
	PF_cvar,
	PF_localcmd,
	PF_nextent,
	PF_particle,
	PF_changeyaw,
	PF_Fixme,
	PF_vectoangles,

	PF_WriteByte,
	PF_WriteChar,
	PF_WriteShort,
	PF_WriteLong,
	PF_WriteCoord,
	PF_WriteAngle,
	PF_WriteString,
	PF_WriteEntity,

	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,

	SV_MoveToGoal,
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

	PF_setspawnparms
};

void PR_InitBuiltins()
{
	pr_builtins = pr_builtin;
	pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);
}
