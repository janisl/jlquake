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

#ifndef __idRenderModel__
#define __idRenderModel__

#include "../shader.h"
#include "../../../common/math/Vec2.h"
#include "../../../common/file_formats/bsp29.h"
#include "../../../common/file_formats/bsp38.h"
#include "../../../common/file_formats/bsp46.h"
#include "../../../common/file_formats/mdl.h"
#include "../../../common/file_formats/md2.h"
#include "../../../common/file_formats/md3.h"
#include "../../../common/file_formats/md4.h"
#include "../../../common/file_formats/mdc.h"
#include "../../../common/file_formats/mds.h"
#include "../../../common/file_formats/mdm.h"
#include "../../../common/file_formats/mdx.h"
#include "../../../common/file_formats/spr.h"
#include "../../../common/file_formats/sp2.h"
#include "Surface.h"

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
#define SMP_FRAMES      2

#define BLOCK_WIDTH     128
#define BLOCK_HEIGHT    128

//==============================================================================
//
//	QUAKE BRUSH MODELS
//
//==============================================================================

#define BRUSH29_VERTEXSIZE  7

#define BRUSH29_SURF_PLANEBACK      2
#define BRUSH29_SURF_DRAWSKY        4
#define BRUSH29_SURF_DRAWTURB       0x10
#define BRUSH29_SURF_DRAWTILED      0x20

struct mbrush29_surface_t;

struct mbrush29_vertex_t {
	vec3_t position;
};

struct mbrush29_texture_t {
	char name[ 16 ];
	unsigned width, height;
	shader_t* shader;
	image_t* gl_texture;
	image_t* fullBrightTexture;
	int anim_total;						// total tenths in sequence ( 0 = no)
	mbrush29_texture_t* anim_next;		// in the animation sequence
	mbrush29_texture_t* anim_base;		// first frame of animation
	mbrush29_texture_t* alternate_anims;	// bmodels in frmae 1 use these
	unsigned offsets[ BSP29_MIPLEVELS ];			// four mip maps stored
};

struct mbrush29_edge_t {
	unsigned short v[ 2 ];
	unsigned int cachededgeoffset;
};

struct mbrush29_texinfo_t {
	float vecs[ 2 ][ 4 ];
	float mipadjust;
	mbrush29_texture_t* texture;
	int flags;
};

struct mbrush29_glpoly_t {
	mbrush29_glpoly_t* next;
	int numverts;
	float verts[ 4 ][ BRUSH29_VERTEXSIZE ];		// variable sized (xyz s1t1 s2t2)
};

struct mbrush29_surface_t : surface_base_t {
	int visframe;				// should be drawn when node is crossed

	cplane_t* plane;
	int flags;

	int firstedge;			// look up in model->surfedges[], negative numbers
	int numedges;			// are backwards edges

	short texturemins[ 2 ];
	short extents[ 2 ];

	int light_s, light_t;			// gl lightmap coordinates

	mbrush29_glpoly_t* polys;				// multiple if warped
	mbrush29_surface_t* texturechain;

	mbrush29_texinfo_t* texinfo;
	shader_t* shader;
	shader_t* altShader;

// lighting info
	int dlightframe;
	int dlightbits;

	int lightmaptexturenum;
	byte styles[ BSP29_MAXLIGHTMAPS ];
	int cached_light[ BSP29_MAXLIGHTMAPS ];				// values currently used in lightmap
	qboolean cached_dlight;					// true if dynamic light in cache
	byte* samples;				// [numstyles*surfsize]
};

struct mbrush29_node_t {
// common with leaf
	int contents;				// 0, to differentiate from leafs
	int visframe;				// node needs to be traversed if current

	float minmaxs[ 6 ];				// for bounding box culling

	mbrush29_node_t* parent;

// node specific
	cplane_t* plane;
	mbrush29_node_t* children[ 2 ];

	unsigned short firstsurface;
	unsigned short numsurfaces;
};

struct mbrush29_leaf_t {
// common with node
	int contents;				// wil be a negative contents number
	int visframe;				// node needs to be traversed if current

	float minmaxs[ 6 ];				// for bounding box culling

	mbrush29_node_t* parent;

// leaf specific
	byte* compressed_vis;

	mbrush29_surface_t** firstmarksurface;
	int nummarksurfaces;
	int key;					// BSP sequence number for leaf's contents
	byte ambient_sound_level[ BSP29_NUM_AMBIENTS ];
};

struct mbrush29_submodel_t {
	float mins[ 3 ];
	float maxs[ 3 ];
	float origin[ 3 ];
	qint32 headnode;
	qint32 visleafs;			// not including the solid leaf 0
	qint32 firstface;
	qint32 numfaces;
};

