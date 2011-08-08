//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	This file implements the binding of GL to QGL function pointers.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"
#include <time.h>

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

#define GLF_0(r, n)				r (APIENTRY * qgl##n)();
#define GLF_V0(n)				void (APIENTRY * qgl##n)();
#define GLF_1(r, n, t1, p1)		r (APIENTRY * qgl##n)(t1 p1);
#define GLF_V1(n, t1, p1)		void (APIENTRY * qgl##n)(t1 p1);
#define GLF_V2(n, t1, p1, t2, p2)	void (APIENTRY * qgl##n)(t1 p1, t2 p2);
#define GLF_3(r, n, t1, p1, t2, p2, t3, p3)	r (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3);
#define GLF_V3(n, t1, p1, t2, p2, t3, p3)	void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3);
#define GLF_V4(n, t1, p1, t2, p2, t3, p3, t4, p4)	void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4);
#define GLF_V5(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)	void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5);
#define GLF_V6(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)	void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6);
#define GLF_V7(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)	void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7);
#define GLF_V8(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)	void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8);
#define GLF_V9(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)	void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9);
#define GLF_V10(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10)	void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10);
#include "render_qgl_functions.h"
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

void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

void ( APIENTRY * qglLockArraysEXT)( GLint, GLint);
void ( APIENTRY * qglUnlockArraysEXT) ( void );

void ( APIENTRY * qglPointParameterfEXT)( GLenum param, GLfloat value );
void ( APIENTRY * qglPointParameterfvEXT)( GLenum param, const GLfloat *value );

#ifdef _WIN32
BOOL ( WINAPI * qwglSwapIntervalEXT)( int interval );
#endif

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static fileHandle_t		log_fp;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QGL_Log
//
//==========================================================================

static void QGL_Log(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[1024];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, 1024, Fmt, ArgPtr);
	va_end(ArgPtr);

	FS_Printf(log_fp, "%s", string);
}

//==========================================================================
//
//	Simple logging
//
//==========================================================================

