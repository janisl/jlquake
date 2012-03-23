/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
** LINUX_QGL.C
**
** This file implements the operating system binding of GL to QGL function
** pointers.  When doing a port of Quake2 you must implement the following
** two functions:
**
** QGL_Init() - loads libraries, assigns function pointers, etc.
** QGL_Shutdown() - unloads libraries, NULLs function pointers
*/

// bk001204
#include <unistd.h>
#include <sys/types.h>


#include <float.h>
#include "../renderer/tr_local.h"

// bk001129 - from cvs1.17 (mkv)
//#if defined(__FX__)
//#include <GL/fxmesa.h>
//#endif
//#include <GL/glx.h> // bk010216 - FIXME: all of the above redundant? renderer/qgl.h

#include <dlfcn.h>

void *OpenGLLib; // instance of OpenGL library

static void ( APIENTRY * dllAccum )( GLenum op, GLfloat value );
static void ( APIENTRY * dllAlphaFunc )( GLenum func, GLclampf ref );
GLboolean ( APIENTRY * dllAreTexturesResident )( GLsizei n, const GLuint *textures, GLboolean *residences );
static void ( APIENTRY * dllArrayElement )( GLint i );
static void ( APIENTRY * dllBegin )( GLenum mode );
static void ( APIENTRY * dllBindTexture )( GLenum target, GLuint texture );
static void ( APIENTRY * dllBitmap )( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap );
static void ( APIENTRY * dllBlendFunc )( GLenum sfactor, GLenum dfactor );
static void ( APIENTRY * dllCallList )( GLuint list );
static void ( APIENTRY * dllCallLists )( GLsizei n, GLenum type, const GLvoid *lists );
static void ( APIENTRY * dllClear )( GLbitfield mask );
static void ( APIENTRY * dllClearAccum )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
static void ( APIENTRY * dllClearColor )( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
static void ( APIENTRY * dllClearDepth )( GLclampd depth );
static void ( APIENTRY * dllClearIndex )( GLfloat c );
static void ( APIENTRY * dllClearStencil )( GLint s );
static void ( APIENTRY * dllClipPlane )( GLenum plane, const GLdouble *equation );
static void ( APIENTRY * dllColor3b )( GLbyte red, GLbyte green, GLbyte blue );
static void ( APIENTRY * dllColor3bv )( const GLbyte *v );
static void ( APIENTRY * dllColor3d )( GLdouble red, GLdouble green, GLdouble blue );
static void ( APIENTRY * dllColor3dv )( const GLdouble *v );
static void ( APIENTRY * dllColor3f )( GLfloat red, GLfloat green, GLfloat blue );
static void ( APIENTRY * dllColor3fv )( const GLfloat *v );
static void ( APIENTRY * dllColor3i )( GLint red, GLint green, GLint blue );
static void ( APIENTRY * dllColor3iv )( const GLint *v );
static void ( APIENTRY * dllColor3s )( GLshort red, GLshort green, GLshort blue );
static void ( APIENTRY * dllColor3sv )( const GLshort *v );
static void ( APIENTRY * dllColor3ub )( GLubyte red, GLubyte green, GLubyte blue );
static void ( APIENTRY * dllColor3ubv )( const GLubyte *v );
static void ( APIENTRY * dllColor3ui )( GLuint red, GLuint green, GLuint blue );
static void ( APIENTRY * dllColor3uiv )( const GLuint *v );
static void ( APIENTRY * dllColor3us )( GLushort red, GLushort green, GLushort blue );
static void ( APIENTRY * dllColor3usv )( const GLushort *v );
static void ( APIENTRY * dllColor4b )( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha );
static void ( APIENTRY * dllColor4bv )( const GLbyte *v );
static void ( APIENTRY * dllColor4d )( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha );
static void ( APIENTRY * dllColor4dv )( const GLdouble *v );
static void ( APIENTRY * dllColor4f )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
static void ( APIENTRY * dllColor4fv )( const GLfloat *v );
static void ( APIENTRY * dllColor4i )( GLint red, GLint green, GLint blue, GLint alpha );
static void ( APIENTRY * dllColor4iv )( const GLint *v );
static void ( APIENTRY * dllColor4s )( GLshort red, GLshort green, GLshort blue, GLshort alpha );
static void ( APIENTRY * dllColor4sv )( const GLshort *v );
static void ( APIENTRY * dllColor4ub )( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
static void ( APIENTRY * dllColor4ubv )( const GLubyte *v );
static void ( APIENTRY * dllColor4ui )( GLuint red, GLuint green, GLuint blue, GLuint alpha );
static void ( APIENTRY * dllColor4uiv )( const GLuint *v );
static void ( APIENTRY * dllColor4us )( GLushort red, GLushort green, GLushort blue, GLushort alpha );
static void ( APIENTRY * dllColor4usv )( const GLushort *v );
static void ( APIENTRY * dllColorMask )( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
static void ( APIENTRY * dllColorMaterial )( GLenum face, GLenum mode );
static void ( APIENTRY * dllColorPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
static void ( APIENTRY * dllCopyPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type );
static void ( APIENTRY * dllCopyTexImage1D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border );
static void ( APIENTRY * dllCopyTexImage2D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
static void ( APIENTRY * dllCopyTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
static void ( APIENTRY * dllCopyTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
static void ( APIENTRY * dllCullFace )( GLenum mode );
static void ( APIENTRY * dllDeleteLists )( GLuint list, GLsizei range );
static void ( APIENTRY * dllDeleteTextures )( GLsizei n, const GLuint *textures );
static void ( APIENTRY * dllDepthFunc )( GLenum func );
static void ( APIENTRY * dllDepthMask )( GLboolean flag );
static void ( APIENTRY * dllDepthRange )( GLclampd zNear, GLclampd zFar );
static void ( APIENTRY * dllDisable )( GLenum cap );
static void ( APIENTRY * dllDisableClientState )( GLenum array );
static void ( APIENTRY * dllDrawArrays )( GLenum mode, GLint first, GLsizei count );
static void ( APIENTRY * dllDrawBuffer )( GLenum mode );
static void ( APIENTRY * dllDrawElements )( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
static void ( APIENTRY * dllDrawPixels )( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
static void ( APIENTRY * dllEdgeFlag )( GLboolean flag );
static void ( APIENTRY * dllEdgeFlagPointer )( GLsizei stride, const GLvoid *pointer );
static void ( APIENTRY * dllEdgeFlagv )( const GLboolean *flag );
static void ( APIENTRY * dllEnable )( GLenum cap );
static void ( APIENTRY * dllEnableClientState )( GLenum array );
static void ( APIENTRY * dllEnd )( void );
static void ( APIENTRY * dllEndList )( void );
static void ( APIENTRY * dllEvalCoord1d )( GLdouble u );
static void ( APIENTRY * dllEvalCoord1dv )( const GLdouble *u );
static void ( APIENTRY * dllEvalCoord1f )( GLfloat u );
static void ( APIENTRY * dllEvalCoord1fv )( const GLfloat *u );
static void ( APIENTRY * dllEvalCoord2d )( GLdouble u, GLdouble v );
static void ( APIENTRY * dllEvalCoord2dv )( const GLdouble *u );
static void ( APIENTRY * dllEvalCoord2f )( GLfloat u, GLfloat v );
static void ( APIENTRY * dllEvalCoord2fv )( const GLfloat *u );
static void ( APIENTRY * dllEvalMesh1 )( GLenum mode, GLint i1, GLint i2 );
static void ( APIENTRY * dllEvalMesh2 )( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
static void ( APIENTRY * dllEvalPoint1 )( GLint i );
static void ( APIENTRY * dllEvalPoint2 )( GLint i, GLint j );
static void ( APIENTRY * dllFeedbackBuffer )( GLsizei size, GLenum type, GLfloat *buffer );
static void ( APIENTRY * dllFinish )( void );
static void ( APIENTRY * dllFlush )( void );
static void ( APIENTRY * dllFogf )( GLenum pname, GLfloat param );
static void ( APIENTRY * dllFogfv )( GLenum pname, const GLfloat *params );
static void ( APIENTRY * dllFogi )( GLenum pname, GLint param );
static void ( APIENTRY * dllFogiv )( GLenum pname, const GLint *params );
static void ( APIENTRY * dllFrontFace )( GLenum mode );
static void ( APIENTRY * dllFrustum )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
GLuint ( APIENTRY * dllGenLists )( GLsizei range );
static void ( APIENTRY * dllGenTextures )( GLsizei n, GLuint *textures );
static void ( APIENTRY * dllGetBooleanv )( GLenum pname, GLboolean *params );
static void ( APIENTRY * dllGetClipPlane )( GLenum plane, GLdouble *equation );
static void ( APIENTRY * dllGetDoublev )( GLenum pname, GLdouble *params );
GLenum ( APIENTRY * dllGetError )( void );
static void ( APIENTRY * dllGetFloatv )( GLenum pname, GLfloat *params );
static void ( APIENTRY * dllGetIntegerv )( GLenum pname, GLint *params );
static void ( APIENTRY * dllGetLightfv )( GLenum light, GLenum pname, GLfloat *params );
static void ( APIENTRY * dllGetLightiv )( GLenum light, GLenum pname, GLint *params );
static void ( APIENTRY * dllGetMapdv )( GLenum target, GLenum query, GLdouble *v );
static void ( APIENTRY * dllGetMapfv )( GLenum target, GLenum query, GLfloat *v );
static void ( APIENTRY * dllGetMapiv )( GLenum target, GLenum query, GLint *v );
static void ( APIENTRY * dllGetMaterialfv )( GLenum face, GLenum pname, GLfloat *params );
static void ( APIENTRY * dllGetMaterialiv )( GLenum face, GLenum pname, GLint *params );
static void ( APIENTRY * dllGetPixelMapfv )( GLenum map, GLfloat *values );
static void ( APIENTRY * dllGetPixelMapuiv )( GLenum map, GLuint *values );
static void ( APIENTRY * dllGetPixelMapusv )( GLenum map, GLushort *values );
static void ( APIENTRY * dllGetPointerv )( GLenum pname, GLvoid* *params );
static void ( APIENTRY * dllGetPolygonStipple )( GLubyte *mask );
const GLubyte * ( APIENTRY * dllGetString )(GLenum name);
static void ( APIENTRY * dllGetTexEnvfv )( GLenum target, GLenum pname, GLfloat *params );
static void ( APIENTRY * dllGetTexEnviv )( GLenum target, GLenum pname, GLint *params );
static void ( APIENTRY * dllGetTexGendv )( GLenum coord, GLenum pname, GLdouble *params );
static void ( APIENTRY * dllGetTexGenfv )( GLenum coord, GLenum pname, GLfloat *params );
static void ( APIENTRY * dllGetTexGeniv )( GLenum coord, GLenum pname, GLint *params );
static void ( APIENTRY * dllGetTexImage )( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels );
static void ( APIENTRY * dllGetTexLevelParameterfv )( GLenum target, GLint level, GLenum pname, GLfloat *params );
static void ( APIENTRY * dllGetTexLevelParameteriv )( GLenum target, GLint level, GLenum pname, GLint *params );
static void ( APIENTRY * dllGetTexParameterfv )( GLenum target, GLenum pname, GLfloat *params );
static void ( APIENTRY * dllGetTexParameteriv )( GLenum target, GLenum pname, GLint *params );
static void ( APIENTRY * dllHint )( GLenum target, GLenum mode );
static void ( APIENTRY * dllIndexMask )( GLuint mask );
static void ( APIENTRY * dllIndexPointer )( GLenum type, GLsizei stride, const GLvoid *pointer );
static void ( APIENTRY * dllIndexd )( GLdouble c );
static void ( APIENTRY * dllIndexdv )( const GLdouble *c );
static void ( APIENTRY * dllIndexf )( GLfloat c );
static void ( APIENTRY * dllIndexfv )( const GLfloat *c );
static void ( APIENTRY * dllIndexi )( GLint c );
static void ( APIENTRY * dllIndexiv )( const GLint *c );
static void ( APIENTRY * dllIndexs )( GLshort c );
static void ( APIENTRY * dllIndexsv )( const GLshort *c );
static void ( APIENTRY * dllIndexub )( GLubyte c );
static void ( APIENTRY * dllIndexubv )( const GLubyte *c );
static void ( APIENTRY * dllInitNames )( void );
static void ( APIENTRY * dllInterleavedArrays )( GLenum format, GLsizei stride, const GLvoid *pointer );
GLboolean ( APIENTRY * dllIsEnabled )( GLenum cap );
GLboolean ( APIENTRY * dllIsList )( GLuint list );
GLboolean ( APIENTRY * dllIsTexture )( GLuint texture );
static void ( APIENTRY * dllLightModelf )( GLenum pname, GLfloat param );
static void ( APIENTRY * dllLightModelfv )( GLenum pname, const GLfloat *params );
static void ( APIENTRY * dllLightModeli )( GLenum pname, GLint param );
static void ( APIENTRY * dllLightModeliv )( GLenum pname, const GLint *params );
static void ( APIENTRY * dllLightf )( GLenum light, GLenum pname, GLfloat param );
static void ( APIENTRY * dllLightfv )( GLenum light, GLenum pname, const GLfloat *params );
static void ( APIENTRY * dllLighti )( GLenum light, GLenum pname, GLint param );
static void ( APIENTRY * dllLightiv )( GLenum light, GLenum pname, const GLint *params );
static void ( APIENTRY * dllLineStipple )( GLint factor, GLushort pattern );
static void ( APIENTRY * dllLineWidth )( GLfloat width );
static void ( APIENTRY * dllListBase )( GLuint base );
static void ( APIENTRY * dllLoadIdentity )( void );
static void ( APIENTRY * dllLoadMatrixd )( const GLdouble *m );
static void ( APIENTRY * dllLoadMatrixf )( const GLfloat *m );
static void ( APIENTRY * dllLoadName )( GLuint name );
static void ( APIENTRY * dllLogicOp )( GLenum opcode );
static void ( APIENTRY * dllMap1d )( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
static void ( APIENTRY * dllMap1f )( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
static void ( APIENTRY * dllMap2d )( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
static void ( APIENTRY * dllMap2f )( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
static void ( APIENTRY * dllMapGrid1d )( GLint un, GLdouble u1, GLdouble u2 );
static void ( APIENTRY * dllMapGrid1f )( GLint un, GLfloat u1, GLfloat u2 );
static void ( APIENTRY * dllMapGrid2d )( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 );
static void ( APIENTRY * dllMapGrid2f )( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 );
static void ( APIENTRY * dllMaterialf )( GLenum face, GLenum pname, GLfloat param );
static void ( APIENTRY * dllMaterialfv )( GLenum face, GLenum pname, const GLfloat *params );
static void ( APIENTRY * dllMateriali )( GLenum face, GLenum pname, GLint param );
static void ( APIENTRY * dllMaterialiv )( GLenum face, GLenum pname, const GLint *params );
static void ( APIENTRY * dllMatrixMode )( GLenum mode );
static void ( APIENTRY * dllMultMatrixd )( const GLdouble *m );
static void ( APIENTRY * dllMultMatrixf )( const GLfloat *m );
static void ( APIENTRY * dllNewList )( GLuint list, GLenum mode );
static void ( APIENTRY * dllNormal3b )( GLbyte nx, GLbyte ny, GLbyte nz );
static void ( APIENTRY * dllNormal3bv )( const GLbyte *v );
static void ( APIENTRY * dllNormal3d )( GLdouble nx, GLdouble ny, GLdouble nz );
static void ( APIENTRY * dllNormal3dv )( const GLdouble *v );
static void ( APIENTRY * dllNormal3f )( GLfloat nx, GLfloat ny, GLfloat nz );
static void ( APIENTRY * dllNormal3fv )( const GLfloat *v );
static void ( APIENTRY * dllNormal3i )( GLint nx, GLint ny, GLint nz );
static void ( APIENTRY * dllNormal3iv )( const GLint *v );
static void ( APIENTRY * dllNormal3s )( GLshort nx, GLshort ny, GLshort nz );
static void ( APIENTRY * dllNormal3sv )( const GLshort *v );
static void ( APIENTRY * dllNormalPointer )( GLenum type, GLsizei stride, const GLvoid *pointer );
static void ( APIENTRY * dllOrtho )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
static void ( APIENTRY * dllPassThrough )( GLfloat token );
static void ( APIENTRY * dllPixelMapfv )( GLenum map, GLsizei mapsize, const GLfloat *values );
static void ( APIENTRY * dllPixelMapuiv )( GLenum map, GLsizei mapsize, const GLuint *values );
static void ( APIENTRY * dllPixelMapusv )( GLenum map, GLsizei mapsize, const GLushort *values );
static void ( APIENTRY * dllPixelStoref )( GLenum pname, GLfloat param );
static void ( APIENTRY * dllPixelStorei )( GLenum pname, GLint param );
static void ( APIENTRY * dllPixelTransferf )( GLenum pname, GLfloat param );
static void ( APIENTRY * dllPixelTransferi )( GLenum pname, GLint param );
static void ( APIENTRY * dllPixelZoom )( GLfloat xfactor, GLfloat yfactor );
static void ( APIENTRY * dllPointSize )( GLfloat size );
static void ( APIENTRY * dllPolygonMode )( GLenum face, GLenum mode );
static void ( APIENTRY * dllPolygonOffset )( GLfloat factor, GLfloat units );
static void ( APIENTRY * dllPolygonStipple )( const GLubyte *mask );
static void ( APIENTRY * dllPopAttrib )( void );
static void ( APIENTRY * dllPopClientAttrib )( void );
static void ( APIENTRY * dllPopMatrix )( void );
static void ( APIENTRY * dllPopName )( void );
static void ( APIENTRY * dllPrioritizeTextures )( GLsizei n, const GLuint *textures, const GLclampf *priorities );
static void ( APIENTRY * dllPushAttrib )( GLbitfield mask );
static void ( APIENTRY * dllPushClientAttrib )( GLbitfield mask );
static void ( APIENTRY * dllPushMatrix )( void );
static void ( APIENTRY * dllPushName )( GLuint name );
static void ( APIENTRY * dllRasterPos2d )( GLdouble x, GLdouble y );
static void ( APIENTRY * dllRasterPos2dv )( const GLdouble *v );
static void ( APIENTRY * dllRasterPos2f )( GLfloat x, GLfloat y );
static void ( APIENTRY * dllRasterPos2fv )( const GLfloat *v );
static void ( APIENTRY * dllRasterPos2i )( GLint x, GLint y );
static void ( APIENTRY * dllRasterPos2iv )( const GLint *v );
static void ( APIENTRY * dllRasterPos2s )( GLshort x, GLshort y );
static void ( APIENTRY * dllRasterPos2sv )( const GLshort *v );
static void ( APIENTRY * dllRasterPos3d )( GLdouble x, GLdouble y, GLdouble z );
static void ( APIENTRY * dllRasterPos3dv )( const GLdouble *v );
static void ( APIENTRY * dllRasterPos3f )( GLfloat x, GLfloat y, GLfloat z );
static void ( APIENTRY * dllRasterPos3fv )( const GLfloat *v );
static void ( APIENTRY * dllRasterPos3i )( GLint x, GLint y, GLint z );
static void ( APIENTRY * dllRasterPos3iv )( const GLint *v );
static void ( APIENTRY * dllRasterPos3s )( GLshort x, GLshort y, GLshort z );
static void ( APIENTRY * dllRasterPos3sv )( const GLshort *v );
static void ( APIENTRY * dllRasterPos4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
static void ( APIENTRY * dllRasterPos4dv )( const GLdouble *v );
static void ( APIENTRY * dllRasterPos4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
static void ( APIENTRY * dllRasterPos4fv )( const GLfloat *v );
static void ( APIENTRY * dllRasterPos4i )( GLint x, GLint y, GLint z, GLint w );
static void ( APIENTRY * dllRasterPos4iv )( const GLint *v );
static void ( APIENTRY * dllRasterPos4s )( GLshort x, GLshort y, GLshort z, GLshort w );
static void ( APIENTRY * dllRasterPos4sv )( const GLshort *v );
static void ( APIENTRY * dllReadBuffer )( GLenum mode );
static void ( APIENTRY * dllReadPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
static void ( APIENTRY * dllRectd )( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 );
static void ( APIENTRY * dllRectdv )( const GLdouble *v1, const GLdouble *v2 );
static void ( APIENTRY * dllRectf )( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );
static void ( APIENTRY * dllRectfv )( const GLfloat *v1, const GLfloat *v2 );
static void ( APIENTRY * dllRecti )( GLint x1, GLint y1, GLint x2, GLint y2 );
static void ( APIENTRY * dllRectiv )( const GLint *v1, const GLint *v2 );
static void ( APIENTRY * dllRects )( GLshort x1, GLshort y1, GLshort x2, GLshort y2 );
static void ( APIENTRY * dllRectsv )( const GLshort *v1, const GLshort *v2 );
GLint ( APIENTRY * dllRenderMode )( GLenum mode );
static void ( APIENTRY * dllRotated )( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
static void ( APIENTRY * dllRotatef )( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
static void ( APIENTRY * dllScaled )( GLdouble x, GLdouble y, GLdouble z );
static void ( APIENTRY * dllScalef )( GLfloat x, GLfloat y, GLfloat z );
static void ( APIENTRY * dllScissor )( GLint x, GLint y, GLsizei width, GLsizei height );
static void ( APIENTRY * dllSelectBuffer )( GLsizei size, GLuint *buffer );
static void ( APIENTRY * dllShadeModel )( GLenum mode );
static void ( APIENTRY * dllStencilFunc )( GLenum func, GLint ref, GLuint mask );
static void ( APIENTRY * dllStencilMask )( GLuint mask );
static void ( APIENTRY * dllStencilOp )( GLenum fail, GLenum zfail, GLenum zpass );
static void ( APIENTRY * dllTexCoord1d )( GLdouble s );
static void ( APIENTRY * dllTexCoord1dv )( const GLdouble *v );
static void ( APIENTRY * dllTexCoord1f )( GLfloat s );
static void ( APIENTRY * dllTexCoord1fv )( const GLfloat *v );
static void ( APIENTRY * dllTexCoord1i )( GLint s );
static void ( APIENTRY * dllTexCoord1iv )( const GLint *v );
static void ( APIENTRY * dllTexCoord1s )( GLshort s );
static void ( APIENTRY * dllTexCoord1sv )( const GLshort *v );
static void ( APIENTRY * dllTexCoord2d )( GLdouble s, GLdouble t );
static void ( APIENTRY * dllTexCoord2dv )( const GLdouble *v );
static void ( APIENTRY * dllTexCoord2f )( GLfloat s, GLfloat t );
static void ( APIENTRY * dllTexCoord2fv )( const GLfloat *v );
static void ( APIENTRY * dllTexCoord2i )( GLint s, GLint t );
static void ( APIENTRY * dllTexCoord2iv )( const GLint *v );
static void ( APIENTRY * dllTexCoord2s )( GLshort s, GLshort t );
static void ( APIENTRY * dllTexCoord2sv )( const GLshort *v );
static void ( APIENTRY * dllTexCoord3d )( GLdouble s, GLdouble t, GLdouble r );
static void ( APIENTRY * dllTexCoord3dv )( const GLdouble *v );
static void ( APIENTRY * dllTexCoord3f )( GLfloat s, GLfloat t, GLfloat r );
static void ( APIENTRY * dllTexCoord3fv )( const GLfloat *v );
static void ( APIENTRY * dllTexCoord3i )( GLint s, GLint t, GLint r );
static void ( APIENTRY * dllTexCoord3iv )( const GLint *v );
static void ( APIENTRY * dllTexCoord3s )( GLshort s, GLshort t, GLshort r );
static void ( APIENTRY * dllTexCoord3sv )( const GLshort *v );
static void ( APIENTRY * dllTexCoord4d )( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
static void ( APIENTRY * dllTexCoord4dv )( const GLdouble *v );
static void ( APIENTRY * dllTexCoord4f )( GLfloat s, GLfloat t, GLfloat r, GLfloat q );
static void ( APIENTRY * dllTexCoord4fv )( const GLfloat *v );
static void ( APIENTRY * dllTexCoord4i )( GLint s, GLint t, GLint r, GLint q );
static void ( APIENTRY * dllTexCoord4iv )( const GLint *v );
static void ( APIENTRY * dllTexCoord4s )( GLshort s, GLshort t, GLshort r, GLshort q );
static void ( APIENTRY * dllTexCoord4sv )( const GLshort *v );
static void ( APIENTRY * dllTexCoordPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
static void ( APIENTRY * dllTexEnvf )( GLenum target, GLenum pname, GLfloat param );
static void ( APIENTRY * dllTexEnvfv )( GLenum target, GLenum pname, const GLfloat *params );
static void ( APIENTRY * dllTexEnvi )( GLenum target, GLenum pname, GLint param );
static void ( APIENTRY * dllTexEnviv )( GLenum target, GLenum pname, const GLint *params );
static void ( APIENTRY * dllTexGend )( GLenum coord, GLenum pname, GLdouble param );
static void ( APIENTRY * dllTexGendv )( GLenum coord, GLenum pname, const GLdouble *params );
static void ( APIENTRY * dllTexGenf )( GLenum coord, GLenum pname, GLfloat param );
static void ( APIENTRY * dllTexGenfv )( GLenum coord, GLenum pname, const GLfloat *params );
static void ( APIENTRY * dllTexGeni )( GLenum coord, GLenum pname, GLint param );
static void ( APIENTRY * dllTexGeniv )( GLenum coord, GLenum pname, const GLint *params );
static void ( APIENTRY * dllTexImage1D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
static void ( APIENTRY * dllTexImage2D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
static void ( APIENTRY * dllTexParameterf )( GLenum target, GLenum pname, GLfloat param );
static void ( APIENTRY * dllTexParameterfv )( GLenum target, GLenum pname, const GLfloat *params );
static void ( APIENTRY * dllTexParameteri )( GLenum target, GLenum pname, GLint param );
static void ( APIENTRY * dllTexParameteriv )( GLenum target, GLenum pname, const GLint *params );
static void ( APIENTRY * dllTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels );
static void ( APIENTRY * dllTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
static void ( APIENTRY * dllTranslated )( GLdouble x, GLdouble y, GLdouble z );
static void ( APIENTRY * dllTranslatef )( GLfloat x, GLfloat y, GLfloat z );
static void ( APIENTRY * dllVertex2d )( GLdouble x, GLdouble y );
static void ( APIENTRY * dllVertex2dv )( const GLdouble *v );
static void ( APIENTRY * dllVertex2f )( GLfloat x, GLfloat y );
static void ( APIENTRY * dllVertex2fv )( const GLfloat *v );
static void ( APIENTRY * dllVertex2i )( GLint x, GLint y );
static void ( APIENTRY * dllVertex2iv )( const GLint *v );
static void ( APIENTRY * dllVertex2s )( GLshort x, GLshort y );
static void ( APIENTRY * dllVertex2sv )( const GLshort *v );
static void ( APIENTRY * dllVertex3d )( GLdouble x, GLdouble y, GLdouble z );
static void ( APIENTRY * dllVertex3dv )( const GLdouble *v );
static void ( APIENTRY * dllVertex3f )( GLfloat x, GLfloat y, GLfloat z );
static void ( APIENTRY * dllVertex3fv )( const GLfloat *v );
static void ( APIENTRY * dllVertex3i )( GLint x, GLint y, GLint z );
static void ( APIENTRY * dllVertex3iv )( const GLint *v );
static void ( APIENTRY * dllVertex3s )( GLshort x, GLshort y, GLshort z );
static void ( APIENTRY * dllVertex3sv )( const GLshort *v );
static void ( APIENTRY * dllVertex4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
static void ( APIENTRY * dllVertex4dv )( const GLdouble *v );
static void ( APIENTRY * dllVertex4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
static void ( APIENTRY * dllVertex4fv )( const GLfloat *v );
static void ( APIENTRY * dllVertex4i )( GLint x, GLint y, GLint z, GLint w );
static void ( APIENTRY * dllVertex4iv )( const GLint *v );
static void ( APIENTRY * dllVertex4s )( GLshort x, GLshort y, GLshort z, GLshort w );
static void ( APIENTRY * dllVertex4sv )( const GLshort *v );
static void ( APIENTRY * dllVertexPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
static void ( APIENTRY * dllViewport )( GLint x, GLint y, GLsizei width, GLsizei height );

/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.
*/
void QGL_Shutdown_();
void QGL_Shutdown( void ) {
	QGL_Shutdown_();
	if ( OpenGLLib ) {
		dlclose( OpenGLLib );
		OpenGLLib = NULL;
	}

	OpenGLLib = NULL;

	qglAccum                     = NULL;
	qglAlphaFunc                 = NULL;
	qglAreTexturesResident       = NULL;
	qglArrayElement              = NULL;
	qglBegin                     = NULL;
	qglBindTexture               = NULL;
	qglBitmap                    = NULL;
	qglBlendFunc                 = NULL;
	qglCallList                  = NULL;
	qglCallLists                 = NULL;
	qglClear                     = NULL;
	qglClearAccum                = NULL;
	qglClearColor                = NULL;
	qglClearDepth                = NULL;
	qglClearIndex                = NULL;
	qglClearStencil              = NULL;
	qglClipPlane                 = NULL;
	qglColor3b                   = NULL;
	qglColor3bv                  = NULL;
	qglColor3d                   = NULL;
	qglColor3dv                  = NULL;
	qglColor3f                   = NULL;
	qglColor3fv                  = NULL;
	qglColor3i                   = NULL;
	qglColor3iv                  = NULL;
	qglColor3s                   = NULL;
	qglColor3sv                  = NULL;
	qglColor3ub                  = NULL;
	qglColor3ubv                 = NULL;
	qglColor3ui                  = NULL;
	qglColor3uiv                 = NULL;
	qglColor3us                  = NULL;
	qglColor3usv                 = NULL;
	qglColor4b                   = NULL;
	qglColor4bv                  = NULL;
	qglColor4d                   = NULL;
	qglColor4dv                  = NULL;
	qglColor4f                   = NULL;
	qglColor4fv                  = NULL;
	qglColor4i                   = NULL;
	qglColor4iv                  = NULL;
	qglColor4s                   = NULL;
	qglColor4sv                  = NULL;
	qglColor4ub                  = NULL;
	qglColor4ubv                 = NULL;
	qglColor4ui                  = NULL;
	qglColor4uiv                 = NULL;
	qglColor4us                  = NULL;
	qglColor4usv                 = NULL;
	qglColorMask                 = NULL;
	qglColorMaterial             = NULL;
	qglColorPointer              = NULL;
	qglCopyPixels                = NULL;
	qglCopyTexImage1D            = NULL;
	qglCopyTexImage2D            = NULL;
	qglCopyTexSubImage1D         = NULL;
	qglCopyTexSubImage2D         = NULL;
	qglCullFace                  = NULL;
	qglDeleteLists               = NULL;
	qglDeleteTextures            = NULL;
	qglDepthFunc                 = NULL;
	qglDepthMask                 = NULL;
	qglDepthRange                = NULL;
	qglDisable                   = NULL;
	qglDisableClientState        = NULL;
	qglDrawArrays                = NULL;
	qglDrawBuffer                = NULL;
	qglDrawElements              = NULL;
	qglDrawPixels                = NULL;
	qglEdgeFlag                  = NULL;
	qglEdgeFlagPointer           = NULL;
	qglEdgeFlagv                 = NULL;
	qglEnable                    = NULL;
	qglEnableClientState         = NULL;
	qglEnd                       = NULL;
	qglEndList                   = NULL;
	qglEvalCoord1d               = NULL;
	qglEvalCoord1dv              = NULL;
	qglEvalCoord1f               = NULL;
	qglEvalCoord1fv              = NULL;
	qglEvalCoord2d               = NULL;
	qglEvalCoord2dv              = NULL;
	qglEvalCoord2f               = NULL;
	qglEvalCoord2fv              = NULL;
	qglEvalMesh1                 = NULL;
	qglEvalMesh2                 = NULL;
	qglEvalPoint1                = NULL;
	qglEvalPoint2                = NULL;
	qglFeedbackBuffer            = NULL;
	qglFinish                    = NULL;
	qglFlush                     = NULL;
	qglFogf                      = NULL;
	qglFogfv                     = NULL;
	qglFogi                      = NULL;
	qglFogiv                     = NULL;
	qglFrontFace                 = NULL;
	qglFrustum                   = NULL;
	qglGenLists                  = NULL;
	qglGenTextures               = NULL;
	qglGetBooleanv               = NULL;
	qglGetClipPlane              = NULL;
	qglGetDoublev                = NULL;
	qglGetError                  = NULL;
	qglGetFloatv                 = NULL;
	qglGetIntegerv               = NULL;
	qglGetLightfv                = NULL;
	qglGetLightiv                = NULL;
	qglGetMapdv                  = NULL;
	qglGetMapfv                  = NULL;
	qglGetMapiv                  = NULL;
	qglGetMaterialfv             = NULL;
	qglGetMaterialiv             = NULL;
	qglGetPixelMapfv             = NULL;
	qglGetPixelMapuiv            = NULL;
	qglGetPixelMapusv            = NULL;
	qglGetPointerv               = NULL;
	qglGetPolygonStipple         = NULL;
	qglGetString                 = NULL;
	qglGetTexEnvfv               = NULL;
	qglGetTexEnviv               = NULL;
	qglGetTexGendv               = NULL;
	qglGetTexGenfv               = NULL;
	qglGetTexGeniv               = NULL;
	qglGetTexImage               = NULL;
	qglGetTexLevelParameterfv    = NULL;
	qglGetTexLevelParameteriv    = NULL;
	qglGetTexParameterfv         = NULL;
	qglGetTexParameteriv         = NULL;
	qglHint                      = NULL;
	qglIndexMask                 = NULL;
	qglIndexPointer              = NULL;
	qglIndexd                    = NULL;
	qglIndexdv                   = NULL;
	qglIndexf                    = NULL;
	qglIndexfv                   = NULL;
	qglIndexi                    = NULL;
	qglIndexiv                   = NULL;
	qglIndexs                    = NULL;
	qglIndexsv                   = NULL;
	qglIndexub                   = NULL;
	qglIndexubv                  = NULL;
	qglInitNames                 = NULL;
	qglInterleavedArrays         = NULL;
	qglIsEnabled                 = NULL;
	qglIsList                    = NULL;
	qglIsTexture                 = NULL;
	qglLightModelf               = NULL;
	qglLightModelfv              = NULL;
	qglLightModeli               = NULL;
	qglLightModeliv              = NULL;
	qglLightf                    = NULL;
	qglLightfv                   = NULL;
	qglLighti                    = NULL;
	qglLightiv                   = NULL;
	qglLineStipple               = NULL;
	qglLineWidth                 = NULL;
	qglListBase                  = NULL;
	qglLoadIdentity              = NULL;
	qglLoadMatrixd               = NULL;
	qglLoadMatrixf               = NULL;
	qglLoadName                  = NULL;
	qglLogicOp                   = NULL;
	qglMap1d                     = NULL;
	qglMap1f                     = NULL;
	qglMap2d                     = NULL;
	qglMap2f                     = NULL;
	qglMapGrid1d                 = NULL;
	qglMapGrid1f                 = NULL;
	qglMapGrid2d                 = NULL;
	qglMapGrid2f                 = NULL;
	qglMaterialf                 = NULL;
	qglMaterialfv                = NULL;
	qglMateriali                 = NULL;
	qglMaterialiv                = NULL;
	qglMatrixMode                = NULL;
	qglMultMatrixd               = NULL;
	qglMultMatrixf               = NULL;
	qglNewList                   = NULL;
	qglNormal3b                  = NULL;
	qglNormal3bv                 = NULL;
	qglNormal3d                  = NULL;
	qglNormal3dv                 = NULL;
	qglNormal3f                  = NULL;
	qglNormal3fv                 = NULL;
	qglNormal3i                  = NULL;
	qglNormal3iv                 = NULL;
	qglNormal3s                  = NULL;
	qglNormal3sv                 = NULL;
	qglNormalPointer             = NULL;
	qglOrtho                     = NULL;
	qglPassThrough               = NULL;
	qglPixelMapfv                = NULL;
	qglPixelMapuiv               = NULL;
	qglPixelMapusv               = NULL;
	qglPixelStoref               = NULL;
	qglPixelStorei               = NULL;
	qglPixelTransferf            = NULL;
	qglPixelTransferi            = NULL;
	qglPixelZoom                 = NULL;
	qglPointSize                 = NULL;
	qglPolygonMode               = NULL;
	qglPolygonOffset             = NULL;
	qglPolygonStipple            = NULL;
	qglPopAttrib                 = NULL;
	qglPopClientAttrib           = NULL;
	qglPopMatrix                 = NULL;
	qglPopName                   = NULL;
	qglPrioritizeTextures        = NULL;
	qglPushAttrib                = NULL;
	qglPushClientAttrib          = NULL;
	qglPushMatrix                = NULL;
	qglPushName                  = NULL;
	qglRasterPos2d               = NULL;
	qglRasterPos2dv              = NULL;
	qglRasterPos2f               = NULL;
	qglRasterPos2fv              = NULL;
	qglRasterPos2i               = NULL;
	qglRasterPos2iv              = NULL;
	qglRasterPos2s               = NULL;
	qglRasterPos2sv              = NULL;
	qglRasterPos3d               = NULL;
	qglRasterPos3dv              = NULL;
	qglRasterPos3f               = NULL;
	qglRasterPos3fv              = NULL;
	qglRasterPos3i               = NULL;
	qglRasterPos3iv              = NULL;
	qglRasterPos3s               = NULL;
	qglRasterPos3sv              = NULL;
	qglRasterPos4d               = NULL;
	qglRasterPos4dv              = NULL;
	qglRasterPos4f               = NULL;
	qglRasterPos4fv              = NULL;
	qglRasterPos4i               = NULL;
	qglRasterPos4iv              = NULL;
	qglRasterPos4s               = NULL;
	qglRasterPos4sv              = NULL;
	qglReadBuffer                = NULL;
	qglReadPixels                = NULL;
	qglRectd                     = NULL;
	qglRectdv                    = NULL;
	qglRectf                     = NULL;
	qglRectfv                    = NULL;
	qglRecti                     = NULL;
	qglRectiv                    = NULL;
	qglRects                     = NULL;
	qglRectsv                    = NULL;
	qglRenderMode                = NULL;
	qglRotated                   = NULL;
	qglRotatef                   = NULL;
	qglScaled                    = NULL;
	qglScalef                    = NULL;
	qglScissor                   = NULL;
	qglSelectBuffer              = NULL;
	qglShadeModel                = NULL;
	qglStencilFunc               = NULL;
	qglStencilMask               = NULL;
	qglStencilOp                 = NULL;
	qglTexCoord1d                = NULL;
	qglTexCoord1dv               = NULL;
	qglTexCoord1f                = NULL;
	qglTexCoord1fv               = NULL;
	qglTexCoord1i                = NULL;
	qglTexCoord1iv               = NULL;
	qglTexCoord1s                = NULL;
	qglTexCoord1sv               = NULL;
	qglTexCoord2d                = NULL;
	qglTexCoord2dv               = NULL;
	qglTexCoord2f                = NULL;
	qglTexCoord2fv               = NULL;
	qglTexCoord2i                = NULL;
	qglTexCoord2iv               = NULL;
	qglTexCoord2s                = NULL;
	qglTexCoord2sv               = NULL;
	qglTexCoord3d                = NULL;
	qglTexCoord3dv               = NULL;
	qglTexCoord3f                = NULL;
	qglTexCoord3fv               = NULL;
	qglTexCoord3i                = NULL;
	qglTexCoord3iv               = NULL;
	qglTexCoord3s                = NULL;
	qglTexCoord3sv               = NULL;
	qglTexCoord4d                = NULL;
	qglTexCoord4dv               = NULL;
	qglTexCoord4f                = NULL;
	qglTexCoord4fv               = NULL;
	qglTexCoord4i                = NULL;
	qglTexCoord4iv               = NULL;
	qglTexCoord4s                = NULL;
	qglTexCoord4sv               = NULL;
	qglTexCoordPointer           = NULL;
	qglTexEnvf                   = NULL;
	qglTexEnvfv                  = NULL;
	qglTexEnvi                   = NULL;
	qglTexEnviv                  = NULL;
	qglTexGend                   = NULL;
	qglTexGendv                  = NULL;
	qglTexGenf                   = NULL;
	qglTexGenfv                  = NULL;
	qglTexGeni                   = NULL;
	qglTexGeniv                  = NULL;
	qglTexImage1D                = NULL;
	qglTexImage2D                = NULL;
	qglTexParameterf             = NULL;
	qglTexParameterfv            = NULL;
	qglTexParameteri             = NULL;
	qglTexParameteriv            = NULL;
	qglTexSubImage1D             = NULL;
	qglTexSubImage2D             = NULL;
	qglTranslated                = NULL;
	qglTranslatef                = NULL;
	qglVertex2d                  = NULL;
	qglVertex2dv                 = NULL;
	qglVertex2f                  = NULL;
	qglVertex2fv                 = NULL;
	qglVertex2i                  = NULL;
	qglVertex2iv                 = NULL;
	qglVertex2s                  = NULL;
	qglVertex2sv                 = NULL;
	qglVertex3d                  = NULL;
	qglVertex3dv                 = NULL;
	qglVertex3f                  = NULL;
	qglVertex3fv                 = NULL;
	qglVertex3i                  = NULL;
	qglVertex3iv                 = NULL;
	qglVertex3s                  = NULL;
	qglVertex3sv                 = NULL;
	qglVertex4d                  = NULL;
	qglVertex4dv                 = NULL;
	qglVertex4f                  = NULL;
	qglVertex4fv                 = NULL;
	qglVertex4i                  = NULL;
	qglVertex4iv                 = NULL;
	qglVertex4s                  = NULL;
	qglVertex4sv                 = NULL;
	qglVertexPointer             = NULL;
	qglViewport                  = NULL;
}

#define GPA( a ) dlsym( OpenGLLib, a )

void *qwglGetProcAddress( char *symbol ) {
	if ( OpenGLLib ) {
		return GPA( symbol );
	}
	return NULL;
}

/*
** QGL_Init
**
** This is responsible for binding our qgl function pointers to
** the appropriate GL stuff.  In Windows this means doing a
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
**
*/

qboolean QGL_Init() {
	if ( ( OpenGLLib = dlopen( "libGL.so.1", RTLD_LAZY | RTLD_GLOBAL ) ) == 0 ) {
		ri.Printf( PRINT_ALL, "QGL_Init: Can't load %s from /etc/ld.so.conf: %s\n", "libGL.so.1", dlerror() );
		return qfalse;
	}

#define GLF_0(r, n)				qgl##n = dll##n = (r (APIENTRY *)())GPA( "gl" #n );
#define GLF_V0(n)				qgl##n = dll##n = (void (APIENTRY *)())GPA( "gl" #n );
#define GLF_1(r, n, t1, p1)		qgl##n = dll##n = (r (APIENTRY *)(t1 p1))GPA( "gl" #n );
#define GLF_V1(n, t1, p1)		qgl##n = dll##n = (void (APIENTRY *)(t1 p1))GPA( "gl" #n );
#define GLF_V2(n, t1, p1, t2, p2)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2))GPA( "gl" #n );
#define GLF_3(r, n, t1, p1, t2, p2, t3, p3)	qgl##n = dll##n = (r (APIENTRY *)(t1 p1, t2 p2, t3 p3))GPA( "gl" #n );
#define GLF_V3(n, t1, p1, t2, p2, t3, p3)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2, t3 p3))GPA( "gl" #n );
#define GLF_V4(n, t1, p1, t2, p2, t3, p3, t4, p4)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2, t3 p3, t4 p4))GPA( "gl" #n );
#define GLF_V5(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5))GPA( "gl" #n );
#define GLF_V6(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6))GPA( "gl" #n );
#define GLF_V7(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7))GPA( "gl" #n );
#define GLF_V8(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8))GPA( "gl" #n );
#define GLF_V9(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9))GPA( "gl" #n );
#define GLF_V10(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10)	qgl##n = dll##n = (void (APIENTRY *)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10))GPA( "gl" #n );
#include "../../client/renderer/qgl_functions.h"
#undef GLF_0
#undef GLF_V0
#undef GLF_1
#undef GLF_V1
#undef GLF_V2
#undef GLF_3
#undef GLF_V3
#undef GLF_V4
#undef GLF_V5
#undef GLF_V6
#undef GLF_V7
#undef GLF_V8
#undef GLF_V9
#undef GLF_V10

	qglLockArraysEXT = 0;
	qglUnlockArraysEXT = 0;
	qglPointParameterfEXT = 0;
	qglPointParameterfvEXT = 0;
	qglActiveTextureARB = 0;
	qglClientActiveTextureARB = 0;
	qglMultiTexCoord2fARB = 0;

	return qtrue;
}

void QGL_EnableLogging_(bool enable);
void QGL_EnableLogging( qboolean enable ) {
	// bk001205 - old code starts here
	QGL_EnableLogging_(enable);
	if ( enable ) {
	} else
	{
		qglAccum                     = dllAccum;
		qglAlphaFunc                 = dllAlphaFunc;
		qglAreTexturesResident       = dllAreTexturesResident;
		qglArrayElement              = dllArrayElement;
		qglBegin                     = dllBegin;
		qglBindTexture               = dllBindTexture;
		qglBitmap                    = dllBitmap;
		qglBlendFunc                 = dllBlendFunc;
		qglCallList                  = dllCallList;
		qglCallLists                 = dllCallLists;
		qglClear                     = dllClear;
		qglClearAccum                = dllClearAccum;
		qglClearColor                = dllClearColor;
		qglClearDepth                = dllClearDepth;
		qglClearIndex                = dllClearIndex;
		qglClearStencil              = dllClearStencil;
		qglClipPlane                 = dllClipPlane;
		qglColor3b                   = dllColor3b;
		qglColor3bv                  = dllColor3bv;
		qglColor3d                   = dllColor3d;
		qglColor3dv                  = dllColor3dv;
		qglColor3f                   = dllColor3f;
		qglColor3fv                  = dllColor3fv;
		qglColor3i                   = dllColor3i;
		qglColor3iv                  = dllColor3iv;
		qglColor3s                   = dllColor3s;
		qglColor3sv                  = dllColor3sv;
		qglColor3ub                  = dllColor3ub;
		qglColor3ubv                 = dllColor3ubv;
		qglColor3ui                  = dllColor3ui;
		qglColor3uiv                 = dllColor3uiv;
		qglColor3us                  = dllColor3us;
		qglColor3usv                 = dllColor3usv;
		qglColor4b                   = dllColor4b;
		qglColor4bv                  = dllColor4bv;
		qglColor4d                   = dllColor4d;
		qglColor4dv                  = dllColor4dv;
		qglColor4f                   = dllColor4f;
		qglColor4fv                  = dllColor4fv;
		qglColor4i                   = dllColor4i;
		qglColor4iv                  = dllColor4iv;
		qglColor4s                   = dllColor4s;
		qglColor4sv                  = dllColor4sv;
		qglColor4ub                  = dllColor4ub;
		qglColor4ubv                 = dllColor4ubv;
		qglColor4ui                  = dllColor4ui;
		qglColor4uiv                 = dllColor4uiv;
		qglColor4us                  = dllColor4us;
		qglColor4usv                 = dllColor4usv;
		qglColorMask                 = dllColorMask;
		qglColorMaterial             = dllColorMaterial;
		qglColorPointer              = dllColorPointer;
		qglCopyPixels                = dllCopyPixels;
		qglCopyTexImage1D            = dllCopyTexImage1D;
		qglCopyTexImage2D            = dllCopyTexImage2D;
		qglCopyTexSubImage1D         = dllCopyTexSubImage1D;
		qglCopyTexSubImage2D         = dllCopyTexSubImage2D;
		qglCullFace                  = dllCullFace;
		qglDeleteLists               = dllDeleteLists ;
		qglDeleteTextures            = dllDeleteTextures ;
		qglDepthFunc                 = dllDepthFunc ;
		qglDepthMask                 = dllDepthMask ;
		qglDepthRange                = dllDepthRange ;
		qglDisable                   = dllDisable ;
		qglDisableClientState        = dllDisableClientState ;
		qglDrawArrays                = dllDrawArrays ;
		qglDrawBuffer                = dllDrawBuffer ;
		qglDrawElements              = dllDrawElements ;
		qglDrawPixels                = dllDrawPixels ;
		qglEdgeFlag                  = dllEdgeFlag ;
		qglEdgeFlagPointer           = dllEdgeFlagPointer ;
		qglEdgeFlagv                 = dllEdgeFlagv ;
		qglEnable                    =  dllEnable                    ;
		qglEnableClientState         =  dllEnableClientState         ;
		qglEnd                       =  dllEnd                       ;
		qglEndList                   =  dllEndList                   ;
		qglEvalCoord1d               =  dllEvalCoord1d               ;
		qglEvalCoord1dv              =  dllEvalCoord1dv              ;
		qglEvalCoord1f               =  dllEvalCoord1f               ;
		qglEvalCoord1fv              =  dllEvalCoord1fv              ;
		qglEvalCoord2d               =  dllEvalCoord2d               ;
		qglEvalCoord2dv              =  dllEvalCoord2dv              ;
		qglEvalCoord2f               =  dllEvalCoord2f               ;
		qglEvalCoord2fv              =  dllEvalCoord2fv              ;
		qglEvalMesh1                 =  dllEvalMesh1                 ;
		qglEvalMesh2                 =  dllEvalMesh2                 ;
		qglEvalPoint1                =  dllEvalPoint1                ;
		qglEvalPoint2                =  dllEvalPoint2                ;
		qglFeedbackBuffer            =  dllFeedbackBuffer            ;
		qglFinish                    =  dllFinish                    ;
		qglFlush                     =  dllFlush                     ;
		qglFogf                      =  dllFogf                      ;
		qglFogfv                     =  dllFogfv                     ;
		qglFogi                      =  dllFogi                      ;
		qglFogiv                     =  dllFogiv                     ;
		qglFrontFace                 =  dllFrontFace                 ;
		qglFrustum                   =  dllFrustum                   ;
		qglGenLists                  =  dllGenLists                  ;
		qglGenTextures               =  dllGenTextures               ;
		qglGetBooleanv               =  dllGetBooleanv               ;
		qglGetClipPlane              =  dllGetClipPlane              ;
		qglGetDoublev                =  dllGetDoublev                ;
		qglGetError                  =  dllGetError                  ;
		qglGetFloatv                 =  dllGetFloatv                 ;
		qglGetIntegerv               =  dllGetIntegerv               ;
		qglGetLightfv                =  dllGetLightfv                ;
		qglGetLightiv                =  dllGetLightiv                ;
		qglGetMapdv                  =  dllGetMapdv                  ;
		qglGetMapfv                  =  dllGetMapfv                  ;
		qglGetMapiv                  =  dllGetMapiv                  ;
		qglGetMaterialfv             =  dllGetMaterialfv             ;
		qglGetMaterialiv             =  dllGetMaterialiv             ;
		qglGetPixelMapfv             =  dllGetPixelMapfv             ;
		qglGetPixelMapuiv            =  dllGetPixelMapuiv            ;
		qglGetPixelMapusv            =  dllGetPixelMapusv            ;
		qglGetPointerv               =  dllGetPointerv               ;
		qglGetPolygonStipple         =  dllGetPolygonStipple         ;
		qglGetString                 =  dllGetString                 ;
		qglGetTexEnvfv               =  dllGetTexEnvfv               ;
		qglGetTexEnviv               =  dllGetTexEnviv               ;
		qglGetTexGendv               =  dllGetTexGendv               ;
		qglGetTexGenfv               =  dllGetTexGenfv               ;
		qglGetTexGeniv               =  dllGetTexGeniv               ;
		qglGetTexImage               =  dllGetTexImage               ;
		qglGetTexLevelParameterfv    =  dllGetTexLevelParameterfv    ;
		qglGetTexLevelParameteriv    =  dllGetTexLevelParameteriv    ;
		qglGetTexParameterfv         =  dllGetTexParameterfv         ;
		qglGetTexParameteriv         =  dllGetTexParameteriv         ;
		qglHint                      =  dllHint                      ;
		qglIndexMask                 =  dllIndexMask                 ;
		qglIndexPointer              =  dllIndexPointer              ;
		qglIndexd                    =  dllIndexd                    ;
		qglIndexdv                   =  dllIndexdv                   ;
		qglIndexf                    =  dllIndexf                    ;
		qglIndexfv                   =  dllIndexfv                   ;
		qglIndexi                    =  dllIndexi                    ;
		qglIndexiv                   =  dllIndexiv                   ;
		qglIndexs                    =  dllIndexs                    ;
		qglIndexsv                   =  dllIndexsv                   ;
		qglIndexub                   =  dllIndexub                   ;
		qglIndexubv                  =  dllIndexubv                  ;
		qglInitNames                 =  dllInitNames                 ;
		qglInterleavedArrays         =  dllInterleavedArrays         ;
		qglIsEnabled                 =  dllIsEnabled                 ;
		qglIsList                    =  dllIsList                    ;
		qglIsTexture                 =  dllIsTexture                 ;
		qglLightModelf               =  dllLightModelf               ;
		qglLightModelfv              =  dllLightModelfv              ;
		qglLightModeli               =  dllLightModeli               ;
		qglLightModeliv              =  dllLightModeliv              ;
		qglLightf                    =  dllLightf                    ;
		qglLightfv                   =  dllLightfv                   ;
		qglLighti                    =  dllLighti                    ;
		qglLightiv                   =  dllLightiv                   ;
		qglLineStipple               =  dllLineStipple               ;
		qglLineWidth                 =  dllLineWidth                 ;
		qglListBase                  =  dllListBase                  ;
		qglLoadIdentity              =  dllLoadIdentity              ;
		qglLoadMatrixd               =  dllLoadMatrixd               ;
		qglLoadMatrixf               =  dllLoadMatrixf               ;
		qglLoadName                  =  dllLoadName                  ;
		qglLogicOp                   =  dllLogicOp                   ;
		qglMap1d                     =  dllMap1d                     ;
		qglMap1f                     =  dllMap1f                     ;
		qglMap2d                     =  dllMap2d                     ;
		qglMap2f                     =  dllMap2f                     ;
		qglMapGrid1d                 =  dllMapGrid1d                 ;
		qglMapGrid1f                 =  dllMapGrid1f                 ;
		qglMapGrid2d                 =  dllMapGrid2d                 ;
		qglMapGrid2f                 =  dllMapGrid2f                 ;
		qglMaterialf                 =  dllMaterialf                 ;
		qglMaterialfv                =  dllMaterialfv                ;
		qglMateriali                 =  dllMateriali                 ;
		qglMaterialiv                =  dllMaterialiv                ;
		qglMatrixMode                =  dllMatrixMode                ;
		qglMultMatrixd               =  dllMultMatrixd               ;
		qglMultMatrixf               =  dllMultMatrixf               ;
		qglNewList                   =  dllNewList                   ;
		qglNormal3b                  =  dllNormal3b                  ;
		qglNormal3bv                 =  dllNormal3bv                 ;
		qglNormal3d                  =  dllNormal3d                  ;
		qglNormal3dv                 =  dllNormal3dv                 ;
		qglNormal3f                  =  dllNormal3f                  ;
		qglNormal3fv                 =  dllNormal3fv                 ;
		qglNormal3i                  =  dllNormal3i                  ;
		qglNormal3iv                 =  dllNormal3iv                 ;
		qglNormal3s                  =  dllNormal3s                  ;
		qglNormal3sv                 =  dllNormal3sv                 ;
		qglNormalPointer             =  dllNormalPointer             ;
		qglOrtho                     =  dllOrtho                     ;
		qglPassThrough               =  dllPassThrough               ;
		qglPixelMapfv                =  dllPixelMapfv                ;
		qglPixelMapuiv               =  dllPixelMapuiv               ;
		qglPixelMapusv               =  dllPixelMapusv               ;
		qglPixelStoref               =  dllPixelStoref               ;
		qglPixelStorei               =  dllPixelStorei               ;
		qglPixelTransferf            =  dllPixelTransferf            ;
		qglPixelTransferi            =  dllPixelTransferi            ;
		qglPixelZoom                 =  dllPixelZoom                 ;
		qglPointSize                 =  dllPointSize                 ;
		qglPolygonMode               =  dllPolygonMode               ;
		qglPolygonOffset             =  dllPolygonOffset             ;
		qglPolygonStipple            =  dllPolygonStipple            ;
		qglPopAttrib                 =  dllPopAttrib                 ;
		qglPopClientAttrib           =  dllPopClientAttrib           ;
		qglPopMatrix                 =  dllPopMatrix                 ;
		qglPopName                   =  dllPopName                   ;
		qglPrioritizeTextures        =  dllPrioritizeTextures        ;
		qglPushAttrib                =  dllPushAttrib                ;
		qglPushClientAttrib          =  dllPushClientAttrib          ;
		qglPushMatrix                =  dllPushMatrix                ;
		qglPushName                  =  dllPushName                  ;
		qglRasterPos2d               =  dllRasterPos2d               ;
		qglRasterPos2dv              =  dllRasterPos2dv              ;
		qglRasterPos2f               =  dllRasterPos2f               ;
		qglRasterPos2fv              =  dllRasterPos2fv              ;
		qglRasterPos2i               =  dllRasterPos2i               ;
		qglRasterPos2iv              =  dllRasterPos2iv              ;
		qglRasterPos2s               =  dllRasterPos2s               ;
		qglRasterPos2sv              =  dllRasterPos2sv              ;
		qglRasterPos3d               =  dllRasterPos3d               ;
		qglRasterPos3dv              =  dllRasterPos3dv              ;
		qglRasterPos3f               =  dllRasterPos3f               ;
		qglRasterPos3fv              =  dllRasterPos3fv              ;
		qglRasterPos3i               =  dllRasterPos3i               ;
		qglRasterPos3iv              =  dllRasterPos3iv              ;
		qglRasterPos3s               =  dllRasterPos3s               ;
		qglRasterPos3sv              =  dllRasterPos3sv              ;
		qglRasterPos4d               =  dllRasterPos4d               ;
		qglRasterPos4dv              =  dllRasterPos4dv              ;
		qglRasterPos4f               =  dllRasterPos4f               ;
		qglRasterPos4fv              =  dllRasterPos4fv              ;
		qglRasterPos4i               =  dllRasterPos4i               ;
		qglRasterPos4iv              =  dllRasterPos4iv              ;
		qglRasterPos4s               =  dllRasterPos4s               ;
		qglRasterPos4sv              =  dllRasterPos4sv              ;
		qglReadBuffer                =  dllReadBuffer                ;
		qglReadPixels                =  dllReadPixels                ;
		qglRectd                     =  dllRectd                     ;
		qglRectdv                    =  dllRectdv                    ;
		qglRectf                     =  dllRectf                     ;
		qglRectfv                    =  dllRectfv                    ;
		qglRecti                     =  dllRecti                     ;
		qglRectiv                    =  dllRectiv                    ;
		qglRects                     =  dllRects                     ;
		qglRectsv                    =  dllRectsv                    ;
		qglRenderMode                =  dllRenderMode                ;
		qglRotated                   =  dllRotated                   ;
		qglRotatef                   =  dllRotatef                   ;
		qglScaled                    =  dllScaled                    ;
		qglScalef                    =  dllScalef                    ;
		qglScissor                   =  dllScissor                   ;
		qglSelectBuffer              =  dllSelectBuffer              ;
		qglShadeModel                =  dllShadeModel                ;
		qglStencilFunc               =  dllStencilFunc               ;
		qglStencilMask               =  dllStencilMask               ;
		qglStencilOp                 =  dllStencilOp                 ;
		qglTexCoord1d                =  dllTexCoord1d                ;
		qglTexCoord1dv               =  dllTexCoord1dv               ;
		qglTexCoord1f                =  dllTexCoord1f                ;
		qglTexCoord1fv               =  dllTexCoord1fv               ;
		qglTexCoord1i                =  dllTexCoord1i                ;
		qglTexCoord1iv               =  dllTexCoord1iv               ;
		qglTexCoord1s                =  dllTexCoord1s                ;
		qglTexCoord1sv               =  dllTexCoord1sv               ;
		qglTexCoord2d                =  dllTexCoord2d                ;
		qglTexCoord2dv               =  dllTexCoord2dv               ;
		qglTexCoord2f                =  dllTexCoord2f                ;
		qglTexCoord2fv               =  dllTexCoord2fv               ;
		qglTexCoord2i                =  dllTexCoord2i                ;
		qglTexCoord2iv               =  dllTexCoord2iv               ;
		qglTexCoord2s                =  dllTexCoord2s                ;
		qglTexCoord2sv               =  dllTexCoord2sv               ;
		qglTexCoord3d                =  dllTexCoord3d                ;
		qglTexCoord3dv               =  dllTexCoord3dv               ;
		qglTexCoord3f                =  dllTexCoord3f                ;
		qglTexCoord3fv               =  dllTexCoord3fv               ;
		qglTexCoord3i                =  dllTexCoord3i                ;
		qglTexCoord3iv               =  dllTexCoord3iv               ;
		qglTexCoord3s                =  dllTexCoord3s                ;
		qglTexCoord3sv               =  dllTexCoord3sv               ;
		qglTexCoord4d                =  dllTexCoord4d                ;
		qglTexCoord4dv               =  dllTexCoord4dv               ;
		qglTexCoord4f                =  dllTexCoord4f                ;
		qglTexCoord4fv               =  dllTexCoord4fv               ;
		qglTexCoord4i                =  dllTexCoord4i                ;
		qglTexCoord4iv               =  dllTexCoord4iv               ;
		qglTexCoord4s                =  dllTexCoord4s                ;
		qglTexCoord4sv               =  dllTexCoord4sv               ;
		qglTexCoordPointer           =  dllTexCoordPointer           ;
		qglTexEnvf                   =  dllTexEnvf                   ;
		qglTexEnvfv                  =  dllTexEnvfv                  ;
		qglTexEnvi                   =  dllTexEnvi                   ;
		qglTexEnviv                  =  dllTexEnviv                  ;
		qglTexGend                   =  dllTexGend                   ;
		qglTexGendv                  =  dllTexGendv                  ;
		qglTexGenf                   =  dllTexGenf                   ;
		qglTexGenfv                  =  dllTexGenfv                  ;
		qglTexGeni                   =  dllTexGeni                   ;
		qglTexGeniv                  =  dllTexGeniv                  ;
		qglTexImage1D                =  dllTexImage1D                ;
		qglTexImage2D                =  dllTexImage2D                ;
		qglTexParameterf             =  dllTexParameterf             ;
		qglTexParameterfv            =  dllTexParameterfv            ;
		qglTexParameteri             =  dllTexParameteri             ;
		qglTexParameteriv            =  dllTexParameteriv            ;
		qglTexSubImage1D             =  dllTexSubImage1D             ;
		qglTexSubImage2D             =  dllTexSubImage2D             ;
		qglTranslated                =  dllTranslated                ;
		qglTranslatef                =  dllTranslatef                ;
		qglVertex2d                  =  dllVertex2d                  ;
		qglVertex2dv                 =  dllVertex2dv                 ;
		qglVertex2f                  =  dllVertex2f                  ;
		qglVertex2fv                 =  dllVertex2fv                 ;
		qglVertex2i                  =  dllVertex2i                  ;
		qglVertex2iv                 =  dllVertex2iv                 ;
		qglVertex2s                  =  dllVertex2s                  ;
		qglVertex2sv                 =  dllVertex2sv                 ;
		qglVertex3d                  =  dllVertex3d                  ;
		qglVertex3dv                 =  dllVertex3dv                 ;
		qglVertex3f                  =  dllVertex3f                  ;
		qglVertex3fv                 =  dllVertex3fv                 ;
		qglVertex3i                  =  dllVertex3i                  ;
		qglVertex3iv                 =  dllVertex3iv                 ;
		qglVertex3s                  =  dllVertex3s                  ;
		qglVertex3sv                 =  dllVertex3sv                 ;
		qglVertex4d                  =  dllVertex4d                  ;
		qglVertex4dv                 =  dllVertex4dv                 ;
		qglVertex4f                  =  dllVertex4f                  ;
		qglVertex4fv                 =  dllVertex4fv                 ;
		qglVertex4i                  =  dllVertex4i                  ;
		qglVertex4iv                 =  dllVertex4iv                 ;
		qglVertex4s                  =  dllVertex4s                  ;
		qglVertex4sv                 =  dllVertex4sv                 ;
		qglVertexPointer             =  dllVertexPointer             ;
		qglViewport                  =  dllViewport                  ;
	}
}


void GLimp_LogNewFrame( void ) {
	QGL_LogComment("*** R_BeginFrame ***\n" );
}