//==============================================================================
//
//	QUAKE 2 BRUSH MODELS
//
//==============================================================================

#define BRUSH38_VERTEXSIZE  7

#define BRUSH38_SURF_PLANEBACK      2

struct mbrush38_vertex_t {
	vec3_t position;
};

struct mbrush38_edge_t {
	unsigned short v[ 2 ];
	unsigned int cachededgeoffset;
};

struct mbrush38_texinfo_t {
	float vecs[ 2 ][ 4 ];
	int flags;
	int numframes;
	mbrush38_texinfo_t* next;		// animation chain
	image_t* image;
};

struct mbrush38_shaderInfo_t {
	int numframes;
	mbrush38_shaderInfo_t* next;	// animation chain
	shader_t* shader;
};

struct mbrush38_glpoly_t {
	mbrush38_glpoly_t* next;
	int numverts;
	float verts[ 4 ][ BRUSH38_VERTEXSIZE ];		// variable sized (xyz s1t1 s2t2)
};

struct mbrush38_surface_t : surface_base_t {
	int visframe;				// should be drawn when node is crossed

	cplane_t* plane;
	int flags;

	int firstedge;			// look up in model->surfedges[], negative numbers
	int numedges;			// are backwards edges

	short texturemins[ 2 ];
	short extents[ 2 ];

	int light_s, light_t;			// gl lightmap coordinates

	mbrush38_glpoly_t* polys;				// multiple if warped
	mbrush38_surface_t* texturechain;

	mbrush38_texinfo_t* texinfo;
	mbrush38_shaderInfo_t* shaderInfo;

// lighting info
	int dlightframe;
	int dlightbits;

	int lightmaptexturenum;
	byte styles[ BSP38_MAXLIGHTMAPS ];
	float cached_light[ BSP38_MAXLIGHTMAPS ];			// values currently used in lightmap
	qboolean cached_dlight;					// true if dynamic light in cache
	byte* samples;				// [numstyles*surfsize]
};

struct mbrush38_node_t {
// common with leaf
	int contents;				// -1, to differentiate from leafs
	int visframe;				// node needs to be traversed if current

	float minmaxs[ 6 ];				// for bounding box culling

	mbrush38_node_t* parent;

// node specific
	cplane_t* plane;
	mbrush38_node_t* children[ 2 ];

	unsigned short firstsurface;
	unsigned short numsurfaces;
};

struct mbrush38_leaf_t {
// common with node
	int contents;				// wil be a negative contents number
	int visframe;				// node needs to be traversed if current

	float minmaxs[ 6 ];				// for bounding box culling

	mbrush38_node_t* parent;

// leaf specific
	int cluster;
	int area;

	mbrush38_surface_t** firstmarksurface;
	int nummarksurfaces;
};

struct mbrush38_model_t {
	vec3_t mins, maxs;
	vec3_t origin;			// for sounds or lights
	float radius;
	int headnode;
	int visleafs;				// not including the solid leaf 0
	int firstface, numfaces;
};

//==============================================================================
//
//	SURFACES
//
//==============================================================================

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

21 - 31	: sorted shader index
11 - 20	: entity index
2 - 6	: fog index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
0 - 1	: dlightmap index

    TTimo - 1.32
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
0-1   : dlightmap index
*/
#define QSORT_FORCED_SORT_SHIFT 32
#define QSORT_SHADERNUM_SHIFT   18
#define QSORT_ENTITYNUM_SHIFT   8
#define QSORT_ATI_TESS_SHIFT    7	//	Remove
#define QSORT_FOGNUM_SHIFT      2
#define QSORT_FRONTFACE_SHIFT   1

// GR - TruForm flags
#define ATI_TESS_NONE       0
#define ATI_TESS_TRUFORM    1

#define BRUSH46_VERTEXSIZE  8

#define MAX_PATCH_SIZE      32			// max dimensions of a patch mesh in map file
#define MAX_GRID_SIZE       65			// max dimensions of a grid mesh in memory

#define MAX_DECAL_VERTS     10	// worst case is triangle clipped by 6 planes

struct drawSurf_t {
	unsigned long long sort;						// bit combination for fast compares
	surfaceType_t* surface;				// any of surface*_t
};

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
struct srfPoly_t : surface_base_t {
	qhandle_t hShader;
	int fogIndex;
	int numVerts;
	polyVert_t* verts;
};

struct srfPolyBuffer_t : surface_base_t {
	int fogIndex;
	polyBuffer_t* pPolyBuffer;
};

