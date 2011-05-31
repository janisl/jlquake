//**************************************************************************
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

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte*			mod_base;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Mod_LoadLighting
//
//==========================================================================

static void Mod_LoadLighting(bsp38_lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->brush38_lightdata = NULL;
		return;
	}
	loadmodel->brush38_lightdata = new byte[l->filelen];
	Com_Memcpy(loadmodel->brush38_lightdata, mod_base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	Mod_LoadVisibility
//
//==========================================================================

static void Mod_LoadVisibility(bsp38_lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->brush38_vis = NULL;
		return;
	}
	loadmodel->brush38_vis = (bsp38_dvis_t*)Mem_Alloc(l->filelen);	
	Com_Memcpy(loadmodel->brush38_vis, mod_base + l->fileofs, l->filelen);

	loadmodel->brush38_vis->numclusters = LittleLong(loadmodel->brush38_vis->numclusters);
	for (int i = 0; i < loadmodel->brush38_vis->numclusters; i++)
	{
		loadmodel->brush38_vis->bitofs[i][0] = LittleLong(loadmodel->brush38_vis->bitofs[i][0]);
		loadmodel->brush38_vis->bitofs[i][1] = LittleLong(loadmodel->brush38_vis->bitofs[i][1]);
	}
}

//==========================================================================
//
//	Mod_LoadVertexes
//
//==========================================================================

