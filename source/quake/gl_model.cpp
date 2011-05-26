/*
Copyright (C) 1996-1997 Id Software, Inc.

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

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "glquake.h"

static char	loadname[32];	// for hunk tags

static void Mod_LoadBrushModel (model_t *mod, void *buffer);
static model_t *Mod_LoadModel (model_t *mod, qboolean crash);

static byte	mod_novis[BSP29_MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	512
static model_t	mod_known[MAX_MOD_KNOWN];
static int		mod_numknown;

QCvar* gl_subdivide_size;

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	gl_subdivide_size = Cvar_Get("gl_subdivide_size", "128", CVAR_ARCHIVE);
	Com_Memset(mod_novis, 0xff, sizeof(mod_novis));

	//	Reserve 0 for default model.
	mod_numknown = 1;
	mod_known[0].type = MOD_BAD;
}

/*
===============
Mod_Init

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	return mod->q1_cache;
}

/*
===============
Mod_PointInLeaf
===============
*/
mbrush29_leaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mbrush29_node_t		*node;
	float		d;
	cplane_t	*plane;
	
	if (!model || !model->brush29_nodes)
		Sys_Error ("Mod_PointInLeaf: bad model");

	node = model->brush29_nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mbrush29_leaf_t *)node;
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
static byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[BSP29_MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->brush29_numleafs+7)>>3;	
	out = decompressed;

#if 0
	Com_Memcpy(out, in, row);
#else
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
#endif
	
	return decompressed;
}

byte *Mod_LeafPVS (mbrush29_leaf_t *leaf, model_t *model)
{
	if (leaf == model->brush29_leafs)
		return mod_novis;
	return Mod_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	int		i;
	model_t	*mod;
	
	for (i=1, mod=&mod_known[1] ; i<mod_numknown ; i++, mod++)
		if (mod->type != MOD_MESH1)
			mod->q1_needload = true;
}

/*
==================
Mod_FindName

==================
*/
static model_t *Mod_FindName (const char *name)
{
	int		i;
	model_t	*mod;
	
	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");
		
//
// search the currently loaded models
//
	for (i=1, mod=&mod_known[1] ; i<mod_numknown ; i++, mod++)
		if (!QStr::Cmp(mod->name, name) )
			break;
			
	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
			Sys_Error ("mod_numknown == MAX_MOD_KNOWN");
		QStr::Cpy(mod->name, name);
		mod->q1_needload = true;
		mod->index = mod_numknown;
		mod_numknown++;
	}

	return mod;
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
static model_t *Mod_LoadModel (model_t *mod, qboolean crash)
{
	void	*d;
	unsigned *buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

	if (!mod->q1_needload)
	{
		return mod;
	}

//
// because the world is so huge, load it one piece at a time
//
	if (!crash)
	{
	
	}
	
//
// load the file
//
	buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		if (crash)
			Sys_Error ("Mod_NumForName: %s not found", mod->name);
		return NULL;
	}
	
//
// allocate a new model
//
	QStr::FileBase (mod->name, loadname);
	
	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	mod->q1_needload = false;
	
	switch (LittleLong(*(unsigned *)buf))
	{
	case IDPOLYHEADER:
		Mod_LoadMdlModel (mod, buf);
		break;
		
	case IDSPRITE1HEADER:
		Mod_LoadSpriteModel (mod, buf);
		break;
	
	default:
		Mod_LoadBrushModel (mod, buf);
		break;
	}

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
qhandle_t Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;
	
	mod = Mod_FindName (name);
	
	mod = Mod_LoadModel (mod, crash);
	if (!mod)
	{
		return 0;
	}
	return mod - mod_known;
}


/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

static byte	*mod_base;