struct srfDisplayList_t : surface_base_t {
	int listNum;
};

struct srfFlare_t : surface_base_t {
	vec3_t origin;
	vec3_t normal;
	vec3_t color;
};

struct srfDecal_t : surface_base_t {
	int numVerts;
	polyVert_t verts[ MAX_DECAL_VERTS ];
};

// ydnar: normal map drawsurfaces must match this header
struct srfGeneric_t : surface_base_t {
	// dynamic lighting information
	int dlightBits[ SMP_FRAMES ];

	// culling information
	vec3_t bounds[ 2 ];
	vec3_t localOrigin;
	float radius;
	cplane_t plane;
};

struct srfGridMesh_t : surface_base_t {
	// dynamic lighting information
	int dlightBits[ SMP_FRAMES ];

	// culling information
	vec3_t bounds[ 2 ];
	vec3_t localOrigin;
	float radius;
	cplane_t plane;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t lodOrigin;
	float lodRadius;
	int lodFixed;
	int lodStitched;

	// vertexes
	int width, height;
	float* widthLodError;
	float* heightLodError;
	bsp46_drawVert_t verts[ 1 ];			// variable sized
};

struct srfSurfaceFace_t : surface_base_t {
	// dynamic lighting information
	int dlightBits[ SMP_FRAMES ];

	// culling information
	vec3_t bounds[ 2 ];
	vec3_t localOrigin;
	float radius;
	cplane_t plane;

	// triangle definitions (no normals at points)
	int numPoints;
	int numIndices;
	int ofsIndices;
	float points[ 1 ][ BRUSH46_VERTEXSIZE ];		// variable sized
	// there is a variable length list of indices here also
};

// misc_models in maps are turned into direct geometry by q3map
struct srfTriangles_t : surface_base_t {
	// dynamic lighting information
	int dlightBits[ SMP_FRAMES ];

	// culling information (FIXME: use this!)
	vec3_t bounds[ 2 ];
	vec3_t localOrigin;
	float radius;
	cplane_t plane;

	// triangle definitions
	int numIndexes;
	int* indexes;

	int numVerts;
	bsp46_drawVert_t* verts;
};

// ydnar: foliage surfaces are autogenerated from models into geometry lists by q3map2
typedef byte fcolor4ub_t[ 4 ];

struct foliageInstance_t {
	vec3_t origin;
	fcolor4ub_t color;
};

struct srfFoliage_t : surface_base_t {
	// dynamic lighting information
	int dlightBits[ SMP_FRAMES ];

	// culling information
	vec3_t bounds[ 2 ];
	vec3_t localOrigin;
	float radius;
	cplane_t plane;

	// triangle definitions
	int numIndexes;
	glIndex_t* indexes;

	int numVerts;
	vec4_t* xyz;
	vec4_t* normal;
	vec2_t* texCoords;
	vec2_t* lmTexCoords;

	// origins
	int numInstances;
	foliageInstance_t* instances;
};

//==============================================================================
//
//	QUAKE 3 BRUSH MODELS
//
//==============================================================================

#define BRUSH46_CONTENTS_NODE       -1

// ydnar: optimization
#define WORLD_MAX_SKY_NODES     32

struct mbrush46_surface_t {
	int viewCount;							// if == tr.viewCount, already added
	shader_t* shader;
	int fogIndex;

	surface_base_t* data;					// any of srf*_t
};

struct mbrush46_node_t {
	// common with leaf and node
	int contents;							// -1 for nodes, to differentiate from leafs
	int visframe;							// node needs to be traversed if current
	vec3_t mins, maxs;						// for bounding box culling
	vec3_t surfMins, surfMaxs;				// ydnar: bounding box including surfaces
	mbrush46_node_t* parent;

	// node specific
	cplane_t* plane;
	mbrush46_node_t* children[ 2 ];

	// leaf specific
	int cluster;
	int area;

	mbrush46_surface_t** firstmarksurface;
	int nummarksurfaces;
};

// ydnar: bsp model decal surfaces
struct mbrush46_decal_t {
	mbrush46_surface_t* parent;
	shader_t* shader;
	float fadeStartTime, fadeEndTime;
	int fogIndex;
	int numVerts;
	polyVert_t verts[ MAX_DECAL_VERTS ];
};

struct mbrush46_model_t {
	vec3_t bounds[ 2 ];							// for culling
	mbrush46_surface_t* firstSurface;
	int numSurfaces;

	// ydnar: decals
	mbrush46_decal_t* decals;

