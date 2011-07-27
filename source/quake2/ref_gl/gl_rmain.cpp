/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_main.c
#include "gl_local.h"
#include "../client/client.h"

refimport_t	ri;

glstate2_t  gl_state;

float		v_blend[4];			// final blending color

QCvar	*gl_polyblend;


/*
============
R_PolyBlend
============
*/
void R_PolyBlend(refdef_t *fd)
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	R_Draw2DQuad(fd->x, fd->y, fd->width, fd->height, NULL, 0, 0, 0, 0, v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
}

//=======================================================================

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (r_clear->value)
	{
		qglClearColor(1,0, 0.5, 0.5);
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame(refdef_t *fd)
{
	tr.frameSceneNum = 0;

	c_brush_polys = 0;
	c_alias_polys = 0;

	glState.finishCalled = false;

	R_RenderScene(fd);

	R_PolyBlend(fd);

	if (r_speeds->value)
	{
		ri.Con_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys, 
			c_alias_polys, 
			c_visible_textures, 
			c_visible_lightmaps); 
	}
}


/*
===============
CL_InitRenderStuff
===============
*/
void CL_InitRenderStuff()
{	
	int		err;

	ri.Con_Printf (PRINT_ALL, "ref_gl version: "REF_VERSION"\n");

	R_BeginRegistration(&cls.glconfig);

	gl_polyblend = Cvar_Get ("gl_polyblend", "1", 0);

	// let the sound and input subsystems know about the new window
	VID_NewWindow(glConfig.vidWidth, glConfig.vidHeight);

	Cvar_SetLatched( "scr_drawall", "0" );

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Con_Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	Draw_InitLocal();
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{
	tr.frameCount++;

	gl_state.camera_separation = camera_separation;

	QGL_EnableLogging(!!r_logFile->integer);

	QGL_LogComment("*** R_BeginFrame ***\n");

	GLimp_BeginFrame( camera_separation );

	/*
	** draw buffer stuff
	*/
	if ( r_drawBuffer->modified )
	{
		r_drawBuffer->modified = false;

		if ( gl_state.camera_separation == 0 || !glConfig.stereoEnabled )
		{
			if ( QStr::ICmp( r_drawBuffer->string, "GL_FRONT" ) == 0 )
				qglDrawBuffer( GL_FRONT );
			else
				qglDrawBuffer( GL_BACK );
		}
	}

	/*
	** texturemode stuff
	*/
	if ( r_textureMode->modified )
	{
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

//===================================================================


void	R_RenderFrame (refdef_t *fd);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.RenderFrame = R_RenderFrame;

	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;

	return re;
}

void R_ClearScreen()
{
	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);
}
