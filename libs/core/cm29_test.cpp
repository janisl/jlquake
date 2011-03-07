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
#include "cm29_local.h"

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
//	QClipMap29::PointLeafnum
//
//==========================================================================

int QClipMap29::PointLeafnum(const vec3_t P) const
{
	int NodeNum = 0;
	while (NodeNum >= 0)
	{
		cnode_t* Node = map_models[0].nodes + NodeNum;
		cplane_t* Plane = Node->plane;
		float d = DotProduct(P, Plane->normal) - Plane->dist;
		if (d > 0)
		{
			NodeNum = Node->children[0];
		}
		else
		{
			NodeNum = Node->children[1];
		}
	}

	return -1 - NodeNum;
}

//==========================================================================
//
//	QClipMap29::BoxLeafnums_r
//
//==========================================================================

void QClipMap29::BoxLeafnums_r(leafList_t* ll, int NodeNum) const
{
	if (NodeNum < 0)
	{
		int LeafNum = -1 - NodeNum;
		const cleaf_t* leaf = leafs + LeafNum;

		if (leaf->contents == BSP29CONTENTS_SOLID)
		{
			return;
		}

		// store the lastLeaf even if the list is overflowed
		if (LeafNum != 0)
		{
			ll->lastLeaf = LeafNum;
		}

		if (ll->count == ll->maxcount)
		{
			return;
		}

		ll->list[ll->count++] = LeafNum;
		return;
	}

	// NODE_MIXED

	const cnode_t* node = map_models[0].nodes + NodeNum;
	const cplane_t* splitplane = node->plane;
	int sides = BOX_ON_PLANE_SIDE(ll->bounds[0], ll->bounds[1], splitplane);

	if (sides == 3 && ll->topnode == -1)
	{
		ll->topnode = node - map_models[0].nodes;
	}

	// recurse down the contacted sides
	if (sides & 1)
	{
		BoxLeafnums_r(ll, node->children[0]);
	}

	if (sides & 2)
	{
		BoxLeafnums_r(ll, node->children[1]);
	}
}

//==========================================================================
//
//	QClipMap29::BoxLeafnums
//
//==========================================================================

