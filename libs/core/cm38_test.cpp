//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"
#include "cm38_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QClipMap38::PointLeafnum
//
//==========================================================================

int QClipMap38::PointLeafnum(const vec3_t P) const
{
	if (!numplanes)
	{
		// sound may call this without map loaded
		return 0;
	}
	return PointLeafnum_r(P, 0);
}

//==========================================================================
//
//	QClipMap38::PointLeafnum_r
//
//==========================================================================

int QClipMap38::PointLeafnum_r(const vec3_t P, int Num) const
{
	while (Num >= 0)
	{
		const cnode_t* Node = nodes + Num;
		const cplane_t* Plane = Node->plane;

		float d;
		if (Plane->type < 3)
		{
			d = P[Plane->type] - Plane->dist;
		}
		else
		{
			d = DotProduct (Plane->normal, P) - Plane->dist;
		}
		if (d < 0)
		{
			Num = Node->children[1];
		}
		else
		{
			Num = Node->children[0];
		}
	}

	c_pointcontents++;		// optimize counter

	return -1 - Num;
}

//==========================================================================
//
//	QClipMap38::BoxLeafnums_r
//
//==========================================================================

void QClipMap38::BoxLeafnums_r(leafList_t* ll, int NodeNum) const
{
	while (1)
	{
		if (NodeNum < 0)
		{
			int LeafNum = -1 - NodeNum;

			// store the lastLeaf even if the list is overflowed
			if (leafs[LeafNum].cluster != -1)
			{
				ll->lastLeaf = LeafNum;
			}

			if (ll->count >= ll->maxcount)
			{
				return;
			}
			ll->list[ll->count++] = LeafNum;
			return;
		}
	
		const cnode_t* node = &nodes[NodeNum];
		const cplane_t* plane = node->plane;
		int s = BOX_ON_PLANE_SIDE(ll->bounds[0], ll->bounds[1], plane);
		if (s == 1)
		{
			NodeNum = node->children[0];
		}
		else if (s == 2)
		{
			NodeNum = node->children[1];
		}
		else
		{
			// go down both
			if (ll->topnode == -1)
			{
				ll->topnode = NodeNum;
			}
			BoxLeafnums_r(ll, node->children[0]);
			NodeNum = node->children[1];
		}
	}
}

//==========================================================================
//
//	QClipMap38::BoxLeafnums
//
//	Fills in a list of all the leafs touched
//
//==========================================================================

int QClipMap38::BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List,
	int ListSize, int* TopNode, int* LastLeaf) const
{
	leafList_t ll;

	VectorCopy(Mins, ll.bounds[0]);
	VectorCopy(Maxs, ll.bounds[1]);
	ll.list = List;
	ll.count = 0;
	ll.maxcount = ListSize;
	ll.topnode = -1;
	ll.lastLeaf = 0;

	BoxLeafnums_r(&ll, cmodels[0].headnode);

	if (TopNode)
	{
		*TopNode = ll.topnode;
	}
	if (LastLeaf)
	{
		*LastLeaf = ll.lastLeaf;
	}
	return ll.count;
}

//==========================================================================
//
//	QClipMap38::PointContentsQ1
//
//==========================================================================

int QClipMap38::PointContentsQ1(const vec3_t P, clipHandle_t Model)
{
	return ContentsToQ1(PointContentsQ2(P, Model));
}

//==========================================================================
//
//	QClipMap38::PointContentsQ2
//
//==========================================================================

int QClipMap38::PointContentsQ2(const vec3_t p, clipHandle_t Handle)
{
	if (!numnodes)
	{
		// map not loaded
		return 0;
	}

	cmodel_t* model = ClipHandleToModel(Handle);
	int l = PointLeafnum_r(p, model->headnode);

	return leafs[l].contents;
}

//==========================================================================
//
//	QClipMap38::PointContentsQ3
//
//==========================================================================

int QClipMap38::PointContentsQ3(const vec3_t P, clipHandle_t Model)
{
	return ContentsToQ3(PointContentsQ2(P, Model));
}

//==========================================================================
//
//	QClipMap38::TransformedPointContentsQ1
//
//==========================================================================

