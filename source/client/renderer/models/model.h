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

#ifndef _R_MODEL_H_
#define _R_MODEL_H_

#include "RenderModel.h"

struct trRefEntity_t;

void R_AddModel( idRenderModel* model );
void R_ModelInit();
void R_FreeModels();
idRenderModel* R_GetModelByHandle( qhandle_t Index );
void R_Modellist_f();

void Mod_FreeSpriteModel( idRenderModel* mod );
void R_AddSprSurfaces( trRefEntity_t* e, int forcedSortIndex );
void RB_SurfaceSpr( msprite1_t* psprite );

void Mod_FreeSprite2Model( idRenderModel* mod );
void R_AddSp2Surfaces( trRefEntity_t* e, int forcedSortIndex );
void RB_SurfaceSp2( dsprite2_t* psprite );

void Mod_FreeMdlModel( idRenderModel* mod );
void R_AddMdlSurfaces( trRefEntity_t* e, int forcedSortIndex );
void RB_SurfaceMdl( mesh1hdr_t* paliashdr );
bool R_MdlHasHexen2Transparency( idRenderModel* Model );

void Mod_FreeMd2Model( idRenderModel* mod );
void R_AddMd2Surfaces( trRefEntity_t* e, int forcedSortIndex );
void RB_SurfaceMd2( mmd2_t* paliashdr );

void R_FreeMd3( idRenderModel* mod );
void R_RegisterMd3Shaders( idRenderModel* mod, int lod );
void R_AddMD3Surfaces( trRefEntity_t* e );
void RB_SurfaceMesh( md3Surface_t* surface );

void R_FreeMd4( idRenderModel* mod );
void R_AddAnimSurfaces( trRefEntity_t* ent );
void RB_SurfaceAnim( md4Surface_t* surfType );

void R_FreeMdc( idRenderModel* mod );
void R_RegisterMdcShaders( idRenderModel* mod, int lod );
void R_AddMDCSurfaces( trRefEntity_t* ent );
void RB_SurfaceCMesh( mdcSurface_t* surface );

void R_FreeMds( idRenderModel* mod );
void R_AddMdsAnimSurfaces( trRefEntity_t* ent );
void RB_SurfaceAnimMds( mdsSurface_t* surfType );
int R_GetBoneTagMds( orientation_t* outTag, mdsHeader_t* mds, int startTagIndex, const refEntity_t* refent, const char* tagName );

void R_FreeMdm( idRenderModel* mod );
void R_FreeMdx( idRenderModel* mod );
void R_MDM_AddAnimSurfaces( trRefEntity_t* ent );
void RB_MDM_SurfaceAnim( mdmSurface_t* surfType );
int R_MDM_GetBoneTag( orientation_t* outTag, mdmHeader_t* mdm, int startTagIndex, const refEntity_t* refent, const char* tagName );

void R_InitBsp29NoTextureMip();
void Mod_FreeBsp29( idRenderModel* mod );
void Mod_FreeBsp29NonMap( idRenderModel* mod );
byte* Mod_LeafPVS( mbrush29_leaf_t* Leaf, idRenderModel* Model );
mbrush29_leaf_t* Mod_PointInLeafQ1( vec3_t P, idRenderModel* Model );
void ModBsp29_LoadTextures( byte* mod_base, bsp29_lump_t* l, int& numtextures, mbrush29_texture_t**& textures );
void ModBsp29_LoadLighting( byte* mod_base, bsp29_lump_t* l, byte*& lightdata );
void ModBsp29_LoadVertexes( byte* mod_base, bsp29_lump_t* l, int& numvertexes, mbrush29_vertex_t*& vertexes );
void ModBsp20_LoadEdges( byte* mod_base, bsp29_lump_t* l, int& numedges, mbrush29_edge_t*& edges );
void ModBsp29_LoadTexinfo( byte* mod_base, bsp29_lump_t* l, int numtextures, mbrush29_texture_t* const* textures, int& numtexinfo, mbrush29_texinfo_t*& texinfo, idTextureInfo*& textureInfos );
void ModBsp29_LoadFaces( byte* mod_base, bsp29_lump_t* l,
	const mbrush29_edge_t* pedges, const int* surfedges, mbrush29_vertex_t* vertexes,
	int numtexinfo, mbrush29_texinfo_t* texinfo, idTextureInfo* textureInfos,
	cplane_t* planes, byte* lightdata,
	int& numsurfaces, idSurfaceFaceQ1*& surfaces );
void ModBsp29_LoadSurfedges( byte* mod_base, bsp29_lump_t* l, int& numsurfedges, int*& surfedges );
void ModBsp29_LoadPlanes( byte* mod_base, bsp29_lump_t* l, int& numplanes, cplane_t*& planes );
void ModBsp29_LoadSubmodelsQ1( byte* mod_base, bsp29_lump_t* l, int& numsubmodels, mbrush29_submodel_t*& submodels );
void ModBsp29_LoadSubmodelsH2( byte* mod_base, bsp29_lump_t* l, int& numsubmodels, mbrush29_submodel_t*& submodels );

void Mod_LoadBrush38Model( idRenderModel* mod, void* buffer );
void Mod_FreeBsp38( idRenderModel* mod );
byte* Mod_ClusterPVS( int cluster, idRenderModel* model );
mbrush38_leaf_t* Mod_PointInLeafQ2( float* p, idRenderModel* model );

float R_ProcessLightmap( byte* buf_p, int in_padding, int width, int height, byte* image );
unsigned ColorBytes4( float r, float g, float b, float a );
void R_LoadBrush46Model( void* buffer );
void R_FreeBsp46( world_t* mod );
void R_FreeBsp46Model( idRenderModel* mod );
const byte* R_ClusterPVS( int cluster );
mbrush46_node_t* R_PointInLeaf( const vec3_t p );

//
//	CURVE TESSELATION
//
srfGridMesh_t* R_SubdividePatchToGrid( class idSurfaceGrid* surf, int width, int height, idWorldVertex points[ MAX_PATCH_SIZE * MAX_PATCH_SIZE ] );
bool R_GridInsertColumn( class idSurfaceGrid* grid, int column, int row, const idVec3& point, float lodError );
bool R_GridInsertRow( class idSurfaceGrid* grid, int row, int column, const idVec3& point, float lodError );
void R_FreeSurfaceGridMesh( class idSurfaceGrid* grid );

extern idRenderModel* loadmodel;
extern mbrush29_texture_t* r_notexture_mip;
extern world_t s_worldData;

#endif