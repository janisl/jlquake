
#ifndef __MODEL__
#define __MODEL__

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

//============================================================================

void	*Mod_Extradata (model_t *mod);	// handles caching

mbrush29_leaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_LeafPVS (mbrush29_leaf_t *leaf, model_t *model);

#endif	// __MODEL__
