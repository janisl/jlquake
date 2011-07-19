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

refimport_t	ri;

glstate2_t  gl_state;

float		v_blend[4];			// final blending color

//
// screen size info
//
refdef_t	r_newrefdef;

QCvar	*r_norefresh;

QCvar	*gl_nosubimage;

QCvar  *gl_driver;
QCvar	*gl_polyblend;

QCvar	*vid_ref;


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);

    qglLoadIdentity ();

	// FIXME: get rid of these
    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

	qglColor4fv (v_blend);

	qglBegin (GL_QUADS);

	qglVertex3f (10, 100, 100);
	qglVertex3f (10, -100, 100);
	qglVertex3f (10, -100, -100);
	qglVertex3f (10, 100, -100);
	qglEnd ();

	qglEnable (GL_TEXTURE_2D);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80);

	qglColor4f(1,1,1,1);
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int i;
	mbrush38_leaf_t	*leaf;

	tr.viewCount++;

// current viewcluster
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeafQ2 (tr.viewParms.orient.origin, tr.worldModel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (tr.viewParms.orient.origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeafQ2 (temp, tr.worldModel);
			if ( !(leaf->contents & BSP38CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (tr.viewParms.orient.origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeafQ2 (temp, tr.worldModel);
			if ( !(leaf->contents & BSP38CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	c_brush_polys = 0;
	c_alias_polys = 0;
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	int		x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = floor((double)tr.refdef.x);
	x2 = ceil((double)(tr.refdef.x + tr.refdef.width));
	y = floor((double)glConfig.vidHeight - tr.refdef.y);
	y2 = ceil((double)glConfig.vidHeight - (tr.refdef.y + tr.refdef.height));

	w = x2 - x;
	h = y - y2;

	tr.viewParms.viewportX = x;
	tr.viewParms.viewportY = y2; 
	tr.viewParms.viewportWidth = w;
	tr.viewParms.viewportHeight = h;

	//
	// set up projection matrix
	//
	R_RotateForViewer();
}

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

void R_Flash( void )
{
	R_PolyBlend ();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	tr.frameSceneNum = 0;

	R_CommonRenderScene(fd);

	r_newrefdef = *fd;

	glState.finishCalled = false;

	if (!tr.worldModel && !(tr.refdef.rdflags & RDF_NOWORLDMODEL))
		ri.Sys_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlightsQ2 ();

	R_SetupFrame ();

	VectorCopy(tr.refdef.vieworg, tr.viewParms.orient.origin);
	VectorCopy(tr.refdef.viewaxis[0], tr.viewParms.orient.axis[0]);
	VectorCopy(tr.refdef.viewaxis[1], tr.viewParms.orient.axis[1]);
	VectorCopy(tr.refdef.viewaxis[2], tr.viewParms.orient.axis[2]);
	tr.viewParms.fovX = tr.refdef.fov_x;
	tr.viewParms.fovY = tr.refdef.fov_y;
	R_SetupFrustum();

	R_SetupGL ();

	R_GenerateDrawSurfs();

	R_Flash();

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
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPointQ2 (tr.refdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );
	R_SetLightLevel ();
	RB_SetGL2D ();
}


void R_Register( void )
{
	R_SharedRegister();
	r_norefresh = Cvar_Get ("r_norefresh", "0", 0);

	gl_nosubimage = Cvar_Get( "gl_nosubimage", "0", 0 );

	gl_polyblend = Cvar_Get ("gl_polyblend", "1", 0);
	gl_driver = Cvar_Get( "gl_driver", "opengl32", CVAR_ARCHIVE );

	vid_ref = Cvar_Get( "vid_ref", "soft", CVAR_ARCHIVE );
}

/*
===============
R_Init
===============
*/
int R_Init()
{	
	int		err;
	int		j;

	ri.Con_Printf (PRINT_ALL, "ref_gl version: "REF_VERSION"\n");

	R_CommonInit1();

	R_Register();

	R_CommonInit2();

	r_fullscreen->modified = false;
	r_mode->modified = false;

	// let the sound and input subsystems know about the new window
	VID_NewWindow(glConfig.vidWidth, glConfig.vidHeight);

	Cvar_SetLatched( "scr_drawall", "0" );

	Draw_InitLocal ();

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Con_Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	tr.registered = true;
	return 0;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{	
	R_CommonShutdown(true);
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

	/*
	** change modes if necessary
	*/
	if ( r_mode->modified || r_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		QCvar	*ref;

		ref = Cvar_Get ("vid_ref", "gl", 0);
		ref->modified = true;
	}

	QGL_EnableLogging(!!r_logFile->integer);

	QGL_LogComment("*** R_BeginFrame ***\n");

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	RB_SetGL2D();

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


void	R_BeginRegistration (const char *map);
image_t	*R_RegisterSkinQ2 (const char *name);
void	R_EndRegistration (void);

void	R_RenderFrame (refdef_t *fd);

image_t	*Draw_FindPic (const char *name);

void	Draw_Pic (int x, int y, const char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterSkin = R_RegisterSkinQ2;
	re.RegisterPic = Draw_FindPic;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen= Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Shutdown = R_Shutdown;

	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;

	return re;
}

void R_ClearScreen()
{
	qglClearColor(0, 0, 0, 0);
	qglClear(GL_COLOR_BUFFER_BIT);
}

void R_RenderView(viewParms_t *parms)
{
}
