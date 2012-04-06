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


#include "tr_local.h"
#include "../../wolfclient/renderer/models/skeletal_model_inlines.h"


/*
=============
R_CullModel
=============
*/
static int R_CullModel( trRefEntity_t *ent ) {
	vec3_t bounds[2];
	mdxHeader_t *oldFrameHeader, *newFrameHeader;
	mdxFrame_t  *oldFrame, *newFrame;
	int i;

	newFrameHeader = R_GetModelByHandle( ent->e.frameModel )->q3_mdx;
	oldFrameHeader = R_GetModelByHandle( ent->e.oldframeModel )->q3_mdx;

	if ( !newFrameHeader || !oldFrameHeader ) {
		return CULL_OUT;
	}

	// compute frame pointers
	newFrame = ( mdxFrame_t * )( ( byte * ) newFrameHeader + newFrameHeader->ofsFrames +
								 ent->e.frame * (int) ( sizeof( mdxBoneFrameCompressed_t ) ) * newFrameHeader->numBones +
								 ent->e.frame * sizeof( mdxFrame_t ) );
	oldFrame = ( mdxFrame_t * )( ( byte * ) oldFrameHeader + oldFrameHeader->ofsFrames +
								 ent->e.oldframe * (int) ( sizeof( mdxBoneFrameCompressed_t ) ) * oldFrameHeader->numBones +
								 ent->e.oldframe * sizeof( mdxFrame_t ) );

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes ) {
		if ( ent->e.frame == ent->e.oldframe && ent->e.frameModel == ent->e.oldframeModel ) {
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
			case CULL_OUT:
				tr.pc.c_sphere_cull_md3_out++;
				return CULL_OUT;

			case CULL_IN:
				tr.pc.c_sphere_cull_md3_in++;
				return CULL_IN;

			case CULL_CLIP:
				tr.pc.c_sphere_cull_md3_clip++;
				break;
			}
		} else
		{
			int sphereCull, sphereCullB;

			sphereCull  = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			if ( newFrame == oldFrame ) {
				sphereCullB = sphereCull;
			} else {
				sphereCullB = R_CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
			}

			if ( sphereCull == sphereCullB ) {
				if ( sphereCull == CULL_OUT ) {
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				} else if ( sphereCull == CULL_IN )   {
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				} else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}

	// calculate a bounding box in the current coordinate system
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch ( R_CullLocalBox( bounds ) )
	{
	case CULL_IN:
		tr.pc.c_box_cull_md3_in++;
		return CULL_IN;
	case CULL_CLIP:
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md3_out++;
		return CULL_OUT;
	}
}

/*
=================
R_ComputeFogNum

=================
*/
static int R_ComputeFogNum( trRefEntity_t *ent ) {
	int i, j;
	mbrush46_fog_t           *fog;
	mdxHeader_t     *header;
	mdxFrame_t      *mdxFrame;
	vec3_t localOrigin;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	header = R_GetModelByHandle( ent->e.frameModel )->q3_mdx;

	// compute frame pointers
	mdxFrame = ( mdxFrame_t * )( ( byte * ) header + header->ofsFrames +
								 ent->e.frame * (int) ( sizeof( mdxBoneFrameCompressed_t ) ) * header->numBones +
								 ent->e.frame * sizeof( mdxFrame_t ) );

	// FIXME: non-normalized axis issues
	VectorAdd( ent->e.origin, mdxFrame->localOrigin, localOrigin );
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( localOrigin[j] - mdxFrame->radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( localOrigin[j] + mdxFrame->radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

/*
==============
R_MDM_AddAnimSurfaces
==============
*/
void R_MDM_AddAnimSurfaces( trRefEntity_t *ent ) {
	mdmHeader_t     *header;
	mdmSurface_t    *surface;
	shader_t        *shader = 0;
	int i, fogNum, cull;
	qboolean personalModel;

	// don't add third_person objects if not in a portal
	personalModel = ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal;

	header = tr.currentModel->q3_mdm;

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullModel( ent );
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
	fogNum = R_ComputeFogNum( ent );

	surface = ( mdmSurface_t * )( (byte *)header + header->ofsSurfaces );
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

			if ( shader == tr.defaultShader ) {  // blink reference in skin was not found
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

			if ( shader == tr.defaultShader ) {
				ri.Printf( PRINT_DEVELOPER, "WARNING: no shader for surface %s in skin %s\n", surface->name, skin->name );
			} else if ( shader->defaultShader )     {
				ri.Printf( PRINT_DEVELOPER, "WARNING: shader %s in skin %s not found\n", shader->name, skin->name );
			}
		} else {
			shader = R_GetShaderByHandle( surface->shaderIndex );
		}

		// don't add third_person objects if not viewing through a portal
		if ( !personalModel ) {
			R_AddDrawSurf( (surfaceType_t*)surface, shader, fogNum, 0, 0 );
		}

		surface = ( mdmSurface_t * )( (byte *)surface + surface->ofsEnd );
	}
}
