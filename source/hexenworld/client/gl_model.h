
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
};

//============================================================================

void	*Mod_Extradata (model_t *mod);	// handles caching

mbrush29_leaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_LeafPVS (mbrush29_leaf_t *leaf, model_t *model);

model_t* Mod_GetModel(qhandle_t handle);

#endif	// __MODEL__
