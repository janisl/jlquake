/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

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
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ( ( ent->e.frame >= tr.currentModel->q3_md3[0]->numFrames )
		 || ( ent->e.frame < 0 )
		 || ( ent->e.oldframe >= tr.currentModel->q3_md3[0]->numFrames )
		 || ( ent->e.oldframe < 0 ) ) {
		ri.Printf( PRINT_DEVELOPER, "R_AddMD3Surfaces: no such frame %d to %d for '%s'\n",
				   ent->e.oldframe, ent->e.frame,
				   tr.currentModel->name );
		ent->e.frame = 0;
		ent->e.oldframe = 0;
	}

	//
	// compute LOD
	//
	lod = R_ComputeLOD( ent );

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
		int j;

//----(SA)	blink will change to be an overlay rather than replacing the head texture.
//		think of it like batman's mask.  the polygons that have eye texture are duplicated
//		and the 'lids' rendered with polygonoffset shader parm over the top of the open eyes.  this gives
//		minimal overdraw/alpha blending/texture use without breaking the model and causing seams
		if ( !String::ICmp( surface->name, "h_blink" ) ) {
			if ( !( ent->e.renderfx & RF_BLINK ) ) {
				surface = ( md3Surface_t * )( (byte *)surface + surface->ofsEnd );
				continue;
			}
		}
//----(SA)	end

		if ( ent->e.customShader ) {
			shader = R_GetShaderByHandle( ent->e.customShader );
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t *skin;

			skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader = tr.defaultShader;
			for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
				// the names have both been lowercased
				if ( !String::Cmp( skin->surfaces[j]->name, surface->name ) ) {
					shader = skin->surfaces[j]->shader;
					break;
				}
			}

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
// GR - tessellate according to model capabilities
			R_AddDrawSurf( (surfaceType_t *)surface, tr.shadowShader, 0, qfalse, 0, tr.currentModel->q3_ATI_tess );
		}

		// projection shadows work fine with personal models
//		if ( r_shadows->integer == 3
//			&& fogNum == 0
//			&& (ent->e.renderfx & RF_SHADOW_PLANE )
//			&& shader->sort == SS_OPAQUE ) {
//			R_AddDrawSurf( (void *)surface, tr.projectionShadowShader, 0, qfalse );
//		}

		// don't add third_person objects if not viewing through a portal
		if ( !personalModel ) {
// GR - tessellate according to model capabilities
			R_AddDrawSurf( (surfaceType_t*)surface, shader, fogNum, qfalse, 0, tr.currentModel->q3_ATI_tess );
		}

		surface = ( md3Surface_t * )( (byte *)surface + surface->ofsEnd );
	}

}