/*
=================
Mod_LoadTextures
=================
*/
static void Mod_LoadTextures (bsp29_lump_t *l)
{
	int		i, j, pixels, num, max, altmax;
	bsp29_miptex_t	*mt;
	mbrush29_texture_t	*tx, *tx2;
	mbrush29_texture_t	*anims[10];
	mbrush29_texture_t	*altanims[10];
	bsp29_dmiptexlump_t *m;

	if (!l->filelen)
	{
		loadmodel->brush29_textures = NULL;
		return;
	}
	m = (bsp29_dmiptexlump_t *)(mod_base + l->fileofs);
	
	m->nummiptex = LittleLong (m->nummiptex);
	
	loadmodel->brush29_numtextures = m->nummiptex;
	loadmodel->brush29_textures = (mbrush29_texture_t**)Hunk_AllocName (m->nummiptex * sizeof(*loadmodel->brush29_textures) , loadname);

	for (i=0 ; i<m->nummiptex ; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;
		mt = (bsp29_miptex_t *)((byte *)m + m->dataofs[i]);
		mt->width = LittleLong (mt->width);
		mt->height = LittleLong (mt->height);
		for (j=0 ; j<BSP29_MIPLEVELS ; j++)
			mt->offsets[j] = LittleLong (mt->offsets[j]);
		
		if ( (mt->width & 15) || (mt->height & 15) )
			Sys_Error ("Texture %s is not 16 aligned", mt->name);
		pixels = mt->width*mt->height/64*85;
		tx = (mbrush29_texture_t*)Hunk_AllocName (sizeof(mbrush29_texture_t) +pixels, loadname );
		loadmodel->brush29_textures[i] = tx;

		Com_Memcpy(tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;
		for (j=0 ; j<BSP29_MIPLEVELS ; j++)
			tx->offsets[j] = mt->offsets[j] + sizeof(mbrush29_texture_t) - sizeof(bsp29_miptex_t);
		// the pixels immediately follow the structures
		Com_Memcpy( tx+1, mt+1, pixels);
		

		if (!QStr::NCmp(mt->name,"sky",3))	
			R_InitSky (tx);
		else
		{
			byte* pic32 = R_ConvertImage8To32((byte*)(tx + 1), tx->width, tx->height, IMG8MODE_Normal);
			tx->gl_texture = GL_LoadTexture(mt->name, tx->width, tx->height, pic32, true);
			delete[] pic32;
		}
	}

//
// sequence the animations
//
	for (i=0 ; i<m->nummiptex ; i++)
	{
		tx = loadmodel->brush29_textures[i];
		if (!tx || tx->name[0] != '+')
			continue;
		if (tx->anim_next)
			continue;	// allready sequenced

	// find the number of frames in the animation
		Com_Memset(anims, 0, sizeof(anims));
		Com_Memset(altanims, 0, sizeof(altanims));

		max = tx->name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
			Sys_Error ("Bad animating texture %s", tx->name);

		for (j=i+1 ; j<m->nummiptex ; j++)
		{
			tx2 = loadmodel->brush29_textures[j];
			if (!tx2 || tx2->name[0] != '+')
				continue;
			if (QStr::Cmp(tx2->name+2, tx->name+2))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num+1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num+1 > altmax)
					altmax = num+1;
			}
			else
				Sys_Error ("Bad animating texture %s", tx->name);
		}
		
#define	ANIM_CYCLE	2
	// link them all together
		for (j=0 ; j<max ; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				Sys_Error ("Missing frame %i of %s",j, tx->name);
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = anims[ (j+1)%max ];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j=0 ; j<altmax ; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				Sys_Error ("Missing frame %i of %s",j, tx->name);
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = altanims[ (j+1)%altmax ];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}
}

