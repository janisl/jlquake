// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"

#define HEXENWORLD_HULLS
#include "../../quake/cmodel_shared.cpp"
