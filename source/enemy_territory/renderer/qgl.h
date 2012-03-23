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

/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#if defined( __LINT__ )

#elif defined( _WIN32 )

#if !defined( __GNUC__ )
#pragma warning (disable: 4201)
#pragma warning (disable: 4214)
#pragma warning (disable: 4514)
#pragma warning (disable: 4032)
#pragma warning (disable: 4201)
#pragma warning (disable: 4214)
#endif /* __GNUC__ */

#elif defined( __MACOS__ )

#include <OpenGL/gl.h>

#elif defined( __linux__ )

// using our local glext.h
// http://oss.sgi.com/projects/ogl-sample/ABI/
#define GL_GLEXT_LEGACY
#define GLX_GLXEXT_LEGACY
// some GL headers define that, but only partially
// define and undefine so GL doesn't attempt anything
#define GL_ARB_multitexture
#include <GL/glx.h>
#undef GL_ARB_multitexture
#include "glext.h"

#elif defined( __FreeBSD__ ) // rb010123

#include <GL/glx.h>
#if defined( __FX__ )
#include <GL/fxmesa.h>
#endif

#else

#include <gl.h>

#endif

#ifndef WINAPI
#define WINAPI
#endif

/*
** extension constants
*/

// GL_ATI_pn_triangles
#ifndef GL_ATI_pn_triangles
#define GL_ATI_pn_triangles 1

#define GL_PN_TRIANGLES_ATI                         0x6090
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI   0x6091
#define GL_PN_TRIANGLES_POINT_MODE_ATI              0x6092
#define GL_PN_TRIANGLES_NORMAL_MODE_ATI             0x6093
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI       0x6094
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI       0x6095
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI        0x6096
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI      0x6097
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI   0x6098

typedef void ( APIENTRY * PFNGLPNTRIANGLESIATIPROC )( GLenum pname, GLint param );
typedef void ( APIENTRY * PFNGLPNTRIANGLESFATIPROC )( GLenum pname, GLfloat param );
#endif

// for NV fog distance
#ifndef GL_NV_fog_distance
#define GL_FOG_DISTANCE_MODE_NV           0x855A
#define GL_EYE_RADIAL_NV                  0x855B
#define GL_EYE_PLANE_ABSOLUTE_NV          0x855C
/* reuse GL_EYE_PLANE */
#endif

// GL_EXT_texture_compression_s3tc constants
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3

// GL_EXT_texture_filter_anisotropic constants
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

// GL_SGIS_generate_mipmap
#ifndef GL_SGIS_generate_mipmap
#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192
#endif

#endif
