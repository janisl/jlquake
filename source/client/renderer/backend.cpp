//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../client_main.h"
#include "local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"

backEndData_t* backEndData[ SMP_FRAMES ];
backEndState_t backEnd;

int max_polys;
int max_polyverts;

volatile bool renderThreadActive;

void R_InitBackEndData() {
	max_polys = r_maxpolys->integer;
	if ( max_polys < MAX_POLYS ) {
		max_polys = MAX_POLYS;
	}

	max_polyverts = r_maxpolyverts->integer;
	if ( max_polyverts < MAX_POLYVERTS ) {
		max_polyverts = MAX_POLYVERTS;
	}

	byte* ptr = ( byte* )Mem_ClearedAlloc( sizeof ( *backEndData[ 0 ] ) + sizeof ( srfPoly_t ) * max_polys + sizeof ( polyVert_t ) * max_polyverts );
	backEndData[ 0 ] = ( backEndData_t* )ptr;
	backEndData[ 0 ]->polys = ( srfPoly_t* )( ( char* )ptr + sizeof ( *backEndData[ 0 ] ) );
	backEndData[ 0 ]->polyVerts = ( polyVert_t* )( ( char* )ptr + sizeof ( *backEndData[ 0 ] ) + sizeof ( srfPoly_t ) * max_polys );
	if ( r_smp->integer ) {
		ptr = ( byte* )Mem_ClearedAlloc( sizeof ( *backEndData[ 1 ] ) + sizeof ( srfPoly_t ) * max_polys + sizeof ( polyVert_t ) * max_polyverts );
		backEndData[ 1 ] = ( backEndData_t* )ptr;
		backEndData[ 1 ]->polys = ( srfPoly_t* )( ( char* )ptr + sizeof ( *backEndData[ 1 ] ) );
		backEndData[ 1 ]->polyVerts = ( polyVert_t* )( ( char* )ptr + sizeof ( *backEndData[ 1 ] ) + sizeof ( srfPoly_t ) * max_polys );
	} else   {
		backEndData[ 1 ] = NULL;
	}
	R_ToggleSmpFrame();
}

void R_FreeBackEndData() {
	Mem_Free( backEndData[ 0 ] );
	backEndData[ 0 ] = NULL;

	if ( backEndData[ 1 ] ) {
		Mem_Free( backEndData[ 1 ] );
		backEndData[ 1 ] = NULL;
	}
}

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

static const void* RB_SetColor( const void* data ) {
	const setColorCommand_t* cmd = ( const setColorCommand_t* )data;

	backEnd.color2D[ 0 ] = cmd->color[ 0 ] * 255;
	backEnd.color2D[ 1 ] = cmd->color[ 1 ] * 255;
	backEnd.color2D[ 2 ] = cmd->color[ 2 ] * 255;
	backEnd.color2D[ 3 ] = cmd->color[ 3 ] * 255;

	return ( const void* )( cmd + 1 );
}

static const void* RB_DrawBuffer( const void* data ) {
	const drawBufferCommand_t* cmd = ( const drawBufferCommand_t* )data;

	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return ( const void* )( cmd + 1 );
}

