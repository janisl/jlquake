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

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte* data, int client, qboolean dirty)
{
	int start, end;

	if (!tr.registered)
	{
		return;
	}
	R_SyncRenderThread();

	// we definately want to sync every frame for the cinematics
	qglFinish();

	start = end = 0;
	if (r_speeds->integer)
	{
		start = ri.Milliseconds();
	}

	R_UploadCinematic(cols, rows, data, client, dirty);

	if (r_speeds->integer)
	{
		end = ri.Milliseconds();
		ri.Printf(PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start);
	}

	RB_SetGL2D();

	qglColor3f(tr.identityLight, tr.identityLight, tr.identityLight);

	qglBegin(GL_QUADS);
	qglTexCoord2f(0.5f / cols,  0.5f / rows);
	qglVertex2f(x, y);
	qglTexCoord2f((cols - 0.5f) / cols,  0.5f / rows);
	qglVertex2f(x + w, y);
	qglTexCoord2f((cols - 0.5f) / cols, (rows - 0.5f) / rows);
	qglVertex2f(x + w, y + h);
	qglTexCoord2f(0.5f / cols, (rows - 0.5f) / rows);
	qglVertex2f(x, y + h);
	qglEnd();
}

const void* RB_SetColor(const void* data);
const void* RB_StretchPic(const void* data);
const void* RB_Draw2dPolys(const void* data);
const void* RB_RotatedPic(const void* data);
const void* RB_StretchPicGradient(const void* data);
const void* RB_DrawSurfs(const void* data);
const void* RB_DrawBuffer(const void* data);

/*
=============
RB_SwapBuffers

=============
*/
const void* RB_SwapBuffers(const void* data)
{
	const swapBuffersCommand_t* cmd;

	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		RB_EndSurface();
	}

	// texture swapping test
	if (r_showImages->integer)
	{
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t*)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if (r_measureOverdraw->integer)
	{
		int i;
		long sum = 0;
		unsigned char* stencilReadback;

		stencilReadback = (byte*)ri.Hunk_AllocateTempMemory(glConfig.vidWidth * glConfig.vidHeight);
		qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback);

		for (i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++)
		{
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory(stencilReadback);
	}


	if (!glState.finishCalled)
	{
		qglFinish();
	}

	QGL_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	GLimp_EndFrame();

	backEnd.projection2D = qfalse;

	return (const void*)(cmd + 1);
}

const void* RB_RenderToTexture(const void* data);
const void* RB_Finish(const void* data);

/*
====================
RB_ExecuteRenderCommands

This function will be called synchronously if running without
smp extensions, or asynchronously by another thread.
====================
*/
void RB_ExecuteRenderCommands(const void* data)
{
	int t1, t2;

	t1 = ri.Milliseconds();

	if (!r_smp->integer || data == backEndData[0]->commands.cmds)
	{
		backEnd.smpFrame = 0;
	}
	else
	{
		backEnd.smpFrame = 1;
	}

	while (1)
	{
		switch (*(const int*)data)
		{
		case RC_SET_COLOR:
			data = RB_SetColor(data);
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic(data);
			break;
		case RC_2DPOLYS:
			data = RB_Draw2dPolys(data);
			break;
		case RC_ROTATED_PIC:
			data = RB_RotatedPic(data);
			break;
		case RC_STRETCH_PIC_GRADIENT:
			data = RB_StretchPicGradient(data);
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs(data);
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer(data);
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers(data);
			break;
		//bani
		case RC_RENDERTOTEXTURE:
			data = RB_RenderToTexture(data);
			break;
		//bani
		case RC_FINISH:
			data = RB_Finish(data);
			break;
		case RC_END_OF_LIST:
		default:
			// stop rendering on this thread
			t2 = ri.Milliseconds();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
}


/*
================
RB_RenderThread
================
*/
void RB_RenderThread(void)
{
	const void* data;

	// wait for either a rendering command or a quit command
	while (1)
	{
		// sleep until we have work to do
		data = GLimp_RendererSleep();

		if (!data)
		{
			return; // all done, renderer is shutting down
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands(data);

		renderThreadActive = qfalse;
	}
}

