// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "glquake.h"

model_t	*loadmodel;
static char	loadname[32];	// for hunk tags

static void Mod_LoadSpriteModel (model_t *mod, void *buffer);
static void Mod_LoadBrushModel (model_t *mod, void *buffer);
static void Mod_LoadAliasModel (model_t *mod, void *buffer);
static void Mod_LoadAliasModelNew (model_t *mod, void *buffer);

static model_t *Mod_LoadModel (model_t *mod, qboolean crash);

static byte	mod_novis[BSP29_MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	1500
static model_t	mod_known[MAX_MOD_KNOWN];
static int		mod_numknown;

static vec3_t	mins,maxs;

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
	void	*r;
	
	r = Cache_Check (&mod->cache);
	if (r)
		return r;

	Mod_LoadModel (mod, true);
	
	if (!mod->cache.data)
		Sys_Error ("Mod_Extradata: caching failed");
	return mod->cache.data;
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
	
	if (!model || !model->nodes)
		Sys_Error ("Mod_PointInLeaf: bad model");

	node = model->nodes;
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

	row = (model->numleafs+7)>>3;	
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
	if (leaf == model->leafs)
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
			mod->needload = true;
}

/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName (char *name)
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
		mod->needload = true;
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

	if (!mod->needload)
	{
		if (mod->type == MOD_MESH1)
		{
			d = Cache_Check (&mod->cache);
			if (d)
				return mod;
		}
		else
			return mod;		// not cached at all
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
	mod->needload = false;
	
	switch (LittleLong(*(unsigned *)buf))
	{
	case RAPOLYHEADER:
		Mod_LoadAliasModelNew (mod, buf);
		break;
	case IDPOLYHEADER:
		Mod_LoadAliasModel (mod, buf);
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
	
	mod = Mod_FindName(name);
	
	mod = Mod_LoadModel(mod, crash);
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
		loadmodel->textures = NULL;
		return;
	}
	m = (bsp29_dmiptexlump_t *)(mod_base + l->fileofs);
	
	m->nummiptex = LittleLong (m->nummiptex);
	
	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = (mbrush29_texture_t**)Hunk_AllocName (m->nummiptex * sizeof(*loadmodel->textures) , loadname);

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
		loadmodel->textures[i] = tx;

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
			byte* pic32 = R_ConvertImage8To32((byte *)(tx+1), tx->width, tx->height, IMG8MODE_Normal);
			tx->gl_texture = GL_LoadTexture(mt->name, tx->width, tx->height, pic32, true);
			delete[] pic32;
		}
	}

