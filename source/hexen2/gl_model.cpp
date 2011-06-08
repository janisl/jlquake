// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

/*
 * $Header: /H2 Mission Pack/gl_model.c 11    3/16/98 4:38p Jmonroe $
 */

#include "quakedef.h"
#include "glquake.h"

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	R_ModelInit();
}

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
	Mod_Init();
}

/*
==================
R_RegisterModel

Loads in a model for the given name
==================
*/
qhandle_t Mod_LoadWorld(const char *name)
{
	if (!name[0])
		Sys_Error ("R_RegisterModel: NULL name");
		
	model_t* mod = R_AllocModel();
	QStr::Cpy(mod->name, name);

	
	unsigned *buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

//
// load the file
//
	buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		return 0;
	}
	
//
// allocate a new model
//
	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	Mod_LoadBrush29Model (mod, buf);

	tr.worldModel = mod;
	return mod->index;
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

void Mod_PrintFrameName(qhandle_t m, int frame)
{
	mesh1hdr_t 			*hdr;
	mmesh1framedesc_t	*pframedesc;

	hdr = (mesh1hdr_t *)Mod_Extradata(R_GetModelByHandle(m));
	if (!hdr)
		return;
	pframedesc = &hdr->frames[frame];
	
	Con_Printf ("frame %i: %s\n", frame, pframedesc->name);
}

bool Mod_IsAliasModel(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->type == MOD_MESH1;
}

const char* Mod_GetName(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->name;
}

int Mod_GetSyncType(qhandle_t Handle)
{
	return R_GetModelByHandle(Handle)->q1_synctype;
}
