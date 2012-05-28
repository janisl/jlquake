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

void AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs)
{
	//bounding box size for each presence type
	vec3_t boxmins[4] = {{-15, -15, -24}, {-15, -15, -24}, {-18, -18, -24}, {-18, -18, -24}};
	vec3_t boxmaxs[4] = {{ 15,  15,  32}, { 15,  15,   8}, { 18,  18,  48}, { 18,  18,  24}};

	int index;
	if (presencetype == PRESENCE_NORMAL)
	{
		index = 0;
	}
	else if (presencetype == PRESENCE_CROUCH)
	{
		index = 1;
	}
	else
	{
		BotImport_Print(PRT_FATAL, "AAS_PresenceTypeBoundingBox: unknown presence type\n");
		index = 1;
	}
	if (GGameType & GAME_ET)
	{
		index += 2;
	}
	VectorCopy(boxmins[index], mins);
	VectorCopy(boxmaxs[index], maxs);
}

void AAS_InitAASLinkHeap()
{
	int max_aaslinks = (*aasworld).linkheapsize;
	//if there's no link heap present
	if (!(*aasworld).linkheap)
	{
		max_aaslinks = GGameType & GAME_Quake3 ? (int)LibVarValue("max_aaslinks", "6144") : (int)4096;
		if (max_aaslinks < 0)
		{
			max_aaslinks = 0;
		}
		(*aasworld).linkheapsize = max_aaslinks;
		(*aasworld).linkheap = (aas_link_t*)Mem_ClearedAlloc(max_aaslinks * sizeof(aas_link_t));
	}
	else
	{
		// just clear the memory
		Com_Memset((*aasworld).linkheap, 0, (*aasworld).linkheapsize * sizeof(aas_link_t));
	}
	//link the links on the heap
	(*aasworld).linkheap[0].prev_ent = NULL;
	(*aasworld).linkheap[0].next_ent = &(*aasworld).linkheap[1];
	for (int i = 1; i < max_aaslinks - 1; i++)
	{
		(*aasworld).linkheap[i].prev_ent = &(*aasworld).linkheap[i - 1];
		(*aasworld).linkheap[i].next_ent = &(*aasworld).linkheap[i + 1];
	}
	(*aasworld).linkheap[max_aaslinks - 1].prev_ent = &(*aasworld).linkheap[max_aaslinks - 2];
	(*aasworld).linkheap[max_aaslinks - 1].next_ent = NULL;
	//pointer to the first free link
	(*aasworld).freelinks = &(*aasworld).linkheap[0];
}

void AAS_FreeAASLinkHeap()
{
	if ((*aasworld).linkheap)
	{
		Mem_Free((*aasworld).linkheap);
	}
	(*aasworld).linkheap = NULL;
	(*aasworld).linkheapsize = 0;
}

aas_link_t* AAS_AllocAASLink()
{
	aas_link_t* link = (*aasworld).freelinks;
	if (!link)
	{
		if (bot_developer)
		{
			BotImport_Print(PRT_FATAL, "empty aas link heap\n");
		}
		return NULL;
	}
	if ((*aasworld).freelinks)
	{
		(*aasworld).freelinks = (*aasworld).freelinks->next_ent;
	}
	if ((*aasworld).freelinks)
	{
		(*aasworld).freelinks->prev_ent = NULL;
	}
	return link;
}

void AAS_DeAllocAASLink(aas_link_t* link)
{
	if ((*aasworld).freelinks)
	{
		(*aasworld).freelinks->prev_ent = link;
	}
	link->prev_ent = NULL;
	link->next_ent = (*aasworld).freelinks;
	link->prev_area = NULL;
	link->next_area = NULL;
	(*aasworld).freelinks = link;
}

void AAS_InitAASLinkedEntities()
{
	if (!(*aasworld).loaded)
	{
		return;
	}
	if ((*aasworld).arealinkedentities)
	{
		Mem_Free((*aasworld).arealinkedentities);
	}
	(*aasworld).arealinkedentities = (aas_link_t**)Mem_ClearedAlloc(
		(*aasworld).numareas * sizeof(aas_link_t*));
}

void AAS_FreeAASLinkedEntities()
{
	if ((*aasworld).arealinkedentities)
	{
		Mem_Free((*aasworld).arealinkedentities);
	}
	(*aasworld).arealinkedentities = NULL;
}
