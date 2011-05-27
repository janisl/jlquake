//**************************************************************************
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

#define	LL(x) x=LittleLong(x)

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_LoadMD3
//
//==========================================================================

bool R_LoadMD3(model_t* mod, int lod, const void* buffer, const char* mod_name)
{
	int					i, j;
	md3Header_t			*pinmodel;
    md3Frame_t			*frame;
	md3Surface_t		*surf;
	md3Shader_t			*shader;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	md3Tag_t			*tag;
	int					version;
	int					size;

	pinmodel = (md3Header_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MD3_VERSION) {
		GLog.Write(S_COLOR_YELLOW "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
		return false;
	}

	mod->type = MOD_MESH3;
	size = LittleLong(pinmodel->ofsEnd);
	mod->q3_dataSize += size;
	mod->q3_md3[lod] = (md3Header_t*)Mem_Alloc(size);

	Com_Memcpy (mod->q3_md3[lod], buffer, LittleLong(pinmodel->ofsEnd) );

    LL(mod->q3_md3[lod]->ident);
    LL(mod->q3_md3[lod]->version);
    LL(mod->q3_md3[lod]->numFrames);
    LL(mod->q3_md3[lod]->numTags);
    LL(mod->q3_md3[lod]->numSurfaces);
    LL(mod->q3_md3[lod]->ofsFrames);
    LL(mod->q3_md3[lod]->ofsTags);
    LL(mod->q3_md3[lod]->ofsSurfaces);
    LL(mod->q3_md3[lod]->ofsEnd);

	if ( mod->q3_md3[lod]->numFrames < 1 ) {
		GLog.Write(S_COLOR_YELLOW "R_LoadMD3: %s has no frames\n", mod_name );
		return false;
	}
    
	// swap all the frames
    frame = (md3Frame_t *) ( (byte *)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsFrames );
    for ( i = 0 ; i < mod->q3_md3[lod]->numFrames ; i++, frame++) {
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
        }
	}

	// swap all the tags
    tag = (md3Tag_t *) ( (byte *)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsTags );
    for ( i = 0 ; i < mod->q3_md3[lod]->numTags * mod->q3_md3[lod]->numFrames ; i++, tag++) {
        for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
        }
	}

	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)mod->q3_md3[lod] + mod->q3_md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->q3_md3[lod]->numSurfaces ; i++) {

        LL(surf->ident);
        LL(surf->flags);
        LL(surf->numFrames);
        LL(surf->numShaders);
        LL(surf->numTriangles);
        LL(surf->ofsTriangles);
        LL(surf->numVerts);
        LL(surf->ofsShaders);
        LL(surf->ofsSt);
        LL(surf->ofsXyzNormals);
        LL(surf->ofsEnd);
		
		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			throw QDropException(va("R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts));
		}
		if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
			throw QDropException(va("R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles));
		}
	
		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		QStr::ToLower( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = QStr::Length( surf->name );
		if ( j > 2 && surf->name[j-2] == '_' ) {
			surf->name[j-2] = 0;
		}

        // register the shaders
        shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );
        for ( j = 0 ; j < surf->numShaders ; j++, shader++ ) {
            shader_t	*sh;

            sh = R_FindShader( shader->name, LIGHTMAP_NONE, true );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
        }

		// swap all the triangles
		tri = (md3Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
        st = (md3St_t *) ( (byte *)surf + surf->ofsSt );
        for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
            st->st[0] = LittleFloat( st->st[0] );
            st->st[1] = LittleFloat( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ ) 
		{
            xyz->xyz[0] = LittleShort( xyz->xyz[0] );
            xyz->xyz[1] = LittleShort( xyz->xyz[1] );
            xyz->xyz[2] = LittleShort( xyz->xyz[2] );

            xyz->normal = LittleShort( xyz->normal );
        }


		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
    
	return true;
}
