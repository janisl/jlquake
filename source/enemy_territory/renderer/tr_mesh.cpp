/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// tr_mesh.c: triangle model functions
//
// !!! NOTE: Any changes made here must be duplicated in tr_cmesh.c for MDC support

#include "tr_local.h"

float ProjectRadius( float r, vec3_t location );
int R_CullModel( md3Header_t *header, trRefEntity_t *ent );
int R_ComputeLOD( trRefEntity_t *ent );
int R_ComputeFogNum( md3Header_t *header, trRefEntity_t *ent );

/*
=================
R_AddMD3Surfaces

=================
*/
void R_AddMD3Surfaces( trRefEntity_t *ent ) {
	int i;
	md3Header_t     *header = 0;
	md3Surface_t    *surface = 0;
	md3Shader_t     *md3Shader = 0;
	shader_t        *shader = 0;
	int cull;
	int lod;
	int fogNum;
	qboolean personalModel;

	// don't add third_person objects if not in a portal
	personalModel = ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal;

	if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
		ent->e.frame %= tr.currentModel->q3_md3[0]->numFrames;
		ent->e.oldframe %= tr.currentModel->q3_md3[0]->numFrames;
	}

	//
	// compute LOD
	//
	if ( ent->e.renderfx & RF_FORCENOLOD ) {
		lod = 0;
	} else {
		lod = R_ComputeLOD( ent );
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ( ( ent->e.frame >= tr.currentModel->q3_md3[lod]->numFrames )
		 || ( ent->e.frame < 0 )
		 || ( ent->e.oldframe >= tr.currentModel->q3_md3[lod]->numFrames )
		 || ( ent->e.oldframe < 0 ) ) {
		ri.Printf( PRINT_DEVELOPER, "R_AddMD3Surfaces: no such frame %d to %d for '%s' (%d)\n",
				   ent->e.oldframe, ent->e.frame,
				   tr.currentModel->name,
				   tr.currentModel->q3_md3[ lod ]->numFrames );
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	header = tr.currentModel->q3_md3[lod];

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullModel( header, ent );
	if ( cull == CULL_OUT ) {
		return;
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel || r_shadows->integer > 1 ) {
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	//
	// see if we are in a fog volume
	//
	fogNum = R_ComputeFogNum( header, ent );

	//
	// draw all surfaces
	//
	surface = ( md3Surface_t * )( (byte *)header + header->ofsSurfaces );
	for ( i = 0 ; i < header->numSurfaces ; i++ ) {

		if ( ent->e.customShader ) {
			shader = R_GetShaderByHandle( ent->e.customShader );
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t *skin;
			int hash;
			int j;

			skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader = tr.defaultShader;

//----(SA)	added blink
			if ( ent->e.renderfx & RF_BLINK ) {
				char *s = va( "%s_b", surface->name ); // append '_b' for 'blink'
				hash = Com_HashKey( s, String::Length( s ) );
				for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
					if ( hash != skin->surfaces[j]->hash ) {
						continue;
					}
					if ( !String::Cmp( skin->surfaces[j]->name, s ) ) {
						shader = skin->surfaces[j]->shader;
						break;
					}
				}
			}

			if ( shader == tr.defaultShader ) {    // blink reference in skin was not found
				hash = Com_HashKey( surface->name, sizeof( surface->name ) );
				for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
					// the names have both been lowercased
					if ( hash != skin->surfaces[j]->hash ) {
						continue;
					}
					if ( !String::Cmp( skin->surfaces[j]->name, surface->name ) ) {
						shader = skin->surfaces[j]->shader;
						break;
					}
				}
			}
//----(SA)	end

			if ( shader == tr.defaultShader ) {
				ri.Printf( PRINT_DEVELOPER, "WARNING: no shader for surface %s in skin %s\n", surface->name, skin->name );
			} else if ( shader->defaultShader )     {
				ri.Printf( PRINT_DEVELOPER, "WARNING: shader %s in skin %s not found\n", shader->name, skin->name );
			}
		} else if ( surface->numShaders <= 0 ) {
			shader = tr.defaultShader;
		} else {
			md3Shader = ( md3Shader_t * )( (byte *)surface + surface->ofsShaders );
			md3Shader += ent->e.skinNum % surface->numShaders;
			shader = tr.shaders[ md3Shader->shaderIndex ];
		}


		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel
			 && r_shadows->integer == 2
			 && fogNum == 0
			 && !( ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			 && shader->sort == SS_OPAQUE ) {
			R_AddDrawSurf( (surfaceType_t*)surface, tr.shadowShader, 0, 0, 0, 0 );
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
			 && fogNum == 0
			 && ( ent->e.renderfx & RF_SHADOW_PLANE )
			 && shader->sort == SS_OPAQUE ) {
			R_AddDrawSurf( (surfaceType_t*)surface, tr.projectionShadowShader, 0, 0, 0, 0 );
		}


		// for testing polygon shadows (on /all/ models)
		if ( r_shadows->integer == 4 ) {
			R_AddDrawSurf( (surfaceType_t*)surface, tr.projectionShadowShader, 0, 0, 0, 0 );
		}


		// don't add third_person objects if not viewing through a portal
		if ( !personalModel ) {
			R_AddDrawSurf( (surfaceType_t*)surface, shader, fogNum, 0, 0, 0 );
		}

		surface = ( md3Surface_t * )( (byte *)surface + surface->ofsEnd );
	}
}

