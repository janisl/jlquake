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

#define BLOCK_WIDTH		128
#define BLOCK_HEIGHT	128

//==============================================================================
//
//	QUAKE BRUSH MODELS
//
//==============================================================================

#define BRUSH29_VERTEXSIZE	7

#define BRUSH29_SURF_PLANEBACK		2
#define BRUSH29_SURF_DRAWSKY		4
#define BRUSH29_SURF_DRAWTURB		0x10
#define BRUSH29_SURF_DRAWTILED		0x20
#define BRUSH29_SURF_UNDERWATER		0x80
#define BRUSH29_SURF_DONTWARP		0x100
#define BRUSH29_SURF_TRANSLUCENT	0x200

struct mbrush29_surface_t;

struct mbrush29_vertex_t
{
	vec3_t		position;
};

struct mbrush29_texture_t
{
	char		name[16];
	unsigned	width, height;
	image_t*	gl_texture;
	mbrush29_surface_t	*texturechain;	// for gl_texsort drawing
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	mbrush29_texture_t*	anim_next;		// in the animation sequence
	mbrush29_texture_t*	alternate_anims;	// bmodels in frmae 1 use these
	unsigned	offsets[BSP29_MIPLEVELS];		// four mip maps stored
};

struct mbrush29_edge_t
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
};

struct mbrush29_texinfo_t
{
	float		vecs[2][4];
	float		mipadjust;
	mbrush29_texture_t	*texture;
	int			flags;
};

struct mbrush29_glpoly_t
{
	mbrush29_glpoly_t*	next;
	mbrush29_glpoly_t*	chain;
	int		numverts;
	int		flags;			// for BRUSH29_SURF_UNDERWATER
	float	verts[4][BRUSH29_VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
};

struct mbrush29_surface_t
{
	int			visframe;		// should be drawn when node is crossed

	cplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
	short		texturemins[2];
	short		extents[2];

	int			light_s, light_t;	// gl lightmap coordinates

	mbrush29_glpoly_t	*polys;				// multiple if warped
	mbrush29_surface_t	*texturechain;

	mbrush29_texinfo_t	*texinfo;
	
// lighting info
	int			dlightframe;
	int			dlightbits;

	int			lightmaptexturenum;
	byte		styles[BSP29_MAXLIGHTMAPS];
	int			cached_light[BSP29_MAXLIGHTMAPS];	// values currently used in lightmap
	qboolean	cached_dlight;				// true if dynamic light in cache
	byte		*samples;		// [numstyles*surfsize]
};

struct mbrush29_node_t
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	float		minmaxs[6];		// for bounding box culling

	mbrush29_node_t	*parent;

// node specific
	cplane_t	*plane;
	mbrush29_node_t	*children[2];	

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
};

struct mbrush29_leaf_t
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	mbrush29_node_t	*parent;

// leaf specific
	byte		*compressed_vis;

	mbrush29_surface_t	**firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	byte		ambient_sound_level[BSP29_NUM_AMBIENTS];
};

struct mbrush29_hull_t
{
	bsp29_dclipnode_t	*clipnodes;
	cplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
};

struct mbrush29_submodel_t
{
	float		mins[3];
	float		maxs[3];
	float		origin[3];
	qint32		headnode[BSP29_MAX_MAP_HULLS_H2];
	qint32		visleafs;		// not including the solid leaf 0
	qint32		firstface;
	qint32		numfaces;
};

//==============================================================================
//
//	QUAKE 2 BRUSH MODELS
//
//==============================================================================

#define BRUSH38_VERTEXSIZE	7

#define BRUSH38_SURF_PLANEBACK		2
#define BRUSH38_SURF_DRAWSKY		4
#define BRUSH38_SURF_DRAWTURB		0x10
#define BRUSH38_SURF_UNDERWATER		0x80

struct mbrush38_vertex_t
{
	vec3_t		position;
};

struct mbrush38_edge_t
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
};

struct mbrush38_texinfo_t
{
	float		vecs[2][4];
	int			flags;
	int			numframes;
	mbrush38_texinfo_t	*next;		// animation chain
	image_t		*image;
};

struct mbrush38_glpoly_t
{
	mbrush38_glpoly_t*	next;
	mbrush38_glpoly_t*	chain;
	int		numverts;
	int		flags;			// for BRUSH29_SURF_UNDERWATER (not needed anymore?)
	float	verts[4][BRUSH38_VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
};

struct mbrush38_surface_t
{
	int			visframe;		// should be drawn when node is crossed

	cplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
	short		texturemins[2];
	short		extents[2];

	int			light_s, light_t;	// gl lightmap coordinates
	int			dlight_s, dlight_t; // gl lightmap coordinates for dynamic lightmaps

	mbrush38_glpoly_t	*polys;				// multiple if warped
	mbrush38_surface_t	*texturechain;
	mbrush38_surface_t	*lightmapchain;

	mbrush38_texinfo_t	*texinfo;
	
// lighting info
	int			dlightframe;
	int			dlightbits;

	int			lightmaptexturenum;
	byte		styles[BSP38_MAXLIGHTMAPS];
	float		cached_light[BSP38_MAXLIGHTMAPS];	// values currently used in lightmap
	byte		*samples;		// [numstyles*surfsize]
};

struct mbrush38_node_t
{
// common with leaf
	int			contents;		// -1, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	float		minmaxs[6];		// for bounding box culling

	mbrush38_node_t	*parent;

// node specific
	cplane_t	*plane;
	mbrush38_node_t	*children[2];	

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
};

struct mbrush38_leaf_t
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	mbrush38_node_t	*parent;

// leaf specific
	int			cluster;
	int			area;

	mbrush38_surface_t	**firstmarksurface;
	int			nummarksurfaces;
};

struct mbrush38_model_t
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	float		radius;
	int			headnode;
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
};

//==============================================================================
//
//	SURFACES
//
//==============================================================================

#define BRUSH46_VERTEXSIZE	8

#define MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file
#define MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
enum surfaceType_t
{
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MD3,
	SF_MD4,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
};

struct drawSurf_t
{
	unsigned			sort;			// bit combination for fast compares
	surfaceType_t*		surface;		// any of surface*_t
};

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
struct srfPoly_t
{
	surfaceType_t	surfaceType;
	qhandle_t		hShader;
	int				fogIndex;
	int				numVerts;
	polyVert_t*		verts;
};

struct srfDisplayList_t
{
	surfaceType_t	surfaceType;
	int				listNum;
};

struct srfFlare_t
{
	surfaceType_t	surfaceType;
	vec3_t			origin;
	vec3_t			normal;
	vec3_t			color;
};

struct srfGridMesh_t
{
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits[SMP_FRAMES];

	// culling information
	vec3_t			meshBounds[2];
	vec3_t			localOrigin;
	float			meshRadius;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float*			widthLodError;
	float*			heightLodError;
	bsp46_drawVert_t	verts[1];		// variable sized
};

struct srfSurfaceFace_t
{
	surfaceType_t	surfaceType;
	cplane_t	plane;

	// dynamic lighting information
	int			dlightBits[SMP_FRAMES];

	// triangle definitions (no normals at points)
	int			numPoints;
	int			numIndices;
	int			ofsIndices;
	float		points[1][BRUSH46_VERTEXSIZE];	// variable sized
										// there is a variable length list of indices here also
};

// misc_models in maps are turned into direct geometry by q3map
struct srfTriangles_t
{
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits[SMP_FRAMES];

	// culling information (FIXME: use this!)
	vec3_t			bounds[2];
	vec3_t			localOrigin;
	float			radius;

	// triangle definitions
	int				numIndexes;
	int				*indexes;

	int				numVerts;
	bsp46_drawVert_t		*verts;
};

//==============================================================================
//
//	QUAKE 3 BRUSH MODELS
//
//==============================================================================

#define BRUSH46_CONTENTS_NODE		-1

struct mbrush46_surface_t
{
	int						viewCount;		// if == tr.viewCount, already added
	shader_t*				shader;
	int						fogIndex;

	surfaceType_t*			data;			// any of srf*_t
};

struct mbrush46_node_t
{
	// common with leaf and node
	int						contents;		// -1 for nodes, to differentiate from leafs
	int						visframe;		// node needs to be traversed if current
	vec3_t					mins, maxs;		// for bounding box culling
	mbrush46_node_t*		parent;

	// node specific
	cplane_t*				plane;
	mbrush46_node_t*		children[2];	

	// leaf specific
	int						cluster;
	int						area;

	mbrush46_surface_t**	firstmarksurface;
	int						nummarksurfaces;
};

struct mbrush46_model_t
{
	vec3_t					bounds[2];		// for culling
	mbrush46_surface_t*		firstSurface;
	int						numSurfaces;
};

struct mbrush46_fog_t
{
	int			originalBrushNumber;
	vec3_t		bounds[2];

