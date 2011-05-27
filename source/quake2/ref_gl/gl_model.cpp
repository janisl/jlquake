/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// models.c -- model loading and caching

#include "gl_local.h"

void Mod_LoadBrushModel (model_t *mod, void *buffer);
model_t *Mod_LoadModel (model_t *mod, qboolean crash);

byte	mod_novis[BSP38MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	512
static model_t	mod_known[MAX_MOD_KNOWN];
static int		mod_numknown;

int		registration_sequence;

/*
===============
Mod_PointInLeaf
===============
*/
mbrush38_leaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mbrush38_node_t		*node;
	float		d;
	cplane_t	*plane;
	
	if (!model || !model->brush38_nodes)
		ri.Sys_Error (ERR_DROP, "Mod_PointInLeaf: bad model");

	node = model->brush38_nodes;
	while (1)
	{
		if (node->contents != -1)
			return (mbrush38_leaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[BSP38MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->brush38_vis->numclusters+7)>>3;	
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}

/*
==============
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->brush38_vis)
		return mod_novis;
	return Mod_DecompressVis ( (byte *)model->brush38_vis + model->brush38_vis->bitofs[cluster][BSP38DVIS_PVS],
		model);
}


//===============================================================================

/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f (void)
{
	int		i;
	model_t	*mod;
	int		total;

	total = 0;
	ri.Con_Printf (PRINT_ALL,"Loaded models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		ri.Con_Printf (PRINT_ALL, "%8i : %s\n",mod->q2_extradatasize, mod->name);
		total += mod->q2_extradatasize;
	}
	ri.Con_Printf (PRINT_ALL, "Total resident: %i\n", total);
}

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	Com_Memset(mod_novis, 0xff, sizeof(mod_novis));

	mod_numknown = 1;
	mod_known[0].type = MOD_BAD;
}

static model_t* Mod_AllocModel()
{
	//
	// find a free model slot spot
	//
	model_t* mod = &mod_known[1];
	for (int i = 1; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
		{
			mod->index = i;
			return mod;	// free spot
		}
	}
	if (mod_numknown == MAX_MOD_KNOWN)
	{
		ri.Sys_Error(ERR_DROP, "mod_numknown == MAX_MOD_KNOWN");
	}
	mod->index = mod_numknown;
	mod_numknown++;
	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;
	unsigned *buf;
	int		i;
	
	if (!name[0])
		ri.Sys_Error (ERR_DROP, "Mod_ForName: NULL name");

	//
	// search the currently loaded models
	//
	for (i=1, mod=&mod_known[1] ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (!QStr::Cmp(mod->name, name) )
			return mod;
	}
	
	mod = Mod_AllocModel();

	QStr::Cpy(mod->name, name);
	
	//
	// load the file
	//
	int modfilelen = FS_ReadFile(mod->name, (void**)&buf);
	if (!buf)
	{
		if (crash)
			ri.Sys_Error (ERR_DROP, "Mod_NumForName: %s not found", mod->name);
		Com_Memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}
	
	loadmodel = mod;

	//
	// fill it in
	//


	// call the apropriate loader
	
	switch (LittleLong(*(unsigned *)buf))
	{
	case IDMESH2HEADER:
		Mod_LoadMd2Model(mod, buf);
		break;
		
	case IDSPRITE2HEADER:
		Mod_LoadSprite2Model (mod, buf, modfilelen);
		break;
	
	case BSP38_HEADER:
		loadmodel->q2_extradata = Hunk_Begin (0x1000000);
		Mod_LoadBrushModel (mod, buf);
		loadmodel->q2_extradatasize = Hunk_End ();
		break;

	default:
		ri.Sys_Error (ERR_DROP,"Mod_NumForName: unknown fileid for %s", mod->name);
		break;
	}

	FS_FreeFile (buf);

	return mod;
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting (bsp38_lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->brush38_lightdata = NULL;
		return;
	}
	loadmodel->brush38_lightdata = (byte*)Hunk_Alloc ( l->filelen);	
	Com_Memcpy(loadmodel->brush38_lightdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility (bsp38_lump_t *l)
{
	int		i;

	if (!l->filelen)
	{
		loadmodel->brush38_vis = NULL;
		return;
	}
	loadmodel->brush38_vis = (bsp38_dvis_t*)Hunk_Alloc ( l->filelen);	
	Com_Memcpy(loadmodel->brush38_vis, mod_base + l->fileofs, l->filelen);

	loadmodel->brush38_vis->numclusters = LittleLong (loadmodel->brush38_vis->numclusters);
	for (i=0 ; i<loadmodel->brush38_vis->numclusters ; i++)
	{
		loadmodel->brush38_vis->bitofs[i][0] = LittleLong (loadmodel->brush38_vis->bitofs[i][0]);
		loadmodel->brush38_vis->bitofs[i][1] = LittleLong (loadmodel->brush38_vis->bitofs[i][1]);
	}
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes (bsp38_lump_t *l)
{
	bsp38_dvertex_t	*in;
	mbrush38_vertex_t	*out;
	int			i, count;

	in = (bsp38_dvertex_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush38_vertex_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->brush38_vertexes = out;
	loadmodel->brush38_numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength (corner);
}


/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels (bsp38_lump_t *l)
{
	bsp38_dmodel_t	*in;
	mbrush38_model_t	*out;
	int			i, j, count;

	in = (bsp38_dmodel_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush38_model_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->brush38_submodels = out;
	loadmodel->brush38_numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->radius = RadiusFromBounds (out->mins, out->maxs);
		out->headnode = LittleLong (in->headnode);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges (bsp38_lump_t *l)
{
	bsp38_dedge_t *in;
	mbrush38_edge_t *out;
	int 	i, count;

	in = (bsp38_dedge_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush38_edge_t*)Hunk_Alloc ( (count + 1) * sizeof(*out));	

	loadmodel->brush38_edges = out;
	loadmodel->brush38_numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo (bsp38_lump_t *l)
{
	bsp38_texinfo_t *in;
	mbrush38_texinfo_t *out, *step;
	int 	i, j, count;
	char	name[MAX_QPATH];
	int		next;

	in = (bsp38_texinfo_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush38_texinfo_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->brush38_texinfo = out;
	loadmodel->brush38_numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<8 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);

		out->flags = LittleLong (in->flags);
		next = LittleLong (in->nexttexinfo);
		if (next > 0)
			out->next = loadmodel->brush38_texinfo + next;
		else
		    out->next = NULL;
		QStr::Sprintf (name, sizeof(name), "textures/%s.wal", in->texture);

		out->image = R_FindImageFile(name, true, true, GL_REPEAT);
		if (!out->image)
		{
			ri.Con_Printf (PRINT_ALL, "Couldn't load %s\n", name);
			out->image = tr.defaultImage;
		}
	}

	// count animation frames
	for (i=0 ; i<count ; i++)
	{
		out = &loadmodel->brush38_texinfo[i];
		out->numframes = 1;
		for (step = out->next ; step && step != out ; step=step->next)
			out->numframes++;
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents (mbrush38_surface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mbrush38_vertex_t	*v;
	mbrush38_texinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->brush38_surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->brush38_vertexes[loadmodel->brush38_edges[e].v[0]];
		else
			v = &loadmodel->brush38_vertexes[loadmodel->brush38_edges[-e].v[1]];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] + 
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{	
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256 */ )
//			ri.Sys_Error (ERR_DROP, "Bad surface extents");
	}
}