#define GLF_0(r, n) \
static r APIENTRY log##n() \
{ \
	QGL_Log("gl" #n "\n"); \
	return gl##n(); \
}
#define GLF_V0(n) \
static void APIENTRY log##n() \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(); \
}
#define GLF_1(r, n, t1, p1) \
static r APIENTRY log##n(t1 p1) \
{ \
	QGL_Log("gl" #n "\n"); \
	return gl##n(p1); \
}
#define GLF_V1(n, t1, p1) \
static void APIENTRY log##n(t1 p1) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1); \
}
#define GLF_V1_X(n, t1, p1)
#define GLF_V2(n, t1, p1, t2, p2) \
static void APIENTRY log##n(t1 p1, t2 p2) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2); \
}
#define GLF_V2_X(n, t1, p1, t2, p2)
#define GLF_3(r, n, t1, p1, t2, p2, t3, p3) \
static r APIENTRY log##n(t1 p1, t2 p2, t3 p3) \
{ \
	QGL_Log("gl" #n "\n"); \
	return gl##n(p1, p2, p3); \
}
#define GLF_V3(n, t1, p1, t2, p2, t3, p3) \
static void APIENTRY log##n(t1 p1, t2 p2, t3 p3) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2, p3); \
}
#define GLF_V3_X(n, t1, p1, t2, p2, t3, p3)
#define GLF_V4(n, t1, p1, t2, p2, t3, p3, t4, p4) \
static void APIENTRY log##n(t1 p1, t2 p2, t3 p3, t4 p4) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2, p3, p4); \
}
#define GLF_V4_X(n, t1, p1, t2, p2, t3, p3, t4, p4)
#define GLF_V5(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5) \
static void APIENTRY log##n(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2, p3, p4, p5); \
}
#define GLF_V6(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6) \
static void APIENTRY log##n(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2, p3, p4, p5, p6); \
}
#define GLF_V7(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7) \
static void APIENTRY log##n(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2, p3, p4, p5, p6, p7); \
}
#define GLF_V8(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8) \
static void APIENTRY log##n(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2, p3, p4, p5, p6, p7, p8); \
}
#define GLF_V9(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9) \
static void APIENTRY log##n(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2, p3, p4, p5, p6, p7, p8, p9); \
}
#define GLF_V10(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10) \
static void APIENTRY log##n(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10) \
{ \
	QGL_Log("gl" #n "\n"); \
	gl##n(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10); \
}
#include "render_qgl_functions.h"
#undef GLF_0
#undef GLF_V0
#undef GLF_1
#undef GLF_V1
#undef GLF_V1_X
#undef GLF_V2
#undef GLF_V2_X
#undef GLF_3
#undef GLF_V3
#undef GLF_V3_X
#undef GLF_V4
#undef GLF_V4_X
#undef GLF_V5
#undef GLF_V6
#undef GLF_V7
#undef GLF_V8
#undef GLF_V9
#undef GLF_V10

//==========================================================================
//
//	BooleanToString
//
//==========================================================================

static const char* BooleanToString(GLboolean b)
{
	if (b == GL_FALSE)
		return "GL_FALSE";
	if (b == GL_TRUE)
		return "GL_TRUE";
	return "OUT OF RANGE FOR BOOLEAN";
}

//==========================================================================
//
//	FuncToString
//
//==========================================================================

static const char* FuncToString(GLenum f)
{
	switch (f)
	{
	case GL_ALWAYS:
		return "GL_ALWAYS";
	case GL_NEVER:
		return "GL_NEVER";
	case GL_LEQUAL:
		return "GL_LEQUAL";
	case GL_LESS:
		return "GL_LESS";
	case GL_EQUAL:
		return "GL_EQUAL";
	case GL_GREATER:
		return "GL_GREATER";
	case GL_GEQUAL:
		return "GL_GEQUAL";
	case GL_NOTEQUAL:
		return "GL_NOTEQUAL";
	default:
		return "!!! UNKNOWN !!!";
	}
}

//==========================================================================
//
//	PrimToString
//
//==========================================================================

static const char* PrimToString(GLenum mode)
{
	static char prim[1024];

	if ( mode == GL_TRIANGLES )
		return "GL_TRIANGLES";
	else if ( mode == GL_TRIANGLE_STRIP )
		return "GL_TRIANGLE_STRIP";
	else if ( mode == GL_TRIANGLE_FAN )
		return "GL_TRIANGLE_FAN";
	else if ( mode == GL_QUADS )
		return "GL_QUADS";
	else if ( mode == GL_QUAD_STRIP )
		return "GL_QUAD_STRIP";
	else if ( mode == GL_POLYGON )
		return "GL_POLYGON";
	else if ( mode == GL_POINTS )
		return "GL_POINTS";
	else if ( mode == GL_LINES )
		return "GL_LINES";
	else if ( mode == GL_LINE_STRIP )
		return "GL_LINE_STRIP";
	else if ( mode == GL_LINE_LOOP )
		return "GL_LINE_LOOP";
	else
		sprintf(prim, "0x%x", mode);

	return prim;
}

//==========================================================================
//
//	CapToString
//
//==========================================================================

static const char* CapToString(GLenum cap)
{
	static char buffer[1024];

	switch (cap)
	{
	case GL_TEXTURE_2D:
		return "GL_TEXTURE_2D";
	case GL_BLEND:
		return "GL_BLEND";
	case GL_DEPTH_TEST:
		return "GL_DEPTH_TEST";
	case GL_CULL_FACE:
		return "GL_CULL_FACE";
	case GL_CLIP_PLANE0:
		return "GL_CLIP_PLANE0";
	case GL_COLOR_ARRAY:
		return "GL_COLOR_ARRAY";
	case GL_TEXTURE_COORD_ARRAY:
		return "GL_TEXTURE_COORD_ARRAY";
	case GL_VERTEX_ARRAY:
		return "GL_VERTEX_ARRAY";
	case GL_ALPHA_TEST:
		return "GL_ALPHA_TEST";
	case GL_STENCIL_TEST:
		return "GL_STENCIL_TEST";
	default:
		sprintf(buffer, "0x%x", cap);
	}

	return buffer;
}

//==========================================================================
//
//	BlendToName
//
//==========================================================================

static const char* TypeToString(GLenum t)
{
	switch (t)
	{
	case GL_BYTE:
		return "GL_BYTE";
	case GL_UNSIGNED_BYTE:
		return "GL_UNSIGNED_BYTE";
	case GL_SHORT:
		return "GL_SHORT";
	case GL_UNSIGNED_SHORT:
		return "GL_UNSIGNED_SHORT";
	case GL_INT:
		return "GL_INT";
	case GL_UNSIGNED_INT:
		return "GL_UNSIGNED_INT";
	case GL_FLOAT:
		return "GL_FLOAT";
	case GL_DOUBLE:
		return "GL_DOUBLE";
	default:
		return "!!! UNKNOWN !!!";
	}
}

//==========================================================================
//
//	BlendToName
//
//==========================================================================

static void BlendToName(char* n, GLenum f)
{
	switch (f)
	{
	case GL_ONE:
		String::Cpy(n, "GL_ONE");
		break;
	case GL_ZERO:
		String::Cpy(n, "GL_ZERO");
		break;
	case GL_SRC_ALPHA:
		String::Cpy(n, "GL_SRC_ALPHA");
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		String::Cpy(n, "GL_ONE_MINUS_SRC_ALPHA");
		break;
	case GL_DST_COLOR:
		String::Cpy(n, "GL_DST_COLOR");
		break;
	case GL_ONE_MINUS_DST_COLOR:
		String::Cpy(n, "GL_ONE_MINUS_DST_COLOR");
		break;
	case GL_DST_ALPHA:
		String::Cpy(n, "GL_DST_ALPHA");
		break;
	default:
		sprintf(n, "0x%x", f);
	}
}

static void APIENTRY logAlphaFunc(GLenum func, GLclampf ref)
{
	QGL_Log("glAlphaFunc( 0x%x, %f )\n", func, ref);
	glAlphaFunc(func, ref);
}

static void APIENTRY logBegin(GLenum mode)
{
	QGL_Log("glBegin( %s )\n", PrimToString(mode));
	glBegin(mode);
}

static void APIENTRY logBindTexture(GLenum target, GLuint texture)
{
	QGL_Log("glBindTexture( 0x%x, %u )\n", target, texture);
	glBindTexture(target, texture);
}

static void APIENTRY logBlendFunc(GLenum sfactor, GLenum dfactor)
{
	char sf[128], df[128];

	BlendToName(sf, sfactor);
	BlendToName(df, dfactor);

	QGL_Log("glBlendFunc( %s, %s )\n", sf, df);
	glBlendFunc( sfactor, dfactor );
}

static void APIENTRY logCallList(GLuint list)
{
	QGL_Log("glCallList( %u )\n", list);
	glCallList(list);
}

static void APIENTRY logClear(GLbitfield mask)
{
	QGL_Log("glClear( 0x%x = ", mask);

	if (mask & GL_COLOR_BUFFER_BIT)
		QGL_Log("GL_COLOR_BUFFER_BIT ");
	if ( mask & GL_DEPTH_BUFFER_BIT )
		QGL_Log("GL_DEPTH_BUFFER_BIT ");
	if ( mask & GL_STENCIL_BUFFER_BIT )
		QGL_Log("GL_STENCIL_BUFFER_BIT ");
	if ( mask & GL_ACCUM_BUFFER_BIT )
		QGL_Log("GL_ACCUM_BUFFER_BIT ");

	QGL_Log(")\n");
	glClear(mask);
}

static void APIENTRY logClearDepth(GLclampd depth)
{
	QGL_Log("glClearDepth( %f )\n", (float)depth);
	glClearDepth(depth);
}

static void APIENTRY logClearStencil(GLint s)
{
	QGL_Log("glClearStencil( %d )\n", s);
	glClearStencil(s);
}

static void APIENTRY logColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	QGL_Log("glColor4f( %f,%f,%f,%f )\n", red, green, blue, alpha);
	glColor4f(red, green, blue, alpha);
}

