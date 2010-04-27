// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

/*
 * $Header: /H2 Mission Pack/gl_model.c 11    3/16/98 4:38p Jmonroe $
 */

#include "quakedef.h"

#define HEXEN2_HULLS
#include "../quake/cmodel_shared.cpp"