static void SetViewportAndScissor() {
	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
	qglMatrixMode( GL_MODELVIEW );

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

//	A player has predicted a teleport, but hasn't arrived yet
static void RB_Hyperspace() {
	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	float c = ( backEnd.refdef.time & 255 ) / 255.0f;
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = true;
}

//	Any mirrored or portaled views have already been drawn, so prepare
// to actually render the visible surfaces for this view
static void RB_BeginDrawingView() {
	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFinish();
		glState.finishCalled = true;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = true;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = false;

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// clear relevant buffers
	int clearBits;
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) {
		// (SA) modified to ensure one glclear() per frame at most
		clearBits = 0;

		if ( r_measureOverdraw->integer || r_shadows->integer == 2 ) {
			clearBits |= GL_STENCIL_BUFFER_BIT;
		}

		if ( GGameType & ( GAME_WolfSP | GAME_WolfMP ) && r_uiFullScreen->integer ) {
			clearBits = GL_DEPTH_BUFFER_BIT;	// (SA) always just clear depth for menus

		} else if ( GGameType & GAME_ET && tr.world && tr.world->globalFog >= 0 )     {
			clearBits |= GL_DEPTH_BUFFER_BIT;
			clearBits |= GL_COLOR_BUFFER_BIT;
			qglClearColor( tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 0 ] * tr.identityLight,
				tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 1 ] * tr.identityLight,
				tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 2 ] * tr.identityLight, 1.0 );
		} else if ( skyboxportal )     {
			if ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) {
				// portal scene, clear whatever is necessary
				clearBits |= GL_DEPTH_BUFFER_BIT;

				if ( r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
					// fastsky: clear color

					// try clearing first with the portal sky fog color, then the world fog color, then finally a default
					clearBits |= GL_COLOR_BUFFER_BIT;
					if ( glfogsettings[ FOG_PORTALVIEW ].registered ) {
						qglClearColor( glfogsettings[ FOG_PORTALVIEW ].color[ 0 ], glfogsettings[ FOG_PORTALVIEW ].color[ 1 ], glfogsettings[ FOG_PORTALVIEW ].color[ 2 ], glfogsettings[ FOG_PORTALVIEW ].color[ 3 ] );
					} else if ( glfogNum > FOG_NONE && glfogsettings[ FOG_CURRENT ].registered )       {
						qglClearColor( glfogsettings[ FOG_CURRENT ].color[ 0 ], glfogsettings[ FOG_CURRENT ].color[ 1 ], glfogsettings[ FOG_CURRENT ].color[ 2 ], glfogsettings[ FOG_CURRENT ].color[ 3 ] );
					} else   {
						qglClearColor( 0.5, 0.5, 0.5, 1.0 );
					}
				} else   {
					// rendered sky (either clear color or draw quake sky)
					if ( glfogsettings[ FOG_PORTALVIEW ].registered ) {
						qglClearColor( glfogsettings[ FOG_PORTALVIEW ].color[ 0 ], glfogsettings[ FOG_PORTALVIEW ].color[ 1 ], glfogsettings[ FOG_PORTALVIEW ].color[ 2 ], glfogsettings[ FOG_PORTALVIEW ].color[ 3 ] );

						if ( glfogsettings[ FOG_PORTALVIEW ].clearscreen ) {
							// portal fog requests a screen clear (distance fog rather than quake sky)
							clearBits |= GL_COLOR_BUFFER_BIT;
						}
					}
				}
			} else   {
				// world scene with portal sky, don't clear any buffers, just set the fog color if there is one
				clearBits |= GL_DEPTH_BUFFER_BIT;	// this will go when I get the portal sky rendering way out in the zbuffer (or not writing to zbuffer at all)

				if ( glfogNum > FOG_NONE && glfogsettings[ FOG_CURRENT ].registered ) {
					if ( backEnd.refdef.rdflags & RDF_UNDERWATER ) {
						if ( glfogsettings[ FOG_CURRENT ].mode == GL_LINEAR ) {
							clearBits |= GL_COLOR_BUFFER_BIT;
						}
					} else if ( !( r_portalsky->integer ) )       {
						// portal skies have been manually turned off, clear bg color
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
					qglClearColor( glfogsettings[ FOG_CURRENT ].color[ 0 ], glfogsettings[ FOG_CURRENT ].color[ 1 ], glfogsettings[ FOG_CURRENT ].color[ 2 ], glfogsettings[ FOG_CURRENT ].color[ 3 ] );
				} else if ( !( r_portalsky->integer ) )       {
					// ydnar: portal skies have been manually turned off, clear bg color
					clearBits |= GL_COLOR_BUFFER_BIT;
					qglClearColor( 0.5, 0.5, 0.5, 1.0 );
				}
			}
		} else   {
			// world scene with no portal sky
			clearBits |= GL_DEPTH_BUFFER_BIT;

			// NERVE - SMF - we don't want to clear the buffer when no world model is specified
			if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
				clearBits &= ~GL_COLOR_BUFFER_BIT;
			}
			// -NERVE - SMF
			else if ( r_fastsky->integer || ( !( GGameType & GAME_WolfSP ) && backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) ) {
				clearBits |= GL_COLOR_BUFFER_BIT;

				if ( glfogsettings[ FOG_CURRENT ].registered ) {
					// try to clear fastsky with current fog color
					qglClearColor( glfogsettings[ FOG_CURRENT ].color[ 0 ], glfogsettings[ FOG_CURRENT ].color[ 1 ], glfogsettings[ FOG_CURRENT ].color[ 2 ], glfogsettings[ FOG_CURRENT ].color[ 3 ] );
				} else   {
					if ( GGameType & GAME_WolfSP ) {
						qglClearColor( 0.5, 0.5, 0.5, 1.0 );
					} else   {
						qglClearColor( 0.05, 0.05, 0.05, 1.0 );		// JPW NERVE changed per id req was 0.5s
					}
				}
			} else   {
				// world scene, no portal sky, not fastsky, clear color if fog says to, otherwise, just set the clearcolor
				if ( glfogsettings[ FOG_CURRENT ].registered ) {
					// try to clear fastsky with current fog color
					qglClearColor( glfogsettings[ FOG_CURRENT ].color[ 0 ], glfogsettings[ FOG_CURRENT ].color[ 1 ], glfogsettings[ FOG_CURRENT ].color[ 2 ], glfogsettings[ FOG_CURRENT ].color[ 3 ] );
					if ( glfogsettings[ FOG_CURRENT ].clearscreen ) {
						// world fog requests a screen clear (distance fog rather than quake sky)
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
				}
			}
		}

		// ydnar: don't clear the color buffer when no world model is specified
		if ( GGameType & GAME_ET && backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
			clearBits &= ~GL_COLOR_BUFFER_BIT;
		}
	} else   {
		clearBits = GL_DEPTH_BUFFER_BIT;

		if ( r_measureOverdraw->integer || ( ( GGameType & GAME_Quake3 ) && r_shadows->integer == 2 ) ) {
			clearBits |= GL_STENCIL_BUFFER_BIT;
		}
		if ( r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) ) {
			clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
#ifdef _DEBUG
			qglClearColor( 0.8f, 0.7f, 0.4f, 1.0f );	// FIXME: get color of sky
#else
			qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
#endif
		}
	}
	if ( clearBits ) {
		qglClear( clearBits );
	}

	if ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) {
		RB_Hyperspace();
		return;
	} else   {
		backEnd.isHyperspace = false;
	}

	glState.faceCulling = -1;		// force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = false;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
		float plane[ 4 ];
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		double plane2[ 4 ];
		plane2[ 0 ] = DotProduct( backEnd.viewParms.orient.axis[ 0 ], plane );
		plane2[ 1 ] = DotProduct( backEnd.viewParms.orient.axis[ 1 ], plane );
		plane2[ 2 ] = DotProduct( backEnd.viewParms.orient.axis[ 2 ], plane );
		plane2[ 3 ] = DotProduct( plane, backEnd.viewParms.orient.origin ) - plane[ 3 ];

		qglLoadMatrixf( s_flipMatrix );
		qglClipPlane( GL_CLIP_PLANE0, plane2 );
		qglEnable( GL_CLIP_PLANE0 );
	} else   {
		qglDisable( GL_CLIP_PLANE0 );
	}
}