int QClipMap38::TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return ContentsToQ1(TransformedPointContentsQ2(P, Model, Origin, Angles));
}

//==========================================================================
//
//	QClipMap38::TransformedPointContentsQ2
//
//	Handles offseting and rotation of the end points for moving and
// rotating entities
//
//==========================================================================

int QClipMap38::TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	// subtract origin offset
	vec3_t PointLocal;
	VectorSubtract(P, Origin, PointLocal);

	// rotate start and end into the models frame of reference
	if (Model != BOX_HANDLE && (Angles[0] || Angles[1] || Angles[2]))
	{
		vec3_t forward, right, up;
		AngleVectors(Angles, forward, right, up);

		vec3_t temp;
		VectorCopy(PointLocal, temp);
		PointLocal[0] = DotProduct(temp, forward);
		PointLocal[1] = -DotProduct(temp, right);
		PointLocal[2] = DotProduct(temp, up);
	}

	return PointContentsQ2(PointLocal, Model);
}

//==========================================================================
//
//	QClipMap38::TransformedPointContentsQ3
//
//==========================================================================

int QClipMap38::TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return ContentsToQ3(TransformedPointContentsQ2(P, Model, Origin, Angles));
}

/*
===============================================================================

PVS / PHS

===============================================================================
*/

//==========================================================================
//
//	QClipMap38::DecompressVis
//
//==========================================================================

void QClipMap38::DecompressVis(const byte* in, byte* out)
{
	int row = (numclusters + 7) >> 3;	
	byte* out_p = out;

	if (!in || !numvisibility)
	{
		// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;		
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}
	
		int c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			GLog.DWrite("warning: Vis decompression overrun\n");
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

//==========================================================================
//
//	QClipMap38::ClusterPVS
//
//==========================================================================

byte* QClipMap38::ClusterPVS(int Cluster)
{
	if (Cluster == -1)
	{
		Com_Memset(pvsrow, 0, (numclusters + 7) >> 3);
	}
	else
	{
		DecompressVis(visibility + vis->bitofs[Cluster][BSP38DVIS_PVS], pvsrow);
	}
	return pvsrow;
}

//==========================================================================
//
//	QClipMap38::ClusterPVS
//
//==========================================================================

byte* QClipMap38::ClusterPHS(int Cluster)
{
	if (Cluster == -1)
	{
		Com_Memset(phsrow, 0, (numclusters + 7) >> 3);
	}
	else
	{
		DecompressVis(visibility + vis->bitofs[Cluster][BSP38DVIS_PHS], phsrow);
	}
	return phsrow;
}

/*
===============================================================================

AREAPORTALS

===============================================================================
*/

//==========================================================================
//
//	QClipMap38::FloodArea
//
//==========================================================================

void QClipMap38::FloodArea(carea_t* area, int floodnum)
{
	if (area->floodvalid == floodvalid)
	{
		if (area->floodnum == floodnum)
		{
			return;
		}
		throw QDropException("QClipMap38.FloodArea: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	bsp38_dareaportal_t* p = &areaportals[area->firstareaportal];
	for (int i = 0; i < area->numareaportals; i++, p++)
	{
		if (portalopen[p->portalnum])
		{
			FloodArea(&areas[p->otherarea], floodnum);
		}
	}
}

//==========================================================================
//
//	QClipMap38::FloodAreaConnections
//
//==========================================================================

void QClipMap38::FloodAreaConnections()
{
	// all current floods are now invalid
	floodvalid++;
	int floodnum = 0;

	// area 0 is not used
	for (int i = 1; i < numareas; i++)
	{
		carea_t* area = &areas[i];
		if (area->floodvalid == floodvalid)
		{
			continue;		// already flooded into
		}
		floodnum++;
		FloodArea(area, floodnum);
	}
}

//==========================================================================
//
//	QClipMap38::ClearPortalOpen
//
//==========================================================================

void QClipMap38::ClearPortalOpen()
{
	Com_Memset(portalopen, 0, sizeof(portalopen));
	FloodAreaConnections();
}
