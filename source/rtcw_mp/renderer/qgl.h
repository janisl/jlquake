/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

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

#pragma warning (disable: 4201)
#pragma warning (disable: 4214)
#pragma warning (disable: 4514)
#pragma warning (disable: 4032)
#pragma warning (disable: 4201)
#pragma warning (disable: 4214)
#include <windows.h>
#include <gl/gl.h>

#elif defined( MACOS_X )

#include "macosx_glimp.h"

#elif defined( __linux__ )

#include <GL/glx.h>
// bk001129 - from cvs1.17 (mkv)
#if defined( __FX__ )
#include <GL/fxmesa.h>
#endif

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


//===========================================================================

// TTimo: FIXME
// linux needs those prototypes
// GL_VERSION_1_2 is defined after #include <gl.h>
#if !defined( GL_VERSION_1_2 ) /* || defined(__linux__) */ || defined( MACOS_X )
typedef void ( APIENTRY * PFNGLMULTITEXCOORD1DARBPROC )( GLenum target, GLdouble s );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD1DVARBPROC )( GLenum target, const GLdouble *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD1FARBPROC )( GLenum target, GLfloat s );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD1FVARBPROC )( GLenum target, const GLfloat *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD1IARBPROC )( GLenum target, GLint s );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD1IVARBPROC )( GLenum target, const GLint *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD1SARBPROC )( GLenum target, GLshort s );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD1SVARBPROC )( GLenum target, const GLshort *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD2DARBPROC )( GLenum target, GLdouble s, GLdouble t );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD2DVARBPROC )( GLenum target, const GLdouble *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD2FARBPROC )( GLenum target, GLfloat s, GLfloat t );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD2FVARBPROC )( GLenum target, const GLfloat *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD2IARBPROC )( GLenum target, GLint s, GLint t );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD2IVARBPROC )( GLenum target, const GLint *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD2SARBPROC )( GLenum target, GLshort s, GLshort t );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD2SVARBPROC )( GLenum target, const GLshort *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD3DARBPROC )( GLenum target, GLdouble s, GLdouble t, GLdouble r );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD3DVARBPROC )( GLenum target, const GLdouble *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD3FARBPROC )( GLenum target, GLfloat s, GLfloat t, GLfloat r );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD3FVARBPROC )( GLenum target, const GLfloat *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD3IARBPROC )( GLenum target, GLint s, GLint t, GLint r );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD3IVARBPROC )( GLenum target, const GLint *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD3SARBPROC )( GLenum target, GLshort s, GLshort t, GLshort r );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD3SVARBPROC )( GLenum target, const GLshort *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD4DARBPROC )( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD4DVARBPROC )( GLenum target, const GLdouble *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD4FARBPROC )( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD4FVARBPROC )( GLenum target, const GLfloat *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD4IARBPROC )( GLenum target, GLint s, GLint t, GLint r, GLint q );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD4IVARBPROC )( GLenum target, const GLint *v );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD4SARBPROC )( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
typedef void ( APIENTRY * PFNGLMULTITEXCOORD4SVARBPROC )( GLenum target, const GLshort *v );
typedef void ( APIENTRY * PFNGLACTIVETEXTUREARBPROC )( GLenum target );
typedef void ( APIENTRY * PFNGLCLIENTACTIVETEXTUREARBPROC )( GLenum target );
#endif

/*
** extension constants
*/

// GL_ATI_pn_triangles
#ifndef GL_ATI_pn_triangles
#define GL_ATI_pn_triangles 1
#ifndef __MACOS__   //DAJ BUGFIX changed the numbers
#define GL_PN_TRIANGLES_ATI                         0x6090
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI   0x6091
#define GL_PN_TRIANGLES_POINT_MODE_ATI              0x6092
#define GL_PN_TRIANGLES_NORMAL_MODE_ATI             0x6093
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI       0x6094
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI       0x6095
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI        0x6096
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI      0x6097
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI   0x6098
#else
#define GL_PN_TRIANGLES_ATI                         0x87F0
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI   0x87F1
#define GL_PN_TRIANGLES_POINT_MODE_ATI              0x87F2
#define GL_PN_TRIANGLES_NORMAL_MODE_ATI             0x87F3
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI       0x87F4
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI       0x87F5
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI        0x87F6
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI      0x87F7
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI   0x87F8
#endif
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

#endif
