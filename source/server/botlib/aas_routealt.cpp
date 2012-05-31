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

midrangearea_t* midrangeareas;
int* clusterareas;
int numclusterareas;

void AAS_AltRoutingFloodCluster_r(int areanum)
{
	//add the current area to the areas of the current cluster
	clusterareas[numclusterareas] = areanum;
	numclusterareas++;
	//remove the area from the mid range areas
	midrangeareas[areanum].valid = false;
	//flood to other areas through the faces of this area
	aas_area_t* area = &aasworld->areas[areanum];
	for (int i = 0; i < area->numfaces; i++)
	{
		aas_face_t* face = &aasworld->faces[abs(aasworld->faceindex[area->firstface + i])];
		//get the area at the other side of the face
		int otherareanum;
		if (face->frontarea == areanum)
		{
			otherareanum = face->backarea;
		}
		else
		{
			otherareanum = face->frontarea;
		}
		//if there is an area at the other side of this face
		if (!otherareanum)
		{
			continue;
		}
		//if the other area is not a midrange area
		if (!midrangeareas[otherareanum].valid)
		{
			continue;
		}

		AAS_AltRoutingFloodCluster_r(otherareanum);
	}
}

void AAS_InitAlternativeRouting()
{
	if (GGameType & (GAME_WolfSP & GAME_WolfMP))
	{
		return;
	}
	if (midrangeareas)
	{
		Mem_Free(midrangeareas);
	}
	midrangeareas = (midrangearea_t*)Mem_Alloc(aasworld->numareas * sizeof(midrangearea_t));
	if (clusterareas)
	{
		Mem_Free(clusterareas);
	}
	clusterareas = (int*)Mem_Alloc(aasworld->numareas * sizeof(int));
}

void AAS_ShutdownAlternativeRouting()
{
	if (GGameType & (GAME_WolfSP & GAME_WolfMP))
	{
		return;
	}
	if (midrangeareas)
	{
		Mem_Free(midrangeareas);
	}
	midrangeareas = NULL;
	if (clusterareas)
	{
		Mem_Free(clusterareas);
	}
	clusterareas = NULL;
	numclusterareas = 0;
}