static void APIENTRY logColor4fv(const GLfloat* v)
{
	QGL_Log("glColor4fv( %f,%f,%f,%f )\n", v[0], v[1], v[2], v[3]);
	glColor4fv(v);
}

static void APIENTRY logColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	QGL_Log("glColorPointer( %d, %s, %d, MEM )\n", size, TypeToString(type), stride);
	glColorPointer(size, type, stride, pointer);
}

static void APIENTRY logCullFace(GLenum mode)
{
	QGL_Log("glCullFace( %s )\n", (mode == GL_FRONT) ? "GL_FRONT" : "GL_BACK");
	glCullFace(mode);
}

static void APIENTRY logDepthFunc(GLenum func)
{
	QGL_Log("glDepthFunc( %s )\n", FuncToString(func));
	glDepthFunc(func);
}

static void APIENTRY logDepthMask(GLboolean flag)
{
	QGL_Log("glDepthMask( %s )\n", BooleanToString(flag));
	glDepthMask(flag);
}

static void APIENTRY logDepthRange(GLclampd zNear, GLclampd zFar)
{
	QGL_Log("glDepthRange( %f, %f )\n", (float)zNear, (float)zFar);
	glDepthRange(zNear, zFar);
}

static void APIENTRY logDisable(GLenum cap)
{
	QGL_Log("glDisable( %s )\n", CapToString(cap));
	glDisable(cap);
}

