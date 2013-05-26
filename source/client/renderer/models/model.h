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

idRenderModel* R_AllocModel();
void R_ModelInit();
void R_FreeModels();
idRenderModel* R_GetModelByHandle( qhandle_t Index );
void R_Modellist_f();
void R_PurgeModels( int count );
void R_BackupModels();

void Mod_LoadSpriteModel( idRenderModel* mod, void* buffer );
void Mod_FreeSpriteModel( idRenderModel* mod );
void R_AddSprSurfaces( trRefEntity_t* e, int forcedSortIndex );
void RB_SurfaceSpr( msprite1_t* psprite );

void Mod_LoadSprite2Model( idRenderModel* mod, void* buffer, int modfilelen );
void Mod_FreeSprite2Model( idRenderModel* mod );
void R_AddSp2Surfaces( trRefEntity_t* e, int forcedSortIndex );
void RB_SurfaceSp2( dsprite2_t* psprite );

void Mod_LoadMdlModel( idRenderModel* mod, const void* buffer, idSkinTranslation* skinTranslation );
void Mod_LoadMdlModelNew( idRenderModel* mod, const void* buffer, idSkinTranslation* skinTranslation );
void Mod_FreeMdlModel( idRenderModel* mod );
void R_AddMdlSurfaces( trRefEntity_t* e, int forcedSortIndex );
void RB_SurfaceMdl( mesh1hdr_t* paliashdr );
bool R_MdlHasHexen2Transparency( idRenderModel* Model );

void Mod_LoadMd2Model( idRenderModel* mod, const void* buffer );
void Mod_FreeMd2Model( idRenderModel* mod );
void R_AddMd2Surfaces( trRefEntity_t* e, int forcedSortIndex );
void RB_SurfaceMd2( mmd2_t* paliashdr );

bool R_LoadMd3( idRenderModel* mod, void* buffer );
void R_FreeMd3( idRenderModel* mod );
void R_RegisterMd3Shaders( idRenderModel* mod, int lod );
void R_AddMD3Surfaces( trRefEntity_t* e );
void RB_SurfaceMesh( md3Surface_t* surface );

bool R_LoadMD4( idRenderModel* mod, void* buffer, const char* mod_name );
void R_FreeMd4( idRenderModel* mod );
void R_AddAnimSurfaces( trRefEntity_t* ent );
void RB_SurfaceAnim( md4Surface_t* surfType );

bool R_LoadMdc( idRenderModel* mod, void* buffer );
void R_FreeMdc( idRenderModel* mod );
void R_RegisterMdcShaders( idRenderModel* mod, int lod );
void R_AddMDCSurfaces( trRefEntity_t* ent );
void RB_SurfaceCMesh( mdcSurface_t* surface );

bool R_LoadMds( idRenderModel* mod, const void* buffer );
void R_FreeMds( idRenderModel* mod );
void R_AddMdsAnimSurfaces( trRefEntity_t* ent );
void RB_SurfaceAnimMds( mdsSurface_t* surfType );
int R_GetBoneTagMds( orientation_t* outTag, mdsHeader_t* mds, int startTagIndex, const refEntity_t* refent, const char* tagName );

bool R_LoadMdm( idRenderModel* mod, const void* buffer );
void R_FreeMdm( idRenderModel* mod );
bool R_LoadMdx( idRenderModel* mod, const void* buffer );
void R_FreeMdx( idRenderModel* mod );
void R_MDM_AddAnimSurfaces( trRefEntity_t* ent );
void RB_MDM_SurfaceAnim( mdmSurface_t* surfType );
int R_MDM_GetBoneTag( orientation_t* outTag, mdmHeader_t* mdm, int startTagIndex, const refEntity_t* refent, const char* tagName );

void R_InitBsp29NoTextureMip();
void Mod_LoadBrush29Model( idRenderModel* mod, void* buffer );
void Mod_FreeBsp29( idRenderModel* mod );
byte* Mod_LeafPVS( mbrush29_leaf_t* Leaf, idRenderModel* Model );
mbrush29_leaf_t* Mod_PointInLeafQ1( vec3_t P, idRenderModel* Model );

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
srfGridMesh_t* R_SubdividePatchToGrid( int Width, int Height,
	bsp46_drawVert_t Points[ MAX_PATCH_SIZE * MAX_PATCH_SIZE ] );
srfGridMesh_t* R_GridInsertColumn( srfGridMesh_t* Grid, int Column, int Row, vec3_t Point, float LodError );
srfGridMesh_t* R_GridInsertRow( srfGridMesh_t* Grid, int Row, int Column, vec3_t Point, float LodError );
void R_FreeSurfaceGridMesh( srfGridMesh_t* Grid );

extern idRenderModel* loadmodel;
extern mbrush29_texture_t* r_notexture_mip;
extern world_t s_worldData;

#endif