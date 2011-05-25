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

#ifndef __MODEL__
#define __MODEL__

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

#define	MAXALIASVERTS	1024
#define	MAXALIASFRAMES	256
#define	MAXALIASTRIS	2048
extern	mesh1hdr_t	*pheader;
extern	dmdl_stvert_t	stverts[MAXALIASVERTS];
extern	mmesh1triangle_t	triangles[MAXALIASTRIS];
extern	dmdl_trivertx_t	*poseverts[MAXALIASFRAMES];

//===================================================================

//
// Whole model
//

struct model_t : model_common_t
{
	qboolean	needload;		// bmodels and sprites don't cache normally

	int			numframes;
	synctype_t	synctype;
	
	int			flags;

//
// volume occupied by the model graphics
//		
	vec3_t		mins, maxs;
	float		radius;

//
// solid volume for clipping 
//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	bsp29_dmodel_q1_t	*submodels;

	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mbrush29_leaf_t		*leafs;

	int			numvertexes;
	mbrush29_vertex_t	*vertexes;

	int			numedges;
	mbrush29_edge_t		*edges;

	int			numnodes;
	mbrush29_node_t		*nodes;

	int			numtexinfo;
	mbrush29_texinfo_t	*texinfo;

	int			numsurfaces;
	mbrush29_surface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			numclipnodes;
	bsp29_dclipnode_t	*clipnodes;

	int			nummarksurfaces;
	mbrush29_surface_t	**marksurfaces;

	mbrush29_hull_t		hulls[BSP29_MAX_MAP_HULLS_Q1];

	int			numtextures;
	mbrush29_texture_t	**textures;

	byte		*visdata;
	byte		*lightdata;
	char		*entities;

	unsigned	checksum;
	unsigned	checksum2;

//
// additional model data
//
	cache_user_t	cache;		// only access through Mod_Extradata

};

//============================================================================

void	*Mod_Extradata (model_t *mod);	// handles caching

mbrush29_leaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_LeafPVS (mbrush29_leaf_t *leaf, model_t *model);

model_t* Mod_GetModel(qhandle_t handle);

#endif	// __MODEL__