static void APIENTRY logDisableClientState(GLenum array)
{
	QGL_Log("glDisableClientState( %s )\n", CapToString(array));
	glDisableClientState(array);
}

static void APIENTRY logDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	QGL_Log("glDrawElements( %s, %d, %s, MEM )\n", PrimToString(mode), count, TypeToString(type));
	glDrawElements(mode, count, type, indices);
}

static void APIENTRY logEnable(GLenum cap)
{
	QGL_Log("glEnable( %s )\n", CapToString(cap));
	glEnable(cap);
}

static void APIENTRY logEnableClientState(GLenum array)
{
	QGL_Log("glEnableClientState( %s )\n", CapToString(array));
	glEnableClientState(array);
}

static void APIENTRY logHint(GLenum target, GLenum mode)
{
	QGL_Log("glHint( 0x%x, 0x%x )\n", target, mode);
	glHint( target, mode);
}

static void APIENTRY logPolygonMode(GLenum face, GLenum mode)
{
	QGL_Log("glPolygonMode( 0x%x, 0x%x )\n", face, mode);
	glPolygonMode(face, mode);
}

static void APIENTRY logScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	QGL_Log("glScissor( %d, %d, %d, %d )\n", x, y, width, height);
	glScissor(x, y, width, height);
}

static void APIENTRY logTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	QGL_Log("glTexCoordPointer( %d, %s, %d, MEM )\n", size, TypeToString(type), stride);
	glTexCoordPointer(size, type, stride, pointer);
}

static void APIENTRY logTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	QGL_Log("glTexEnvf( 0x%x, 0x%x, %f )\n", target, pname, param);
	glTexEnvf(target, pname, param);
}

static void APIENTRY logTexEnvi(GLenum target, GLenum pname, GLint param)
{
	QGL_Log("glTexEnvi( 0x%x, 0x%x, 0x%x )\n", target, pname, param);
	glTexEnvi(target, pname, param);
}

static void APIENTRY logTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	QGL_Log("glTexParameterf( 0x%x, 0x%x, %f )\n", target, pname, param);
	glTexParameterf(target, pname, param);
}

static void APIENTRY logTexParameteri(GLenum target, GLenum pname, GLint param)
{
	QGL_Log("glTexParameteri( 0x%x, 0x%x, 0x%x )\n", target, pname, param);
	glTexParameteri(target, pname, param);
}

static void APIENTRY logVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	QGL_Log("glVertexPointer( %d, %s, %d, MEM )\n", size, TypeToString(type), stride);
	glVertexPointer(size, type, stride, pointer);
}

