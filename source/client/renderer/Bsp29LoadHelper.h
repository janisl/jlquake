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

#ifndef __Bsp29SurfacesLoader__
#define __Bsp29SurfacesLoader__

#include "../../common/Str.h"
#include "models/RenderModel.h"
#include "BspSurfaceBuilder.h"

class idBsp29LoadHelper : public idBspSurfaceBuilder {
public:
	int numplanes;
	cplane_t* planes;

	byte* lightdata;

	int numtextures;
	mbrush29_texture_t** textures;

	int numtexinfo;
	mbrush29_texinfo_t* texinfo;
	idTextureInfo* textureInfos;

	int numsurfaces;
	idSurfaceFaceQ1* surfaces;

	int numsubmodels;
	mbrush29_submodel_t* submodels;

	idBsp29LoadHelper( const idStr& name, byte* fileBase );
	~idBsp29LoadHelper();
	void LoadVertexes( bsp_lump_t* l );
	void LoadEdges( bsp_lump_t* l );
	void LoadSurfedges( bsp_lump_t* l );
	void LoadPlanes( bsp_lump_t* l );
	void LoadLighting( bsp_lump_t* l );
	void LoadTextures( bsp_lump_t* l );
	void LoadTexinfo( bsp_lump_t* l );
	void LoadFaces( bsp_lump_t* l );
	void LoadSubmodelsQ1( bsp_lump_t* l );
	void LoadSubmodelsH2( bsp_lump_t* l );

private:
	idStr name;
	byte* fileBase;

	int numvertexes;
	mbrush_vertex_t* vertexes;

	int numedges;
	mbrush_edge_t* edges;

	int numsurfedges;
	int* surfedges;

	void BuildSurfaceVertexesList( idSurfaceFaceQ1Q2* fa, int firstedge, int lnumverts );
};

#endif