	// ydnar: for fog volumes
	int firstBrush;
	int numBrushes;
	orientation_t orientation[ SMP_FRAMES ];
	bool visible[ SMP_FRAMES ];
	int entityNum[ SMP_FRAMES ];
};

struct mbrush46_fog_t {
	int modelNum;						// ydnar: bsp model the fog belongs to
	int originalBrushNumber;
	vec3_t bounds[ 2 ];

	unsigned colorInt;					// in packed byte format
	float tcScale;						// texture coordinate vector scales
	shader_t* shader;					// fog shader to get colorInt and tcScale from
	fogParms_t parms;

	// for clipping distance in fog when outside
	qboolean hasSurface;
	float surface[ 4 ];
};

struct world_t {
	char name[ MAX_QPATH ];					// ie: maps/tim_dm2.bsp
	char baseName[ MAX_QPATH ];				// ie: tim_dm2

	int numShaders;
	bsp46_dshader_t* shaders;

	mbrush46_model_t* bmodels;

	int numplanes;
	cplane_t* planes;

	int numnodes;				// includes leafs
	int numDecisionNodes;
	mbrush46_node_t* nodes;

	int numsurfaces;
	mbrush46_surface_t* surfaces;

	int nummarksurfaces;
	mbrush46_surface_t** marksurfaces;

	int numfogs;
	mbrush46_fog_t* fogs;

	vec3_t lightGridOrigin;
	vec3_t lightGridSize;
	vec3_t lightGridInverseSize;
	int lightGridBounds[ 3 ];
	byte* lightGridData;


	int numClusters;
	int clusterBytes;
	const byte* vis;			// may be passed in by CM_LoadMap to save space

	byte* novis;				// clusterBytes of 0xff

	char* entityString;
	char* entityParsePoint;

	int dataSize;

	int numBModels;

	int numSkyNodes;
	mbrush46_node_t** skyNodes;		// ydnar: don't walk the entire bsp when rendering sky

	int globalFog;						// Arnout: index of global fog
	vec4_t globalOriginalFog;			// Arnout: to be able to restore original global fog
	vec4_t globalTransStartFog;			// Arnout: start fog for switch fog transition
	vec4_t globalTransEndFog;			// Arnout: end fog for switch fog transition
	int globalFogTransStartTime;
	int globalFogTransEndTime;
};

//==============================================================================
//
//	QUAKE MESH MODELS
//
//==============================================================================

#define MAX_MESH1_SKINS     32

#define H2EF_FACE_VIEW      65536		// Poly Model always faces you

struct mmesh1framedesc_t {
	int firstpose;
	int numposes;
	float interval;
	dmdl_trivertx_t bboxmin;
	dmdl_trivertx_t bboxmax;
	int frame;
	char name[ 16 ];
};

struct mesh1hdr_t {
	int ident;
	int version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int numskins;
	int skinwidth;
	int skinheight;
	int numverts;
	int numtris;
	int numframes;
	synctype_t synctype;
	int flags;
	float size;

	int numposes;
	int poseverts;
	int numIndexes;
	dmdl_trivertx_t* posedata;		// numposes*poseverts trivert_t
	idVec2* texCoords;
	glIndex_t* indexes;
	shader_t* shaders[ MAX_MESH1_SKINS ];
	mmesh1framedesc_t frames[ 1 ];		// variable sized
};

//==============================================================================
//
//	QUAKE SPRITE MODELS
//
//==============================================================================

struct msprite1frame_t {
	int width;
	int height;
	float up, down, left, right;
	shader_t* shader;
};

struct msprite1group_t {
	int numframes;
	float* intervals;
	msprite1frame_t* frames[ 1 ];
};

struct msprite1framedesc_t {
	sprite1frametype_t type;
	msprite1frame_t* frameptr;
};

struct msprite1_t {
	surfaceType_t surfaceType;
	int type;
	int maxwidth;
	int maxheight;
	int numframes;
	float beamlength;					// remove?
	msprite1framedesc_t frames[ 1 ];
};

//==============================================================================
//
//	QUAKE 2 MESH MODELS
//
//==============================================================================

struct mmd2_t {
	surfaceType_t surfaceType;

	int framesize;				// byte size of each frame

	int num_skins;
	int numVertexes;
	int numIndexes;
	int num_frames;

	idVec2* texCoords;
	glIndex_t* indexes;
	byte* frames;
};

//==============================================================================
//
//	Whole model
//
//==============================================================================