static void APIENTRY logViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	QGL_Log("glViewport( %d, %d, %d, %d )\n", x, y, width, height);
	glViewport(x, y, width, height);
}

//==========================================================================
//
//	CheckExtension
//
//==========================================================================

static bool CheckExtension(const char* Extension)
{
	Array<String> Extensions;
	String((char*)qglGetString(GL_EXTENSIONS)).Split(' ', Extensions);
	for (int i = 0; i < Extensions.Num(); i++)
	{
		if (Extensions[i] == Extension)
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
//	QGL_Init
//
//	This is responsible for binding our qgl function pointers to the
// appropriate GL stuff.
//
//==========================================================================

void QGL_Init()
{
	GLog.Write("...initializing QGL\n");

#define GLF_0(r, n)				qgl##n = gl##n;
#define GLF_V0(n)				qgl##n = gl##n;
#define GLF_1(r, n, t1, p1)		qgl##n = gl##n;
#define GLF_V1(n, t1, p1)		qgl##n = gl##n;
#define GLF_V2(n, t1, p1, t2, p2)	qgl##n = gl##n;
#define GLF_3(r, n, t1, p1, t2, p2, t3, p3)	qgl##n = gl##n;
#define GLF_V3(n, t1, p1, t2, p2, t3, p3)	qgl##n = gl##n;
#define GLF_V4(n, t1, p1, t2, p2, t3, p3, t4, p4)	qgl##n = gl##n;
#define GLF_V5(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)	qgl##n = gl##n;
#define GLF_V6(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)	qgl##n = gl##n;
#define GLF_V7(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)	qgl##n = gl##n;
#define GLF_V8(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)	qgl##n = gl##n;
#define GLF_V9(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)	qgl##n = gl##n;
#define GLF_V10(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10)	qgl##n = gl##n;
#include "render_qgl_functions.h"
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

	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	qglMultiTexCoord2fARB = NULL;

	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;

	qglPointParameterfEXT = NULL;
	qglPointParameterfvEXT = NULL;

#ifdef _WIN32
	qwglSwapIntervalEXT = NULL;
#endif

	GLog.Write("Initializing OpenGL extensions\n");

	// GL_S3_s3tc
	glConfig.textureCompression = TC_NONE;
	if (CheckExtension("GL_S3_s3tc"))
	{
		if (r_ext_compressed_textures->integer)
		{
			glConfig.textureCompression = TC_S3TC;
			GLog.Write("...using GL_S3_s3tc\n");
		}
		else
		{
			glConfig.textureCompression = TC_NONE;
			GLog.Write("...ignoring GL_S3_s3tc\n");
		}
	}
	else
	{
		GLog.Write("...GL_S3_s3tc not found\n");
	}

	// GL_ARB_multitexture
	if (CheckExtension("GL_ARB_multitexture"))
	{
		if (r_ext_multitexture->integer)
		{
			qglMultiTexCoord2fARB = (void (APIENTRY*)(GLenum target, GLfloat s, GLfloat t))GLimp_GetProcAddress("glMultiTexCoord2fARB");
			qglActiveTextureARB = (void (APIENTRY*)(GLenum target))GLimp_GetProcAddress("glActiveTextureARB");
			qglClientActiveTextureARB = (void (APIENTRY*)(GLenum target))GLimp_GetProcAddress("glClientActiveTextureARB");

			if (qglActiveTextureARB)
			{
				qglGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glConfig.maxActiveTextures);

				if (glConfig.maxActiveTextures > 1)
				{
					GLog.Write("...using GL_ARB_multitexture\n");
				}
				else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					GLog.Write("...not using GL_ARB_multitexture, < 2 texture units\n");
				}
			}
		}
		else
		{
			GLog.Write("...ignoring GL_ARB_multitexture\n");
		}
	}
	else
	{
		GLog.Write("...GL_ARB_multitexture not found\n");
	}

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = false;
	if (CheckExtension("GL_EXT_texture_env_add"))
	{
		if (r_ext_texture_env_add->integer)
		{
			glConfig.textureEnvAddAvailable = true;
			GLog.Write("...using GL_EXT_texture_env_add\n");
		}
		else
		{
			glConfig.textureEnvAddAvailable = false;
			GLog.Write("...ignoring GL_EXT_texture_env_add\n");
		}
	}
	else
	{
		GLog.Write("...GL_EXT_texture_env_add not found\n");
	}

#ifdef _WIN32
	// WGL_EXT_swap_control
	if (CheckExtension("WGL_EXT_swap_control"))
	{
		if (r_ext_gamma_control->integer)
		{
			qwglSwapIntervalEXT = (BOOL (WINAPI*)(int))GLimp_GetProcAddress("wglSwapIntervalEXT");
			if (qwglSwapIntervalEXT)
			{
				GLog.Write("...using WGL_EXT_swap_control\n");
				r_swapInterval->modified = true;	// force a set next frame
			}
			else
			{
				GLog.Write("...WGL_EXT_swap_control not found\n");
			}
		}
		else
		{
			GLog.Write("...ignoring WGL_EXT_swap_control\n");
		}
	}
	else
	{
		GLog.Write("...WGL_EXT_swap_control not found\n");
	}
#endif

	// GL_EXT_compiled_vertex_array
	if (CheckExtension("GL_EXT_compiled_vertex_array"))
	{
		if (r_ext_compiled_vertex_array->integer)
		{
			GLog.Write("...using GL_EXT_compiled_vertex_array\n");
			qglLockArraysEXT = (void (APIENTRY*)(int, int))GLimp_GetProcAddress("glLockArraysEXT");
			qglUnlockArraysEXT = (void (APIENTRY*)())GLimp_GetProcAddress("glUnlockArraysEXT");
			if (!qglLockArraysEXT || !qglUnlockArraysEXT)
			{
				throw QException("bad getprocaddress");
			}
		}
		else
		{
			GLog.Write("...ignoring GL_EXT_compiled_vertex_array\n");
		}
	}
	else
	{
		GLog.Write("...GL_EXT_compiled_vertex_array not found\n");
	}

	if (CheckExtension("GL_EXT_point_parameters"))
	{
		if (r_ext_point_parameters->integer)
		{
			GLog.Write("...using GL_EXT_point_parameters\n");
			qglPointParameterfEXT = (void (APIENTRY*)(GLenum, GLfloat))GLimp_GetProcAddress("glPointParameterfEXT");
			qglPointParameterfvEXT = (void (APIENTRY*)(GLenum, const GLfloat*))GLimp_GetProcAddress("glPointParameterfvEXT");
			if (!qglPointParameterfEXT || !qglPointParameterfvEXT)
			{
				throw QException("bad getprocaddress");
			}
		}
		else
		{
			GLog.Write("...ignoring GL_EXT_point_parameters\n");
		}
	}
	else
	{
		GLog.Write("...GL_EXT_point_parameters not found\n");
	}

	// check logging
	QGL_EnableLogging(!!r_logFile->integer);
}

