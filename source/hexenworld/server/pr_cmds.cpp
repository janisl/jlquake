
#include "qwsvdef.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "../../common/file_formats/bsp29.h"

#define RETURN_EDICT(e) (((int*)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))
#define RETURN_STRING(s) (((int*)pr_globals)[OFS_RETURN] = PR_SetString(s))

QMsg* WriteDest(void);

#define MSG_BROADCAST   0		// unreliable to all
#define MSG_ONE         1		// reliable to one (msg_entity)
#define MSG_ALL         2		// reliable to all
#define MSG_INIT        3		// write to the init string
#define MSG_MULTICAST   4		// for multicast()

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
	ed = PROG_TO_EDICT(*pr_globalVars.self);
	ED_Print(ed);

	SV_Error("Program error");
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
	ed = PROG_TO_EDICT(*pr_globalVars.self);
	ED_Print(ed);
	ED_Free(ed);

	SV_Error("Program error");
}



/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void PF_makevectors(void)
{
	AngleVectors(G_VECTOR(OFS_PARM0), pr_globalVars.v_forward, pr_globalVars.v_right, pr_globalVars.v_up);
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
//	VectorCopy (min, e->v.mins);
//	VectorCopy (max, e->v.maxs);
//	VectorSubtract (max, min, e->v.size);
//	SV_LinkEdict (e, false);
	SetMinMaxSize(e, min, max, false);
}