static void RB_RenderDrawSurfList( drawSurf_t* drawSurfs, int numDrawSurfs ) {
	// save original time for entity shader offsets
	float originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView();

	// draw everything
	int oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	shader_t* oldShader = NULL;
	int oldFogNum = -1;
	bool oldDepthRange = false;
	int oldDlighted = false;
	unsigned int oldSort = -1;
	bool depthRange = false;
	int oldAtiTess = -1;

	backEnd.pc.c_surfaces += numDrawSurfs;

	drawSurf_t* drawSurf = drawSurfs;
	for ( int i = 0; i < numDrawSurfs; i++, drawSurf++ ) {
		if ( drawSurf->sort == oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
		int entityNum;
		shader_t* shader;
		int fogNum;
		int dlighted;
		int frontFace;
		int atiTess;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &frontFace, &atiTess );

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted ||
			 ( entityNum != oldEntityNum && !shader->entityMergable ) ||
			 ( atiTess != oldAtiTess ) ) {
			if ( oldShader != NULL ) {
				// GR - pass tessellation flag to the shader command
				//		make sure to use oldAtiTess!!!
				tess.ATI_tess = ( oldAtiTess == ATI_TESS_TRUFORM );
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
			oldAtiTess = atiTess;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = false;

			if ( entityNum != REF_ENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[ entityNum ];
				if ( GGameType & ( GAME_WolfMP | GAME_ET ) ) {
					backEnd.refdef.floatTime = originalTime;
				} else   {
					backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				}
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				if ( !( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) ) {
					tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				}

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orient );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->dlightBits ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orient );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
					depthRange = true;
				}
			} else   {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.orient = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				if ( !( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) ) {
					tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				}
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orient );
			}

			qglLoadMatrixf( backEnd.orient.modelMatrix );

			//
			// change depthrange if needed
			//
			if ( oldDepthRange != depthRange ) {
				if ( depthRange ) {
					qglDepthRange( 0, 0.3 );
				} else   {
					qglDepthRange( 0, 1 );
				}
				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	// draw the contents of the last shader batch
	if ( oldShader != NULL ) {
		// GR - pass tessellation flag to the shader command
		//		make sure to use oldAtiTess!!!
		tess.ATI_tess = ( oldAtiTess == ATI_TESS_TRUFORM );
		RB_EndSurface();
	}

	// go back to the world modelview matrix
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.refdef.floatTime = originalTime;
	backEnd.orient = backEnd.viewParms.world;
	R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orient );
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	if ( depthRange ) {
		qglDepthRange( 0, 1 );
	}

	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) {
		// (SA) draw sun
		RB_DrawSun();
	}

	// darken down any stencil shadows
	RB_ShadowFinish();

	// add light flares on lights that aren't obscured
	RB_RenderFlares();
}