//==========================================================================
//
//	QGL_Shutdown
//
//	Nulls out all the proc pointers.  This is only called during a hard
// shutdown of the OGL subsystem (e.g. vid_restart).
//
//==========================================================================

void QGL_Shutdown()
{
	GLog.Write("...shutting down QGL\n");

	// close the r_logFile
	if (log_fp)
	{
		FS_FCloseFile(log_fp);
		log_fp = 0;
	}

#define GLF_0(r, n)				qgl##n = NULL;
#define GLF_V0(n)				qgl##n = NULL;
#define GLF_1(r, n, t1, p1)		qgl##n = NULL;
#define GLF_V1(n, t1, p1)		qgl##n = NULL;
#define GLF_V2(n, t1, p1, t2, p2)	qgl##n = NULL;
#define GLF_3(r, n, t1, p1, t2, p2, t3, p3)	qgl##n = NULL;
#define GLF_V3(n, t1, p1, t2, p2, t3, p3)	qgl##n = NULL;
#define GLF_V4(n, t1, p1, t2, p2, t3, p3, t4, p4)	qgl##n = NULL;
#define GLF_V5(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)	qgl##n = NULL;
#define GLF_V6(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)	qgl##n = NULL;
#define GLF_V7(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)	qgl##n = NULL;
#define GLF_V8(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)	qgl##n = NULL;
#define GLF_V9(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)	qgl##n = NULL;
#define GLF_V10(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10)	qgl##n = NULL;
#include "render_qgl_functions.h"
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

	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	qglMultiTexCoord2fARB = NULL;

	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;

	qglPointParameterfEXT = NULL;
	qglPointParameterfvEXT = NULL;

#ifdef _WIN32
	qwglSwapIntervalEXT = NULL;
#endif
}

