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
	if ((GGameType & GAME_Quake3) && r_smp->integer)
	{
		GLog.Write("Trying SMP acceleration...\n");
		if (GLimp_SpawnRenderThread(RB_RenderThread))
		{
			GLog.Write("...succeeded.\n");
			glConfig.smpActive = true;
		}
		else
		{
			GLog.Write("...failed.\n");
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
		return;
	}

	if (r_speeds->integer == 1)
	{
		GLog.Write("%i/%i shaders/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes,
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3,
			R_SumOfUsedImages() / 1000000.0f, backEnd.pc.c_overDraw / (float)(glConfig.vidWidth * glConfig.vidHeight));
	}
	else if (r_speeds->integer == 2)
	{
		GLog.Write("(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out,
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out);
		GLog.Write("(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out,
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out);
	}
	else if (r_speeds->integer == 3)
	{
		GLog.Write("viewcluster: %i\n", tr.viewCluster);
	}
	else if (r_speeds->integer == 4)
	{
		if (backEnd.pc.c_dlightVertexes)
		{
			GLog.Write("dlight srf:%i  culled:%i  verts:%i  tris:%i\n", 
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3);
		}
	} 
	else if (r_speeds->integer == 5)
	{
		GLog.Write("zFar: %.0f\n", tr.viewParms.zFar);
	}
	else if (r_speeds->integer == 6)
	{
		GLog.Write("flare adds:%i tests:%i renders:%i\n", 
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders);
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
				GLog.Write("R");
			}
		}
		else
		{
			if (r_showSmp->integer)
			{
				GLog.Write(".");
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

	// always leave room for the end of list command
	if (cmdList->used + bytes + 4 > MAX_RENDER_COMMANDS)
	{
		if (bytes > MAX_RENDER_COMMANDS - 4)
		{
			throw QException(va("R_GetCommandBuffer: bad size %i", bytes));
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