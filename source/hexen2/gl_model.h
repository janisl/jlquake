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

//============================================================================

void	*Mod_Extradata (model_t *mod);	// handles caching

#endif	// __MODEL__
