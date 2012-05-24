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

void AAS_RemoveClusterAreas()
{
	for (int i = 1; i < aasworld->numareas; i++)
	{
		aasworld->areasettings[i].cluster = 0;
	}
}

static bool AAS_UpdatePortal(int areanum, int clusternum)
{
	//find the portal of the area
	int portalnum;
	for (portalnum = 1; portalnum < aasworld->numportals; portalnum++)
	{
		if (aasworld->portals[portalnum].areanum == areanum)
		{
			break;
		}
	}

	if (portalnum == aasworld->numportals)
	{
		AAS_Error("no portal of area %d", areanum);
		return true;
	}

	aas_portal_t* portal = &aasworld->portals[portalnum];
	//if the portal is already fully updated
	if (portal->frontcluster == clusternum)
	{
		return true;
	}
	if (portal->backcluster == clusternum)
	{
		return true;
	}
	//if the portal has no front cluster yet
	if (!portal->frontcluster)
	{
		portal->frontcluster = clusternum;
	}
	//if the portal has no back cluster yet
	else if (!portal->backcluster)
	{
		portal->backcluster = clusternum;
	}
	else
	{
		//remove the cluster portal flag contents
		aasworld->areasettings[areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
		Log_Write("portal area %d is seperating more than two clusters\r\n", areanum);
		return false;
	}
	if (aasworld->portalindexsize >= AAS_MAX_PORTALINDEXSIZE)
	{
		AAS_Error("AAS_MAX_PORTALINDEXSIZE");
		return true;
	}
	//set the area cluster number to the negative portal number
	aasworld->areasettings[areanum].cluster = -portalnum;
	//add the portal to the cluster using the portal index
	aas_cluster_t* cluster = &aasworld->clusters[clusternum];
	aasworld->portalindex[cluster->firstportal + cluster->numportals] = portalnum;
	aasworld->portalindexsize++;
	cluster->numportals++;
	return true;
}

bool AAS_FloodClusterAreas_r(int areanum, int clusternum)
{
	if (areanum <= 0 || areanum >= aasworld->numareas)
	{
		AAS_Error("AAS_FloodClusterAreas_r: areanum out of range");
		return false;
	}
	//if the area is already part of a cluster
	if (aasworld->areasettings[areanum].cluster > 0)
	{
		if (aasworld->areasettings[areanum].cluster == clusternum)
		{
			return true;
		}
		//
		//there's a reachability going from one cluster to another only in one direction
		//
		AAS_Error("cluster %d touched cluster %d at area %d\r\n",
			clusternum, aasworld->areasettings[areanum].cluster, areanum);
		return false;
	}
	//don't add the cluster portal areas to the clusters
	if (aasworld->areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL)
	{
		return AAS_UpdatePortal(areanum, clusternum);
	}
	//set the area cluster number
	aasworld->areasettings[areanum].cluster = clusternum;
	aasworld->areasettings[areanum].clusterareanum =
		aasworld->clusters[clusternum].numareas;
	//the cluster has an extra area
	aasworld->clusters[clusternum].numareas++;

	//use the reachabilities to flood into other areas
	for (int i = 0; i < aasworld->areasettings[areanum].numreachableareas; i++)
	{
		if (!aasworld->reachability[
			aasworld->areasettings[areanum].firstreachablearea + i].areanum)
		{
			continue;
		}
		if (!AAS_FloodClusterAreas_r(aasworld->reachability[
			aasworld->areasettings[areanum].firstreachablearea + i].areanum, clusternum))
		{
			return false;
		}
	}
	return true;
}

// try to flood from all areas without cluster into areas with a cluster set
bool AAS_FloodClusterAreasUsingReachabilities(int clusternum)
{
	for (int i = 1; i < aasworld->numareas; i++)
	{
		//if this area already has a cluster set
		if (aasworld->areasettings[i].cluster)
		{
			continue;
		}
		//if this area is a cluster portal
		if (aasworld->areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			continue;
		}
		//loop over the reachable areas from this area
		for (int j = 0; j < aasworld->areasettings[i].numreachableareas; j++)
		{
			//the reachable area
			int areanum = aasworld->reachability[aasworld->areasettings[i].firstreachablearea + j].areanum;
			//if this area is a cluster portal
			if (aasworld->areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL)
			{
				continue;
			}
			//if this area has a cluster set
			if (aasworld->areasettings[areanum].cluster)
			{
				if (!AAS_FloodClusterAreas_r(i, clusternum))
				{
					return false;
				}
				i = 0;
				break;
			}
		}
	}
	return true;
}

void AAS_CreatePortals()
{
	for (int i = 1; i < aasworld->numareas; i++)
	{
		//if the area is a cluster portal
		if (aasworld->areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			if (aasworld->numportals >= AAS_MAX_PORTALS)
			{
				AAS_Error("AAS_MAX_PORTALS");
				return;
			}
			aas_portal_t* portal = &aasworld->portals[aasworld->numportals];
			portal->areanum = i;
			portal->frontcluster = 0;
			portal->backcluster = 0;
			aasworld->numportals++;
		}
	}
}

static void AAS_ConnectedAreas_r(int* areanums, int numareas, int* connectedareas, int curarea)
{
	connectedareas[curarea] = true;
	aas_area_t* area = &aasworld->areas[areanums[curarea]];
	for (int i = 0; i < area->numfaces; i++)
	{
		int facenum = abs(aasworld->faceindex[area->firstface + i]);
		aas_face_t* face = &aasworld->faces[facenum];
		//if the face is solid
		if (face->faceflags & FACE_SOLID)
		{
			continue;
		}
		//get the area at the other side of the face
		int otherareanum;
		if (face->frontarea != areanums[curarea])
		{
			otherareanum = face->frontarea;
		}
		else
		{
			otherareanum = face->backarea;
		}
		//check if the face is leading to one of the other areas
		int j;
		for (j = 0; j < numareas; j++)
		{
			if (areanums[j] == otherareanum)
			{
				break;
			}
		}
		//if the face isn't leading to one of the other areas
		if (j == numareas)
		{
			continue;
		}
		//if the other area is already connected
		if (connectedareas[j])
		{
			continue;
		}
		//recursively proceed with the other area
		AAS_ConnectedAreas_r(areanums, numareas, connectedareas, j);
	}
}

static bool AAS_ConnectedAreas(int* areanums, int numareas)
{
	int connectedareas[MAX_PORTALAREAS];
	Com_Memset(connectedareas, 0, sizeof(connectedareas));
	if (numareas < 1)
	{
		return false;
	}
	if (numareas == 1)
	{
		return true;
	}
	AAS_ConnectedAreas_r(areanums, numareas, connectedareas, 0);
	for (int i = 0; i < numareas; i++)
	{
		if (!connectedareas[i])
		{
			return false;
		}
	}
	return true;
}

static int AAS_GetAdjacentAreasWithLessPresenceTypes_r(int* areanums, int numareas, int curareanum)
{
	areanums[numareas++] = curareanum;
	aas_area_t* area = &aasworld->areas[curareanum];
	int presencetype = aasworld->areasettings[curareanum].presencetype;
	for (int i = 0; i < area->numfaces; i++)
	{
		int facenum = abs(aasworld->faceindex[area->firstface + i]);
		aas_face_t* face = &aasworld->faces[facenum];
		//if the face is solid
		if (face->faceflags & FACE_SOLID)
		{
			continue;
		}
		//the area at the other side of the face
		int otherareanum;
		if (face->frontarea != curareanum)
		{
			otherareanum = face->frontarea;
		}
		else
		{
			otherareanum = face->backarea;
		}

		int otherpresencetype = aasworld->areasettings[otherareanum].presencetype;
		//if the other area has less presence types
		if ((presencetype & ~otherpresencetype) &&
			!(otherpresencetype & ~presencetype))
		{
			//check if the other area isn't already in the list
			int j;
			for (j = 0; j < numareas; j++)
			{
				if (otherareanum == areanums[j])
				{
					break;
				}
			}
			//if the other area isn't already in the list
			if (j == numareas)
			{
				if (numareas >= MAX_PORTALAREAS)
				{
					AAS_Error("MAX_PORTALAREAS");
					return numareas;
				}
				numareas = AAS_GetAdjacentAreasWithLessPresenceTypes_r(areanums, numareas, otherareanum);
			}
		}
	}
	return numareas;
}

static int AAS_CheckAreaForPossiblePortals(int areanum)
{
	//if it isn't already a portal
	if (aasworld->areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL)
	{
		return 0;
	}
	//it must be a grounded area
	if (!(aasworld->areasettings[areanum].areaflags & AREA_GROUNDED))
	{
		return 0;
	}

	int numareafrontfaces[MAX_PORTALAREAS], numareabackfaces[MAX_PORTALAREAS];
	Com_Memset(numareafrontfaces, 0, sizeof(numareafrontfaces));
	Com_Memset(numareabackfaces, 0, sizeof(numareabackfaces));
	int frontfacenums[MAX_PORTALAREAS], backfacenums[MAX_PORTALAREAS];
	int frontareanums[MAX_PORTALAREAS], backareanums[MAX_PORTALAREAS];
	int numfrontfaces = 0;
	int numbackfaces = 0;
	int numfrontareas = 0;
	int numbackareas = 0;
	int frontplanenum = -1;
	int backplanenum = -1;
	//add any adjacent areas with less presence types
	int areanums[MAX_PORTALAREAS];
	int numareas = AAS_GetAdjacentAreasWithLessPresenceTypes_r(areanums, 0, areanum);

	for (int i = 0; i < numareas; i++)
	{
		aas_area_t* area = &aasworld->areas[areanums[i]];
		for (int j = 0; j < area->numfaces; j++)
		{
			int facenum = abs(aasworld->faceindex[area->firstface + j]);
			aas_face_t* face = &aasworld->faces[facenum];
			//if the face is solid
			if (face->faceflags & FACE_SOLID)
			{
				continue;
			}
			//check if the face is shared with one of the other areas
			int k;
			for (k = 0; k < numareas; k++)
			{
				if (k == i)
				{
					continue;
				}
				if (face->frontarea == areanums[k] || face->backarea == areanums[k])
				{
					break;
				}
			}
			//if the face is shared
			if (k != numareas)
			{
				continue;
			}
			//the number of the area at the other side of the face
			int otherareanum;
			if (face->frontarea == areanums[i])
			{
				otherareanum = face->backarea;
			}
			else
			{
				otherareanum = face->frontarea;
			}
			//if the other area already is a cluter portal
			if (aasworld->areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL)
			{
				return 0;
			}
			//number of the plane of the area
			int faceplanenum = face->planenum & ~1;
			//
			if (frontplanenum < 0 || faceplanenum == frontplanenum)
			{
				frontplanenum = faceplanenum;
				frontfacenums[numfrontfaces++] = facenum;
				for (k = 0; k < numfrontareas; k++)
				{
					if (frontareanums[k] == otherareanum)
					{
						break;
					}
				}
				if (k == numfrontareas)
				{
					frontareanums[numfrontareas++] = otherareanum;
				}
				numareafrontfaces[i]++;
			}
			else if (backplanenum < 0 || faceplanenum == backplanenum)
			{
				backplanenum = faceplanenum;
				backfacenums[numbackfaces++] = facenum;
				for (k = 0; k < numbackareas; k++)
				{
					if (backareanums[k] == otherareanum)
					{
						break;
					}
				}
				if (k == numbackareas)
				{
					backareanums[numbackareas++] = otherareanum;
				}
				numareabackfaces[i]++;
			}
			else
			{
				return 0;
			}
		}
	}
	//every area should have at least one front face and one back face
	for (int i = 0; i < numareas; i++)
	{
		if (!numareafrontfaces[i] || !numareabackfaces[i])
		{
			return 0;
		}
	}
	//the front areas should all be connected
	if (!AAS_ConnectedAreas(frontareanums, numfrontareas))
	{
		return 0;
	}
	//the back areas should all be connected
	if (!AAS_ConnectedAreas(backareanums, numbackareas))
	{
		return 0;
	}
	//none of the front faces should have a shared edge with a back face
	int i;
	for (i = 0; i < numfrontfaces; i++)
	{
		aas_face_t* frontface = &aasworld->faces[frontfacenums[i]];
		int fen;
		for (fen = 0; fen < frontface->numedges; fen++)
		{
			int frontedgenum = abs(aasworld->edgeindex[frontface->firstedge + fen]);
			int j;
			for (j = 0; j < numbackfaces; j++)
			{
				aas_face_t* backface = &aasworld->faces[backfacenums[j]];
				int ben;
				for (ben = 0; ben < backface->numedges; ben++)
				{
					int backedgenum = abs(aasworld->edgeindex[backface->firstedge + ben]);
					if (frontedgenum == backedgenum)
					{
						break;
					}
				}
				if (ben != backface->numedges)
				{
					break;
				}
			}
			if (j != numbackfaces)
			{
				break;
			}
		}
		if (fen != frontface->numedges)
		{
			break;
		}
	}
	if (i != numfrontfaces)
	{
		return 0;
	}
	//set the cluster portal contents
	for (int i = 0; i < numareas; i++)
	{
		aasworld->areasettings[areanums[i]].contents |= AREACONTENTS_CLUSTERPORTAL;
		//this area can be used as a route portal
		aasworld->areasettings[areanums[i]].contents |= AREACONTENTS_ROUTEPORTAL;
		Log_Write("possible portal: %d\r\n", areanums[i]);
	}

	return numareas;
}

void AAS_FindPossiblePortals()
{
	int numpossibleportals = 0;
	for (int i = 1; i < aasworld->numareas; i++)
	{
		numpossibleportals += AAS_CheckAreaForPossiblePortals(i);
	}
	BotImport_Print(PRT_MESSAGE, "\r%6d possible portal areas\n", numpossibleportals);
}

bool AAS_TestPortals()
{
	for (int i = 1; i < aasworld->numportals; i++)
	{
		aas_portal_t* portal = &aasworld->portals[i];
		if (!portal->frontcluster)
		{
			aasworld->areasettings[portal->areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
			Log_Write("portal area %d has no front cluster\r\n", portal->areanum);
			return false;
		}
		if (!portal->backcluster)
		{
			aasworld->areasettings[portal->areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
			Log_Write("portal area %d has no back cluster\r\n", portal->areanum);
			return false;
		}
	}
	return true;
}

void AAS_CountForcedClusterPortals()
{
	int num = 0;
	for (int i = 1; i < aasworld->numareas; i++)
	{
		if (aasworld->areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			Log_Write("area %d is a forced portal area\r\n", i);
			num++;
		}
	}
	BotImport_Print(PRT_MESSAGE, "%6d forced portal areas\n", num);
}

void AAS_CreateViewPortals()
{
	for (int i = 1; i < aasworld->numareas; i++)
	{
		if (aasworld->areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			aasworld->areasettings[i].contents |= AREACONTENTS_VIEWPORTAL;
		}
	}
}

void AAS_SetViewPortalsAsClusterPortals()
{
	for (int i = 1; i < aasworld->numareas; i++)
	{
		if (aasworld->areasettings[i].contents & AREACONTENTS_VIEWPORTAL)
		{
			aasworld->areasettings[i].contents |= AREACONTENTS_CLUSTERPORTAL;
		}
	}
}
