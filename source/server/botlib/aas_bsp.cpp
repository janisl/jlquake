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
#include "local.h"
#include "../tech3/local.h"

#define MAX_BSPENTITIES     4096

//bsp entity epair
struct bsp_epair_t
{
	char* key;
	char* value;
	bsp_epair_t* next;
};

//bsp data entity
struct bsp_entity_t
{
	bsp_epair_t* epairs;
};

//id Sofware BSP data
struct bsp_t
{
	//true when bsp file is loaded
	int loaded;
	//entity data
	int entdatasize;
	char* dentdata;
	//bsp entities
	int numentities;
	bsp_entity_t entities[MAX_BSPENTITIES];
	//memory used for strings and epairs
	byte* ebuffer;
};

//global bsp
static bsp_t bspworld;

int AAS_NextBSPEntity(int ent)
{
	ent++;
	if (ent >= 1 && ent < bspworld.numentities)
	{
		return ent;
	}
	return 0;
}

static int AAS_BSPEntityInRange(int ent)
{
	if (ent <= 0 || ent >= bspworld.numentities)
	{
		BotImport_Print(PRT_MESSAGE, "bsp entity out of range\n");
		return false;
	}
	return true;
}

bool AAS_ValueForBSPEpairKey(int ent, const char* key, char* value, int size)
{
	value[0] = '\0';
	if (!AAS_BSPEntityInRange(ent))
	{
		return false;
	}
	for (bsp_epair_t* epair = bspworld.entities[ent].epairs; epair; epair = epair->next)
	{
		if (!String::Cmp(epair->key, key))
		{
			String::NCpyZ(value, epair->value, size);
			return true;
		}
	}
	return false;
}

bool AAS_VectorForBSPEpairKey(int ent, const char* key, vec3_t v)
{
	VectorClear(v);
	char buf[MAX_EPAIRKEY];
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return false;
	}
	//scanf into doubles, then assign, so it is vec_t size independent
	double v1, v2, v3;
	v1 = v2 = v3 = 0;
	sscanf(buf, "%lf %lf %lf", &v1, &v2, &v3);
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	return true;
}

bool AAS_FloatForBSPEpairKey(int ent, const char* key, float* value)
{
	*value = 0;
	char buf[MAX_EPAIRKEY];
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return false;
	}
	*value = String::Atof(buf);
	return true;
}

bool AAS_IntForBSPEpairKey(int ent, const char* key, int* value)
{
	*value = 0;
	char buf[MAX_EPAIRKEY];
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return false;
	}
	*value = String::Atoi(buf);
	return true;
}

static void AAS_FreeBSPEntities()
{
	if (bspworld.ebuffer)
	{
		Mem_Free(bspworld.ebuffer);
	}
	bspworld.numentities = 0;
}

static void AAS_ParseBSPEntities()
{
	// RF, modified this, so that it first gathers up memory requirements, then allocates a single chunk,
	// and places the strings all in there

	bspworld.ebuffer = NULL;

	script_t* script = LoadScriptMemory(bspworld.dentdata, bspworld.entdatasize, "entdata");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES | SCFL_NOSTRINGESCAPECHARS);

	int bufsize = 0;

	token_t token;
	while (PS_ReadToken(script, &token))
	{
		if (String::Cmp(token.string, "{"))
		{
			ScriptError(script, "invalid %s\n", token.string);
			AAS_FreeBSPEntities();
			FreeScript(script);
			return;
		}
		if (bspworld.numentities >= MAX_BSPENTITIES)
		{
			BotImport_Print(PRT_MESSAGE, "too many entities in BSP file\n");
			break;
		}
		while (PS_ReadToken(script, &token))
		{
			if (!String::Cmp(token.string, "}"))
			{
				break;
			}
			bufsize += sizeof(bsp_epair_t);
			if (token.type != TT_STRING)
			{
				ScriptError(script, "invalid %s\n", token.string);
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}
			StripDoubleQuotes(token.string);
			bufsize += String::Length(token.string) + 1;
			if (!PS_ExpectTokenType(script, TT_STRING, 0, &token))
			{
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}
			StripDoubleQuotes(token.string);
			bufsize += String::Length(token.string) + 1;
		}
		if (String::Cmp(token.string, "}"))
		{
			ScriptError(script, "missing }\n");
			AAS_FreeBSPEntities();
			FreeScript(script);
			return;
		}
	}
	FreeScript(script);

	byte* buffer = (byte*)Mem_ClearedAlloc(bufsize);
	byte* buftrav = buffer;
	bspworld.ebuffer = buffer;

	// RF, now parse the entities into memory
	// RF, NOTE: removed error checks for speed, no need to do them twice

	script = LoadScriptMemory(bspworld.dentdata, bspworld.entdatasize, "entdata");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES | SCFL_NOSTRINGESCAPECHARS);

	bspworld.numentities = 1;

	while (PS_ReadToken(script, &token))
	{
		bsp_entity_t* ent = &bspworld.entities[bspworld.numentities];
		bspworld.numentities++;
		ent->epairs = NULL;
		while (PS_ReadToken(script, &token))
		{
			if (!String::Cmp(token.string, "}"))
			{
				break;
			}
			bsp_epair_t* epair = (bsp_epair_t*)buftrav; buftrav += sizeof(bsp_epair_t);
			epair->next = ent->epairs;
			ent->epairs = epair;
			StripDoubleQuotes(token.string);
			epair->key = (char*)buftrav; buftrav += (String::Length(token.string) + 1);
			String::Cpy(epair->key, token.string);
			if (!PS_ExpectTokenType(script, TT_STRING, 0, &token))
			{
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}
			StripDoubleQuotes(token.string);
			epair->value = (char*)buftrav; buftrav += (String::Length(token.string) + 1);
			String::Cpy(epair->value, token.string);
		}
	}
	FreeScript(script);
}