void GL_BuildPolygonFromSurface(mbrush38_surface_t *fa);
void GL_CreateSurfaceLightmap (mbrush38_surface_t *surf);
void GL_EndBuildingLightmaps (void);
void GL_BeginBuildingLightmaps (model_t *m);

/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces (bsp38_lump_t *l)
{
	bsp38_dface_t		*in;
	mbrush38_surface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;
	int			ti;

	in = (bsp38_dface_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush38_surface_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->brush38_surfaces = out;
	loadmodel->brush38_numsurfaces = count;

	currentmodel = loadmodel;

	GL_BeginBuildingLightmaps (loadmodel);

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);		
		out->flags = 0;
		out->polys = NULL;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= BRUSH38_SURF_PLANEBACK;			

		out->plane = loadmodel->brush38_planes + planenum;

		ti = LittleShort (in->texinfo);
		if (ti < 0 || ti >= loadmodel->brush38_numtexinfo)
			ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: bad texinfo number");
		out->texinfo = loadmodel->brush38_texinfo + ti;

		CalcSurfaceExtents (out);
				
	// lighting info

		for (i=0 ; i<BSP38_MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->brush38_lightdata + i;
		
	// set the drawing flags
		
		if (out->texinfo->flags & BSP38SURF_WARP)
		{
			out->flags |= BRUSH38_SURF_DRAWTURB;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface (out);	// cut up polygon for warps
		}

		// create lightmaps and polygons
		if ( !(out->texinfo->flags & (BSP38SURF_SKY|BSP38SURF_TRANS33|BSP38SURF_TRANS66|BSP38SURF_WARP) ) )
			GL_CreateSurfaceLightmap (out);

		if (! (out->texinfo->flags & BSP38SURF_WARP) ) 
			GL_BuildPolygonFromSurface(out);

	}

	GL_EndBuildingLightmaps ();
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mbrush38_node_t *node, mbrush38_node_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes (bsp38_lump_t *l)
{
	int			i, j, count, p;
	bsp38_dnode_t		*in;
	mbrush38_node_t 	*out;

	in = (bsp38_dnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush38_node_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->brush38_nodes = out;
	loadmodel->brush38_numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}
	
		p = LittleLong(in->planenum);
		out->plane = loadmodel->brush38_planes + p;

		out->firstsurface = LittleShort (in->firstface);
		out->numsurfaces = LittleShort (in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->brush38_nodes + p;
			else
				out->children[j] = (mbrush38_node_t *)(loadmodel->brush38_leafs + (-1 - p));
		}
	}
	
	Mod_SetParent (loadmodel->brush38_nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs (bsp38_lump_t *l)
{
	bsp38_dleaf_t 	*in;
	mbrush38_leaf_t 	*out;
	int			i, j, count, p;
//	mbrush38_glpoly_t	*poly;

	in = (bsp38_dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush38_leaf_t*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->brush38_leafs = out;
	loadmodel->brush38_numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstmarksurface = loadmodel->brush38_marksurfaces +
			LittleShort(in->firstleafface);
		out->nummarksurfaces = LittleShort(in->numleaffaces);
		
		// gl underwater warp
#if 0
		if (out->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_THINWATER) )
		{
			for (j=0 ; j<out->nummarksurfaces ; j++)
			{
				out->firstmarksurface[j]->flags |= BRUSH38_SURF_UNDERWATER;
				for (poly = out->firstmarksurface[j]->polys ; poly ; poly=poly->next)
					poly->flags |= BRUSH38_SURF_UNDERWATER;
			}
		}
#endif
	}	
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces (bsp38_lump_t *l)
{	
	int		i, j, count;
	short		*in;
	mbrush38_surface_t **out;
	
	in = (short*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush38_surface_t**)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->brush38_marksurfaces = out;
	loadmodel->brush38_nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleShort(in[i]);
		if (j < 0 ||  j >= loadmodel->brush38_numsurfaces)
			ri.Sys_Error (ERR_DROP, "Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->brush38_surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges (bsp38_lump_t *l)
{	
	int		i, count;
	int		*in, *out;
	
	in = (int*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	if (count < 1 || count >= BSP38MAX_MAP_SURFEDGES)
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: bad surfedges count in %s: %i",
		loadmodel->name, count);

	out = (int*)Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->brush38_surfedges = out;
	loadmodel->brush38_numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (bsp38_lump_t *l)
{
	int			i, j;
	cplane_t	*out;
	bsp38_dplane_t 	*in;
	int			count;
	
	in = (bsp38_dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Sys_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (cplane_t*)Hunk_Alloc ( count*2*sizeof(*out));	
	
	loadmodel->brush38_planes = out;
	loadmodel->brush38_numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
		}
		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);

		SetPlaneSignbits(out);
	}
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel (model_t *mod, void *buffer)
{
	int			i;
	bsp38_dheader_t	*header;
	mbrush38_model_t 	*bm;
	
	loadmodel->type = MOD_BRUSH38;
	if (loadmodel != &mod_known[1])
		ri.Sys_Error (ERR_DROP, "Loaded a brush model after the world");

	header = (bsp38_dheader_t *)buffer;

	i = LittleLong (header->version);
	if (i != BSP38_VERSION)
		ri.Sys_Error (ERR_DROP, "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSP38_VERSION);

// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(bsp38_dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	
	Mod_LoadVertexes (&header->lumps[BSP38LUMP_VERTEXES]);
	Mod_LoadEdges (&header->lumps[BSP38LUMP_EDGES]);
	Mod_LoadSurfedges (&header->lumps[BSP38LUMP_SURFEDGES]);
	Mod_LoadLighting (&header->lumps[BSP38LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[BSP38LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[BSP38LUMP_TEXINFO]);
	Mod_LoadFaces (&header->lumps[BSP38LUMP_FACES]);
	Mod_LoadMarksurfaces (&header->lumps[BSP38LUMP_LEAFFACES]);
	Mod_LoadVisibility (&header->lumps[BSP38LUMP_VISIBILITY]);
	Mod_LoadLeafs (&header->lumps[BSP38LUMP_LEAFS]);
	Mod_LoadNodes (&header->lumps[BSP38LUMP_NODES]);
	Mod_LoadSubmodels (&header->lumps[BSP38LUMP_MODELS]);
	mod->q2_numframes = 2;		// regular and alternate animation
	
//
// set up the submodels
//
	for (i=0 ; i<mod->brush38_numsubmodels ; i++)
	{
		model_t	*starmod;

		bm = &mod->brush38_submodels[i];
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
			ri.Sys_Error (ERR_DROP, "Inline model %i has bad firstnode", i);

		VectorCopy (bm->maxs, starmod->q2_maxs);
		VectorCopy (bm->mins, starmod->q2_mins);
		starmod->q2_radius = bm->radius;
	}
}

//=============================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration (char *model)
{
	char	fullname[MAX_QPATH];
	QCvar	*flushmap;

	registration_sequence++;
	r_oldviewcluster = -1;		// force markleafs

	QStr::Sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	// explicitly free the old map if different
	// this guarantees that mod_known[1] is the world map
	flushmap = Cvar_Get ("flushmap", "0", 0);
	if ( QStr::Cmp(mod_known[1].name, fullname) || flushmap->value)
	{
		Mod_Free (&mod_known[1]);
		//	Clear submodels.
		for (int i = 1; i < mod_numknown; i++)
		{
			if (mod_known[i].name[0] == '*')
			{
				Com_Memset(&mod_known[i], 0, sizeof(mod_known[i]));
			}
		}
	}
	r_worldmodel = Mod_ForName(fullname, true);

	r_viewcluster = -1;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/
qhandle_t R_RegisterModel (char *name)
{
	model_t	*mod;

	mod = Mod_ForName (name, false);
	if (!mod)
	{
		return 0;
	}
	mod->q2_registration_sequence = registration_sequence;

	return mod - mod_known;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration (void)
{
	int		i;
	model_t	*mod;

	for (i=1, mod=&mod_known[1] ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0] || mod->name[0] == '*')
			continue;
		if (mod->q2_registration_sequence != registration_sequence)
		{	// don't need this model
			Mod_Free (mod);
		}
	}
}


//=============================================================================


/*
================
Mod_Free
================
*/
void Mod_Free(model_t* mod)
{
	if (mod->type == MOD_SPRITE2)
	{
		Mod_FreeSprite2Model(mod);
	}
	else if (mod->type == MOD_MESH2)
	{
		Mod_FreeMd2Model(mod);
	}
	else if (mod->type == MOD_BRUSH38)
	{
		Hunk_Free(mod->q2_extradata);
	}
	Com_Memset(mod, 0, sizeof(*mod));
}

/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll (void)
{
	int		i;

	for (i=1; i<mod_numknown ; i++)
	{
		if (mod_known[i].q2_extradatasize && mod_known[i].name[0] != '*')
			Mod_Free (&mod_known[i]);
	}
}

model_t* Mod_GetModel(qhandle_t handle)
{
	if (handle < 1 || handle >= mod_numknown)
	{
		return &mod_known[0];
	}
	return &mod_known[handle];
}