	unsigned	colorInt;				// in packed byte format
	float		tcScale;				// texture coordinate vector scales
	fogParms_t	parms;

	// for clipping distance in fog when outside
	qboolean	hasSurface;
	float		surface[4];
};

struct world_t
{
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2

	int			dataSize;

	int			numShaders;
	bsp46_dshader_t	*shaders;

	mbrush46_model_t	*bmodels;

	int			numplanes;
	cplane_t	*planes;

	int			numnodes;		// includes leafs
	int			numDecisionNodes;
	mbrush46_node_t		*nodes;

	int			numsurfaces;
	mbrush46_surface_t	*surfaces;

	int			nummarksurfaces;
	mbrush46_surface_t	**marksurfaces;

	int			numfogs;
	mbrush46_fog_t		*fogs;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];
	byte		*lightGridData;


	int			numClusters;
	int			clusterBytes;
	const byte	*vis;			// may be passed in by CM_LoadMap to save space

	byte		*novis;			// clusterBytes of 0xff

	char		*entityString;
	char		*entityParsePoint;
};

//==============================================================================
//
//	QUAKE MESH MODELS
//
//==============================================================================

#define MAX_MESH1_SKINS		32

struct mmesh1framedesc_t
{
	int					firstpose;
	int					numposes;
	float				interval;
	dmdl_trivertx_t		bboxmin;
	dmdl_trivertx_t		bboxmax;
	int					frame;
	char				name[16];
};

struct mmesh1groupframedesc_t
{
	dmdl_trivertx_t		bboxmin;
	dmdl_trivertx_t		bboxmax;
	int					frame;
};

struct mmesh1group_t
{
	int						numframes;
	int						intervals;
	mmesh1groupframedesc_t	frames[1];
};

struct mmesh1triangle_t
{
	int					facesfront;
	int					vertindex[3];
	int					stindex[3];
};

struct mesh1hdr_t
{
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
	int			flags;
	float		size;

	int					numposes;
	int					poseverts;
	dmdl_trivertx_t*	posedata;	// numposes*poseverts trivert_t
	int*				commands;	// gl command list with embedded s/t
	image_t*			gl_texture[MAX_MESH1_SKINS][4];
	mmesh1framedesc_t	frames[1];	// variable sized
};

//==============================================================================
//
//	QUAKE SPRITE MODELS
//
//==============================================================================

struct msprite1frame_t
{
	int			width;
	int			height;
	float		up, down, left, right;
	image_t*	gl_texture;
};

struct msprite1group_t
{
	int					numframes;
	float*				intervals;
	msprite1frame_t*	frames[1];
};

struct msprite1framedesc_t
{
	sprite1frametype_t	type;
	msprite1frame_t		*frameptr;
};

struct msprite1_t
{
	int					type;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	float				beamlength;		// remove?
	msprite1framedesc_t	frames[1];
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
	MOD_MD4
};

struct model_t
{
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;				// model = tr.models[model->index]

	qboolean	q1_needload;		// bmodels and sprites don't cache normally

	int			q1_numframes;
	synctype_t	q1_synctype;
	
	int			q1_flags;

//
// volume occupied by the model graphics
//		
	vec3_t		q1_mins, q1_maxs;
	float		q1_radius;

//
// Quake brush model
//
	int			brush29_firstmodelsurface;
	int			brush29_nummodelsurfaces;

	int			brush29_numsubmodels;
	mbrush29_submodel_t*	brush29_submodels;

	int			brush29_numplanes;
	cplane_t*	brush29_planes;

	int			brush29_numleafs;		// number of visible leafs, not counting 0
	mbrush29_leaf_t		*brush29_leafs;

	int			brush29_numvertexes;
	mbrush29_vertex_t	*brush29_vertexes;

	int			brush29_numedges;
	mbrush29_edge_t		*brush29_edges;

	int			brush29_numnodes;
	mbrush29_node_t		*brush29_nodes;

	int			brush29_numtexinfo;
	mbrush29_texinfo_t	*brush29_texinfo;

	int			brush29_numsurfaces;
	mbrush29_surface_t	*brush29_surfaces;

	int			brush29_numsurfedges;
	int			*brush29_surfedges;

	int			brush29_numclipnodes;
	bsp29_dclipnode_t	*brush29_clipnodes;

	int			brush29_nummarksurfaces;
	mbrush29_surface_t	**brush29_marksurfaces;

	mbrush29_hull_t		brush29_hulls[BSP29_MAX_MAP_HULLS_H2];

