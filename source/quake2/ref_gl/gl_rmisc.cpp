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
// r_misc.c

#include "gl_local.h"
#include "../client/client.h"

refexport_t	re;

viddef_t	viddef;				// global video state; used by other modules
qboolean	reflib_active = 0;

/*
==================
R_InitParticleTexture
==================
*/
byte	dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	r_particletexture = R_CreateImage("***particle***", (byte *)data, 8, 8, true, true, GL_CLAMP, false);
}

/* 
================== 
GL_ScreenShot_f
================== 
*/  
void GL_ScreenShot_f (void) 
{
	byte		*buffer;
	char		picname[80]; 
	int			i;

	// 
	// find a file name to save it to 
	// 
	QStr::Cpy(picname, "scrnshot/quake00.tga");

	for (i = 0; i <= 99; i++) 
	{
		picname[14] = i/10 + '0';
		picname[15] = i%10 + '0';
		if (!FS_FileExists(picname))
		{
			break;	// file doesn't exist
		}
	} 
	if (i==100) 
	{
		ri.Con_Printf(PRINT_ALL, "SCR_ScreenShot_f: Couldn't create a file\n");
		return;
	}

	buffer = (byte*)malloc(glConfig.vidWidth * glConfig.vidHeight * 3);

	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	R_SaveTGA(picname, buffer, glConfig.vidWidth, glConfig.vidHeight, false);

	free(buffer);
	ri.Con_Printf(PRINT_ALL, "Wrote %s\n", picname);
} 

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearColor (1,0, 0.5 , 0.5);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglDisable (GL_CULL_FACE);
	qglDepthFunc (GL_LEQUAL);

	qglColor4f (1,1,1,1);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_FLAT);

	GL_TextureMode( r_textureMode->string );

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

	if ( qglPointParameterfEXT )
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		qglEnable( GL_POINT_SMOOTH );
		qglPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		qglPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		qglPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}

	GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval( void )
{
	if ( gl_swapinterval->modified )
	{
		gl_swapinterval->modified = false;

		if ( !glConfig.stereoEnabled ) 
		{
#ifdef _WIN32
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( gl_swapinterval->value );
#endif
		}
	}
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_separation )
{
	if ( camera_separation < 0 && glConfig.stereoEnabled )
	{
		qglDrawBuffer( GL_BACK_LEFT );
	}
	else if ( camera_separation > 0 && glConfig.stereoEnabled )
	{
		qglDrawBuffer( GL_BACK_RIGHT );
	}
	else
	{
		qglDrawBuffer( GL_BACK );
	}
}

/*
==========================================================================

DLL GLUE

==========================================================================
*/

static void VID_Printf (int print_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start (argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end (argptr);

	if (print_level == PRINT_ALL)
	{
		Com_Printf ("%s", msg);
	}
	else
	{
		Com_DPrintf ("%s", msg);
	}
}

static void VID_Error (int err_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start (argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end (argptr);

	Com_Error (err_level,"%s", msg);
}

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f (void)
{
	vid_ref->modified = true;
}

/*
** VID_NewWindow
*/
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;		// can't use a paused refdef
}

void VID_FreeReflib (void)
{
	Com_Memset(&re, 0, sizeof(re));
	reflib_active  = false;
}

/*
==============
VID_LoadRefresh
==============
*/
qboolean VID_LoadRefresh()
{
	refimport_t	ri;
	
	if ( reflib_active )
	{
		re.Shutdown();
		VID_FreeReflib ();
	}

	ri.Con_Printf = VID_Printf;
	ri.Sys_Error = VID_Error;

	re = GetRefAPI( ri );

	if (R_Init() == -1 )
	{
		re.Shutdown();
		VID_FreeReflib ();
		return false;
	}

	Com_Printf( "------------------------------------\n");
	reflib_active = true;
	return true;
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if ( reflib_active )
	{
		re.Shutdown ();
		VID_FreeReflib ();
	}
}

/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
	int err = qglGetError();
	qassert(err == GL_NO_ERROR);

	if (QStr::ICmp(gl_drawbuffer->string, "GL_BACK") == 0)
	{
		GLimp_SwapBuffers();
	}
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void)
{
	vid_ref = Cvar_Get( "vid_ref", "soft", CVAR_ARCHIVE );

	if ( vid_ref->modified )
	{
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds();
		/*
		** refresh has changed
		*/
		vid_ref->modified = false;
		//FIXME
		if (r_fullscreen)
			r_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		if (!VID_LoadRefresh())
		{
			Com_Error(ERR_FATAL, "Couldn't init refresh!");
		}
		cls.disable_screen = false;
	}
}

/*
============
VID_Init
============
*/
void VID_Init (void)
{
	/* Add some console commands that we want to handle */
	Cmd_AddCommand ("vid_restart", VID_Restart_f);

	/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}
