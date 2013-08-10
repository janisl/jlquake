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

#include "RenderModel.h"

idRenderModel::idRenderModel() {
	Com_Memset( name, 0, MAX_QPATH );
	type = MOD_BAD;
	index = 0;
	q1_numframes = 0;
	q1_synctype = ST_SYNC;
	q1_flags = 0;
	VectorClear( q1_mins );
	VectorClear( q1_maxs );
	q1_radius = 0;
	brush29_firstmodelsurface = 0;
	brush29_nummodelsurfaces = 0;
	brush29_numsubmodels = 0;
	brush29_submodels = NULL;
	brush29_numplanes = 0;
	brush29_planes = NULL;
	brush29_numleafs = 0;
	brush29_leafs = NULL;
	brush29_numvertexes = 0;
	brush29_vertexes = NULL;
	brush29_numedges = 0;
	brush29_edges = NULL;
	brush29_numnodes = 0;
	brush29_firstnode = 0;
	brush29_nodes = NULL;
	brush29_numtexinfo = 0;
	brush29_texinfo = NULL;
	brush29_numsurfaces = 0;
	brush29_surfaces = NULL;
	brush29_numsurfedges = 0;
	brush29_surfedges = NULL;
	brush29_nummarksurfaces = 0;
	brush29_marksurfaces = NULL;
	brush29_numtextures = 0;
	brush29_textures = NULL;
	brush29_visdata = NULL;
	brush29_lightdata = NULL;
	brush29_entities = NULL;
	q1_mdl = NULL;
	q1_spr = NULL;
	q2_numframes = 0;
	q2_flags = 0;
	VectorClear( q2_mins );
	VectorClear( q2_maxs );
	q2_radius = 0;
	brush38_firstmodelsurface = 0;
	brush38_nummodelsurfaces = 0;
	brush38_lightmap = 0;
	brush38_numsubmodels = 0;
	brush38_submodels = NULL;
	brush38_numplanes = 0;
	brush38_planes = NULL;
	brush38_numleafs = 0;
	brush38_leafs = NULL;
	brush38_numvertexes = 0;
	brush38_vertexes = NULL;
	brush38_numedges = 0;
	brush38_edges = NULL;
	brush38_numnodes = 0;
	brush38_firstnode = 0;
	brush38_nodes = NULL;
	brush38_numtexinfo = 0;
	brush38_texinfo = NULL;
	brush38_numShaderInfo = 0;
	brush38_shaderInfo = NULL;
	brush38_numsurfaces = 0;
	brush38_surfaces = NULL;
	brush38_numsurfedges = 0;
	brush38_surfedges = NULL;
	brush38_nummarksurfaces = 0;
	brush38_marksurfaces = NULL;
	brush38_vis = NULL;
	brush38_lightdata = NULL;
	Com_Memset( q2_skins_shader, 0, sizeof( q2_skins_shader ) );
	q2_extradatasize = 0;
	q2_sp2 = NULL;
	q2_md2 = NULL;
	q3_dataSize = 0;
	q3_bmodel = NULL;
	Com_Memset( q3_md3, 0, sizeof( q3_md3 ) );
	q3_md4 = NULL;
	q3_md4Lods = NULL;
	q3_mds = NULL;
	q3_mdsSurfaces = NULL;
	Com_Memset( q3_mdc, 0, sizeof( q3_mdc ) );
	q3_mdm = NULL;
	q3_mdx = NULL;
	q3_numLods = 0;
	q3_shadowShader = 0;
	Com_Memset( q3_shadowParms, 0, sizeof( q3_shadowParms ) );
	q3_ATI_tess = 0;
}

idRenderModel::~idRenderModel() {
}

bool idRenderModel::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	return false;
}
