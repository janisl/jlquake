/*
 * $Header: /H2 Mission Pack/gl_model.h 7     3/12/98 1:12p Jmonroe $
 */

#ifndef __MODEL__
#define __MODEL__

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

// entity effects

#define	EF_BRIGHTFIELD			1
#define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vec3_t		position;
} mvertex_t;

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


typedef struct texture_s
{
	char		name[16];
	unsigned	width, height;
	image_t*	gl_texture;
	struct msurface_s	*texturechain;	// for gl_texsort drawing
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s *anim_next;		// in the animation sequence
	struct texture_s *alternate_anims;	// bmodels in frmae 1 use these
	unsigned	offsets[BSP29_MIPLEVELS];		// four mip maps stored
} texture_t;


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80
#define SURF_TRANSLUCENT	0x100
#define SURF_DRAWBLACK		0x200

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct
{
	float		vecs[2][4];
	float		mipadjust;
	texture_t	*texture;
	int			flags;
} mtexinfo_t;

#define	VERTEXSIZE	7

typedef struct glpoly_s
{
	struct	glpoly_s	*next;
	struct	glpoly_s	*chain;
	int		numverts;
	int		flags;			// for SURF_UNDERWATER
	float	verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
} glpoly_t;

typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed

	cplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
	short		texturemins[2];
	short		extents[2];

	int			light_s, light_t;	// gl lightmap coordinates

	glpoly_t	*polys;				// multiple if warped
	struct	msurface_s	*texturechain;

	mtexinfo_t	*texinfo;
	
// lighting info
	int			dlightframe;
	int			dlightbits;

	int			lightmaptexturenum;
	byte		styles[BSP29_MAXLIGHTMAPS];
	int			cached_light[BSP29_MAXLIGHTMAPS];	// values currently used in lightmap
	qboolean	cached_dlight;				// true if dynamic light in cache
	byte		*samples;		// [numstyles*surfsize]
} msurface_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];	

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	byte		*compressed_vis;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	byte		ambient_sound_level[BSP29_NUM_AMBIENTS];
} mleaf_t;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	bsp29_dclipnode_t	*clipnodes;
	cplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
} hull_t;

/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

#define	MAXALIASVERTS	2000
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
	bsp29_dmodel_h2_t	*submodels;

	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			numclipnodes;
	bsp29_dclipnode_t	*clipnodes;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	hull_t		hulls[BSP29_MAX_MAP_HULLS_H2];

	int			numtextures;
	texture_t	**textures;

	byte		*visdata;
	byte		*lightdata;
	char		*entities;

//
// additional model data
//
	cache_user_t	cache;		// only access through Mod_Extradata

};

//============================================================================

void	*Mod_Extradata (model_t *mod);	// handles caching

mleaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_LeafPVS (mleaf_t *leaf, model_t *model);

model_t* Mod_GetModel(qhandle_t handle);

#endif	// __MODEL__
