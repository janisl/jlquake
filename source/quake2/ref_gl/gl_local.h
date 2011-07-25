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
#include <stdio.h>

#include <math.h>

#include "../../client/client.h"
#include "../client/ref.h"

#include "qgl.h"

#include <GL/glu.h>

#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif

#define	REF_VERSION	"GL 0.01"

//===================================================================

#include "gl_model.h"

void GL_EndRendering (void);

void GL_UpdateSwapInterval( void );

//====================================================

extern	QCvar	*gl_polyblend;

void R_TranslatePlayerSkin (int playernum);

//====================================================================


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

void CL_InitRenderStuff();

mbrush38_glpoly_t *WaterWarpPolyVerts (mbrush38_glpoly_t *p);

void COM_StripExtension (char *in, char *out);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );

/*
** GL extension emulation functions
*/
void GL_DrawParticles( int n, const particle_t particles[], const unsigned colortable[768] );

typedef struct
{
	float camera_separation;
} glstate2_t;

extern glstate2_t   gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;

extern qboolean		reflib_active;

refexport_t GetRefAPI (refimport_t rimp);

void VID_Restart_f (void);
void VID_NewWindow ( int width, int height);
void VID_FreeReflib (void);


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
