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
#include "cm46_local.h"

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
//	QClipMap46::PointLeafnum
//
//==========================================================================

int QClipMap46::PointLeafnum(const vec3_t p) const
{
	if (!numNodes)
	{
		// map not loaded
		return 0;
	}
	return PointLeafnum_r(p, 0);
}

//==========================================================================
//
//	QClipMap46::PointLeafnum_r
//
//==========================================================================

int QClipMap46::PointLeafnum_r(const vec3_t P, int Num) const
{
	while (Num >= 0)
	{
		const cNode_t* Node = nodes + Num;
		const cplane_t* Plane = Node->plane;

		float d;
		if (Plane->type < 3)
		{
			d = P[Plane->type] - Plane->dist;
		}
		else
		{
			d = DotProduct(Plane->normal, P) - Plane->dist;
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

/*
======================================================================

LEAF LISTING

======================================================================
*/

//==========================================================================
//
//	QClipMap46::StoreLeafs
//
//==========================================================================

void QClipMap46::StoreLeafs(leafList_t* ll, int NodeNum) const
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
}

//==========================================================================
//
//	QClipMap46::BoxLeafnums_r
//
//	Fills in a list of all the leafs touched
//
//==========================================================================

void QClipMap46::BoxLeafnums_r(leafList_t* ll, int nodenum) const
{
	while (1)
	{
		if (nodenum < 0)
		{
			StoreLeafs(ll, nodenum);
			return;
		}
	
		const cNode_t* node = &nodes[nodenum];
		const cplane_t* plane = node->plane;
		int s = BoxOnPlaneSide(ll->bounds[0], ll->bounds[1], plane);
		if (s == 1)
		{
			nodenum = node->children[0];
		}
		else if (s == 2)
		{
			nodenum = node->children[1];
		}
		else
		{
			// go down both
			if (ll->topnode == -1)
			{
				ll->topnode = nodenum;
			}
			BoxLeafnums_r(ll, node->children[0]);
			nodenum = node->children[1];
		}
	}
}

//==========================================================================
//
//	QClipMap46::BoxLeafnums
//
//==========================================================================

int QClipMap46::BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int *List,
	int ListSize, int *TopNode, int *LastLeaf) const
{
	leafList_t	ll;

	VectorCopy(Mins, ll.bounds[0]);
	VectorCopy(Maxs, ll.bounds[1]);
	ll.count = 0;
	ll.maxcount = ListSize;
	ll.list = List;
	ll.topnode = -1;
	ll.lastLeaf = 0;

	BoxLeafnums_r(&ll, 0);

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
//	QClipMap46::PointContentsQ1
//
//==========================================================================

int QClipMap46::PointContentsQ1(const vec3_t P, clipHandle_t Model)
{
	return ContentsToQ1(PointContentsQ3(P, Model));
}

//==========================================================================
//
//	QClipMap46::PointContentsQ2
//
//==========================================================================

int QClipMap46::PointContentsQ2(const vec3_t P, clipHandle_t Model)
{
	return ContentsToQ2(PointContentsQ3(P, Model));
}

//==========================================================================
//
//	QClipMap46::PointContentsQ3
//
//==========================================================================

int QClipMap46::PointContentsQ3(const vec3_t P, clipHandle_t Model)
{
	if (!numNodes)
	{
		// map not loaded
		return 0;
	}

	cLeaf_t* leaf;
	if (Model)
	{
		cmodel_t* clipm = ClipHandleToModel(Model);
		leaf = &clipm->leaf;
	}
	else
	{
		int leafnum = PointLeafnum_r(P, 0);
		leaf = &leafs[leafnum];
	}

	int contents = 0;
	for (int k = 0; k < leaf->numLeafBrushes; k++)
	{
		int brushnum = leafbrushes[leaf->firstLeafBrush+k];
		cbrush_t* b = &brushes[brushnum];

		// see if the point is in the brush
		int i;
		for (i = 0; i < b->numsides; i++)
		{
			float d = DotProduct(P, b->sides[i].plane->normal);
// FIXME test for Cash
//			if (d >= b->sides[i].plane->dist ) {
			if (d > b->sides[i].plane->dist)
			{
				break;
			}
		}

		if (i == b->numsides)
		{
			contents |= b->contents;
		}
	}

	return contents;
}

//==========================================================================
//
//	QClipMap46::TransformedPointContentsQ1
//
//==========================================================================

int QClipMap46::TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return ContentsToQ1(TransformedPointContentsQ3(P, Model, Origin, Angles));
}

//==========================================================================
//
//	QClipMap46::TransformedPointContentsQ2
//
//==========================================================================

int QClipMap46::TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return ContentsToQ2(TransformedPointContentsQ3(P, Model, Origin, Angles));
}

//==========================================================================
//
//	QClipMap46::TransformedPointContentsQ3
//
//	Handles offseting and rotation of the end points for moving and
// rotating entities
//
//==========================================================================

int QClipMap46::TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	// subtract origin offset
	vec3_t p_l;
	VectorSubtract(P, Origin, p_l);

	// rotate start and end into the models frame of reference
	if (Model != BOX_MODEL_HANDLE && (Angles[0] || Angles[1] || Angles[2]))
	{
		vec3_t forward, right, up;
		AngleVectors(Angles, forward, right, up);

		vec3_t temp;
		VectorCopy(p_l, temp);
		p_l[0] = DotProduct(temp, forward);
		p_l[1] = -DotProduct(temp, right);
		p_l[2] = DotProduct(temp, up);
	}

	return PointContentsQ3(p_l, Model);
}

/*
===============================================================================

PVS

===============================================================================
*/

//==========================================================================
//
//	QClipMap46::ClusterPVS
//
//==========================================================================

byte* QClipMap46::ClusterPVS(int Cluster)
{
	if (Cluster < 0 || Cluster >= numClusters || !vised)
	{
		return visibility;
	}

	return visibility + Cluster * clusterBytes;
}

//==========================================================================
//
//	QClipMap46::ClusterPHS
//
//==========================================================================

byte* QClipMap46::ClusterPHS(int Cluster)
{
	//	FIXME: Could calculate it like QuakeWorld does.
	return ClusterPVS(Cluster);
}