static const void* RB_DrawSurfs( const void* data ) {
	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	const drawSurfsCommand_t* cmd = ( const drawSurfsCommand_t* )data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

	return ( const void* )( cmd + 1 );
}

void RB_SetGL2D() {
	backEnd.projection2D = true;

	// set 2D virtual screen size
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();

	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	if ( GGameType & GAME_WolfSP ) {
		qglDisable( GL_FOG );
	}

	qglDisable( GL_CULL_FACE );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = CL_ScaledMilliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}

static void RB_Set2DVertexCoords( const stretchPicCommand_t* cmd, int numVerts ) {
	tess.xyz[ numVerts ][ 0 ] = cmd->x;
	tess.xyz[ numVerts ][ 1 ] = cmd->y;
	tess.xyz[ numVerts ][ 2 ] = 0;

	tess.xyz[ numVerts + 1 ][ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][ 1 ] = cmd->y;
	tess.xyz[ numVerts + 1 ][ 2 ] = 0;

	tess.xyz[ numVerts + 2 ][ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][ 2 ] = 0;

	tess.xyz[ numVerts + 3 ][ 0 ] = cmd->x;
	tess.xyz[ numVerts + 3 ][ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][ 2 ] = 0;
}

static const void* RB_Draw2DQuad( const void* data ) {
	const stretchPicCommand_t* cmd = ( const stretchPicCommand_t* )data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	if ( scrap_dirty ) {
		R_ScrapUpload();
	}
	GL_Bind( cmd->image );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	RB_Set2DVertexCoords( cmd, 0 );

	tess.svars.colors[ 0 ][ 0 ] = cmd->r * 255;
	tess.svars.colors[ 0 ][ 1 ] = cmd->g * 255;
	tess.svars.colors[ 0 ][ 2 ] = cmd->b * 255;
	tess.svars.colors[ 0 ][ 3 ] = cmd->a * 255;

	tess.svars.texcoords[ 0 ][ 0 ][ 0 ] = cmd->s1;
	tess.svars.texcoords[ 0 ][ 0 ][ 1 ] = cmd->t1;

	tess.svars.colors[ 1 ][ 0 ] = cmd->r * 255;
	tess.svars.colors[ 1 ][ 1 ] = cmd->g * 255;
	tess.svars.colors[ 1 ][ 2 ] = cmd->b * 255;
	tess.svars.colors[ 1 ][ 3 ] = cmd->a * 255;

	tess.svars.texcoords[ 0 ][ 1 ][ 0 ] = cmd->s2;
	tess.svars.texcoords[ 0 ][ 1 ][ 1 ] = cmd->t1;

	tess.svars.colors[ 2 ][ 0 ] = cmd->r * 255;
	tess.svars.colors[ 2 ][ 1 ] = cmd->g * 255;
	tess.svars.colors[ 2 ][ 2 ] = cmd->b * 255;
	tess.svars.colors[ 2 ][ 3 ] = cmd->a * 255;

	tess.svars.texcoords[ 0 ][ 2 ][ 0 ] = cmd->s2;
	tess.svars.texcoords[ 0 ][ 2 ][ 1 ] = cmd->t2;

	tess.svars.colors[ 3 ][ 0 ] = cmd->r * 255;
	tess.svars.colors[ 3 ][ 1 ] = cmd->g * 255;
	tess.svars.colors[ 3 ][ 2 ] = cmd->b * 255;
	tess.svars.colors[ 3 ][ 3 ] = cmd->a * 255;

	tess.svars.texcoords[ 0 ][ 3 ][ 0 ] = cmd->s1;
	tess.svars.texcoords[ 0 ][ 3 ][ 1 ] = cmd->t2;

	EnableArrays( 4 );
	qglBegin( GL_QUADS );
	qglArrayElement( 0 );
	qglArrayElement( 1 );
	qglArrayElement( 2 );
	qglArrayElement( 3 );
	qglEnd();
	DisableArrays();

	return ( const void* )( cmd + 1 );
}

