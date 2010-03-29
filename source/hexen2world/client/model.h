
#ifndef __MODEL__
#define __MODEL__

#include "genmodel.h"
#include "spritegn.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

// entity effects
#define EF_ONFIRE				0x00000001
#define	EF_MUZZLEFLASH 			0x00000002
#define	EF_BRIGHTLIGHT 			0x00000004
#define	EF_DIMLIGHT 			0x00000008
#define EF_DARKLIGHT			0x00000010
#define EF_DARKFIELD			0x00000020
#define EF_LIGHT				0x00000040
#define EF_NODRAW				0x00000080

#define	EF_BRIGHTFIELD			0x00000400
#define EF_POWERFLAMEBURN		0x00000800
#define EF_UPDATESOUND			0x00002000

#define EF_POISON_GAS			0x00200000
#define EF_ACIDBLOB				0x00400000
//#define EF_PURIFY2_EFFECT		0x00200000
//#define EF_AXE_EFFECT			0x00400000
//#define EF_SWORD_EFFECT		0x00800000
//#define EF_TORNADO_EFFECT		0x01000000
#define EF_ICESTORM_EFFECT		0x02000000
//#define EF_ICEBALL_EFFECT		0x04000000
//#define EF_METEOR_EFFECT		0x08000000
#define EF_HAMMER_EFFECTS		0x10000000
#define EF_BEETLE_EFFECTS		0x20000000

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


// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;

typedef struct texture_s
{
	char		name[16];
	unsigned	width, height;
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s *anim_next;		// in the animation sequence
	struct texture_s *alternate_anims;	// bmodels in frmae 1 use these
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} texture_t;


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_TRANSLUCENT	0x80


#define SURF_DRAWBLACK		0x400

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

typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed

	int			dlightframe;
	int			dlightbits;

	mplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
// surface generation data
	struct surfcache_s	*cachespots[MIPLEVELS];

	short		texturemins[2];
	short		extents[2];

	mtexinfo_t	*texinfo;
	
// lighting info
	byte		styles[MAXLIGHTMAPS];
	byte		*samples;		// [numstyles*surfsize]
} msurface_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	short		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// node specific
	mplane_t	*plane;
	struct mnode_s	*children[2];	

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	short		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	byte		*compressed_vis;
	struct efrag_s	*efrags;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	byte		ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	dclipnode_t	*clipnodes;
	mplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
} hull_t;

#define HULL_IMPLICIT		0	//Choose the hull based on bounding box- like in Quake
#define HULL_POINT			1	//0 0 0, 0 0 0
#define HULL_PLAYER			2	//'-16 -16 0', '16 16 56'
#define HULL_SCORPION		3	//'-24 -24 -20', '24 24 20'
#define HULL_CROUCH			4	//'-16 -16 0', '16 16 28'
#define HULL_HYDRA			5	//'-28 -28 -24', '28 28 24'
#define HULL_GOLEM			6	//???,???

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s
{
	short	width;
	short	height;
//	void	*pcachespot;			// remove?
	float	up, down, left, right;
	byte	pixels[4];
} mspriteframe_t;

typedef struct
{
	short			numframes;
	float			*intervals;
	mspriteframe_t	*frames[1];
} mspritegroup_t;

typedef struct
{
	spriteframetype_t	type;
	mspriteframe_t		*frameptr;
} mspriteframedesc_t;

typedef struct
{
	short				type;
	short				maxwidth;
	short				maxheight;
	short				numframes;
	float				beamlength;		// remove?
//	void				*cachespot;		// remove?
	mspriteframedesc_t	frames[1];
} msprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct
{
	aliasframetype_t	type;
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
	char				name[16];
} maliasframedesc_t;

typedef struct
{
	aliasskintype_t		type;
	void				*pcachespot;
	int					skin;
} maliasskindesc_t;

typedef struct
{
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
} maliasgroupframedesc_t;

typedef struct
{
	int						numframes;
	int						intervals;
	maliasgroupframedesc_t	frames[1];
} maliasgroup_t;

typedef struct
{
	int					numskins;
	int					intervals;
	maliasskindesc_t	skindescs[1];
} maliasskingroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s {
	int					facesfront;
	unsigned short		vertindex[3];
	unsigned short		stindex[3];
} mtriangle_t;

typedef struct {
	int					model;
	int					stverts;
	int					skindesc;
	int					triangles;
	maliasframedesc_t	frames[1];
} aliashdr_t;

//===================================================================

//
// Whole model
//

typedef enum {mod_brush, mod_sprite, mod_alias} modtype_t;

// EF_ changes must also be made in gl_model.h
#define	EF_ROCKET		       1			// leave a trail
#define	EF_GRENADE		       2			// leave a trail
#define	EF_GIB			       4			// leave a trail
#define	EF_ROTATE		       8			// rotate (bonus items)
#define	EF_TRACER		      16			// green split trail
#define	EF_ZOMGIB			  32			// small blood trail
#define	EF_TRACER2			  64			// orange split trail + rotate
#define	EF_TRACER3			 128			// purple trail
#define  EF_FIREBALL		 256			// Yellow transparent trail in all directions
#define  EF_ICE				 512			// Blue-white transparent trail, with gravity
#define  EF_MIP_MAP			1024			// This model has mip-maps
#define  EF_SPIT			2048			// Black transparent trail with negative light
#define  EF_TRANSPARENT		4096		    // Transparent sprite
#define  EF_SPELL			8192			// Vertical spray of particles
#define  EF_HOLEY		   16384			// Solid model with color 0
#define  EF_SPECIAL_TRANS  32768			// Translucency through the particle table
#define  EF_FACE_VIEW	   65536			// Poly Model always faces you
#define  EF_VORP_MISSILE  131072			// leave a trail at top and bottom of model
#define  EF_SET_STAFF     262144			// slowly move up and left/right
#define  EF_MAGICMISSILE  524288            // a trickle of blue/white particles with gravity
#define  EF_BONESHARD    1048576           // a trickle of brown particles with gravity
#define  EF_SCARAB       2097152           // white transparent particles with little gravity
#define  EF_ACIDBALL	 4194304			// Green drippy acid shit
#define  EF_BLOODSHOT	 8388608			// Blood rain shot trail

#define  EF_MIP_MAP_FAR	  0x1000000	// Set per frame, this model will use the far mip map


typedef struct model_s
{
	char		name[MAX_QPATH];
	qboolean	needload;		// bmodels and sprites don't cache normally

	modtype_t	type;
	int			numframes;
	synctype_t	synctype;
	
	int			flags;

//
// volume occupied by the model graphics
//		
	vec3_t		mins, maxs;
	float		radius;

//
// solid volume for clipping (sent from server)
//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	dmodel_t	*submodels;

	int			numplanes;
	mplane_t	*planes;

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
	dclipnode_t	*clipnodes;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	hull_t		hulls[MAX_MAP_HULLS];

	int			numtextures;
	texture_t	**textures;

	byte		*visdata;
	byte		*lightdata;
	char		*entities;

//
// additional model data
//
	cache_user_t	cache;		// only access through Mod_Extradata

} model_t;

//============================================================================

void	Mod_Init (void);
void	Mod_ClearAll (void);
model_t *Mod_ForName (char *name, qboolean crash);
void	*Mod_Extradata (model_t *mod);	// handles caching
void	Mod_TouchModel (char *name);

mleaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_LeafPVS (mleaf_t *leaf, model_t *model);

#endif	// __MODEL__