//==========================================================================
//
//	QGL_EnableLogging
//
//==========================================================================

void QGL_EnableLogging(bool enable)
{
	static bool isEnabled;

	// return if we're already active
	if (isEnabled && enable)
	{
		// decrement log counter and stop if it has reached 0
		Cvar_Set("r_logFile", va("%d", r_logFile->integer - 1));
		if (r_logFile->integer)
		{
			return;
		}
		enable = false;
	}

	// return if we're already disabled
	if (!enable && !isEnabled)
	{
		return;
	}

	isEnabled = enable;

	if (enable)
	{
		if (!log_fp)
		{
			struct tm *newtime;
			time_t aclock;

			time(&aclock);
			newtime = localtime(&aclock);

			log_fp = FS_FOpenFileWrite("gl.log");

			QGL_Log("%s\n", asctime(newtime));
		}

#define GLF_0(r, n)				qgl##n = log##n;
#define GLF_V0(n)				qgl##n = log##n;
#define GLF_1(r, n, t1, p1)		qgl##n = log##n;
#define GLF_V1(n, t1, p1)		qgl##n = log##n;
#define GLF_V2(n, t1, p1, t2, p2)	qgl##n = log##n;
#define GLF_3(r, n, t1, p1, t2, p2, t3, p3)	qgl##n = log##n;
#define GLF_V3(n, t1, p1, t2, p2, t3, p3)	qgl##n = log##n;
#define GLF_V4(n, t1, p1, t2, p2, t3, p3, t4, p4)	qgl##n = log##n;
#define GLF_V5(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)	qgl##n = log##n;
#define GLF_V6(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)	qgl##n = log##n;
#define GLF_V7(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)	qgl##n = log##n;
#define GLF_V8(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)	qgl##n = log##n;
#define GLF_V9(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)	qgl##n = log##n;
#define GLF_V10(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10)	qgl##n = log##n;
#include "render_qgl_functions.h"
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
	}
	else
	{
		if (log_fp)
		{
			QGL_Log("*** CLOSING LOG ***\n");
			FS_FCloseFile(log_fp);
			log_fp = 0;
		}

#define GLF_0(r, n)				qgl##n = gl##n;
#define GLF_V0(n)				qgl##n = gl##n;
#define GLF_1(r, n, t1, p1)		qgl##n = gl##n;
#define GLF_V1(n, t1, p1)		qgl##n = gl##n;
#define GLF_V2(n, t1, p1, t2, p2)	qgl##n = gl##n;
#define GLF_3(r, n, t1, p1, t2, p2, t3, p3)	qgl##n = gl##n;
#define GLF_V3(n, t1, p1, t2, p2, t3, p3)	qgl##n = gl##n;
#define GLF_V4(n, t1, p1, t2, p2, t3, p3, t4, p4)	qgl##n = gl##n;
#define GLF_V5(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)	qgl##n = gl##n;
#define GLF_V6(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)	qgl##n = gl##n;
#define GLF_V7(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)	qgl##n = gl##n;
#define GLF_V8(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)	qgl##n = gl##n;
#define GLF_V9(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)	qgl##n = gl##n;
#define GLF_V10(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10)	qgl##n = gl##n;
#include "render_qgl_functions.h"
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
	}
}

//==========================================================================
//
//	QGL_LogComment
//
//==========================================================================

void QGL_LogComment(const char* Comment)
{
	if (log_fp)
	{
		QGL_Log("%s", Comment);
	}
}