static const void* RB_StretchPic( const void* data ) {
	const stretchPicCommand_t* cmd = ( const stretchPicCommand_t* )data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader_t* shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	*( int* )tess.vertexColors[ numVerts ] =
		*( int* )tess.vertexColors[ numVerts + 1 ] =
			*( int* )tess.vertexColors[ numVerts + 2 ] =
				*( int* )tess.vertexColors[ numVerts + 3 ] = *( int* )backEnd.color2D;

	RB_Set2DVertexCoords( cmd, numVerts );

	tess.texCoords[ numVerts ][ 0 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts ][ 0 ][ 1 ] = cmd->t1;

	tess.texCoords[ numVerts + 1 ][ 0 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][ 0 ][ 1 ] = cmd->t1;

	tess.texCoords[ numVerts + 2 ][ 0 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][ 0 ][ 1 ] = cmd->t2;

	tess.texCoords[ numVerts + 3 ][ 0 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][ 0 ][ 1 ] = cmd->t2;

	return ( const void* )( cmd + 1 );
}

static const void* RB_StretchPicGradient( const void* data ) {
	const stretchPicCommand_t* cmd = ( const stretchPicCommand_t* )data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader_t* shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	*( int* )tess.vertexColors[ numVerts ] =
		*( int* )tess.vertexColors[ numVerts + 1 ] = *( int* )backEnd.color2D;

	*( int* )tess.vertexColors[ numVerts + 2 ] =
		*( int* )tess.vertexColors[ numVerts + 3 ] = *( int* )cmd->gradientColor;

	RB_Set2DVertexCoords( cmd, numVerts );

	tess.texCoords[ numVerts ][ 0 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts ][ 0 ][ 1 ] = cmd->t1;

	tess.texCoords[ numVerts + 1 ][ 0 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][ 0 ][ 1 ] = cmd->t1;

	tess.texCoords[ numVerts + 2 ][ 0 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][ 0 ][ 1 ] = cmd->t2;

	tess.texCoords[ numVerts + 3 ][ 0 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][ 0 ][ 1 ] = cmd->t2;

	return ( const void* )( cmd + 1 );
}

static const void* RB_RotatedPic( const void* data ) {
	const stretchPicCommand_t* cmd = ( const stretchPicCommand_t* )data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader_t* shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	int numVerts = tess.numVertexes;
	int numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	*( int* )tess.vertexColors[ numVerts ] =
		*( int* )tess.vertexColors[ numVerts + 1 ] =
			*( int* )tess.vertexColors[ numVerts + 2 ] =
				*( int* )tess.vertexColors[ numVerts + 3 ] = *( int* )backEnd.color2D;

	float angle = cmd->angle * idMath::TWO_PI;
	tess.xyz[ numVerts ][ 0 ] = cmd->x + ( cos( angle ) * cmd->w );
	tess.xyz[ numVerts ][ 1 ] = cmd->y + ( sin( angle ) * cmd->h );
	tess.xyz[ numVerts ][ 2 ] = 0;

	tess.texCoords[ numVerts ][ 0 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts ][ 0 ][ 1 ] = cmd->t1;

	angle = cmd->angle * idMath::TWO_PI + 0.25 * idMath::TWO_PI;
	tess.xyz[ numVerts + 1 ][ 0 ] = cmd->x + ( cos( angle ) * cmd->w );
	tess.xyz[ numVerts + 1 ][ 1 ] = cmd->y + ( sin( angle ) * cmd->h );
	tess.xyz[ numVerts + 1 ][ 2 ] = 0;

	tess.texCoords[ numVerts + 1 ][ 0 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][ 0 ][ 1 ] = cmd->t1;

	angle = cmd->angle * idMath::TWO_PI + 0.50 * idMath::TWO_PI;
	tess.xyz[ numVerts + 2 ][ 0 ] = cmd->x + ( cos( angle ) * cmd->w );
	tess.xyz[ numVerts + 2 ][ 1 ] = cmd->y + ( sin( angle ) * cmd->h );
	tess.xyz[ numVerts + 2 ][ 2 ] = 0;

	tess.texCoords[ numVerts + 2 ][ 0 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][ 0 ][ 1 ] = cmd->t2;

	angle = cmd->angle * idMath::TWO_PI + 0.75 * idMath::TWO_PI;
	tess.xyz[ numVerts + 3 ][ 0 ] = cmd->x + ( cos( angle ) * cmd->w );
	tess.xyz[ numVerts + 3 ][ 1 ] = cmd->y + ( sin( angle ) * cmd->h );
	tess.xyz[ numVerts + 3 ][ 2 ] = 0;

	tess.texCoords[ numVerts + 3 ][ 0 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][ 0 ][ 1 ] = cmd->t2;

	return ( const void* )( cmd + 1 );
}