//
// sequence the animations
//
	for (i=0 ; i<m->nummiptex ; i++)
	{
		tx = loadmodel->textures[i];
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
			tx2 = loadmodel->textures[j];
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
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = (byte*)Hunk_AllocName ( l->filelen, loadname);	
	Com_Memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
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
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = (byte*)Hunk_AllocName ( l->filelen, loadname);	
	Com_Memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
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
		loadmodel->entities = NULL;
		return;
	}
	loadmodel->entities = (char*)Hunk_AllocName ( l->filelen, loadname);	
	Com_Memcpy(loadmodel->entities, mod_base + l->fileofs, l->filelen);
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

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

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
	bsp29_dmodel_h2_t	*in;
	bsp29_dmodel_h2_t	*out;
	int			i, j, count;

	in = (bsp29_dmodel_h2_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (bsp29_dmodel_h2_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		for (j=0 ; j<BSP29_MAX_MAP_HULLS_H2 ; j++)
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

	loadmodel->edges = out;
	loadmodel->numedges = count;

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

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

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
	
		if (!loadmodel->textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->numtextures)
				Sys_Error ("miptex >= loadmodel->numtextures");
			out->texture = loadmodel->textures[miptex];
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
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		
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

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);		
		out->flags = 0;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= BRUSH29_SURF_PLANEBACK;			

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + LittleShort (in->texinfo);

		CalcSurfaceExtents (out);
				
	// lighting info

		for (i=0 ; i<BSP29_MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;
		
	// set the drawing flags flag
		
		if (!QStr::NCmp(out->texinfo->texture->name,"sky",3))	// sky
		{
			out->flags |= (BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTILED);
			GL_SubdivideSurface (out);	// cut up polygon for warps
			continue;
		}
		
		if (out->texinfo->texture->name[0]=='*')		// turbulent
		{
			out->flags |= (BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_DRAWTILED);
		
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface (out);	// cut up polygon for warps

			if ((!QStr::NICmp(out->texinfo->texture->name,"*rtex078",8)) ||
				(!QStr::NICmp(out->texinfo->texture->name,"*lowlight",9)))
				out->flags |= BRUSH29_SURF_TRANSLUCENT;

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

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}
	
		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort (in->firstface);
		out->numsurfaces = LittleShort (in->numfaces);
		
		for (j=0 ; j<2 ; j++)
		{
			p = LittleShort (in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mbrush29_node_t *)(loadmodel->leafs + (-1 - p));
		}
	}
	
	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
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
	char s[80];
	qboolean isnotmap = true;

	in = (bsp29_dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mbrush29_leaf_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->leafs = out;
	loadmodel->numleafs = count;
	sprintf(s, "maps/%s.bsp", Info_ValueForKey(cl.serverinfo,"map"));
	if (!QStr::Cmp(s, loadmodel->name))
		isnotmap = false;
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);
		
		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		
		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];

		// gl underwater warp
		if (out->contents != BSP29CONTENTS_EMPTY)
		{
			for (j=0 ; j<out->nummarksurfaces ; j++)
				out->firstmarksurface[j]->flags |= BRUSH29_SURF_UNDERWATER;
		}
		if (isnotmap)
		{
			for (j=0 ; j<out->nummarksurfaces ; j++)
				out->firstmarksurface[j]->flags |= BRUSH29_SURF_DONTWARP;
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

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -24;
	hull->clip_mins[1] = -24;
	hull->clip_mins[2] = -20;
	hull->clip_maxs[0] = 24;
	hull->clip_maxs[1] = 24;
	hull->clip_maxs[2] = 20;

	hull = &loadmodel->hulls[3];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -12;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 16;

	hull = &loadmodel->hulls[4];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -40;
	hull->clip_mins[1] = -40;
	hull->clip_mins[2] = -42;
	hull->clip_maxs[0] = 40;
	hull->clip_maxs[1] = 40;
	hull->clip_maxs[2] = 42;

	hull = &loadmodel->hulls[5];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -48;
	hull->clip_mins[1] = -48;
	hull->clip_mins[2] = -50;
	hull->clip_maxs[0] = 48;
	hull->clip_maxs[1] = 48;
	hull->clip_maxs[2] = 50;

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
	
	hull = &loadmodel->hulls[0];	
	
	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = (bsp29_dclipnode_t*)Hunk_AllocName ( count*sizeof(*out), loadname);	

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
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
	out = (mbrush29_surface_t**)Hunk_AllocName ( count*sizeof(*out), loadname);	

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleShort(in[i]);
		if (j >= loadmodel->numsurfaces)
			Sys_Error ("Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
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

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

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
	
	in = (bsp29_dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (cplane_t*)Hunk_AllocName ( count*2*sizeof(*out), loadname);	
	
	loadmodel->planes = out;
	loadmodel->numplanes = count;

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
		corner[i] = Q_fabs(mins[i]) > Q_fabs(maxs[i]) ? Q_fabs(mins[i]) : Q_fabs(maxs[i]);
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
	bsp29_dmodel_h2_t	*bm;
	
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
	
	mod->numframes = 2;		// regular and alternate animation
	
//
// set up the submodels (FIXME: this is confusing)
//
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (j=1 ; j<BSP29_MAX_MAP_HULLS_H2 ; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes-1;
		}
		
		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;
		
		VectorCopy (bm->maxs, mod->maxs);
		VectorCopy (bm->mins, mod->mins);

		mod->radius = RadiusFromBounds (mod->mins, mod->maxs);

		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels-1)
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

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

mesh1hdr_t	*pheader;

dmdl_stvert_t	stverts[MAXALIASVERTS];
mmesh1triangle_t	triangles[MAXALIASTRIS];

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
dmdl_trivertx_t	*poseverts[MAXALIASFRAMES];
static int			posenum;

byte		player_8bit_texels[MAX_PLAYER_CLASS][620*245];

static float	aliastransform[3][4];

static void R_AliasTransformVector (vec3_t in, vec3_t out)
{
	out[0] = DotProduct(in, aliastransform[0]) + aliastransform[0][3];
	out[1] = DotProduct(in, aliastransform[1]) + aliastransform[1][3];
	out[2] = DotProduct(in, aliastransform[2]) + aliastransform[2][3];
}

/*
=================
Mod_LoadAliasFrame
=================
*/
static void * Mod_LoadAliasFrame (void * pin, mmesh1framedesc_t *frame)
{
	dmdl_trivertx_t		*pframe, *pinframe;
	int				i, j;
	dmdl_frame_t	*pdaliasframe;
	vec3_t			in,out;
	
	pdaliasframe = (dmdl_frame_t *)pin;

	QStr::Cpy(frame->name, pdaliasframe->name);
	frame->firstpose = posenum;
	frame->numposes = 1;

	for (i=0 ; i<3 ; i++)
	{
	// these are byte values, so we don't have to worry about
	// endianness
		frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
		frame->bboxmax.v[i] = pdaliasframe->bboxmax.v[i];
	}

	pinframe = (dmdl_trivertx_t *)(pdaliasframe + 1);

	if (mins[0] == 32768)
	{
		aliastransform[0][0] = pheader->scale[0];
		aliastransform[1][1] = pheader->scale[1];
		aliastransform[2][2] = pheader->scale[2];
		aliastransform[0][3] = pheader->scale_origin[0];
		aliastransform[1][3] = pheader->scale_origin[1];
		aliastransform[2][3] = pheader->scale_origin[2];

		for (j=0;j<pheader->numverts;j++)
		{
			in[0] = pinframe[j].v[0];
			in[1] = pinframe[j].v[1];
			in[2] = pinframe[j].v[2];
			R_AliasTransformVector(in,out);
			for (i=0 ; i<3 ; i++)
			{
				if (mins[i] > out[i])
					mins[i] = out[i];
				if (maxs[i] < out[i])
					maxs[i] = out[i];
			}
		}
	}

	poseverts[posenum] = pinframe;
	posenum++;

	pinframe += pheader->numverts;

	return (void *)pinframe;
}


/*
=================
Mod_LoadAliasGroup
=================
*/
static void *Mod_LoadAliasGroup (void * pin,  mmesh1framedesc_t *frame)
{
	dmdl_group_t		*pingroup;
	int					i, j, k, numframes;
	dmdl_interval_t	*pin_intervals;
	void				*ptemp;
	vec3_t				in,out;
	
	pingroup = (dmdl_group_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	frame->firstpose = posenum;
	frame->numposes = numframes;

	for (i=0 ; i<3 ; i++)
	{
	// these are byte values, so we don't have to worry about endianness
		frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
		frame->bboxmax.v[i] = pingroup->bboxmax.v[i];
	}

	pin_intervals = (dmdl_interval_t *)(pingroup + 1);

	frame->interval = LittleFloat (pin_intervals->interval);

	pin_intervals += numframes;

	ptemp = (void *)pin_intervals;

	aliastransform[0][0] = pheader->scale[0];
	aliastransform[1][1] = pheader->scale[1];
	aliastransform[2][2] = pheader->scale[2];
	aliastransform[0][3] = pheader->scale_origin[0];
	aliastransform[1][3] = pheader->scale_origin[1];
	aliastransform[2][3] = pheader->scale_origin[2];

	for (i=0 ; i<numframes ; i++)
	{
		poseverts[posenum] = (dmdl_trivertx_t *)((dmdl_frame_t *)ptemp + 1);

		if (mins[0] == 32768)
		{
			for (j=0;j<pheader->numverts;j++)
			{
				in[0] = poseverts[posenum][j].v[0];
				in[1] = poseverts[posenum][j].v[1];
				in[2] = poseverts[posenum][j].v[2];
				R_AliasTransformVector(in,out);
				for (k=0 ; k<3 ; k++)
				{
					if (mins[k] > out[k])
						mins[k] = out[k];
					if (maxs[k] < out[k])
						maxs[k] = out[k];
				}
			}
		}

		posenum++;

		ptemp = (dmdl_trivertx_t *)((dmdl_frame_t *)ptemp + 1) + pheader->numverts;
	}

	return ptemp;
}

//=========================================================

/*
===============
Mod_LoadAllSkins
===============
*/
static void *Mod_LoadAllSkins (int numskins, dmdl_skintype_t *pskintype, int mdl_flags)
{
	int		i, j, k;
	char	name[32];
	int		s;
	byte	*copy;
	byte	*skin;
	byte	*texels;
	dmdl_skingroup_t		*pinskingroup;
	int		groupskins;
	dmdl_skininterval_t	*pinskinintervals;
	int		texture_mode;
	
	skin = (byte *)(pskintype + 1);

	if (numskins < 1 || numskins > MAX_MESH1_SKINS)
		Sys_Error ("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

	s = pheader->skinwidth * pheader->skinheight;

	if( mdl_flags & EF_HOLEY )
		texture_mode = IMG8MODE_SkinHoley;
	else if( mdl_flags & EF_TRANSPARENT )
		texture_mode = IMG8MODE_SkinTransparent;
	else if( mdl_flags & EF_SPECIAL_TRANS )
		texture_mode = IMG8MODE_SkinSpecialTrans;
	else
		texture_mode = IMG8MODE_Skin;

	for (i=0 ; i<numskins ; i++)
	{
		if (pskintype->type == ALIAS_SKIN_SINGLE)
		{
			byte* pic32 = R_ConvertImage8To32((byte *)(pskintype + 1), pheader->skinwidth, pheader->skinheight, texture_mode);

			// save 8 bit texels for the player model to remap
			if (!QStr::Cmp(loadmodel->name,"models/paladin.mdl"))
			{
				if (s > sizeof(player_8bit_texels[0]))
					Sys_Error ("Player skin too large");
				Com_Memcpy(player_8bit_texels[0], (byte *)(pskintype + 1), s);
			}
			else if (!QStr::Cmp(loadmodel->name,"models/crusader.mdl"))
			{
				if (s > sizeof(player_8bit_texels[1]))
					Sys_Error ("Player skin too large");
				Com_Memcpy(player_8bit_texels[1], (byte *)(pskintype + 1), s);
			}
			else if (!QStr::Cmp(loadmodel->name,"models/necro.mdl"))
			{
				if (s > sizeof(player_8bit_texels[2]))
					Sys_Error ("Player skin too large");
				Com_Memcpy(player_8bit_texels[2], (byte *)(pskintype + 1), s);
			}
			else if (!QStr::Cmp(loadmodel->name,"models/assassin.mdl"))
			{
				if (s > sizeof(player_8bit_texels[3]))
					Sys_Error ("Player skin too large");
				Com_Memcpy(player_8bit_texels[3], (byte *)(pskintype + 1), s);
			}
			else if (!QStr::Cmp(loadmodel->name,"models/succubus.mdl"))
			{
				if (s > sizeof(player_8bit_texels[4]))
					Sys_Error ("Player skin too large");
				Com_Memcpy(player_8bit_texels[4], (byte *)(pskintype + 1), s);
			}
			else if (!QStr::Cmp(loadmodel->name,"models/hank.mdl"))
			{
				if (s > sizeof(player_8bit_texels[5]))
					Sys_Error ("Player skin too large");
				Com_Memcpy(player_8bit_texels[5], (byte *)(pskintype + 1), s);
			}

			sprintf (name, "%s_%i", loadmodel->name, i);

			pheader->gl_texture[i][0] =
			pheader->gl_texture[i][1] =
			pheader->gl_texture[i][2] =
			pheader->gl_texture[i][3] =
				R_CreateImage(name, pic32, pheader->skinwidth, pheader->skinheight, true, true, GL_REPEAT, false);
			delete[] pic32;
			pskintype = (dmdl_skintype_t *)((byte *)(pskintype+1) + s);
		} 
		else 
		{
			// animating skin group.  yuck.
			pskintype++;
			pinskingroup = (dmdl_skingroup_t *)pskintype;
			groupskins = LittleLong (pinskingroup->numskins);
			pinskinintervals = (dmdl_skininterval_t *)(pinskingroup + 1);

			pskintype = (dmdl_skintype_t*)(pinskinintervals + groupskins);


			for (j=0 ; j<groupskins ; j++)
			{
					sprintf (name, "%s_%i_%i", loadmodel->name, i,j);
					byte* pic32 = R_ConvertImage8To32((byte *)(pskintype), pheader->skinwidth, pheader->skinheight, texture_mode);
					pheader->gl_texture[i][j&3] = R_CreateImage(name, pic32, pheader->skinwidth, pheader->skinheight, true, true, GL_REPEAT, false);
					delete[] pic32;
					pskintype = (dmdl_skintype_t *)((byte *)(pskintype) + s);
			}
			k = j;
			for (/* */; j < 4; j++)
				pheader->gl_texture[i][j&3] = 
				pheader->gl_texture[i][j - k]; 
		}
	}

	return (void *)pskintype;
}


//=========================================================================
/*
=================
Mod_LoadAliasModelNew
reads extra field for num ST verts, and extra index list of them
=================
*/
static void Mod_LoadAliasModelNew (model_t *mod, void *buffer)
{
	int					i, j;
	newmdl_t			*pinmodel;
	dmdl_stvert_t			*pinstverts;
	dmdl_newtriangle_t		*pintriangles;
	int					version, numframes, numskins;
	int					size;
	dmdl_frametype_t	*pframetype;
	dmdl_skintype_t	*pskintype;
	int					start, end, total;
	
	start = Hunk_LowMark ();

	pinmodel = (newmdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MESH1_NEWVERSION)
		Sys_Error ("%s has wrong version number (%i should be %i)",
				 mod->name, version, MESH1_NEWVERSION);

//	Con_Printf("Loading NEW model %s\n",mod->name);
//
// allocate space for a working header, plus all the data except the frames,
// skin and group info
//
	size = 	sizeof (mesh1hdr_t) 
			+ (LittleLong (pinmodel->numframes) - 1) *
			sizeof (pheader->frames[0]);
	pheader = (mesh1hdr_t*)Hunk_AllocName (size, loadname);
	
	mod->flags = LittleLong (pinmodel->flags);

//
// endian-adjust and copy the data, starting with the alias model header
//
	pheader->boundingradius = LittleFloat (pinmodel->boundingradius);
	pheader->numskins = LittleLong (pinmodel->numskins);
	pheader->skinwidth = LittleLong (pinmodel->skinwidth);
	pheader->skinheight = LittleLong (pinmodel->skinheight);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
		Sys_Error ("model %s has a skin taller than %d", mod->name,
				   MAX_LBM_HEIGHT);

	pheader->numverts = LittleLong (pinmodel->numverts);
	pheader->version = LittleLong (pinmodel->num_st_verts);	//hide num_st in version
	
	if (pheader->numverts <= 0)
		Sys_Error ("model %s has no vertices", mod->name);

	if (pheader->numverts > MAXALIASVERTS)
		Sys_Error ("model %s has too many vertices", mod->name);

	pheader->numtris = LittleLong (pinmodel->numtris);

	if (pheader->numtris <= 0)
		Sys_Error ("model %s has no triangles", mod->name);

	pheader->numframes = LittleLong (pinmodel->numframes);
	numframes = pheader->numframes;
	if (numframes < 1)
		Sys_Error ("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

	pheader->size = LittleFloat (pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = (synctype_t)LittleLong (pinmodel->synctype);
	mod->numframes = pheader->numframes;

	for (i=0 ; i<3 ; i++)
	{
		pheader->scale[i] = LittleFloat (pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
		pheader->eyeposition[i] = LittleFloat (pinmodel->eyeposition[i]);
	}

//
// load the skins
//
	pskintype = (dmdl_skintype_t *)&pinmodel[1];
	pskintype = (dmdl_skintype_t *)Mod_LoadAllSkins (pheader->numskins, pskintype, mod->flags);

//
// load base s and t vertices
//
	pinstverts = (dmdl_stvert_t *)pskintype;

	for (i=0 ; i<pheader->version ; i++)	//version holds num_st_verts
	{
		stverts[i].onseam = LittleLong (pinstverts[i].onseam);
		stverts[i].s = LittleLong (pinstverts[i].s);
		stverts[i].t = LittleLong (pinstverts[i].t);
	}

//
// load triangle lists
//
	pintriangles = (dmdl_newtriangle_t *)&pinstverts[pheader->version];

	for (i=0 ; i<pheader->numtris ; i++)
	{
		triangles[i].facesfront = LittleLong (pintriangles[i].facesfront);

		for (j=0 ; j<3 ; j++)
		{
			triangles[i].vertindex[j] = LittleShort (pintriangles[i].vertindex[j]);
			triangles[i].stindex[j]	  = LittleShort (pintriangles[i].stindex[j]);
		}
	}

//
// load the frames
//
	posenum = 0;
	pframetype = (dmdl_frametype_t *)&pintriangles[pheader->numtris];

	mins[0] = mins[1] = mins[2] = 32768;
	maxs[0] = maxs[1] = maxs[2] = -32768;

	for (i=0 ; i<numframes ; i++)
	{
		mdl_frametype_t	frametype;

		frametype = (mdl_frametype_t)LittleLong (pframetype->type);

		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (dmdl_frametype_t *)
					Mod_LoadAliasFrame (pframetype + 1, &pheader->frames[i]);
		}
		else
		{
			pframetype = (dmdl_frametype_t *)
					Mod_LoadAliasGroup (pframetype + 1, &pheader->frames[i]);
		}
	}

	//Con_Printf("Model is %s\n",mod->name);
	//Con_Printf("   Mins is %5.2f, %5.2f, %5.2f\n",mins[0],mins[1],mins[2]);
	//Con_Printf("   Maxs is %5.2f, %5.2f, %5.2f\n",maxs[0],maxs[1],maxs[2]);

	pheader->numposes = posenum;

	mod->type = MOD_MESH1;

// FIXME: do this right
//	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
//	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;
	mod->mins[0] = mins[0] - 10;
	mod->mins[1] = mins[1] - 10;
	mod->mins[2] = mins[2] - 10;
	mod->maxs[0] = maxs[0] + 10;
	mod->maxs[1] = maxs[1] + 10;
	mod->maxs[2] = maxs[2] + 10;


	//
	// build the draw lists
	//
	GL_MakeAliasModelDisplayLists (mod, pheader);

//
// move the complete, relocatable alias model to the cache
//	
	end = Hunk_LowMark ();
	total = end - start;
	
	Cache_Alloc (&mod->cache, total, loadname);
	if (!mod->cache.data)
		return;
	Com_Memcpy(mod->cache.data, pheader, total);

	Hunk_FreeToLowMark (start);
}

/*
=================
Mod_LoadAliasModel
=================
*/
static void Mod_LoadAliasModel (model_t *mod, void *buffer)
{
	int					i, j;
	mdl_t				*pinmodel;
	dmdl_stvert_t			*pinstverts;
	dmdl_triangle_t			*pintriangles;
	int					version, numframes, numskins;
	int					size;
	dmdl_frametype_t	*pframetype;
	dmdl_skintype_t	*pskintype;
	int					start, end, total;
	
	start = Hunk_LowMark ();

	pinmodel = (mdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MESH1_VERSION)
		Sys_Error ("%s has wrong version number (%i should be %i)",
				 mod->name, version, MESH1_VERSION);

//
// allocate space for a working header, plus all the data except the frames,
// skin and group info
//
	size = 	sizeof (mesh1hdr_t) 
			+ (LittleLong (pinmodel->numframes) - 1) *
			sizeof (pheader->frames[0]);
	pheader = (mesh1hdr_t*)Hunk_AllocName (size, loadname);
	
	mod->flags = LittleLong (pinmodel->flags);

//
// endian-adjust and copy the data, starting with the alias model header
//
	pheader->boundingradius = LittleFloat (pinmodel->boundingradius);
	pheader->numskins = LittleLong (pinmodel->numskins);
	pheader->skinwidth = LittleLong (pinmodel->skinwidth);
	pheader->skinheight = LittleLong (pinmodel->skinheight);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
		Sys_Error ("model %s has a skin taller than %d", mod->name,
				   MAX_LBM_HEIGHT);

	pheader->numverts = LittleLong (pinmodel->numverts);
	pheader->version = pheader->numverts;	//hide num_st in version

	if (pheader->numverts <= 0)
		Sys_Error ("model %s has no vertices", mod->name);

	if (pheader->numverts > MAXALIASVERTS)
		Sys_Error ("model %s has too many vertices", mod->name);

	pheader->numtris = LittleLong (pinmodel->numtris);

	if (pheader->numtris <= 0)
		Sys_Error ("model %s has no triangles", mod->name);

	pheader->numframes = LittleLong (pinmodel->numframes);
	numframes = pheader->numframes;
	if (numframes < 1)
		Sys_Error ("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

	pheader->size = LittleFloat (pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = (synctype_t)LittleLong (pinmodel->synctype);
	mod->numframes = pheader->numframes;

	for (i=0 ; i<3 ; i++)
	{
		pheader->scale[i] = LittleFloat (pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
		pheader->eyeposition[i] = LittleFloat (pinmodel->eyeposition[i]);
	}


//
// load the skins
//
	pskintype = (dmdl_skintype_t *)&pinmodel[1];
	pskintype = (dmdl_skintype_t *)Mod_LoadAllSkins (pheader->numskins, pskintype, mod->flags);

//
// load base s and t vertices
//
	pinstverts = (dmdl_stvert_t *)pskintype;

	for (i=0 ; i<pheader->numverts ; i++)
	{
		stverts[i].onseam = LittleLong (pinstverts[i].onseam);
		stverts[i].s = LittleLong (pinstverts[i].s);
		stverts[i].t = LittleLong (pinstverts[i].t);
	}

//
// load triangle lists
//
	pintriangles = (dmdl_triangle_t *)&pinstverts[pheader->numverts];

	for (i=0 ; i<pheader->numtris ; i++)
	{
		triangles[i].facesfront = LittleLong (pintriangles[i].facesfront);

		for (j=0 ; j<3 ; j++)
		{
			triangles[i].vertindex[j] =	LittleLong (pintriangles[i].vertindex[j]);
			triangles[i].stindex[j]	  = triangles[i].vertindex[j];
		}
	}

//
// load the frames
//
	posenum = 0;
	pframetype = (dmdl_frametype_t *)&pintriangles[pheader->numtris];

	mins[0] = mins[1] = mins[2] = 32768;
	maxs[0] = maxs[1] = maxs[2] = -32768;

	for (i=0 ; i<numframes ; i++)
	{
		mdl_frametype_t	frametype;

		frametype = (mdl_frametype_t)LittleLong (pframetype->type);

		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (dmdl_frametype_t *)
					Mod_LoadAliasFrame (pframetype + 1, &pheader->frames[i]);
		}
		else
		{
			pframetype = (dmdl_frametype_t *)
					Mod_LoadAliasGroup (pframetype + 1, &pheader->frames[i]);
		}
	}

	pheader->numposes = posenum;

	mod->type = MOD_MESH1;

// FIXME: do this right
//	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
//	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;

	mod->mins[0] = mins[0] - 10;
	mod->mins[1] = mins[1] - 10;
	mod->mins[2] = mins[2] - 10;
	mod->maxs[0] = maxs[0] + 10;
	mod->maxs[1] = maxs[1] + 10;
	mod->maxs[2] = maxs[2] + 10;

	//
	// build the draw lists
	//
	GL_MakeAliasModelDisplayLists (mod, pheader);

//
// move the complete, relocatable alias model to the cache
//	
	end = Hunk_LowMark ();
	total = end - start;
	
	Cache_Alloc (&mod->cache, total, loadname);
	if (!mod->cache.data)
		return;
	Com_Memcpy(mod->cache.data, pheader, total);

	Hunk_FreeToLowMark (start);
}

//=============================================================================

/*
=================
Mod_LoadSpriteFrame
=================
*/
static void * Mod_LoadSpriteFrame (void * pin, msprite1frame_t **ppframe, int framenum)
{
	dsprite1frame_t		*pinframe;
	msprite1frame_t		*pspriteframe;
	int					i, width, height, size, origin[2];
	unsigned short		*ppixout;
	byte				*ppixin;
	char				name[64];

	pinframe = (dsprite1frame_t *)pin;

	width = LittleLong (pinframe->width);
	height = LittleLong (pinframe->height);
	size = width * height;

	pspriteframe = (msprite1frame_t*)Hunk_AllocName (sizeof (msprite1frame_t),loadname);

	Com_Memset(pspriteframe, 0, sizeof (msprite1frame_t));

	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	origin[0] = LittleLong (pinframe->origin[0]);
	origin[1] = LittleLong (pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];

	sprintf (name, "%s_%i", loadmodel->name, framenum);
	byte* pic32 = R_ConvertImage8To32((byte *)(pinframe + 1), width, height, IMG8MODE_Normal);
	pspriteframe->gl_texture = R_CreateImage(name, pic32, width, height, true, true, GL_CLAMP, false);
	delete[] pic32;

	return (void *)((byte *)pinframe + sizeof (dsprite1frame_t) + size);
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
static void * Mod_LoadSpriteGroup (void * pin, msprite1frame_t **ppframe, int framenum)
{
	dsprite1group_t		*pingroup;
	msprite1group_t		*pspritegroup;
	int					i, numframes;
	dsprite1interval_t	*pin_intervals;
	float				*poutintervals;
	void				*ptemp;

	pingroup = (dsprite1group_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	pspritegroup = (msprite1group_t*)Hunk_AllocName (sizeof (msprite1group_t) +
				(numframes - 1) * sizeof (pspritegroup->frames[0]), loadname);

	pspritegroup->numframes = numframes;

	*ppframe = (msprite1frame_t *)pspritegroup;

	pin_intervals = (dsprite1interval_t *)(pingroup + 1);

	poutintervals = (float*)Hunk_AllocName (numframes * sizeof (float), loadname);

	pspritegroup->intervals = poutintervals;

	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Sys_Error ("Mod_LoadSpriteGroup: interval<=0");

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		ptemp = Mod_LoadSpriteFrame (ptemp, &pspritegroup->frames[i], framenum * 100 + i);
	}

	return ptemp;
}


/*
=================
Mod_LoadSpriteModel
=================
*/
static void Mod_LoadSpriteModel (model_t *mod, void *buffer)
{
	int					i;
	int					version;
	dsprite1_t			*pin;
	msprite1_t			*psprite;
	int					numframes;
	int					size;
	dsprite1frametype_t	*pframetype;
	
	pin = (dsprite1_t *)buffer;

	version = LittleLong (pin->version);
	if (version != SPRITE1_VERSION)
		Sys_Error ("%s has wrong version number "
				 "(%i should be %i)", mod->name, version, SPRITE1_VERSION);

	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite1_t) +	(numframes - 1) * sizeof (psprite->frames);

	psprite = (msprite1_t*)Hunk_AllocName (size, loadname);

	mod->cache.data = psprite;

	psprite->type = LittleLong (pin->type);
	psprite->maxwidth = LittleLong (pin->width);
	psprite->maxheight = LittleLong (pin->height);
	psprite->beamlength = LittleFloat (pin->beamlength);
	mod->synctype = (synctype_t)LittleLong (pin->synctype);
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;
	
//
// load the frames
//
	if (numframes < 1)
		Sys_Error ("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);

	mod->numframes = numframes;

	pframetype = (dsprite1frametype_t *)(pin + 1);

	for (i=0 ; i<numframes ; i++)
	{
		sprite1frametype_t	frametype;

		frametype = (sprite1frametype_t)LittleLong (pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dsprite1frametype_t *)
					Mod_LoadSpriteFrame (pframetype + 1,
										 &psprite->frames[i].frameptr, i);
		}
		else
		{
			pframetype = (dsprite1frametype_t *)
					Mod_LoadSpriteGroup (pframetype + 1,
										 &psprite->frames[i].frameptr, i);
		}
	}

	mod->type = MOD_SPRITE;
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
		Con_Printf ("%8p : %s\n",mod->cache.data, mod->name);
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

void Mod_CalcScaleOffset(qhandle_t Handle, float ScaleX, float ScaleY, float ScaleZ, float ScaleZOrigin, vec3_t Out)
{
	model_t* Model = Mod_GetModel(Handle);
	if (Model->type != MOD_MESH1)
	{
		throw QException("Not an alias model");
	}

	mesh1hdr_t* AliasHdr = (mesh1hdr_t*)Mod_Extradata(Model);

	Out[0] = -(ScaleX - 1.0) * (AliasHdr->scale[0] * 127.95 + AliasHdr->scale_origin[0]);
	Out[1] = -(ScaleY - 1.0) * (AliasHdr->scale[1] * 127.95 + AliasHdr->scale_origin[1]);
	Out[2] = -(ScaleZ - 1.0) * (ScaleZOrigin * 2.0 * AliasHdr->scale[2] * 127.95 + AliasHdr->scale_origin[2]);
}

int Mod_GetNumFrames(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->numframes;
}

int Mod_GetFlags(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->flags;
}

bool Mod_IsAliasModel(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->type == MOD_MESH1;
}

const char* Mod_GetName(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->name;
}