void AAS_DumpBSPData()
{
	AAS_FreeBSPEntities();

	if (bspworld.dentdata)
	{
		Mem_Free(bspworld.dentdata);
	}
	bspworld.dentdata = NULL;
	bspworld.entdatasize = 0;

	bspworld.loaded = false;
	Com_Memset(&bspworld, 0, sizeof(bspworld));
}

int AAS_LoadBSPFile()
{
	AAS_DumpBSPData();
	bspworld.entdatasize = String::Length(CM_EntityString()) + 1;
	bspworld.dentdata = (char*)Mem_ClearedAlloc(bspworld.entdatasize);
	Com_Memcpy(bspworld.dentdata, CM_EntityString(), bspworld.entdatasize);
	AAS_ParseBSPEntities();
	bspworld.loaded = true;
	return BLERR_NOERROR;
}

void AAS_BSPModelMinsMaxs(int modelnum, const vec3_t angles, vec3_t outmins, vec3_t outmaxs)
{
	clipHandle_t h = CM_InlineModel(modelnum);
	vec3_t mins, maxs;
	CM_ModelBounds(h, mins, maxs);
	//if the model is rotated
	if ((angles[0] || angles[1] || angles[2]))
	{
		// expand for rotation
		float max = RadiusFromBounds(mins, maxs);
		for (int i = 0; i < 3; i++)
		{
			mins[i] = -max;
			maxs[i] = max;
		}
	}
	if (outmins)
	{
		VectorCopy(mins, outmins);
	}
	if (outmaxs)
	{
		VectorCopy(maxs, outmaxs);
	}
}

// traces axial boxes of any size through the world
bsp_trace_t AAS_Trace(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
	int passent, int contentmask)
{
	q3trace_t trace;
	SVT3_Trace(&trace, start, mins, maxs, end, passent, contentmask, false);
	bsp_trace_t bsptrace;
	//copy the trace information
	bsptrace.allsolid = trace.allsolid;
	bsptrace.startsolid = trace.startsolid;
	bsptrace.fraction = trace.fraction;
	VectorCopy(trace.endpos, bsptrace.endpos);
	bsptrace.plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace.plane.normal);
	bsptrace.plane.signbits = trace.plane.signbits;
	bsptrace.plane.type = trace.plane.type;
	bsptrace.surface.value = trace.surfaceFlags;
	bsptrace.ent = trace.entityNum;
	bsptrace.exp_dist = 0;
	bsptrace.sidenum = 0;
	bsptrace.contents = 0;
	return bsptrace;
}

bool AAS_EntityCollision(int entnum, const vec3_t start, const vec3_t boxmins, const vec3_t boxmaxs,
	const vec3_t end, int contentmask, bsp_trace_t* trace)
{
	q3trace_t enttrace;
	SVT3_ClipToEntity(&enttrace, start, boxmins, boxmaxs, end, entnum, contentmask, false);
	if (enttrace.fraction >= trace->fraction)
	{
		return false;
	}
	//copy the trace information
	trace->allsolid = enttrace.allsolid;
	trace->startsolid = enttrace.startsolid;
	trace->fraction = enttrace.fraction;
	VectorCopy(enttrace.endpos, trace->endpos);
	trace->plane.dist = enttrace.plane.dist;
	VectorCopy(enttrace.plane.normal, trace->plane.normal);
	trace->plane.signbits = enttrace.plane.signbits;
	trace->plane.type = enttrace.plane.type;
	trace->surface.value = enttrace.surfaceFlags;
	trace->ent = enttrace.entityNum;
	trace->exp_dist = 0;
	trace->sidenum = 0;
	trace->contents = 0;
	return true;
}

// returns the contents at the given point
int AAS_PointContents(const vec3_t point)
{
	return SVT3_PointContents(point, -1);
}