static const void* RB_Draw2dPolys( const void* data ) {
	const poly2dCommand_t* cmd;
	shader_t* shader;
	int i;

	cmd = ( const poly2dCommand_t* )data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( cmd->numverts, ( cmd->numverts - 2 ) * 3 );

	for ( i = 0; i < cmd->numverts - 2; i++ ) {
		tess.indexes[ tess.numIndexes + 0 ] = tess.numVertexes;
		tess.indexes[ tess.numIndexes + 1 ] = tess.numVertexes + i + 1;
		tess.indexes[ tess.numIndexes + 2 ] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	for ( i = 0; i < cmd->numverts; i++ ) {
		tess.xyz[ tess.numVertexes ][ 0 ] = cmd->verts[ i ].xyz[ 0 ];
		tess.xyz[ tess.numVertexes ][ 1 ] = cmd->verts[ i ].xyz[ 1 ];
		tess.xyz[ tess.numVertexes ][ 2 ] = 0;

		tess.texCoords[ tess.numVertexes ][ 0 ][ 0 ] = cmd->verts[ i ].st[ 0 ];
		tess.texCoords[ tess.numVertexes ][ 0 ][ 1 ] = cmd->verts[ i ].st[ 1 ];

		tess.vertexColors[ tess.numVertexes ][ 0 ] = cmd->verts[ i ].modulate[ 0 ];
		tess.vertexColors[ tess.numVertexes ][ 1 ] = cmd->verts[ i ].modulate[ 1 ];
		tess.vertexColors[ tess.numVertexes ][ 2 ] = cmd->verts[ i ].modulate[ 2 ];
		tess.vertexColors[ tess.numVertexes ][ 3 ] = cmd->verts[ i ].modulate[ 3 ];
		tess.numVertexes++;
	}

	return ( const void* )( cmd + 1 );
}

static const void* RB_RenderToTexture( const void* data ) {
	const renderToTextureCommand_t* cmd = ( const renderToTextureCommand_t* )data;

	GL_Bind( cmd->image );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
	qglCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, cmd->x, cmd->y, cmd->w, cmd->h, 0 );

	return ( const void* )( cmd + 1 );
}

static const void* RB_Finish( const void* data ) {
	const renderFinishCommand_t* cmd = ( const renderFinishCommand_t* )data;

	qglFinish();

	return ( const void* )( cmd + 1 );
}

//	Draw all the images to the screen, on top of whatever was there.  This
// is used to test for texture thrashing.
//	Also called by RE_EndRegistration
void RB_ShowImages() {
	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();

	int start = CL_ScaledMilliseconds();

	for ( int i = 0; i < tr.numImages; i++ ) {
		image_t* image = tr.images[ i ];

		float w = glConfig.vidWidth / 20;
		float h = glConfig.vidHeight / 15;
		float x = i % 20 * w;
		float y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		GL_Bind( image );
		qglBegin( GL_QUADS );
		qglTexCoord2f( 0, 0 );
		qglVertex2f( x, y );
		qglTexCoord2f( 1, 0 );
		qglVertex2f( x + w, y );
		qglTexCoord2f( 1, 1 );
		qglVertex2f( x + w, y + h );
		qglTexCoord2f( 0, 1 );
		qglVertex2f( x, y + h );
		qglEnd();
	}

	qglFinish();

	int end = CL_ScaledMilliseconds();
	common->Printf( "%i msec to draw all images\n", end - start );
}