/*
=================
Mod_LoadLighting
=================
*/
static void Mod_LoadLighting (bsp29_lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->brush29_lightdata = NULL;
		return;
	}
	loadmodel->brush29_lightdata = (byte*)Hunk_AllocName ( l->filelen, loadname);	
	Com_Memcpy(loadmodel->brush29_lightdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVisibility
=================
*/
static void Mod_LoadVisibility (bsp29_lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->brush29_visdata = NULL;
		return;
	}
	loadmodel->brush29_visdata = (byte*)Hunk_AllocName ( l->filelen, loadname);	
	Com_Memcpy(loadmodel->brush29_visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
static void Mod_LoadEntities (bsp29_lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->brush29_entities = NULL;
		return;
	}
	loadmodel->brush29_entities = (char*)Hunk_AllocName ( l->filelen, loadname);	
	Com_Memcpy(loadmodel->brush29_entities, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVertexes
=================
*/
static void Mod_LoadVertexes (bsp29_lump_t *l)
{
	bsp29_dvertex_t	*in;
	mbrush29_vertex_t	*out;
	int			i, count;

	in = (bsp29_dvertex_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush29_vertex_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_vertexes = out;
	loadmodel->brush29_numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

/*
=================
Mod_LoadSubmodels
=================
*/
static void Mod_LoadSubmodels (bsp29_lump_t *l)
{
	bsp29_dmodel_q1_t	*in;
	bsp29_dmodel_q1_t	*out;
	int			i, j, count;

	in = (bsp29_dmodel_q1_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (bsp29_dmodel_q1_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_submodels_q1 = out;
	loadmodel->brush29_numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		for (j=0 ; j<BSP29_MAX_MAP_HULLS_Q1 ; j++)
			out->headnode[j] = LittleLong (in->headnode[j]);
		out->visleafs = LittleLong (in->visleafs);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
static void Mod_LoadEdges (bsp29_lump_t *l)
{
	bsp29_dedge_t *in;
	mbrush29_edge_t *out;
	int 	i, count;

	in = (bsp29_dedge_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush29_edge_t*)Hunk_AllocName ( (count + 1) * sizeof(*out), loadname);	

	loadmodel->brush29_edges = out;
	loadmodel->brush29_numedges = count;

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
static void Mod_LoadTexinfo (bsp29_lump_t *l)
{
	bsp29_texinfo_t *in;
	mbrush29_texinfo_t *out;
	int 	i, j, count;
	int		miptex;
	float	len1, len2;

	in = (bsp29_texinfo_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush29_texinfo_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_texinfo = out;
	loadmodel->brush29_numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<8 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
		len1 = VectorLength(out->vecs[0]);
		len2 = VectorLength(out->vecs[1]);
		len1 = (len1 + len2)/2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;
#if 0
		if (len1 + len2 < 0.001)
			out->mipadjust = 1;		// don't crash
		else
			out->mipadjust = 1 / floor( (len1+len2)/2 + 0.1 );
#endif

		miptex = LittleLong (in->miptex);
		out->flags = LittleLong (in->flags);
	
		if (!loadmodel->brush29_textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->brush29_numtextures)
				Sys_Error ("miptex >= loadmodel->numtextures");
			out->texture = loadmodel->brush29_textures[miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
static void CalcSurfaceExtents (mbrush29_surface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mbrush29_vertex_t	*v;
	mbrush29_texinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->brush29_surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->brush29_vertexes[loadmodel->brush29_edges[e].v[0]];
		else
			v = &loadmodel->brush29_vertexes[loadmodel->brush29_edges[-e].v[1]];
		
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
		if ( !(tex->flags & BSP29TEX_SPECIAL) && s->extents[i] > 512 /* 256 */ )
			Sys_Error ("Bad surface extents");
	}
}


/*
=================
Mod_LoadFaces
=================
*/
static void Mod_LoadFaces (bsp29_lump_t *l)
{
	bsp29_dface_t		*in;
	mbrush29_surface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;

	in = (bsp29_dface_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush29_surface_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_surfaces = out;
	loadmodel->brush29_numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);		
		out->flags = 0;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= BRUSH29_SURF_PLANEBACK;			

		out->plane = loadmodel->brush29_planes + planenum;

		out->texinfo = loadmodel->brush29_texinfo + LittleShort (in->texinfo);

		CalcSurfaceExtents (out);
				
	// lighting info

		for (i=0 ; i<BSP29_MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->brush29_lightdata + i;
		
	// set the drawing flags flag
		
		if (!QStr::NCmp(out->texinfo->texture->name,"sky",3))	// sky
		{
			out->flags |= (BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTILED);
			GL_SubdivideSurface (out);	// cut up polygon for warps
			continue;
		}
		
		if (!QStr::NCmp(out->texinfo->texture->name,"*",1))		// turbulent
		{
			out->flags |= (BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_DRAWTILED);
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface (out);	// cut up polygon for warps
			continue;
		}

	}
}


/*
=================
Mod_SetParent
=================
*/
static void Mod_SetParent (mbrush29_node_t *node, mbrush29_node_t *parent)
{
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
static void Mod_LoadNodes (bsp29_lump_t *l)
{
	int			i, j, count, p;
	bsp29_dnode_t		*in;
	mbrush29_node_t 	*out;

	in = (bsp29_dnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush29_node_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_nodes = out;
	loadmodel->brush29_numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}
	
		p = LittleLong(in->planenum);
		out->plane = loadmodel->brush29_planes + p;

		out->firstsurface = LittleShort (in->firstface);
		out->numsurfaces = LittleShort (in->numfaces);
		
		for (j=0 ; j<2 ; j++)
		{
			p = LittleShort (in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->brush29_nodes + p;
			else
				out->children[j] = (mbrush29_node_t *)(loadmodel->brush29_leafs + (-1 - p));
		}
	}
	
	Mod_SetParent (loadmodel->brush29_nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
static void Mod_LoadLeafs (bsp29_lump_t *l)
{
	bsp29_dleaf_t 	*in;
	mbrush29_leaf_t 	*out;
	int			i, j, count, p;

	in = (bsp29_dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush29_leaf_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_leafs = out;
	loadmodel->brush29_numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->brush29_marksurfaces +
			(quint16)LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);
		
		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->brush29_visdata + p;
		
		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];

		// gl underwater warp
		if (out->contents != BSP29CONTENTS_EMPTY)
		{
			for (j=0 ; j<out->nummarksurfaces ; j++)
				out->firstmarksurface[j]->flags |= BRUSH29_SURF_UNDERWATER;
		}
	}	
}

/*
=================
Mod_LoadClipnodes
=================
*/
static void Mod_LoadClipnodes (bsp29_lump_t *l)
{
	bsp29_dclipnode_t *in, *out;
	int			i, count;
	mbrush29_hull_t		*hull;

	in = (bsp29_dclipnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (bsp29_dclipnode_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_clipnodes = out;
	loadmodel->brush29_numclipnodes = count;

	hull = &loadmodel->brush29_hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->brush29_planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->brush29_hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->brush29_planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
	}
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
static void Mod_MakeHull0 (void)
{
	mbrush29_node_t		*in, *child;
	bsp29_dclipnode_t *out;
	int			i, j, count;
	mbrush29_hull_t		*hull;
	
	hull = &loadmodel->brush29_hulls[0];	
	
	in = loadmodel->brush29_nodes;
	count = loadmodel->brush29_numnodes;
	out = (bsp29_dclipnode_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->brush29_planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->brush29_planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->brush29_nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
static void Mod_LoadMarksurfaces (bsp29_lump_t *l)
{	
	int		i, j, count;
	short		*in;
	mbrush29_surface_t **out;
	
	in = (short*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush29_surface_t **)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_marksurfaces = out;
	loadmodel->brush29_nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = (quint16)LittleShort(in[i]);
		if (j >= loadmodel->brush29_numsurfaces)
			Sys_Error ("Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->brush29_surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
static void Mod_LoadSurfedges (bsp29_lump_t *l)
{	
	int		i, count;
	int		*in, *out;
	
	in = (int*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (int*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->brush29_surfedges = out;
	loadmodel->brush29_numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
static void Mod_LoadPlanes (bsp29_lump_t *l)
{
	int			i, j;
	cplane_t	*out;
	bsp29_dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (bsp29_dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (cplane_t*)Hunk_AllocName ( count*2*sizeof(*out), loadname);	
	
	loadmodel->brush29_planes = out;
	loadmodel->brush29_numplanes = count;

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
RadiusFromBounds
=================
*/
static float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength(corner);
}

/*
=================
Mod_LoadBrushModel
=================
*/
static void Mod_LoadBrushModel (model_t *mod, void *buffer)
{
	int			i, j;
	bsp29_dheader_t	*header;
	bsp29_dmodel_q1_t 	*bm;
	
	loadmodel->type = MOD_BRUSH29;
	
	header = (bsp29_dheader_t *)buffer;

	i = LittleLong (header->version);
	if (i != BSP29_VERSION)
		Sys_Error ("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSP29_VERSION);

// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(bsp29_dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	
	Mod_LoadVertexes (&header->lumps[BSP29LUMP_VERTEXES]);
	Mod_LoadEdges (&header->lumps[BSP29LUMP_EDGES]);
	Mod_LoadSurfedges (&header->lumps[BSP29LUMP_SURFEDGES]);
	Mod_LoadTextures (&header->lumps[BSP29LUMP_TEXTURES]);
	Mod_LoadLighting (&header->lumps[BSP29LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[BSP29LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[BSP29LUMP_TEXINFO]);
	Mod_LoadFaces (&header->lumps[BSP29LUMP_FACES]);
	Mod_LoadMarksurfaces (&header->lumps[BSP29LUMP_MARKSURFACES]);
	Mod_LoadVisibility (&header->lumps[BSP29LUMP_VISIBILITY]);
	Mod_LoadLeafs (&header->lumps[BSP29LUMP_LEAFS]);
	Mod_LoadNodes (&header->lumps[BSP29LUMP_NODES]);
	Mod_LoadClipnodes (&header->lumps[BSP29LUMP_CLIPNODES]);
	Mod_LoadEntities (&header->lumps[BSP29LUMP_ENTITIES]);
	Mod_LoadSubmodels (&header->lumps[BSP29LUMP_MODELS]);

	Mod_MakeHull0 ();
	
	mod->q1_numframes = 2;		// regular and alternate animation
	
//
// set up the submodels (FIXME: this is confusing)
//
	for (i=0 ; i<mod->brush29_numsubmodels ; i++)
	{
		bm = &mod->brush29_submodels_q1[i];

		mod->brush29_hulls[0].firstclipnode = bm->headnode[0];
		for (j=1 ; j<BSP29_MAX_MAP_HULLS_Q1; j++)
		{
			mod->brush29_hulls[j].firstclipnode = bm->headnode[j];
			mod->brush29_hulls[j].lastclipnode = mod->brush29_numclipnodes-1;
		}
		
		mod->brush29_firstmodelsurface = bm->firstface;
		mod->brush29_nummodelsurfaces = bm->numfaces;
		
		VectorCopy (bm->maxs, mod->q1_maxs);
		VectorCopy (bm->mins, mod->q1_mins);

		mod->q1_radius = RadiusFromBounds (mod->q1_mins, mod->q1_maxs);

		mod->brush29_numleafs = bm->visleafs;

		if (i < mod->brush29_numsubmodels-1)
		{	// duplicate the basic information
			char	name[10];

			sprintf (name, "*%i", i+1);
			loadmodel = Mod_FindName (name);
			*loadmodel = *mod;
			QStr::Cpy(loadmodel->name, name);
			mod = loadmodel;
		}
	}
}

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	int		i;
	model_t	*mod;

	Con_Printf ("Cached models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		Con_Printf ("%8p : %s\n",mod->q1_cache, mod->name);
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

int Mod_GetNumFrames(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->q1_numframes;
}

int Mod_GetFlags(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->q1_flags;
}

void Mod_PrintFrameName (qhandle_t m, int frame)
{
	mesh1hdr_t 			*hdr;
	mmesh1framedesc_t	*pframedesc;

	hdr = (mesh1hdr_t *)Mod_Extradata (Mod_GetModel(m));
	if (!hdr)
		return;
	pframedesc = &hdr->frames[frame];
	
	Con_Printf ("frame %i: %s\n", frame, pframedesc->name);
}

bool Mod_IsAliasModel(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->type == MOD_MESH1;
}

const char* Mod_GetName(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->name;
}

int Mod_GetSyncType(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->q1_synctype;
}