/*
=================
PF_setmodel

setmodel(entity, model)
Also sets size, mins, and maxs for inline bmodels
=================
*/
void PF_setmodel(void)
{
	qhedict_t* e;
	const char* m, ** check;
	int i;
	clipHandle_t mod;

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

void PF_setpuzzlemodel(void)
{
	qhedict_t* e;
	const char* m, ** check;
	clipHandle_t mod;
	int i;
	char NewName[256];

	e = G_EDICT(OFS_PARM0);
	m = G_STRING(OFS_PARM1);

	sprintf(NewName,"models/puzzle/%s.mdl",m);
// check to see if model was properly precached
	for (i = 0, check = sv.qh_model_precache; *check; i++, check++)
		if (!String::Cmp(*check, NewName))
		{
			break;
		}

	e->SetModel(PR_SetString(ED_NewString(NewName)));

	if (!*check)
	{
//		PR_RunError ("no precache: %s\n", NewName);
		Con_Printf("**** NO PRECACHE FOR PUZZLE PIECE:");
		Con_Printf("**** %s\n",NewName);

		sv.qh_model_precache[i] = PR_GetString(e->GetModel());
	}

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
	int level;

	level = G_FLOAT(OFS_PARM0);

	s = PF_VarString(1);

	if (spartanPrint->value == 1 && level < 2)
	{
		return;
	}

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
	char* s;
	client_t* client;
	int entnum;
	int level;

	entnum = G_EDICTNUM(OFS_PARM0);
	level = G_FLOAT(OFS_PARM1);

	s = PF_VarString(2);

	if (entnum < 1 || entnum > HWMAX_CLIENTS)
	{
		Con_Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	if (spartanPrint->value == 1 && level < 2)
	{
		return;
	}


	SV_ClientPrintf(client, level, "%s", s);
}

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


	if (spartanPrint->value == 1 && Style < 2)
	{
		return;
	}

	if (Index <= 0)
	{
		PR_RunError("PF_name_print: index(%d) <= 0",Index);
	}

	if (Index > HWMAX_CLIENTS)
	{
		PR_RunError("PF_name_print: index(%d) > HWMAX_CLIENTS",Index);
	}


	if ((int)G_FLOAT(OFS_PARM0) == MSG_BROADCAST)	//broadcast message--send like bprint, print it out on server too.
	{
		client_t* cl;
		int i;

		Con_Printf("%s",&svs.clients[Index - 1].name);

		for (i = 0, cl = svs.clients; i < HWMAX_CLIENTS; i++, cl++)
		{
			if (Style < cl->messagelevel)
			{
				continue;
			}
			if (cl->state != CS_ACTIVE)//should i be checking CS_CONNECTED too?
			{
				if (cl->state)	//not fully in so won't know name yet, explicitly say the name
				{
					cl->netchan.message.WriteByte(h2svc_print);
					cl->netchan.message.WriteByte(Style);
					cl->netchan.message.WriteString2((char*)&svs.clients[Index - 1].name);
				}
				continue;
			}
			cl->netchan.message.WriteByte(hwsvc_name_print);
			cl->netchan.message.WriteByte(Style);
			cl->netchan.message.WriteByte(Index - 1);	//knows the name, send the index.
		}
		return;
	}

	WriteDest()->WriteByte(hwsvc_name_print);
	WriteDest()->WriteByte(Style);
	WriteDest()->WriteByte(Index - 1);	//heh, don't need a short here.
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

	if (spartanPrint->value == 1 && Style < 2)
	{
		return;
	}

	if (Index <= 0)
	{
		PR_RunError("PF_sprint_indexed: index(%d) < 1",Index);
	}

	if (Index > pr_string_count)
	{
		PR_RunError("PF_sprint_indexed: index(%d) >= pr_string_count(%d)",Index,pr_string_count);
	}

	if ((int)G_FLOAT(OFS_PARM0) == MSG_BROADCAST)	//broadcast message--send like bprint, print it out on server too.
	{
		client_t* cl;
		int i;

		Con_Printf("%s",&pr_global_strings[pr_string_index[Index - 1]]);

		for (i = 0, cl = svs.clients; i < HWMAX_CLIENTS; i++, cl++)
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

	WriteDest()->WriteByte(hwsvc_indexed_print);
	WriteDest()->WriteByte(Style);
	WriteDest()->WriteShort(Index);
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

	if (entnum < 1 || entnum > HWMAX_CLIENTS)
	{
		Con_Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	client->netchan.message.WriteChar(h2svc_centerprint);
	client->netchan.message.WriteString2(s);
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
	char* s;
	client_t* cl;
	int i;

	s = PF_VarString(0);

	for (i = 0, cl = svs.clients; i < HWMAX_CLIENTS; i++, cl++)
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
	char* s;
	client_t* client;
	int entnum;

	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);

	if (entnum < 1 || entnum > HWMAX_CLIENTS)
	{
		Con_Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	client->netchan.message.WriteChar(h2svc_centerprint);
	client->netchan.message.WriteString2(s);
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
	float newl;

	value1 = G_VECTOR(OFS_PARM0);

	newl = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	newl = sqrt(newl);

	if (newl == 0)
	{
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	}
	else
	{
		newl = 1 / newl;
		newvalue[0] = value1[0] * newl;
		newvalue[1] = value1[1] * newl;
		newvalue[2] = value1[2] * newl;
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
	float newl;

	value1 = G_VECTOR(OFS_PARM0);

	newl = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	newl = sqrt(newl);

	G_FLOAT(OFS_RETURN) = newl;
}

/*
=================
PF_vhlen

scalar vhlen(vector)
=================
*/
void PF_vhlen(void)
{
	float* value1;
	float newl;

	value1 = G_VECTOR(OFS_PARM0);

	newl = value1[0] * value1[0] + value1[1] * value1[1];
	newl = sqrt(newl);

	G_FLOAT(OFS_RETURN) = newl;
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

	sv.qh_signon.WriteByte(h2svc_spawnstaticsound);
	for (i = 0; i < 3; i++)
		sv.qh_signon.WriteCoord(pos[i]);

	sv.qh_signon.WriteByte(soundnum);

	sv.qh_signon.WriteByte(vol * 255);
	sv.qh_signon.WriteByte(attenuation * 64);

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
		Sys_Error("SV_StartSound: channel = %i", channel);
	}

	SV_StopSound(entity, channel);
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
		Sys_Error("SV_StartSound: channel = %i", channel);
	}

	SV_UpdateSoundPos(entity, channel);
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
	static qboolean DidIt = false;

	if (!DidIt)
	{
		DidIt = true;

		Con_Printf("break statement\n");
#ifdef _WIN32
		DebugBreak();
#else
		*(int*)-4 = 0;	// dump to debugger
#endif
	}
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
	float save_hull;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	nomonsters = G_FLOAT(OFS_PARM2);
	ent = G_EDICT(OFS_PARM3);

	save_hull = ent->GetHull();
	ent->SetHull(0);
	trace = SV_Move(v1, vec3_origin, vec3_origin, v2, nomonsters, ent);
	ent->SetHull(save_hull);

	*pr_globalVars.trace_allsolid = trace.allsolid;
	*pr_globalVars.trace_startsolid = trace.startsolid;
	*pr_globalVars.trace_fraction = trace.fraction;
	*pr_globalVars.trace_inwater = trace.inwater;
	*pr_globalVars.trace_inopen = trace.inopen;
	VectorCopy(trace.endpos, pr_globalVars.trace_endpos);
	VectorCopy(trace.plane.normal, pr_globalVars.trace_plane_normal);
	*pr_globalVars.trace_plane_dist =  trace.plane.dist;
	if (trace.entityNum >= 0)
	{
		*pr_globalVars.trace_ent = EDICT_TO_PROG(QH_EDICT_NUM(trace.entityNum));
	}
	else
	{
		*pr_globalVars.trace_ent = EDICT_TO_PROG(sv.qh_edicts);
	}
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
	trace = SV_Move(v1, mins, maxs, v2, nomonsters, ent);
	ent->SetHull(save_hull);

	*pr_globalVars.trace_allsolid = trace.allsolid;
	*pr_globalVars.trace_startsolid = trace.startsolid;
	*pr_globalVars.trace_fraction = trace.fraction;
	*pr_globalVars.trace_inwater = trace.inwater;
	*pr_globalVars.trace_inopen = trace.inopen;
	VectorCopy(trace.endpos, pr_globalVars.trace_endpos);
	VectorCopy(trace.plane.normal, pr_globalVars.trace_plane_normal);
	*pr_globalVars.trace_plane_dist =  trace.plane.dist;
	if (trace.entityNum >= 0)
	{
		*pr_globalVars.trace_ent = EDICT_TO_PROG(QH_EDICT_NUM(trace.entityNum));
	}
	else
	{
		*pr_globalVars.trace_ent = EDICT_TO_PROG(sv.qh_edicts);
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
	if (check > HWMAX_CLIENTS)
	{
		check = HWMAX_CLIENTS;
	}

	if (check == HWMAX_CLIENTS)
	{
		i = 1;
	}
	else
	{
		i = check + 1;
	}

	for (;; i++)
	{
		if (i == HWMAX_CLIENTS + 1)
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

// find a new check if on a new frame
	if (sv.qh_time - sv.qh_lastchecktime >= HX_FRAME_TIME)
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
	self = PROG_TO_EDICT(*pr_globalVars.self);
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
	if (entnum < 1 || entnum > HWMAX_CLIENTS)
	{
		PR_RunError("Parm 0 not a client");
	}
	str = G_STRING(OFS_PARM1);

	old = host_client;
	host_client = &svs.clients[entnum - 1];

	host_client->netchan.message.WriteByte(h2svc_stufftext);
	host_client->netchan.message.WriteString2(str);

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
	Con_Printf("%s",PF_VarString(0));
}

void PF_dprintf(void)
{
	char temp[256];
	float v;

	v = G_FLOAT(OFS_PARM1);

	if (v == (int)v)
	{
		sprintf(temp, "%d",(int)v);
	}
	else
	{
		sprintf(temp, "%5.1f",v);
	}

	Con_Printf(G_STRING(OFS_PARM0),temp);
}

void PF_dprintv(void)
{
	char temp[256];

	sprintf(temp, "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM1)[0], G_VECTOR(OFS_PARM1)[1], G_VECTOR(OFS_PARM1)[2]);

	Con_Printf(G_STRING(OFS_PARM0),temp);
}

char pr_string_temp[1024];

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
	G_FLOAT(OFS_RETURN) = Q_fabs(v);
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

void PF_SpawnTemp(void)
{
	qhedict_t* ed;

	ed = ED_Alloc_Temp();

	RETURN_EDICT(ed);
}

void PF_Remove(void)
{
	qhedict_t* ed;
	int i;

	ed = G_EDICT(OFS_PARM0);
	if (ed == sv.qh_edicts)
	{
		Con_Printf("Tried to remove the world at %s in %s!\n",
			PR_GetString(pr_xfunction->s_name), PR_GetString(pr_xfunction->s_file));
		return;
	}

	i = QH_NUM_FOR_EDICT(ed);
	if (i <= HWMAX_CLIENTS)
	{
		Con_Printf("Tried to remove a client at %s in %s!\n",
			PR_GetString(pr_xfunction->s_name), PR_GetString(pr_xfunction->s_file));
		return;
	}
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

	if (sv.state != SS_LOADING && !ignore_precache)
	{
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
	}

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (i = 0; i < MAX_SOUNDS_HW; i++)
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

void PF_precache_sound2(void)
{
	if (!registered->value)
	{
		return;
	}

	PF_precache_sound();
}

void PF_precache_sound3(void)
{
	if (!registered->value)
	{
		return;
	}

	PF_precache_sound();
}

void PF_precache_model(void)
{
	const char* s;
	int i;

	if (sv.state != SS_LOADING && !ignore_precache)
	{
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
	}

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (i = 0; i < MAX_MODELS_H2; i++)
	{
		if (!sv.qh_model_precache[i])
		{
			sv.qh_model_precache[i] = s;
			return;
		}
		if (!String::Cmp(sv.qh_model_precache[i], s))
		{
			return;
		}
	}
	PR_RunError("PF_precache_model: overflow");
}

void PF_precache_model2(void)
{
	if (!registered->value)
	{
		return;
	}

	PF_precache_model();
}

void PF_precache_model3(void)
{
	if (!registered->value)
	{
		return;
	}

	PF_precache_model();
}

void PF_precache_puzzle_model(void)
{
	int i;
	const char* s;
	char temp[256];
	const char* m;

	if (sv.state != SS_LOADING && !ignore_precache)
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
			return;
		}
		if (!String::Cmp(sv.qh_model_precache[i], s))
		{
			return;
		}
	}
	PR_RunError("PF_precache_puzzle_model: overflow");
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
	qboolean set_trace;

	ent = PROG_TO_EDICT(*pr_globalVars.self);
	yaw = G_FLOAT(OFS_PARM0);
	dist = G_FLOAT(OFS_PARM1);
	set_trace = G_FLOAT(OFS_PARM2);

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
	oldself = *pr_globalVars.self;

	G_FLOAT(OFS_RETURN) = SV_movestep(ent, move, true, true, set_trace);


// restore program state
	pr_xfunction = oldf;
	*pr_globalVars.self = oldself;
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

	ent = PROG_TO_EDICT(*pr_globalVars.self);

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

	for (j = 0, client = svs.clients; j < HWMAX_CLIENTS; j++, client++)
		if (client->state == CS_ACTIVE)
		{
			client->netchan.message.WriteChar(h2svc_lightstyle);
			client->netchan.message.WriteChar(style);
			client->netchan.message.WriteString2(val);
		}
}

//==========================================================================
//
// PF_lightstylevalue
//
// void lightstylevalue(float style);
//
//==========================================================================


//extern
int cl_lightstylevalue[256];

// rjr
//I need this!  MG
void PF_lightstylevalue(void)
{
	int style;

	style = G_FLOAT(OFS_PARM0);
	if (style < 0 || style >= MAX_LIGHTSTYLES_H2)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	//G_FLOAT(OFS_RETURN) = 0;//
	G_FLOAT(OFS_RETURN) = (int)cl_lightstylevalue[style];
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
	int j;
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
	cl_lightstylevalue[styleNumber] = value;

	if (sv.state != SS_GAME)
	{
		return;
	}

	// Send message to all clients on this server
	for (j = 0, client = svs.clients; j < HWMAX_CLIENTS; j++, client++)
		if (client->state == CS_ACTIVE)
		{
			client->netchan.message.WriteChar(h2svc_lightstyle);
			client->netchan.message.WriteChar(styleNumber);
			client->netchan.message.WriteString2(styleString);
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
	vec3_t start, dir, end, bestdir,hold_org;
	int i, j;
	q1trace_t tr;
	float dist, bestdist;
	float speed;
	float* shot_org;
	float save_hull;

	ent = G_EDICT(OFS_PARM0);
	shot_org = G_VECTOR(OFS_PARM1);
	speed = G_FLOAT(OFS_PARM2);

//	VectorCopy (ent->v.origin, start);
	VectorCopy(shot_org, start);
	start[2] += 20;

// try sending a trace straight
	VectorCopy(pr_globalVars.v_forward, dir);
	VectorMA(start, 2048, dir, end);
	save_hull = ent->GetHull();
	ent->SetHull(0);
	tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
	ent->SetHull(save_hull);
	if (tr.entityNum >= 0 && QH_EDICT_NUM(tr.entityNum)->GetTakeDamage() == DAMAGE_YES &&
		(!teamplay->value || ent->GetTeam() <= 0 || ent->GetTeam() != QH_EDICT_NUM(tr.entityNum)->GetTeam()))
	{
		VectorCopy(pr_globalVars.v_forward, G_VECTOR(OFS_RETURN));
		return;
	}


// try all possible entities
	VectorCopy(dir, bestdir);
	bestdist = sv_aim->value;
	bestent = NULL;

	check = NEXT_EDICT(sv.qh_edicts);
	for (i = 1; i < sv.qh_num_edicts; i++, check = NEXT_EDICT(check))
	{
		if (check->GetTakeDamage() != DAMAGE_YES)
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
		dist = DotProduct(dir, pr_globalVars.v_forward);
		if (dist < bestdist)
		{
			continue;	// to far to turn
		}
		save_hull = ent->GetHull();
		ent->SetHull(0);
		tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
		ent->SetHull(save_hull);
		if (QH_EDICT_NUM(tr.entityNum) == check)
		{	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}

	if (bestent)
	{	// Since all origins are at the base, move the point to the middle of the victim model
		hold_org[0] = bestent->GetOrigin()[0];
		hold_org[1] = bestent->GetOrigin()[1];
		hold_org[2] = bestent->GetOrigin()[2] + (0.5 * bestent->GetMaxs()[2]);

		VectorSubtract(hold_org,shot_org,dir);
		dist = DotProduct(dir, pr_globalVars.v_forward);
		VectorScale(pr_globalVars.v_forward, dist, end);
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

	ent = PROG_TO_EDICT(*pr_globalVars.self);
	current = AngleMod(ent->GetAngles()[1]);
	ideal = ent->GetIdealYaw();
	speed = ent->GetYawSpeed();

	if (current == ideal)
	{
		G_FLOAT(OFS_RETURN) = 0;
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

	G_FLOAT(OFS_RETURN) = move;

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
		ent = PROG_TO_EDICT(*pr_globalVars.msg_entity);
		entnum = QH_NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > HWMAX_CLIENTS)
		{
			PR_RunError("WriteDest: not a client");
		}
		return &svs.clients[entnum - 1].netchan.message;

	case MSG_ALL:
		return &sv.qh_reliable_datagram;

	case MSG_INIT:
		if (sv.state != SS_LOADING)
		{
			PR_RunError("PF_Write_*: MSG_INIT can only be written in spawn functions");
		}
		return &sv.qh_signon;

	case MSG_MULTICAST:
		return &sv.multicast;

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

void PF_WriteChar(void)
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
	if (i < 1 || i > HWMAX_CLIENTS)
	{
		PR_RunError("Entity is not a client");
	}

	// copy spawn parms out of the client_t
	client = svs.clients + (i - 1);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		pr_globalVars.parm1[i] = client->qh_spawn_parms[i];
}

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
		Cbuf_AddText(va("map %s %s\n",s1, s2));
	}
	else
	{
		Cbuf_AddText(va("changelevel2 %s %s\n",s1, s2));
	}
}


/*
==============
PF_logfrag

logfrag (killer, killee)
==============
*/
void PF_logfrag(void)
{
	qhedict_t* ent1, * ent2;
	int e1, e2;
	char* s;

	ent1 = G_EDICT(OFS_PARM0);
	ent2 = G_EDICT(OFS_PARM1);

	e1 = QH_NUM_FOR_EDICT(ent1);
	e2 = QH_NUM_FOR_EDICT(ent2);

	if (e1 < 1 || e1 > HWMAX_CLIENTS ||
		e2 < 1 || e2 > HWMAX_CLIENTS)
	{
		return;
	}

	s = va("\\%s\\%s\\\n",svs.clients[e1 - 1].name, svs.clients[e2 - 1].name);

	svs.qh_log[svs.qh_logsequence & 1].Print(s);
	if (sv_fraglogfile)
	{
		FS_Printf(sv_fraglogfile, s);
	}
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
	else if (e1 <= HWMAX_CLIENTS)
	{
		value = Info_ValueForKey(svs.clients[e1 - 1].userinfo, key);
	}
	else
	{
		value = "";
	}

	RETURN_STRING(value);
}

/*
==============
PF_stof

float(string s) stof
==============
*/
void PF_stof(void)
{
	const char* s;

	s = G_STRING(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = String::Atof(s);
}


/*
==============
PF_multicast

void(vector where, float set) multicast
==============
*/
void PF_multicast(void)
{
	float* o;
	int to;

	o = G_VECTOR(OFS_PARM0);
	to = G_FLOAT(OFS_PARM1);

	SV_Multicast(o, to);
}


void PF_sqrt(void)
{
	G_FLOAT(OFS_RETURN) = sqrt(G_FLOAT(OFS_PARM0));
}

void PF_Fixme(void)
{
	PR_RunError("unimplemented builtin");
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

	WriteDest()->WriteByte(hwsvc_plaque);
	WriteDest()->WriteShort(Index);
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

// save program state, because SV_movestep may call other progs
	oldf = pr_xfunction;
	oldself = *pr_globalVars.self;

	G_INT(OFS_RETURN) = SV_movestep(ent, v, false, true, set_trace);

// restore program state
	pr_xfunction = oldf;
	*pr_globalVars.self = oldself;
}

/*
#define MAX_LEVELS 10

int PaladinExp[MAX_LEVELS+1] =
{
    0,				// Level 1
    500,        // Level 2
    1000,       // Level 3
    1500,       // Level 4
    2000,       // Level 5
    2500,       // Level 6
    3000,       // Level 7
    3500,       // Level 8
    4000,       // Level 9
    4500,       // Level 10
    1000        // Required amount for each level afterwards
};

int ClericExp[MAX_LEVELS+1] =
{
    0,				// Level 1
    500,        // Level 2
    1000,       // Level 3
    1500,       // Level 4
    2000,       // Level 5
    2500,       // Level 6
    3000,       // Level 7
    3500,       // Level 8
    4000,       // Level 9
    4500,       // Level 10
    1000        // Required amount for each level afterwards
};

int NecroExp[MAX_LEVELS+1] =
{
    0,				// Level 1
    500,        // Level 2
    1000,       // Level 3
    1500,       // Level 4
    2000,       // Level 5
    2500,       // Level 6
    3000,       // Level 7
    3500,       // Level 8
    4000,       // Level 9
    4500,       // Level 10
    1000        // Required amount for each level afterwards
};

int TheifExp[MAX_LEVELS+1] =
{
    0,				// Level 1
    500,        // Level 2
    1000,       // Level 3
    1500,       // Level 4
    2000,       // Level 5
    2500,       // Level 6
    3000,       // Level 7
    3500,       // Level 8
    4000,       // Level 9
    4500,       // Level 10
    1000        // Required amount for each level afterwards
};

int FindLevel(qhedict_t *WhichPlayer)
{
    int *Chart;
    int Amount,counter,Level;

    switch((int)WhichPlayer->v.playerclass)
    {
        case CLASS_PALADIN:
            Chart = PaladinExp;
            break;
        case CLASS_CLERIC:
            Chart = ClericExp;
            break;
        case CLASS_NECROMANCER:
            Chart = NecroExp;
            break;
        case CLASS_THEIF:
            Chart = TheifExp;
            break;
    }

    Level = 0;
    for(counter=0;counter<MAX_LEVELS;counter++)
    {
        if (WhichPlayer->v.experience <= Chart[counter])
        {
            Level = counter+1;
            break;
        }
    }

    if (!Level)
    {
        Amount = WhichPlayer->v.experience - Chart[MAX_LEVELS-1];
        Level = (Amount % Chart[MAX_LEVELS]) + MAX_LEVELS;
    }

    return Level;
}

void PF_AwardExperience(void)
{
    qhedict_t	*ToEnt, *FromEnt;
    float Amount;
    int AfterLevel;
    qboolean IsPlayer;
//	client_t	*client;
    int			entnum;
//	char temp[200];
    globalvars_t	pr_save;

    ToEnt = G_EDICT(OFS_PARM0);
    FromEnt = G_EDICT(OFS_PARM1);
    Amount = G_FLOAT(OFS_PARM2);

    if (!Amount) return;

    IsPlayer = (String::ICmp(PR_GetString(ToEnt->v.classname), "player") == 0);

    if (FromEnt && Amount == 0.0)
    {
        Amount = FromEnt->v.experience_value;
    }

    ToEnt->v.experience += Amount;

    if (IsPlayer)
    {
        AfterLevel = FindLevel(ToEnt);

        Con_Printf("Total Experience: %d\n",(int)ToEnt->v.experience);

        if (ToEnt->v.level != AfterLevel)
        {
            ToEnt->v.level = AfterLevel;
            entnum = QH_NUM_FOR_EDICT(ToEnt);

            if (entnum >= 1 && entnum <= svs.qh_maxclients)
            {
                pr_save = *pr_global_struct;
                *pr_globalVars.time = sv.time;
                *pr_globalVars.self = EDICT_TO_PROG(ToEnt);
                PR_ExecuteProgram (pr_global_struct->PlayerAdvanceLevel);

                *pr_global_struct = pr_save;

            }
        }
    }
}

*/

/*				client = &svs.clients[entnum-1];

                sprintf(temp,"You are now level %d\n",AfterLevel);

                client->message.WriteChar(h2svc_print);
                client->message.WriteString2(temp );
*/

void PF_Cos(void)
{
	float angle;

	angle = G_FLOAT(OFS_PARM0);

	angle = angle * M_PI * 2 / 360;

	G_FLOAT(OFS_RETURN) = cos(angle);
}

void PF_Sin(void)
{
	float angle;

	angle = G_FLOAT(OFS_PARM0);

	angle = angle * M_PI * 2 / 360;

	G_FLOAT(OFS_RETURN) = sin(angle);
}

void PF_AdvanceFrame(void)
{
	qhedict_t* Ent;
	float Start,End,Result;

	Ent = PROG_TO_EDICT(*pr_globalVars.self);
	Start = G_FLOAT(OFS_PARM0);
	End = G_FLOAT(OFS_PARM1);

	if ((Start < End && (Ent->GetFrame() < Start || Ent->GetFrame() > End)) ||
		(Start > End && (Ent->GetFrame() > Start || Ent->GetFrame() < End)))
	{	// Didn't start in the range
		Ent->SetFrame(Start);
		Result = 0;
	}
	else if (Ent->GetFrame() == End)
	{	// Wrapping
		Ent->SetFrame(Start);
		Result = 1;
	}
	else if (End > Start)
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
	else if (End < Start)
	{	// Reverse Advance
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
	else
	{
		Ent->SetFrame(End);
		Result = 1;
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
	char temp[1024];

	entnum = G_EDICTNUM(OFS_PARM0);
	e = G_EDICT(OFS_PARM0);
	NewClass = G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > HWMAX_CLIENTS)
	{
		Con_Printf("tried to change class of a non-client\n");
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
	Info_SetValueForKey(host_client->userinfo, "playerclass", temp, HWMAX_INFO_STRING, 64, 64, !sv_highchars->value);
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

	if (entnum < 1 || entnum > HWMAX_CLIENTS)
	{
		Con_Printf("tried to change siege_team of a non-client\n");
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

	for (j = 0, client = svs.clients; j < HWMAX_CLIENTS; j++, client++)
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
	SV_ParseEffect(NULL);
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
	SV_Multicast(vec3_origin, MULTICAST_ALL_R);
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

void PF_randomrange(void)
{
	float num,minv,maxv;

	minv = G_FLOAT(OFS_PARM0);
	maxv = G_FLOAT(OFS_PARM1);

	num = (rand() & 0x7fff) / ((float)0x7fff);

	G_FLOAT(OFS_RETURN) = ((maxv - minv) * num) + minv;
}

void PF_randomvalue(void)
{
	float num,range;

	range = G_FLOAT(OFS_PARM0);

	num = (rand() & 0x7fff) / ((float)0x7fff);

	G_FLOAT(OFS_RETURN) = range * num;
}

void PF_randomvrange(void)
{
	float num,* minv,* maxv;
	vec3_t result;

	minv = G_VECTOR(OFS_PARM0);
	maxv = G_VECTOR(OFS_PARM1);

	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[0] = ((maxv[0] - minv[0]) * num) + minv[0];
	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[1] = ((maxv[1] - minv[1]) * num) + minv[1];
	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[2] = ((maxv[2] - minv[2]) * num) + minv[2];

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

void PF_randomvvalue(void)
{
	float num,* range;
	vec3_t result;

	range = G_VECTOR(OFS_PARM0);

	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[0] = range[0] * num;
	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[1] = range[1] * num;
	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[2] = range[2] * num;

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

void PF_concatv(void)
{
	float* in,* range;
	vec3_t result;

	in = G_VECTOR(OFS_PARM0);
	range = G_VECTOR(OFS_PARM1);

	VectorCopy(in, result);
	if (result[0] < -range[0])
	{
		result[0] = -range[0];
	}
	if (result[0] > range[0])
	{
		result[0] = range[0];
	}
	if (result[1] < -range[1])
	{
		result[1] = -range[1];
	}
	if (result[1] > range[1])
	{
		result[1] = range[1];
	}
	if (result[2] < -range[2])
	{
		result[2] = -range[2];
	}
	if (result[2] > range[2])
	{
		result[2] = range[2];
	}

	VectorCopy(result, G_VECTOR(OFS_RETURN));
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


void PF_v_factor(void)
// returns (v_right * factor_x) + (v_forward * factor_y) + (v_up * factor_z)
{
	float* range;
	vec3_t result;

	range = G_VECTOR(OFS_PARM0);

	result[0] = (pr_globalVars.v_right[0] * range[0]) +
				(pr_globalVars.v_forward[0] * range[1]) +
				(pr_globalVars.v_up[0] * range[2]);

	result[1] = (pr_globalVars.v_right[1] * range[0]) +
				(pr_globalVars.v_forward[1] * range[1]) +
				(pr_globalVars.v_up[1] * range[2]);

	result[2] = (pr_globalVars.v_right[2] * range[0]) +
				(pr_globalVars.v_forward[2] * range[1]) +
				(pr_globalVars.v_up[2] * range[2]);

	VectorCopy(result, G_VECTOR(OFS_RETURN));
}

void PF_v_factorrange(void)
// returns (v_right * factor_x) + (v_forward * factor_y) + (v_up * factor_z)
{
	float num,* minv,* maxv;
	vec3_t result,r2;

	minv = G_VECTOR(OFS_PARM0);
	maxv = G_VECTOR(OFS_PARM1);

	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[0] = ((maxv[0] - minv[0]) * num) + minv[0];
	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[1] = ((maxv[1] - minv[1]) * num) + minv[1];
	num = (rand() & 0x7fff) / ((float)0x7fff);
	result[2] = ((maxv[2] - minv[2]) * num) + minv[2];

	r2[0] = (pr_globalVars.v_right[0] * result[0]) +
			(pr_globalVars.v_forward[0] * result[1]) +
			(pr_globalVars.v_up[0] * result[2]);

	r2[1] = (pr_globalVars.v_right[1] * result[0]) +
			(pr_globalVars.v_forward[1] * result[1]) +
			(pr_globalVars.v_up[1] * result[2]);

	r2[2] = (pr_globalVars.v_right[2] * result[0]) +
			(pr_globalVars.v_forward[2] * result[1]) +
			(pr_globalVars.v_up[2] * result[2]);

	VectorCopy(r2, G_VECTOR(OFS_RETURN));
}

void SV_setseed(int seed);
float SV_seedrand(void);
float SV_GetMultiEffectId(void);
void SV_ParseMultiEffect(QMsg* sb);


void PF_setseed(void)
{
	SV_setseed(G_FLOAT(OFS_PARM0));
}

void PF_seedrand(void)
{
	G_FLOAT(OFS_RETURN) = SV_seedrand();
}

void PF_multieffect(void)
{
	SV_ParseMultiEffect(&sv.qh_reliable_datagram);

}

void PF_getmeid(void)
{
	G_FLOAT(OFS_RETURN) = SV_GetMultiEffectId();
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
		Con_Printf("SV_StartSound: %s not precacheed\n", sample);
		return;
	}
	entity->SetWpnSound(sound_num);
}


builtin_t pr_builtin[] =
{
	PF_Fixme,
	PF_makevectors,	// void(entity e)	makevectors         = #1;
	PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
	PF_setmodel,// void(entity e, string m) setmodel	= #3;
	PF_setsize,	// void(entity e, vector min, vector max) setsize = #4;
	PF_lightstylestatic,// 5
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
	PF_tracearea,							// float(vector v1, vector v2, vector mins, vector maxs,
	PF_droptofloor,
	PF_lightstyle,
	PF_rint,
	PF_floor,
	PF_ceil,
	PF_Fixme,
	PF_checkbottom,
	PF_pointcontents,
	PF_particle2,
	PF_fabs,
	PF_aim,
	PF_cvar,
	PF_localcmd,
	PF_nextent,
	PF_particle,							//																			= #48
	PF_changeyaw,
	PF_vhlen,								// float(vector v) vhlen											= #50
	PF_vectoangles,

	PF_WriteByte,
	PF_WriteChar,
	PF_WriteShort,
	PF_WriteLong,
	PF_WriteCoord,
	PF_WriteAngle,
	PF_WriteString,
	PF_WriteEntity,

	PF_dprintf,								// void(string s1, string s2) dprint										= #60
	PF_Cos,									//																			= #61
	PF_Sin,									//																			= #62
	PF_AdvanceFrame,						//																			= #63
	PF_dprintv,								// void(string s1, string s2) dprint										= #64
	PF_RewindFrame,							//																			= #65
	PF_setclass,

	SV_MoveToGoal,
	PF_precache_file,
	PF_makestatic,

	PF_changelevel,
	PF_lightstylevalue,	// 71

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

	PF_infokey,	//100
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