static const void* RB_SwapBuffers( const void* data ) {
	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		byte* stencilReadback = new byte[ glConfig.vidWidth * glConfig.vidHeight ];
		qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		long sum = 0;
		for ( int i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[ i ];
		}

		backEnd.pc.c_overDraw += sum;
		delete[] stencilReadback;
	}

	if ( !glState.finishCalled ) {
		qglFinish();
	}

	QGL_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	//
	// swapinterval stuff
	//
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = false;

		if ( !glConfig.stereoEnabled ) {	// why?
#ifdef _WIN32
			if ( qwglSwapIntervalEXT ) {
				qwglSwapIntervalEXT( r_swapInterval->integer );
			}
#else
			if ( qglXSwapIntervalSGI ) {
				qglXSwapIntervalSGI( r_swapInterval->integer );
			}
#endif
		}
	}

	// don't flip if drawing to front buffer
	if ( String::ICmp( r_drawBuffer->string, "GL_FRONT" ) != 0 ) {
		GLimp_SwapBuffers();
	}

	// check logging
	QGL_EnableLogging( !!r_logFile->integer );

	backEnd.projection2D = false;

	const swapBuffersCommand_t* cmd = ( const swapBuffersCommand_t* )data;
	return ( const void* )( cmd + 1 );
}

//	This function will be called synchronously if running without smp
// extensions, or asynchronously by another thread.
void RB_ExecuteRenderCommands( const void* data ) {
	int t1 = CL_ScaledMilliseconds();

	if ( !r_smp->integer || data == backEndData[ 0 ]->commands.cmds ) {
		backEnd.smpFrame = 0;
	} else   {
		backEnd.smpFrame = 1;
	}

	while ( 1 ) {
		switch ( *( const int* )data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;

		case RC_DRAW_2D_QUAD:
			data = RB_Draw2DQuad( data );
			break;

		case RC_STRETCH_PIC:
			data = RB_StretchPic( data );
			break;

		case RC_STRETCH_PIC_GRADIENT:
			data = RB_StretchPicGradient( data );
			break;

		case RC_ROTATED_PIC:
			data = RB_RotatedPic( data );
			break;

		case RC_2DPOLYS:
			data = RB_Draw2dPolys( data );
			break;

		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;

		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;

		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;

		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd( data );
			break;

		case RC_RENDERTOTEXTURE:
			data = RB_RenderToTexture( data );
			break;

		case RC_FINISH:
			data = RB_Finish( data );
			break;

		case RC_END_OF_LIST:
		default:
			// stop rendering on this thread
			int t2 = CL_ScaledMilliseconds();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
}

void RB_RenderThread() {
	// wait for either a rendering command or a quit command
	while ( 1 ) {
		// sleep until we have work to do
		const void* data = GLimp_RendererSleep();

		if ( !data ) {
			return;	// all done, renderer is shutting down
		}

		renderThreadActive = true;

		RB_ExecuteRenderCommands( data );

		renderThreadActive = false;
	}
}

//	FIXME: not exactly backend
//	Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
//	Used for cinematics.
void R_StretchRaw( int x, int y, int w, int h, int cols, int rows, const byte* data, int client, bool dirty ) {
	if ( !tr.registered ) {
		return;
	}
	R_SyncRenderThread();

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// we definately want to sync every frame for the cinematics
	qglFinish();

	int start = 0;
	if ( r_speeds->integer ) {
		start = CL_ScaledMilliseconds();
	}

	R_UploadCinematic( cols, rows, data, client, dirty );

	if ( r_speeds->integer ) {
		int end = CL_ScaledMilliseconds();
		common->Printf( "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	RB_SetGL2D();

	qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );

	qglBegin( GL_QUADS );
	qglTexCoord2f( 0, 0 );
	qglVertex2f( x, y );
	qglTexCoord2f( 1, 0 );
	qglVertex2f( x + w, y );
	qglTexCoord2f( 1, 1 );
	qglVertex2f( x + w, y + h );
	qglTexCoord2f( 0, 1 );
	qglVertex2f( x, y + h );
	qglEnd();
}