static void Mod_LoadVertexes(bsp38_lump_t* l)
{
	bsp38_dvertex_t* in = (bsp38_dvertex_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush38_vertex_t* out = new mbrush38_vertex_t[count];

	loadmodel->brush38_vertexes = out;
	loadmodel->brush38_numvertexes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

//==========================================================================
//
//	Mod_LoadEdges
//
//==========================================================================

static void Mod_LoadEdges(bsp38_lump_t* l)
{
	bsp38_dedge_t* in = (bsp38_dedge_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	//JL What's the extra edge?
	mbrush38_edge_t* out = new mbrush38_edge_t[count + 1];
	Com_Memset(out, 0, sizeof (mbrush38_edge_t) * (count + 1));

	loadmodel->brush38_edges = out;
	loadmodel->brush38_numedges = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

//==========================================================================
//
//	Mod_LoadTexinfo
//
//==========================================================================

static void Mod_LoadTexinfo(bsp38_lump_t* l)
{
	bsp38_texinfo_t* in = (bsp38_texinfo_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush38_texinfo_t* out = new mbrush38_texinfo_t[count];

	loadmodel->brush38_texinfo = out;
	loadmodel->brush38_numtexinfo = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 8; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
		}

		out->flags = LittleLong(in->flags);
		int next = LittleLong(in->nexttexinfo);
		if (next > 0)
		{
			out->next = loadmodel->brush38_texinfo + next;
		}
		else
		{
		    out->next = NULL;
		}
		char name[MAX_QPATH];
		QStr::Sprintf(name, sizeof(name), "textures/%s.wal", in->texture);

		out->image = R_FindImageFile(name, true, true, GL_REPEAT);
		if (!out->image)
		{
			GLog.Write("Couldn't load %s\n", name);
			out->image = tr.defaultImage;
		}
	}

	// count animation frames
	for (int i = 0; i < count; i++)
	{
		out = &loadmodel->brush38_texinfo[i];
		out->numframes = 1;
		for (mbrush38_texinfo_t* step = out->next; step && step != out ; step=step->next)
		{
			out->numframes++;
		}
	}
}

//==========================================================================
//
//	CalcSurfaceExtents
//
//	Fills in s->texturemins[] and s->extents[]
//
//==========================================================================

static void CalcSurfaceExtents(mbrush38_surface_t* s)
{
	float mins[2];
	mins[0] = mins[1] = 999999;
	float maxs[2];
	maxs[0] = maxs[1] = -99999;

	mbrush38_texinfo_t* tex = s->texinfo;
	
	for (int i = 0; i < s->numedges; i++)
	{
		int e = loadmodel->brush38_surfedges[s->firstedge + i];
		mbrush38_vertex_t* v;
		if (e >= 0)
		{
			v = &loadmodel->brush38_vertexes[loadmodel->brush38_edges[e].v[0]];
		}
		else
		{
			v = &loadmodel->brush38_vertexes[loadmodel->brush38_edges[-e].v[1]];
		}
		
		for (int j = 0; j < 2; j++)
		{
			float val = v->position[0] * tex->vecs[j][0] + 
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
			{
				mins[j] = val;
			}
			if (val > maxs[j])
			{
				maxs[j] = val;
			}
		}
	}

	int bmins[2], bmaxs[2];
	for (int i = 0; i < 2; i++)
	{	
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}

//==========================================================================
//
//	Mod_LoadFaces
//
//==========================================================================

static void Mod_LoadFaces(bsp38_lump_t* l)
{
	bsp38_dface_t* in = (bsp38_dface_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush38_surface_t* out = new mbrush38_surface_t[count];
	Com_Memset(out, 0, sizeof(mbrush38_surface_t) * count);

	loadmodel->brush38_surfaces = out;
	loadmodel->brush38_numsurfaces = count;

	currentmodel = loadmodel;

	GL_BeginBuildingLightmaps(loadmodel);

	for (int surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);		
		out->flags = 0;
		out->polys = NULL;

		int planenum = LittleShort(in->planenum);
		int side = LittleShort(in->side);
		if (side)
		{
			out->flags |= BRUSH38_SURF_PLANEBACK;
		}

		out->plane = loadmodel->brush38_planes + planenum;

		int ti = LittleShort(in->texinfo);
		if (ti < 0 || ti >= loadmodel->brush38_numtexinfo)
		{
			throw QDropException("MOD_LoadBmodel: bad texinfo number");
		}
		out->texinfo = loadmodel->brush38_texinfo + ti;

		CalcSurfaceExtents(out);
				
		// lighting info

		for (int i = 0; i < BSP38_MAXLIGHTMAPS; i++)
		{
			out->styles[i] = in->styles[i];
		}
		int lightofs = LittleLong(in->lightofs);
		if (lightofs == -1)
		{
			out->samples = NULL;
		}
		else
		{
			out->samples = loadmodel->brush38_lightdata + lightofs;
		}

		// set the drawing flags

		if (out->texinfo->flags & BSP38SURF_WARP)
		{
			out->flags |= BRUSH38_SURF_DRAWTURB;
			for (int i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface(out);	// cut up polygon for warps
		}

		// create lightmaps and polygons
		if (!(out->texinfo->flags & (BSP38SURF_SKY | BSP38SURF_TRANS33 | BSP38SURF_TRANS66 | BSP38SURF_WARP)))
		{
			GL_CreateSurfaceLightmap(out);
		}

		if (!(out->texinfo->flags & BSP38SURF_WARP))
		{
			GL_BuildPolygonFromSurface(out);
		}
	}

	GL_EndBuildingLightmaps();
}

//==========================================================================
//
//	Mod_SetParent
//
//==========================================================================

static void Mod_SetParent(mbrush38_node_t* node, mbrush38_node_t* parent)
{
	node->parent = parent;
	if (node->contents != -1)
	{
		return;
	}
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

//==========================================================================
//
//	Mod_LoadNodes
//
//==========================================================================

static void Mod_LoadNodes(bsp38_lump_t* l)
{
	bsp38_dnode_t* in = (bsp38_dnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush38_node_t* out = new mbrush38_node_t[count];
	Com_Memset(out, 0, sizeof(mbrush38_node_t) * count);

	loadmodel->brush38_nodes = out;
	loadmodel->brush38_numnodes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3+j] = LittleShort(in->maxs[j]);
		}
	
		int p = LittleLong(in->planenum);
		out->plane = loadmodel->brush38_planes + p;

		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (int j = 0; j < 2; j++)
		{
			p = LittleLong(in->children[j]);
			if (p >= 0)
			{
				out->children[j] = loadmodel->brush38_nodes + p;
			}
			else
			{
				out->children[j] = (mbrush38_node_t*)(loadmodel->brush38_leafs + (-1 - p));
			}
		}
	}
	
	Mod_SetParent (loadmodel->brush38_nodes, NULL);	// sets nodes and leafs
}

//==========================================================================
//
//	Mod_LoadLeafs
//
//==========================================================================

static void Mod_LoadLeafs(bsp38_lump_t* l)
{
	bsp38_dleaf_t* in = (bsp38_dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush38_leaf_t* out = new mbrush38_leaf_t[count];
	Com_Memset(out, 0, sizeof(mbrush38_leaf_t) * count);

	loadmodel->brush38_leafs = out;
	loadmodel->brush38_numleafs = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		int p = LittleLong(in->contents);
		out->contents = p;

		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstmarksurface = loadmodel->brush38_marksurfaces +
			LittleShort(in->firstleafface);
		out->nummarksurfaces = LittleShort(in->numleaffaces);
	}
}

//==========================================================================
//
//	Mod_LoadMarksurfaces
//
//==========================================================================

static void Mod_LoadMarksurfaces(bsp38_lump_t* l)
{
	short* in = (short*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush38_surface_t** out = new mbrush38_surface_t*[count];

	loadmodel->brush38_marksurfaces = out;
	loadmodel->brush38_nummarksurfaces = count;

	for (int i = 0; i < count; i++)
	{
		int j = LittleShort(in[i]);
		if (j < 0 ||  j >= loadmodel->brush38_numsurfaces)
		{
			throw QDropException("Mod_ParseMarksurfaces: bad surface number");
		}
		out[i] = loadmodel->brush38_surfaces + j;
	}
}

//==========================================================================
//
//	Mod_LoadSurfedges
//
//==========================================================================

static void Mod_LoadSurfedges(bsp38_lump_t* l)
{
	int* in = (int*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	if (count < 1 || count >= BSP38MAX_MAP_SURFEDGES)
	{
		throw QDropException(va("MOD_LoadBmodel: bad surfedges count in %s: %i",
			loadmodel->name, count));
	}

	int* out = new int[count];

	loadmodel->brush38_surfedges = out;
	loadmodel->brush38_numsurfedges = count;

	for (int i = 0; i < count; i++)
	{
		out[i] = LittleLong(in[i]);
	}
}

//==========================================================================
//
//	Mod_LoadPlanes
//
//==========================================================================

static void Mod_LoadPlanes(bsp38_lump_t* l)
{
	bsp38_dplane_t* in = (bsp38_dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	//JL Why 2 times more?
	cplane_t* out = new cplane_t[count * 2];
	Com_Memset(out, 0, sizeof(cplane_t) * count * 2);
	
	loadmodel->brush38_planes = out;
	loadmodel->brush38_numplanes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
		}
		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);

		SetPlaneSignbits(out);
	}
}

//==========================================================================
//
//	Mod_LoadSubmodels
//
//==========================================================================

static void Mod_LoadSubmodels(bsp38_lump_t* l)
{
	bsp38_dmodel_t* in = (bsp38_dmodel_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QDropException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush38_model_t* out = new mbrush38_model_t[count];

	loadmodel->brush38_submodels = out;
	loadmodel->brush38_numsubmodels = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		out->radius = RadiusFromBounds(out->mins, out->maxs);
		out->headnode = LittleLong(in->headnode);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
		out->visleafs = 0;
	}
}

//==========================================================================
//
//	Mod_FreeBsp38
//
//==========================================================================

void Mod_LoadBrush38Model(model_t* mod, void* buffer)
{
	loadmodel->type = MOD_BRUSH38;

	bsp38_dheader_t* header = (bsp38_dheader_t*)buffer;

	int version = LittleLong(header->version);
	if (version != BSP38_VERSION)
	{
		throw QDropException(va("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, version, BSP38_VERSION));
	}

	// swap all the lumps
	mod_base = (byte*)header;

	for (int i = 0; i < (int)sizeof(bsp38_dheader_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	// load into heap

	Mod_LoadVertexes(&header->lumps[BSP38LUMP_VERTEXES]);
	Mod_LoadEdges(&header->lumps[BSP38LUMP_EDGES]);
	Mod_LoadSurfedges(&header->lumps[BSP38LUMP_SURFEDGES]);
	Mod_LoadLighting(&header->lumps[BSP38LUMP_LIGHTING]);
	Mod_LoadPlanes(&header->lumps[BSP38LUMP_PLANES]);
	Mod_LoadTexinfo(&header->lumps[BSP38LUMP_TEXINFO]);
	Mod_LoadFaces(&header->lumps[BSP38LUMP_FACES]);
	Mod_LoadMarksurfaces(&header->lumps[BSP38LUMP_LEAFFACES]);
	Mod_LoadVisibility(&header->lumps[BSP38LUMP_VISIBILITY]);
	Mod_LoadLeafs(&header->lumps[BSP38LUMP_LEAFS]);
	Mod_LoadNodes(&header->lumps[BSP38LUMP_NODES]);
	Mod_LoadSubmodels(&header->lumps[BSP38LUMP_MODELS]);
	mod->q2_numframes = 2;		// regular and alternate animation

	//
	// set up the submodels
	//
	for (int i = 0; i < mod->brush38_numsubmodels; i++)
	{
		model_t* starmod;

		mbrush38_model_t* bm = &mod->brush38_submodels[i];
		if (i == 0)
		{
			starmod = loadmodel;
		}
		else
		{
			starmod = Mod_AllocModel();

			*starmod = *loadmodel;
			QStr::Sprintf(starmod->name, sizeof(starmod->name), "*%d", i);
	
			starmod->brush38_numleafs = bm->visleafs;
		}

		starmod->brush38_firstmodelsurface = bm->firstface;
		starmod->brush38_nummodelsurfaces = bm->numfaces;
		starmod->brush38_firstnode = bm->headnode;
		if (starmod->brush38_firstnode >= loadmodel->brush38_numnodes)
		{
			throw QDropException(va("Inline model %i has bad firstnode", i));
		}

		VectorCopy(bm->maxs, starmod->q2_maxs);
		VectorCopy(bm->mins, starmod->q2_mins);
		starmod->q2_radius = bm->radius;
	}
}

//==========================================================================
//
//	Mod_FreeBsp38
//
//==========================================================================

void Mod_FreeBsp38(model_t* mod)
{
	if (mod->name[0] == '*')
	{
		return;
	}

	if (mod->brush38_lightdata)
	{
		delete[] mod->brush38_lightdata;
	}
	if (mod->brush38_vis)
	{
		Mem_Free(mod->brush38_vis);
	}
	delete[] mod->brush38_vertexes;
	delete[] mod->brush38_edges;
	delete[] mod->brush38_texinfo;
	delete[] mod->brush38_surfaces;
	delete[] mod->brush38_nodes;
	delete[] mod->brush38_leafs;
	delete[] mod->brush38_marksurfaces;
	delete[] mod->brush38_surfedges;
	delete[] mod->brush38_planes;
	delete[] mod->brush38_submodels;
}
