/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		be_aas_bspq3.c
 *
 * desc:		BSP, Environment Sampling
 *
 * $Archive: /MissionPack/code/botlib/be_aas_bspq3.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "l_memory.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
// traces axial boxes of any size through the world
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bsp_trace_t AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask)
{
	bsp_trace_t bsptrace;
	botimport.Trace(&bsptrace, start, mins, maxs, end, passent, contentmask);
	return bsptrace;
}	//end of the function AAS_Trace
//===========================================================================
// returns the contents at the given point
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_PointContents(vec3_t point)
{
	return botimport.PointContents(point);
}	//end of the function AAS_PointContents
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_EntityCollision(int entnum,
	vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end,
	int contentmask, bsp_trace_t* trace)
{
	bsp_trace_t enttrace;

	botimport.EntityTrace(&enttrace, start, boxmins, boxmaxs, end, entnum, contentmask);
	if (enttrace.fraction < trace->fraction)
	{
		Com_Memcpy(trace, &enttrace, sizeof(bsp_trace_t));
		return true;
	}	//end if
	return false;
}	//end of the function AAS_EntityCollision
//===========================================================================
// returns true if in Potentially Hearable Set
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_inPVS(vec3_t p1, vec3_t p2)
{
	return botimport.inPVS(p1, p2);
}	//end of the function AAS_InPVS
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin)
{
	BotImport_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, origin);
}	//end of the function AAS_BSPModelMinsMaxs
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_FreeBSPEntities(void)
{
	int i;
	bsp_entity_t* ent;
	bsp_epair_t* epair, * nextepair;

	for (i = 1; i < bspworld.numentities; i++)
	{
		ent = &bspworld.entities[i];
		for (epair = ent->epairs; epair; epair = nextepair)
		{
			nextepair = epair->next;
			//
			if (epair->key)
			{
				FreeMemory(epair->key);
			}
			if (epair->value)
			{
				FreeMemory(epair->value);
			}
			FreeMemory(epair);
		}	//end for
	}	//end for
	bspworld.numentities = 0;
}	//end of the function AAS_FreeBSPEntities
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_ParseBSPEntities(void)
{
	script_t* script;
	token_t token;
	bsp_entity_t* ent;
	bsp_epair_t* epair;

	script = LoadScriptMemory(bspworld.dentdata, bspworld.entdatasize, "entdata");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES | SCFL_NOSTRINGESCAPECHARS);//SCFL_PRIMITIVE);

	bspworld.numentities = 1;

	while (PS_ReadToken(script, &token))
	{
		if (String::Cmp(token.string, "{"))
		{
			ScriptError(script, "invalid %s\n", token.string);
			AAS_FreeBSPEntities();
			FreeScript(script);
			return;
		}	//end if
		if (bspworld.numentities >= MAX_BSPENTITIES)
		{
			BotImport_Print(PRT_MESSAGE, "too many entities in BSP file\n");
			break;
		}	//end if
		ent = &bspworld.entities[bspworld.numentities];
		bspworld.numentities++;
		ent->epairs = NULL;
		while (PS_ReadToken(script, &token))
		{
			if (!String::Cmp(token.string, "}"))
			{
				break;
			}
			epair = (bsp_epair_t*)GetClearedHunkMemory(sizeof(bsp_epair_t));
			epair->next = ent->epairs;
			ent->epairs = epair;
			if (token.type != TT_STRING)
			{
				ScriptError(script, "invalid %s\n", token.string);
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}	//end if
			StripDoubleQuotes(token.string);
			epair->key = (char*)GetHunkMemory(String::Length(token.string) + 1);
			String::Cpy(epair->key, token.string);
			if (!PS_ExpectTokenType(script, TT_STRING, 0, &token))
			{
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}	//end if
			StripDoubleQuotes(token.string);
			epair->value = (char*)GetHunkMemory(String::Length(token.string) + 1);
			String::Cpy(epair->value, token.string);
		}	//end while
		if (String::Cmp(token.string, "}"))
		{
			ScriptError(script, "missing }\n");
			AAS_FreeBSPEntities();
			FreeScript(script);
			return;
		}	//end if
	}	//end while
	FreeScript(script);
}	//end of the function AAS_ParseBSPEntities
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_DumpBSPData(void)
{
	AAS_FreeBSPEntities();

	if (bspworld.dentdata)
	{
		FreeMemory(bspworld.dentdata);
	}
	bspworld.dentdata = NULL;
	bspworld.entdatasize = 0;
	//
	bspworld.loaded = false;
	Com_Memset(&bspworld, 0, sizeof(bspworld));
}	//end of the function AAS_DumpBSPData
//===========================================================================
// load an bsp file
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_LoadBSPFile(void)
{
	AAS_DumpBSPData();
	bspworld.entdatasize = String::Length(CM_EntityString()) + 1;
	bspworld.dentdata = (char*)GetClearedHunkMemory(bspworld.entdatasize);
	Com_Memcpy(bspworld.dentdata, CM_EntityString(), bspworld.entdatasize);
	AAS_ParseBSPEntities();
	bspworld.loaded = true;
	return BLERR_NOERROR;
}	//end of the function AAS_LoadBSPFile
