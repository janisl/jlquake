//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#define GLF_0(r, n)			extern r (APIENTRY * qgl##n)();
#define GLF_V0(n)			extern void (APIENTRY * qgl##n)();
#define GLF_1(r, n, t1, p1)	extern r (APIENTRY * qgl##n)(t1 p1);
#define GLF_V1(n, t1, p1)	extern void (APIENTRY * qgl##n)(t1 p1);
#define GLF_V2(n, t1, p1, t2, p2)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2);
#define GLF_3(r, n, t1, p1, t2, p2, t3, p3)	extern r (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3);
#define GLF_V3(n, t1, p1, t2, p2, t3, p3)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3);
#define GLF_V4(n, t1, p1, t2, p2, t3, p3, t4, p4)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4);
#define GLF_V5(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5);
#define GLF_V6(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6);
#define GLF_V7(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7);
#define GLF_V8(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8);
#define GLF_V9(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9);
#define GLF_V10(n, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9, t10, p10)	extern void (APIENTRY * qgl##n)(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10);
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

#if 0
void QGL_Init();
void QGL_Shutdown();
#endif
void QGL_EnableLogging(bool Enable);
void QGL_LogComment(const char* Comment);

extern	void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
extern	void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
extern	void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

extern	void ( APIENTRY * qglLockArraysEXT) (GLint, GLint);
extern	void ( APIENTRY * qglUnlockArraysEXT) (void);

extern	void ( APIENTRY * qglPointParameterfEXT)( GLenum param, GLfloat value );
extern	void ( APIENTRY * qglPointParameterfvEXT)( GLenum param, const GLfloat *value );

#if defined( _WIN32 )
extern BOOL ( WINAPI * qwglSwapIntervalEXT)( int interval );
#endif

#if 0
// S3TC compression constants
#ifndef GL_RGB_S3TC
#define GL_RGB_S3TC							0x83A0
#define GL_RGB4_S3TC						0x83A1
#endif

//	multitexture extension constants
#ifndef GL_ACTIVE_TEXTURE_ARB
#define GL_ACTIVE_TEXTURE_ARB				0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB		0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB			0x84E2

#define GL_TEXTURE0_ARB						0x84C0
#define GL_TEXTURE1_ARB						0x84C1
#define GL_TEXTURE2_ARB						0x84C2
#define GL_TEXTURE3_ARB						0x84C3
#endif

//	point parameter extension constants
#ifndef GL_POINT_SIZE_MIN_EXT
#define GL_POINT_SIZE_MIN_EXT				0x8126
#define GL_POINT_SIZE_MAX_EXT				0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT	0x8128
#define GL_DISTANCE_ATTENUATION_EXT			0x8129
#endif
#endif