	int			brush29_numtextures;
	mbrush29_texture_t	**brush29_textures;

	byte		*brush29_visdata;
	byte		*brush29_lightdata;
	char		*brush29_entities;

//
// additional model data
//
	void*		q1_cache;		// only access through Mod_Extradata

	int			q2_registration_sequence;

	int			q2_numframes;
	
	int			q2_flags;

//
// volume occupied by the model graphics
//		
	vec3_t		q2_mins, q2_maxs;
	float		q2_radius;

//
// Quake 2 brush model
//
	int			brush38_firstmodelsurface, brush38_nummodelsurfaces;
	int			brush38_lightmap;		// only for submodels

	int			brush38_numsubmodels;
	mbrush38_model_t	*brush38_submodels;

	int			brush38_numplanes;
	cplane_t	*brush38_planes;

	int			brush38_numleafs;		// number of visible leafs, not counting 0
	mbrush38_leaf_t		*brush38_leafs;

	int			brush38_numvertexes;
	mbrush38_vertex_t	*brush38_vertexes;

	int			brush38_numedges;
	mbrush38_edge_t		*brush38_edges;

	int			brush38_numnodes;
	int			brush38_firstnode;
	mbrush38_node_t		*brush38_nodes;

	int			brush38_numtexinfo;
	mbrush38_texinfo_t	*brush38_texinfo;

	int			brush38_numsurfaces;
	mbrush38_surface_t	*brush38_surfaces;

	int			brush38_numsurfedges;
	int			*brush38_surfedges;

	int			brush38_nummarksurfaces;
	mbrush38_surface_t	**brush38_marksurfaces;

	bsp38_dvis_t		*brush38_vis;

	byte		*brush38_lightdata;

	// for alias models and skins
	image_t		*q2_skins[MAX_MD2_SKINS];

	int			q2_extradatasize;
	void		*q2_extradata;

	int			q3_dataSize;			// just for listing purposes
	mbrush46_model_t	*q3_bmodel;			// only if type == MOD_BRUSH
	md3Header_t	*q3_md3[MD3_MAX_LODS];	// only if type == MOD_MESH
	md4Header_t	*q3_md4;				// only if type == MOD_MD4

	int			 q3_numLods;
};

void R_FreeModel(model_t* mod);
void R_FreeModels();

void Mod_LoadSpriteModel(model_t* mod, void* buffer);
void Mod_FreeSpriteModel(model_t* mod);
void Mod_LoadSprite2Model(model_t* mod, void* buffer, int modfilelen);
void Mod_FreeSprite2Model(model_t* mod);
void Mod_LoadMdlModel(model_t* mod, const void* buffer);
void Mod_LoadMdlModelNew(model_t* mod, const void* buffer);
void Mod_FreeMdlModel(model_t* mod);
void Mod_LoadMd2Model(model_t* mod, const void* buffer);
void Mod_FreeMd2Model(model_t* mod);
bool R_LoadMd3(model_t* mod, void* buffer);
void R_FreeMd3(model_t* mod);
bool R_LoadMD4(model_t* mod, void* buffer, const char* mod_name);
void R_FreeMd4(model_t* mod);
void Mod_LoadBrush29Model(model_t* mod, void* buffer);
void Mod_FreeBsp29(model_t* mod);
void Mod_LoadBrush38Model(model_t* mod, void* buffer);
void Mod_FreeBsp38(model_t* mod);
void R_LoadBrush46Model(void* buffer);
void R_FreeBsp46(world_t* mod);
void R_FreeBsp46Model(model_t* mod);

//
//	CURVE TESSELATION
//
srfGridMesh_t* R_SubdividePatchToGrid(int Width, int Height,
	bsp46_drawVert_t Points[MAX_PATCH_SIZE*MAX_PATCH_SIZE]);
srfGridMesh_t* R_GridInsertColumn(srfGridMesh_t* Grid, int Column, int Row, vec3_t Point, float LodError);
srfGridMesh_t* R_GridInsertRow(srfGridMesh_t* Grid, int Row, int Column, vec3_t Point, float LodError);
void R_FreeSurfaceGridMesh(srfGridMesh_t* Grid);

extern model_t*	currentmodel;
extern model_t*	loadmodel;
extern byte		q1_player_8bit_texels[320 * 200];
extern byte		h2_player_8bit_texels[6][620 * 245];
extern mbrush29_texture_t	*r_notexture_mip;
extern world_t		s_worldData;
