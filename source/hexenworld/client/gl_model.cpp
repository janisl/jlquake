// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "glquake.h"

/*
===============
Mod_Init

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	return mod->q1_cache;
}

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

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	Con_Printf ("Cached models:\n");
	for (int i = 0; i < tr.numModels; i++)
	{
		Con_Printf ("%8p : %s\n", tr.models[i]->q1_cache, tr.models[i]->name);
	}
}

void Mod_CalcScaleOffset(qhandle_t Handle, float ScaleX, float ScaleY, float ScaleZ, float ScaleZOrigin, vec3_t Out)
{
	model_t* Model = R_GetModelByHandle(Handle);
	if (Model->type != MOD_MESH1)
	{
		throw QException("Not an alias model");
	}

	mesh1hdr_t* AliasHdr = (mesh1hdr_t*)Mod_Extradata(Model);

	Out[0] = -(ScaleX - 1.0) * (AliasHdr->scale[0] * 127.95 + AliasHdr->scale_origin[0]);
	Out[1] = -(ScaleY - 1.0) * (AliasHdr->scale[1] * 127.95 + AliasHdr->scale_origin[1]);
	Out[2] = -(ScaleZ - 1.0) * (ScaleZOrigin * 2.0 * AliasHdr->scale[2] * 127.95 + AliasHdr->scale_origin[2]);
}

int Mod_GetNumFrames(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->q1_numframes;
}

int Mod_GetFlags(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->q1_flags;
}

bool Mod_IsAliasModel(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->type == MOD_MESH1;
}

const char* Mod_GetName(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->name;
}
