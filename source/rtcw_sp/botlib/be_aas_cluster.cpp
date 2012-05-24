/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_aas_cluster.c
 *
 * desc:		area clustering
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_memory.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_NumberClusterAreas(int clusternum)
{
	int i, portalnum;
	aas_cluster_t* cluster;
	aas_portal_t* portal;

	aasworld->clusters[clusternum].numareas = 0;
	aasworld->clusters[clusternum].numreachabilityareas = 0;
	//number all areas in this cluster WITH reachabilities
	for (i = 1; i < aasworld->numareas; i++)
	{
		//
		if (aasworld->areasettings[i].cluster != clusternum)
		{
			continue;
		}
		//
		if (!AAS_AreaReachability(i))
		{
			continue;
		}
		//
		aasworld->areasettings[i].clusterareanum = aasworld->clusters[clusternum].numareas;
		//the cluster has an extra area
		aasworld->clusters[clusternum].numareas++;
		aasworld->clusters[clusternum].numreachabilityareas++;
	}	//end for
		//number all portals in this cluster WITH reachabilities
	cluster = &aasworld->clusters[clusternum];
	for (i = 0; i < cluster->numportals; i++)
	{
		portalnum = aasworld->portalindex[cluster->firstportal + i];
		portal = &aasworld->portals[portalnum];
		if (!AAS_AreaReachability(portal->areanum))
		{
			continue;
		}
		if (portal->frontcluster == clusternum)
		{
			portal->clusterareanum[0] = cluster->numareas++;
			aasworld->clusters[clusternum].numreachabilityareas++;
		}	//end if
		else
		{
			portal->clusterareanum[1] = cluster->numareas++;
			aasworld->clusters[clusternum].numreachabilityareas++;
		}	//end else
	}	//end for
		//number all areas in this cluster WITHOUT reachabilities
	for (i = 1; i < aasworld->numareas; i++)
	{
		//
		if (aasworld->areasettings[i].cluster != clusternum)
		{
			continue;
		}
		//
		if (AAS_AreaReachability(i))
		{
			continue;
		}
		//
		aasworld->areasettings[i].clusterareanum = aasworld->clusters[clusternum].numareas;
		//the cluster has an extra area
		aasworld->clusters[clusternum].numareas++;
	}	//end for
		//number all portals in this cluster WITHOUT reachabilities
	cluster = &aasworld->clusters[clusternum];
	for (i = 0; i < cluster->numportals; i++)
	{
		portalnum = aasworld->portalindex[cluster->firstportal + i];
		portal = &aasworld->portals[portalnum];
		if (AAS_AreaReachability(portal->areanum))
		{
			continue;
		}
		if (portal->frontcluster == clusternum)
		{
			portal->clusterareanum[0] = cluster->numareas++;
		}	//end if
		else
		{
			portal->clusterareanum[1] = cluster->numareas++;
		}	//end else
	}	//end for
}	//end of the function AAS_NumberClusterAreas
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_FindClusters(void)
{
	int i;
	aas_cluster_t* cluster;

	AAS_RemoveClusterAreas();
	//
	for (i = 1; i < aasworld->numareas; i++)
	{
		//if the area is already part of a cluster
		if (aasworld->areasettings[i].cluster)
		{
			continue;
		}
		// if not flooding through faces only use areas that have reachabilities
		if (!aasworld->areasettings[i].numreachableareas)
		{
			continue;
		}
		//if the area is a cluster portal
		if (aasworld->areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			continue;
		}
		if (aasworld->numclusters >= AAS_MAX_CLUSTERS)
		{
			AAS_Error("AAS_MAX_CLUSTERS");
			return qfalse;
		}	//end if
		cluster = &aasworld->clusters[aasworld->numclusters];
		cluster->numareas = 0;
		cluster->numreachabilityareas = 0;
		cluster->firstportal = aasworld->portalindexsize;
		cluster->numportals = 0;
		//flood the areas in this cluster
		if (!AAS_FloodClusterAreas_r(i, aasworld->numclusters))
		{
			return qfalse;
		}
		if (!AAS_FloodClusterAreasUsingReachabilities(aasworld->numclusters))
		{
			return qfalse;
		}
		//number the cluster areas
		AAS_NumberClusterAreas(aasworld->numclusters);
		//Log_Write("cluster %d has %d areas\r\n", aasworld->numclusters, cluster->numareas);
		aasworld->numclusters++;
	}	//end for
	return qtrue;
}	//end of the function AAS_FindClusters
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitClustering(void)
{
	int i, removedPortalAreas;
	int n, total, numreachabilityareas;

	if (!aasworld->loaded)
	{
		return;
	}
	//if there are clusters
	if (aasworld->numclusters >= 1)
	{
		//if clustering isn't forced
		if (!((int)LibVarGetValue("forceclustering")) &&
			!((int)LibVarGetValue("forcereachability")))
		{
			return;
		}
	}	//end if
		//
	AAS_CountForcedClusterPortals();
	//remove all area cluster marks
	AAS_RemoveClusterAreas();
	//find possible cluster portals
	AAS_FindPossiblePortals();
	//craete portals to for the bot view
	AAS_CreateViewPortals();
	//remove all portals that are not closing a cluster
	//AAS_RemoveNotClusterClosingPortals();
	//initialize portal memory
	if (aasworld->portals)
	{
		FreeMemory(aasworld->portals);
	}
	aasworld->portals = (aas_portal_t*)GetClearedMemory(AAS_MAX_PORTALS * sizeof(aas_portal_t));
	//initialize portal index memory
	if (aasworld->portalindex)
	{
		FreeMemory(aasworld->portalindex);
	}
	aasworld->portalindex = (aas_portalindex_t*)GetClearedMemory(AAS_MAX_PORTALINDEXSIZE * sizeof(aas_portalindex_t));
	//initialize cluster memory
	if (aasworld->clusters)
	{
		FreeMemory(aasworld->clusters);
	}
	aasworld->clusters = (aas_cluster_t*)GetClearedMemory(AAS_MAX_CLUSTERS * sizeof(aas_cluster_t));
	//
	removedPortalAreas = 0;
	BotImport_Print(PRT_MESSAGE, "\r%6d removed portal areas", removedPortalAreas);
	while (1)
	{
		BotImport_Print(PRT_MESSAGE, "\r%6d", removedPortalAreas);
		//initialize the number of portals and clusters
		aasworld->numportals = 1;			//portal 0 is a dummy
		aasworld->portalindexsize = 0;
		aasworld->numclusters = 1;			//cluster 0 is a dummy
		//create the portals from the portal areas
		AAS_CreatePortals();
		//
		removedPortalAreas++;
		//find the clusters
		if (!AAS_FindClusters())
		{
			continue;
		}
		//test the portals
		if (!AAS_TestPortals())
		{
			continue;
		}
		//
		break;
	}	//end while
	BotImport_Print(PRT_MESSAGE, "\n");
	//the AAS file should be saved
	aasworld->savefile = qtrue;
	// report cluster info
	BotImport_Print(PRT_MESSAGE, "%6d portals created\n", aasworld->numportals);
	BotImport_Print(PRT_MESSAGE, "%6d clusters created\n", aasworld->numclusters);
	for (i = 1; i < aasworld->numclusters; i++)
	{
		BotImport_Print(PRT_MESSAGE, "cluster %d has %d reachability areas\n", i,
			aasworld->clusters[i].numreachabilityareas);
	}	//end for
		// report AAS file efficiency
	numreachabilityareas = 0;
	total = 0;
	for (i = 0; i < aasworld->numclusters; i++)
	{
		n = aasworld->clusters[i].numreachabilityareas;
		numreachabilityareas += n;
		total += n * n;
	}
	total += numreachabilityareas * aasworld->numportals;
	//
	BotImport_Print(PRT_MESSAGE, "%6i total reachability areas\n", numreachabilityareas);
	BotImport_Print(PRT_MESSAGE, "%6i AAS memory/CPU usage (the lower the better)\n", total * 3);
}	//end of the function AAS_InitClustering
