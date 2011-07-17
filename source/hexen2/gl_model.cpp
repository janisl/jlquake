// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

/*
 * $Header: /H2 Mission Pack/gl_model.c 11    3/16/98 4:38p Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	R_FreeModels();
	R_ModelInit();
}