int QClipMap29::BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List,
	int ListSize, int* TopNode, int* LastLeaf) const
{
	leafList_t ll;

	VectorCopy(Mins, ll.bounds[0]);
	VectorCopy(Maxs, ll.bounds[1]);
	ll.count = 0;
	ll.list = List;
	ll.maxcount = ListSize;
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
//	QClipMap29::HullPointContents
//
//==========================================================================

int QClipMap29::HullPointContents(const chull_t* Hull, int NodeNum, const vec3_t P) const
{
	while (NodeNum >= 0)
	{
		if (NodeNum < Hull->firstclipnode || NodeNum > Hull->lastclipnode)
		{
			throw QException("SV_HullPointContents: bad node number");
		}
	
		bsp29_dclipnode_t* node = Hull->clipnodes + NodeNum;
		cplane_t* plane = Hull->planes + node->planenum;

		float d;
		if (plane->type < 3)
		{
			d = P[plane->type] - plane->dist;
		}
		else
		{
			d = DotProduct(plane->normal, P) - plane->dist;
		}
		if (d < 0)
		{
			NodeNum = node->children[1];
		}
		else
		{
			NodeNum = node->children[0];
		}
	}

	return NodeNum;
}

//==========================================================================
//
//	QClipMap29::PointContentsQ1
//
//==========================================================================

int QClipMap29::PointContentsQ1(const vec3_t P, clipHandle_t Model)
{
	chull_t* hull = ClipHandleToHull(Model);
	return HullPointContents(hull, hull->firstclipnode, P);
}

//==========================================================================
//
//	QClipMap29::PointContentsQ2
//
//==========================================================================

int QClipMap29::PointContentsQ2(const vec3_t P, clipHandle_t Model)
{
	return ContentsToQ2(PointContentsQ1(P, Model));
}

//==========================================================================
//
//	QClipMap29::PointContentsQ3
//
//==========================================================================

int QClipMap29::PointContentsQ3(const vec3_t P, clipHandle_t Model)
{
	return ContentsToQ3(PointContentsQ1(P, Model));
}

//==========================================================================
//
//	QClipMap29::TransformedPointContentsQ1
//
//	Handles offseting and rotation of the end points for moving and
// rotating entities
//
//==========================================================================

int QClipMap29::TransformedPointContentsQ1(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	// subtract origin offset
	vec3_t p_l;
	VectorSubtract(P, Origin, p_l);

	// rotate start and end into the models frame of reference
	if (Model != BOX_HULL_HANDLE && (Angles[0] || Angles[1] || Angles[2]))
	{
		vec3_t forward, right, up;
		AngleVectors(Angles, forward, right, up);

		vec3_t temp;
		VectorCopy(p_l, temp);
		p_l[0] = DotProduct(temp, forward);
		p_l[1] = -DotProduct(temp, right);
		p_l[2] = DotProduct(temp, up);
	}

	return PointContentsQ1(p_l, Model);
}

//==========================================================================
//
//	QClipMap29::TransformedPointContentsQ2
//
//==========================================================================

int QClipMap29::TransformedPointContentsQ2(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return ContentsToQ2(TransformedPointContentsQ1(P, Model, Origin, Angles));
}

//==========================================================================
//
//	QClipMap29::TransformedPointContentsQ3
//
//==========================================================================

int QClipMap29::TransformedPointContentsQ3(const vec3_t P, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles)
{
	return ContentsToQ3(TransformedPointContentsQ1(P, Model, Origin, Angles));
}

//==========================================================================
//
//	QClipMap29::HeadnodeVisible
//
//	Returns true if any leaf under headnode has a cluster that
// is potentially visible
//
//==========================================================================

bool QClipMap29::HeadnodeVisible(int NodeNum, byte* VisBits)
{
	if (NodeNum < 0)
	{
		int LeafNum = -1 - NodeNum;
		int cluster = LeafNum - 1;
		if (cluster == -1)
		{
			return false;
		}
		if (VisBits[cluster >> 3] & (1 << (cluster & 7)))
		{
			return true;
		}
		return false;
	}

	const cnode_t* Node = &map_models[0].nodes[NodeNum];
	if (HeadnodeVisible(Node->children[0], VisBits))
	{
		return true;
	}
	return HeadnodeVisible(Node->children[1], VisBits);
}

//==========================================================================
//
//	QClipMap29::DecompressVis
//
//==========================================================================

byte* QClipMap29::DecompressVis(byte* in)
{
	static byte		decompressed[BSP29_MAX_MAP_LEAFS / 8];

	if (!in)
	{
		// no vis info
		return mod_novis;
	}

	int row = (numleafs + 7) >> 3;
	byte* out = decompressed;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		int c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

//==========================================================================
//
//	QClipMap29::ClusterPVS
//
//==========================================================================

byte* QClipMap29::ClusterPVS(int Cluster)
{
	if (Cluster < 0)
	{
		return mod_novis;
	}
	return DecompressVis(leafs[Cluster + 1].compressed_vis);
}

//==========================================================================
//
//	QClipMap29::CalcPHS
//
//	Calculates the PHS (Potentially Hearable Set)
//
//==========================================================================

void QClipMap29::CalcPHS()
{
	GLog.Write("Building PHS...\n");

	int num = numleafs;
	int rowwords = (num + 31) >> 5;
	int rowbytes = rowwords * 4;

	byte* pvs = new byte[rowbytes * num];
	byte* scan = pvs;
	int vcount = 0;
	for (int i = 0 ; i < num; i++, scan += rowbytes)
	{
		Com_Memcpy(scan, ClusterPVS(LeafCluster(i)), rowbytes);
		if (i == 0)
		{
			continue;
		}
		for (int j = 0; j < num; j++)
		{
			if (scan[j >> 3] & (1 << (j & 7)))
			{
				vcount++;
			}
		}
	}

	phs = new byte[rowbytes * num];
	int count = 0;
	scan = pvs;
	unsigned* dest = (unsigned*)phs;
	for (int i = 0; i < num; i++, dest += rowwords, scan += rowbytes)
	{
		Com_Memcpy(dest, scan, rowbytes);
		for (int j = 0; j < rowbytes; j++)
		{
			int bitbyte = scan[j];
			if (!bitbyte)
			{
				continue;
			}
			for (int k = 0; k < 8; k++)
			{
				if (!(bitbyte & (1 << k)))
				{
					continue;
				}
				// or this pvs row into the phs
				// +1 because pvs is 1 based
				int index = ((j << 3) + k + 1);
				if (index >= num)
				{
					continue;
				}
				unsigned* src = (unsigned*)pvs + index * rowwords;
				for (int l = 0; l < rowwords; l++)
				{
					dest[l] |= src[l];
				}
			}
		}

		if (i == 0)
		{
			continue;
		}
		for (int j = 0; j < num; j++)
		{
			if (((byte*)dest)[j >> 3] & (1 << (j & 7)))
			{
				count++;
			}
		}
	}

	delete[] pvs;

	GLog.Write("Average leafs visible / hearable / total: %i / %i / %i\n",
		vcount / num, count / num, num);
}

//==========================================================================
//
//	QClipMap29::ClusterPHS
//
//==========================================================================

byte* QClipMap29::ClusterPHS(int Cluster)
{
	return phs + (Cluster + 1) * 4 * ((numleafs + 31) >> 5);
}

//==========================================================================
//
//	QClipMap29::SetAreaPortalState
//
//==========================================================================

void QClipMap29::SetAreaPortalState(int portalnum, qboolean open)
{
}

//==========================================================================
//
//	QClipMap29::AdjustAreaPortalState
//
//==========================================================================

void QClipMap29::AdjustAreaPortalState(int Area1, int Area2, bool Open)
{
}

//==========================================================================
//
//	QClipMap29::AreasConnected
//
//==========================================================================

qboolean QClipMap29::AreasConnected(int Area1, int Area2)
{
	return true;
}

//==========================================================================
//
//	QClipMap29::WriteAreaBits
//
//==========================================================================

int QClipMap29::WriteAreaBits(byte* Buffer, int Area)
{
	return 0;
}

//==========================================================================
//
//	QClipMap29::WritePortalState
//
//==========================================================================

void QClipMap29::WritePortalState(fileHandle_t f)
{
}

//==========================================================================
//
//	QClipMap29::ReadPortalState
//
//==========================================================================

void QClipMap29::ReadPortalState(fileHandle_t f)
{
}
