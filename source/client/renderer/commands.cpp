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

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

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
//	R_InitCommandBuffers
//
//==========================================================================

void R_InitCommandBuffers()
{
	glConfig.smpActive = false;
	if ((GGameType & GAME_Tech3) && r_smp->integer)
	{
		Log::write("Trying SMP acceleration...\n");
		if (GLimp_SpawnRenderThread(RB_RenderThread))
		{
			Log::write("...succeeded.\n");
			glConfig.smpActive = true;
		}
		else
		{
			Log::write("...failed.\n");
		}
	}
}

//==========================================================================
//
//	R_ShutdownCommandBuffers
//
//==========================================================================

void R_ShutdownCommandBuffers()
{
	// kill the rendering thread
	if (glConfig.smpActive)
	{
		GLimp_WakeRenderer(NULL);
		glConfig.smpActive = false;
	}
}

//==========================================================================
//
//	R_PerformanceCounters
//
//==========================================================================

static void R_PerformanceCounters()
{
	if (!r_speeds->integer)
	{
		// clear the counters even if we aren't printing
		Com_Memset(&tr.pc, 0, sizeof(tr.pc));
		Com_Memset(&backEnd.pc, 0, sizeof(backEnd.pc));
		c_brush_polys = 0;
		c_alias_polys = 0;
		return;
	}

	if (GGameType & GAME_Quake)
	{
		Log::write("%4i wpoly %4i epoly\n", c_brush_polys, c_alias_polys); 
		c_brush_polys = 0;
		c_alias_polys = 0;
		return;
	}
	if (GGameType & GAME_Hexen2)
	{
		Log::write("%4i wpoly  %4i epoly  %4i(%i) edicts\n",
			c_brush_polys, c_alias_polys, r_numentities, cl_numtransvisedicts + cl_numtranswateredicts);
		c_brush_polys = 0;
		c_alias_polys = 0;
		return;
	}
	if (GGameType & GAME_Quake2)
	{
		Log::write("%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys, c_alias_polys, c_visible_textures, c_visible_lightmaps);
		c_brush_polys = 0;
		c_alias_polys = 0;
		return;
	}

	if (r_speeds->integer == 1)
	{
		Log::write("%i/%i shaders/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes,
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3,
			R_SumOfUsedImages() / 1000000.0f, backEnd.pc.c_overDraw / (float)(glConfig.vidWidth * glConfig.vidHeight));
	}
	else if (r_speeds->integer == 2)
	{
		Log::write("(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out,
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out);
		Log::write("(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out,
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out);
		common->Printf("(gen) %i sin %i sout %i pin %i pout\n",
			tr.pc.c_sphere_cull_in, tr.pc.c_sphere_cull_out,
			tr.pc.c_plane_cull_in, tr.pc.c_plane_cull_out );
	}
	else if (r_speeds->integer == 3)
	{
		Log::write("viewcluster: %i\n", tr.viewCluster);
	}
	else if (r_speeds->integer == 4)
	{
		if (backEnd.pc.c_dlightVertexes)
		{
			Log::write("dlight srf:%i  culled:%i  verts:%i  tris:%i\n", 
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3);
		}
	} 
	else if (r_speeds->integer == 5)
	{
		Log::write("zFar: %.0f\n", tr.viewParms.zFar);
	}
	else if (r_speeds->integer == 6)
	{
		Log::write("flare adds:%i tests:%i renders:%i\n", 
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders);
	}
	else if (r_speeds->integer == 7)
	{
		common->Printf("decal projectors: %d test surfs: %d clip surfs: %d decal surfs: %d created: %d\n",
			tr.pc.c_decalProjectors, tr.pc.c_decalTestSurfaces, tr.pc.c_decalClipSurfaces, tr.pc.c_decalSurfaces, tr.pc.c_decalSurfacesCreated);
	}

	Com_Memset(&tr.pc, 0, sizeof(tr.pc));
	Com_Memset(&backEnd.pc, 0, sizeof(backEnd.pc));
}

//==========================================================================
//
//	R_IssueRenderCommands
//
//==========================================================================

void R_IssueRenderCommands(bool runPerformanceCounters)
{
	renderCommandList_t* cmdList = &backEndData[tr.smpFrame]->commands;
	qassert(cmdList); // bk001205

	// add an end-of-list command
	*(int*)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	if (glConfig.smpActive)
	{
		// if the render thread is not idle, wait for it
		if (renderThreadActive)
		{
			if (r_showSmp->integer)
			{
				Log::write("R");
			}
		}
		else
		{
			if (r_showSmp->integer)
			{
				Log::write(".");
			}
		}

		// sleep until the renderer has completed
		GLimp_FrontEndSleep();
	}

	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if (runPerformanceCounters)
	{
		R_PerformanceCounters();
	}

	// actually start the commands going
	if (!r_skipBackEnd->integer)
	{
		// let it start on the new batch
		if (!glConfig.smpActive)
		{
			RB_ExecuteRenderCommands(cmdList->cmds);
		}
		else
		{
			GLimp_WakeRenderer(cmdList);
		}
	}
}

//==========================================================================
//
//	R_SyncRenderThread
//
//	Issue any pending commands and wait for them to complete. After exiting,
// the render thread will have completed its work and will remain idle and
// the main thread is free to issue OpenGL calls until R_IssueRenderCommands
// is called.
//
//==========================================================================

void R_SyncRenderThread()
{
	if (!tr.registered)
	{
		return;
	}
	R_IssueRenderCommands(false);

	if (!glConfig.smpActive)
	{
		return;
	}
	GLimp_FrontEndSleep();
}

//==========================================================================
//
//	R_GetCommandBuffer
//
//	make sure there is enough command space, waiting on the render thread if needed.
//
//==========================================================================

void* R_GetCommandBuffer(int bytes)
{
	renderCommandList_t* cmdList = &backEndData[tr.smpFrame]->commands;

	// always leave room for the swap buffers and end of list commands
	if (cmdList->used + bytes + sizeof(swapBuffersCommand_t) + 4 > MAX_RENDER_COMMANDS)
	{
		if (bytes > MAX_RENDER_COMMANDS - (int)sizeof(swapBuffersCommand_t) - 4)
		{
			throw Exception(va("R_GetCommandBuffer: bad size %i", bytes));
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}

//==========================================================================
//
//	R_AddDrawSurfCmd
//
//==========================================================================

void R_AddDrawSurfCmd(drawSurf_t* drawSurfs, int numDrawSurfs)
{
	drawSurfsCommand_t* cmd = (drawSurfsCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

//==========================================================================
//
//	R_SetColor
//
//	Passing NULL will set the color to white
//
//==========================================================================

void R_SetColor(const float* rgba)
{
	if (!tr.registered)
	{
		return;
	}
	setColorCommand_t* cmd = (setColorCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	if (!rgba)
	{
		static float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}

//==========================================================================
//
//	R_StretchPic
//
//==========================================================================

void R_StretchPic(float x, float y, float w, float h, 
	float s1, float t1, float s2, float t2, qhandle_t hShader)
{
	if (!tr.registered)
	{
		return;
	}
	stretchPicCommand_t* cmd = (stretchPicCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader = R_GetShaderByHandle(hShader);
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

void R_StretchPicGradient(float x, float y, float w, float h,
	float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType)
{
	if (!tr.registered)
	{
		return;
	}
	stretchPicCommand_t* cmd = (stretchPicCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_STRETCH_PIC_GRADIENT;
	cmd->shader = R_GetShaderByHandle(hShader);
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;

	if (!gradientColor)
	{
		static float colorWhite[4] = { 1, 1, 1, 1 };

		gradientColor = colorWhite;
	}

	cmd->gradientColor[0] = gradientColor[0] * 255;
	cmd->gradientColor[1] = gradientColor[1] * 255;
	cmd->gradientColor[2] = gradientColor[2] * 255;
	cmd->gradientColor[3] = gradientColor[3] * 255;
	cmd->gradientType = gradientType;
}

void R_RotatedPic(float x, float y, float w, float h,
	float s1, float t1, float s2, float t2, qhandle_t hShader, float angle)
{
	if (!tr.registered)
	{
		return;
	}
	stretchPicCommand_t* cmd = (stretchPicCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_ROTATED_PIC;
	cmd->shader = R_GetShaderByHandle(hShader);
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;

	// fixup
	cmd->w /= 2;
	cmd->h /= 2;
	cmd->x += cmd->w;
	cmd->y += cmd->h;
	cmd->w = sqrt((cmd->w * cmd->w) + (cmd->h * cmd->h));
	cmd->h = cmd->w;

	cmd->angle = angle;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

void R_2DPolyies(polyVert_t* verts, int numverts, qhandle_t hShader)
{
	if (!tr.registered)
	{
		return;
	}

	if (r_numpolyverts + numverts > max_polyverts)
	{
		return;
	}

	poly2dCommand_t* cmd = (poly2dCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_2DPOLYS;
	cmd->verts = &backEndData[tr.smpFrame]->polyVerts[r_numpolyverts];
	cmd->numverts = numverts;
	memcpy(cmd->verts, verts, sizeof(polyVert_t) * numverts);
	cmd->shader = R_GetShaderByHandle(hShader);

	r_numpolyverts += numverts;
}

//==========================================================================
//
//	R_BeginFrame
//
//	If running in stereo, RE_BeginFrame will be called twice
// for each R_EndFrame
//
//==========================================================================

void R_BeginFrame(stereoFrame_t stereoFrame)
{
	if (!tr.registered)
	{
		return;
	}
	glState.finishCalled = false;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	//
	// do overdraw measurement
	//
	if (r_measureOverdraw->integer)
	{
		if (glConfig.stencilBits < 4)
		{
			Log::write("Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits);
			Cvar_Set("r_measureOverdraw", "0");
			r_measureOverdraw->modified = false;
		}
		else if (r_shadows->integer == 2)
		{
			Log::write("Warning: stencil shadows and overdraw measurement are mutually exclusive\n");
			Cvar_Set("r_measureOverdraw", "0");
			r_measureOverdraw->modified = false;
		}
		else
		{
			R_SyncRenderThread();
			qglEnable(GL_STENCIL_TEST);
			qglStencilMask(~0U);
			qglClearStencil(0U);
			qglStencilFunc(GL_ALWAYS, 0U, ~0U);
			qglStencilOp(GL_KEEP, GL_INCR, GL_INCR);
		}
		r_measureOverdraw->modified = false;
	}
	else
	{
		// this is only reached if it was on and is now off
		if (r_measureOverdraw->modified)
		{
			R_SyncRenderThread();
			qglDisable(GL_STENCIL_TEST);
		}
		r_measureOverdraw->modified = false;
	}

	//
	// texturemode stuff
	//
	if (r_textureMode->modified)
	{
		R_SyncRenderThread();
		GL_TextureMode(r_textureMode->string);
		r_textureMode->modified = false;
	}

	//
	// anisotropic filtering stuff
	//
	if (r_textureAnisotropy->modified)
	{
		R_SyncRenderThread();
		GL_TextureAnisotropy(r_textureAnisotropy->value);
		r_textureAnisotropy->modified = false;
	}

	//
	// ATI stuff
	//
	// TRUFORM
	if (qglPNTrianglesiATI)
	{
		// tess
		if (r_ati_truform_tess->modified)
		{
			r_ati_truform_tess->modified = false;
			// cap if necessary
			if (r_ati_truform_tess->value > glConfig.ATIMaxTruformTess)
			{
				Cvar_Set("r_ati_truform_tess", va("%d", glConfig.ATIMaxTruformTess));
			}

			qglPNTrianglesiATI(GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI, r_ati_truform_tess->value);
		}

		// point mode
		if (r_ati_truform_pointmode->modified)
		{
			r_ati_truform_pointmode->modified = false;
			// GR - shorten the mode name
			if (!String::ICmp(r_ati_truform_pointmode->string, "LINEAR"))
			{
				glConfig.ATIPointMode = (int)GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI;
				// GR - fix point mode change
			}
			else if (!String::ICmp(r_ati_truform_pointmode->string, "CUBIC"))
			{
				glConfig.ATIPointMode = (int)GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI;
			}
			else
			{
				// bogus value, set to valid
				glConfig.ATIPointMode = (int)GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI;
				Cvar_Set("r_ati_truform_pointmode", "LINEAR");
			}
			qglPNTrianglesiATI(GL_PN_TRIANGLES_POINT_MODE_ATI, glConfig.ATIPointMode);
		}

		// normal mode
		if (r_ati_truform_normalmode->modified)
		{
			r_ati_truform_normalmode->modified = false;
			// GR - shorten the mode name
			if (!String::ICmp(r_ati_truform_normalmode->string, "LINEAR"))
			{
				glConfig.ATINormalMode = (int)GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI;
				// GR - fix normal mode change
			}
			else if (!String::ICmp(r_ati_truform_normalmode->string, "QUADRATIC"))
			{
				glConfig.ATINormalMode = (int)GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI;
			}
			else
			{
				// bogus value, set to valid
				glConfig.ATINormalMode = (int)GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI;
				Cvar_Set("r_ati_truform_normalmode", "LINEAR");
			}
			qglPNTrianglesiATI(GL_PN_TRIANGLES_NORMAL_MODE_ATI, glConfig.ATINormalMode);
		}
	}

	//
	// NVidia stuff
	//
	// fog control
	if (glConfig.NVFogAvailable && r_nv_fogdist_mode->modified)
	{
		r_nv_fogdist_mode->modified = false;
		if (!String::ICmp(r_nv_fogdist_mode->string, "GL_EYE_PLANE_ABSOLUTE_NV"))
		{
			glConfig.NVFogMode = (int)GL_EYE_PLANE_ABSOLUTE_NV;
		}
		else if (!String::ICmp(r_nv_fogdist_mode->string, "GL_EYE_PLANE"))
		{
			glConfig.NVFogMode = (int)GL_EYE_PLANE;
		}
		else if (!String::ICmp(r_nv_fogdist_mode->string, "GL_EYE_RADIAL_NV"))
		{
			glConfig.NVFogMode = (int)GL_EYE_RADIAL_NV;
		}
		else
		{
			// in case this was really 'else', store a valid value for next time
			glConfig.NVFogMode = (int)GL_EYE_RADIAL_NV;
			Cvar_Set("r_nv_fogdist_mode", "GL_EYE_RADIAL_NV");
		}
	}

	//
	// gamma stuff
	//
	if (r_gamma->modified)
	{
		r_gamma->modified = false;

		R_SyncRenderThread();
		R_SetColorMappings();
	}

	// check for errors
	if (!r_ignoreGLErrors->integer)
	{
		R_SyncRenderThread();
		int err = qglGetError();
		if (err != GL_NO_ERROR)
		{
			throw Exception(va("RE_BeginFrame() - glGetError() failed (0x%x)!\n", err));
		}
	}

	//
	// draw buffer stuff
	//
	drawBufferCommand_t* cmd = (drawBufferCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_DRAW_BUFFER;

	if (glConfig.stereoEnabled)
	{
		if (stereoFrame == STEREO_LEFT)
		{
			cmd->buffer = (int)GL_BACK_LEFT;
		}
		else if (stereoFrame == STEREO_RIGHT)
		{
			cmd->buffer = (int)GL_BACK_RIGHT;
		}
		else
		{
			throw Exception(va("RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame));
		}
	}
	else
	{
		if (stereoFrame != STEREO_CENTER)
		{
			throw Exception(va("R_BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame));
		}
		if (!String::ICmp(r_drawBuffer->string, "GL_FRONT"))
		{
			cmd->buffer = (int)GL_FRONT;
		}
		else
		{
			cmd->buffer = (int)GL_BACK;
		}
	}

	//FIXME Temporary hack
	if (!(GGameType & GAME_Tech3))
	{
		R_IssueRenderCommands(false);
	}
}

//==========================================================================
//
//	R_EndFrame
//
//	Returns the number of msec spent in the back end
//
//==========================================================================

void R_EndFrame(int* frontEndMsec, int* backEndMsec)
{
	if (!tr.registered)
	{
		return;
	}
	if (GGameType & GAME_ET)
	{
		// Needs to use reserved space, so no R_GetCommandBuffer.
		renderCommandList_t* cmdList = &backEndData[tr.smpFrame]->commands;
		qassert(cmdList);
		// add swap-buffers command
		*(int*)(cmdList->cmds + cmdList->used) = RC_SWAP_BUFFERS;
		cmdList->used += sizeof(swapBuffersCommand_t);
	}
	else
	{
		swapBuffersCommand_t* cmd = (swapBuffersCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
		if (!cmd)
		{
			return;
		}
		cmd->commandId = RC_SWAP_BUFFERS;
	}

	R_IssueRenderCommands(true);

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_ToggleSmpFrame();

	if (frontEndMsec)
	{
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if (backEndMsec)
	{
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}

void R_RenderToTexture(qhandle_t textureid, int x, int y, int w, int h)
{
	if (!tr.registered)
	{
		return;
	}

	if (textureid > tr.numImages || textureid < 0)
	{
		common->Printf("Warning: trap_R_RenderToTexture textureid %d out of range.\n", textureid);
		return;
	}

	renderToTextureCommand_t* cmd = (renderToTextureCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_RENDERTOTEXTURE;
	cmd->image = tr.images[textureid];
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
}

void R_Finish()
{
	if (!tr.registered)
	{
		return;
	}

	common->Printf("R_Finish\n");

	renderFinishCommand_t* cmd = (renderFinishCommand_t*)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_FINISH;
}