enum modtype_t
{
	MOD_BAD,
	MOD_BRUSH29,
	MOD_BRUSH38,
	MOD_BRUSH46,
	MOD_SPRITE,
	MOD_SPRITE2,
	MOD_MESH1,
	MOD_MESH2,
	MOD_MESH3,
	MOD_MD4,
	MOD_MDS,
	MOD_MDC,
	MOD_MDM,
	MOD_MDX
};

class idRenderModel {
public:
	char name[ MAX_QPATH ];
	modtype_t type;
	int index;						// model = tr.models[model->index]

	int q1_numframes;
	synctype_t q1_synctype;

	int q1_flags;

//
// volume occupied by the model graphics
//
	vec3_t q1_mins, q1_maxs;
	float q1_radius;

//
// Quake brush model
//
	int brush29_firstmodelsurface;
	int brush29_nummodelsurfaces;

	int brush29_numsubmodels;
	mbrush29_submodel_t* brush29_submodels;

	int brush29_numplanes;
	cplane_t* brush29_planes;

	int brush29_numleafs;				// number of visible leafs, not counting 0
	mbrush29_leaf_t* brush29_leafs;

	int brush29_numvertexes;
	mbrush29_vertex_t* brush29_vertexes;

	int brush29_numedges;
	mbrush29_edge_t* brush29_edges;

	int brush29_numnodes;
	int brush29_firstnode;
	mbrush29_node_t* brush29_nodes;

	int brush29_numtexinfo;
	mbrush29_texinfo_t* brush29_texinfo;

	int brush29_numsurfaces;
	mbrush29_surface_t* brush29_surfaces;

	int brush29_numsurfedges;
	int* brush29_surfedges;

	int brush29_nummarksurfaces;
	mbrush29_surface_t** brush29_marksurfaces;

	int brush29_numtextures;
	mbrush29_texture_t** brush29_textures;

	byte* brush29_visdata;
	byte* brush29_lightdata;
	char* brush29_entities;

//
// additional model data
//
	void* q1_cache;				// only access through Mod_Extradata

	int q2_numframes;

	int q2_flags;

//
// volume occupied by the model graphics
//
	vec3_t q2_mins, q2_maxs;
	float q2_radius;

//
// Quake 2 brush model
//
	int brush38_firstmodelsurface, brush38_nummodelsurfaces;
	int brush38_lightmap;				// only for submodels

	int brush38_numsubmodels;
	mbrush38_model_t* brush38_submodels;

	int brush38_numplanes;
	cplane_t* brush38_planes;

	int brush38_numleafs;				// number of visible leafs, not counting 0
	mbrush38_leaf_t* brush38_leafs;

	int brush38_numvertexes;
	mbrush38_vertex_t* brush38_vertexes;

	int brush38_numedges;
	mbrush38_edge_t* brush38_edges;

	int brush38_numnodes;
	int brush38_firstnode;
	mbrush38_node_t* brush38_nodes;

	int brush38_numtexinfo;
	mbrush38_texinfo_t* brush38_texinfo;

	int brush38_numShaderInfo;
	mbrush38_shaderInfo_t* brush38_shaderInfo;

	int brush38_numsurfaces;
	mbrush38_surface_t* brush38_surfaces;

	int brush38_numsurfedges;
	int* brush38_surfedges;

	int brush38_nummarksurfaces;
	mbrush38_surface_t** brush38_marksurfaces;

	bsp38_dvis_t* brush38_vis;

	byte* brush38_lightdata;

	// for alias models and skins
	shader_t* q2_skins_shader[ MAX_MD2_SKINS ];

	int q2_extradatasize;
	dsprite2_t* q2_sp2;
	mmd2_t* q2_md2;

	int q3_dataSize;					// just for listing purposes
	mbrush46_model_t* q3_bmodel;			// only if type == MOD_BRUSH
	md3Header_t* q3_md3[ MD3_MAX_LODS ];	// only if type == MOD_MESH
	md4Header_t* q3_md4;				// only if type == MOD_MD4
	mdsHeader_t* q3_mds;				// only if type == MOD_MDS
	mdcHeader_t* q3_mdc[ MD3_MAX_LODS ];	// only if type == MOD_MDC
	mdmHeader_t* q3_mdm;				// only if type == MOD_MDM
	mdxHeader_t* q3_mdx;				// only if type == MOD_MDX

	int q3_numLods;

	qhandle_t q3_shadowShader;
	float q3_shadowParms[ 6 ];				// x, y, width, height, depth, z offset

	// GR - model tessellation capability flag
	int q3_ATI_tess;

	idRenderModel();
	virtual ~idRenderModel();

	virtual bool Load( idList<byte>& buffer, idSkinTranslation* skinTranslation );
};

#endif
