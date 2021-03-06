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

#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#include "qcommon.h"
#include "math/Math.h"

#define SQRTFAST( x )     ( ( x ) * idMath::RSqrt( x ) )

// angle indexes
#define PITCH               0		// up / down
#define YAW                 1		// left / right
#define ROLL                2		// fall over

typedef float vec_t;
typedef vec_t vec2_t[ 2 ];
typedef vec_t vec3_t[ 3 ];
typedef vec_t vec4_t[ 4 ];
typedef vec_t vec5_t[ 5 ];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

extern vec3_t oldvec3_origin;

#define nanmask                 ( 255 << 23 )

#define IS_NAN( x )               ( ( ( *( int* )&x ) & nanmask ) == nanmask )

#define DotProduct( x,y )         ( ( x )[ 0 ] * ( y )[ 0 ] + ( x )[ 1 ] * ( y )[ 1 ] + ( x )[ 2 ] * ( y )[ 2 ] )
#define VectorSubtract( a,b,c )   ( ( c )[ 0 ] = ( a )[ 0 ] - ( b )[ 0 ],( c )[ 1 ] = ( a )[ 1 ] - ( b )[ 1 ],( c )[ 2 ] = ( a )[ 2 ] - ( b )[ 2 ] )
#define VectorAdd( a,b,c )        ( ( c )[ 0 ] = ( a )[ 0 ] + ( b )[ 0 ],( c )[ 1 ] = ( a )[ 1 ] + ( b )[ 1 ],( c )[ 2 ] = ( a )[ 2 ] + ( b )[ 2 ] )
#define VectorCopy( a,b )         ( ( b )[ 0 ] = ( a )[ 0 ],( b )[ 1 ] = ( a )[ 1 ],( b )[ 2 ] = ( a )[ 2 ] )
#define VectorScale( v, s, o )    ( ( o )[ 0 ] = ( v )[ 0 ] * ( s ),( o )[ 1 ] = ( v )[ 1 ] * ( s ),( o )[ 2 ] = ( v )[ 2 ] * ( s ) )
#define VectorMA( v, s, b, o )    ( ( o )[ 0 ] = ( v )[ 0 ] + ( b )[ 0 ] * ( s ),( o )[ 1 ] = ( v )[ 1 ] + ( b )[ 1 ] * ( s ),( o )[ 2 ] = ( v )[ 2 ] + ( b )[ 2 ] * ( s ) )
#define VectorClear( a )          ( ( a )[ 0 ] = ( a )[ 1 ] = ( a )[ 2 ] = 0 )
#define VectorNegate( a,b )       ( ( b )[ 0 ] = -( a )[ 0 ],( b )[ 1 ] = -( a )[ 1 ],( b )[ 2 ] = -( a )[ 2 ] )
#define VectorSet( v, x, y, z )   ( ( v )[ 0 ] = ( x ), ( v )[ 1 ] = ( y ), ( v )[ 2 ] = ( z ) )

#define Vector4Copy( a,b )        ( ( b )[ 0 ] = ( a )[ 0 ],( b )[ 1 ] = ( a )[ 1 ],( b )[ 2 ] = ( a )[ 2 ],( b )[ 3 ] = ( a )[ 3 ] )
#define Vector4Set( v, x, y, z, w )   ( ( v )[ 0 ] = ( x ), ( v )[ 1 ] = ( y ), ( v )[ 2 ] = ( z ), ( v )[ 3 ] = ( w ) )

#define SnapVector( v ) {v[ 0 ] = ( ( int )( v[ 0 ] ) ); v[ 1 ] = ( ( int )( v[ 1 ] ) ); v[ 2 ] = ( ( int )( v[ 2 ] ) ); }

// just in case you do't want to use the macros
vec_t _DotProduct( const vec3_t v1, const vec3_t v2 );
void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out );
void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out );
void _VectorCopy( const vec3_t in, vec3_t out );
void _VectorScale( const vec3_t in, float scale, vec3_t out );
void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc );

inline int VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	if ( v1[ 0 ] != v2[ 0 ] || v1[ 1 ] != v2[ 1 ] || v1[ 2 ] != v2[ 2 ] ) {
		return 0;
	}
	return 1;
}

inline vec_t VectorLength( const vec3_t v ) {
	return ( vec_t )sqrt( v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ] );
}

inline vec_t VectorLengthSquared( const vec3_t v ) {
	return ( v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ] );
}

inline vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t v;

	VectorSubtract( p2, p1, v );
	return VectorLength( v );
}

inline vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t v;

	VectorSubtract( p2, p1, v );
	return v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ];
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
inline void VectorNormalizeFast( vec3_t v ) {
	float ilength;

	ilength = idMath::RSqrt( DotProduct( v, v ) );

	v[ 0 ] *= ilength;
	v[ 1 ] *= ilength;
	v[ 2 ] *= ilength;
}

inline void VectorInverse( vec3_t v ) {
	v[ 0 ] = -v[ 0 ];
	v[ 1 ] = -v[ 1 ];
	v[ 2 ] = -v[ 2 ];
}

inline void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[ 0 ] = v1[ 1 ] * v2[ 2 ] - v1[ 2 ] * v2[ 1 ];
	cross[ 1 ] = v1[ 2 ] * v2[ 0 ] - v1[ 0 ] * v2[ 2 ];
	cross[ 2 ] = v1[ 0 ] * v2[ 1 ] - v1[ 1 ] * v2[ 0 ];
}

vec_t VectorNormalize( vec3_t v );			// returns vector length
vec_t VectorNormalize2( const vec3_t v, vec3_t out );

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void PerpendicularVector( vec3_t dst, const vec3_t src );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );
void VectorRotate( const vec3_t in, const vec3_t matrix[ 3 ], vec3_t out );

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out );

void ClearBounds( vec3_t mins, vec3_t maxs );
void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs );
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs );

void MatrixMultiply( const float in1[ 3 ][ 3 ], const float in2[ 3 ][ 3 ], float out[ 3 ][ 3 ] );

#define AngleMod( a )     AngleNormalize360( a )
float AngleNormalize360( float angle );
float AngleNormalize180( float angle );
float LerpAngle( float a1, float a2, float frac );
float AngleSubtract( float a1, float a2 );
void AnglesSubtract( const vec3_t v1, const vec3_t v2, vec3_t v3 );
float AngleDelta( float angle1, float angle2 );

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );

// plane types are used to speed some tests
// 0-2 are axial planes
#define PLANE_X             0
#define PLANE_Y             1
#define PLANE_Z             2
#define PLANE_NON_AXIAL     3
#define PLANE_NON_PLANAR    4

// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX      3
#define PLANE_ANYY      4
#define PLANE_ANYZ      5

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
struct cplane_t {
	vec3_t normal;
	float dist;
	byte type;				// for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte signbits;			// signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte pad[ 2 ];
};

/*
=================
PlaneTypeForNormal
=================
*/

#define PlaneTypeForNormal( x ) ( x[ 0 ] == 1.0 ? PLANE_X : ( x[ 1 ] == 1.0 ? PLANE_Y : ( x[ 2 ] == 1.0 ? PLANE_Z : ( x[ 0 ] == 0.f && x[ 1 ] == 0.f && x[ 2 ] == 0.f ? PLANE_NON_PLANAR : PLANE_NON_AXIAL ) ) ) )

void SetPlaneSignbits( cplane_t* out );
extern "C" int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, const cplane_t* plane );
bool PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c );

#define BOX_ON_PLANE_SIDE( emins, emaxs, p )  \
	( ( ( p )->type < 3 ) ?						  \
	  (										  \
		  ( ( p )->dist <= ( emins )[ ( p )->type ] ) ?	 \
		  1								  \
		  :									  \
		  (									  \
			  ( ( p )->dist >= ( emaxs )[ ( p )->type ] ) ?	\
			  2							  \
			  :								  \
			  3							  \
		  )									  \
	  )										  \
	  :										  \
	  BoxOnPlaneSide( ( emins ), ( emaxs ), ( p ) ) )

float VecToYaw( const vec3_t vector );
void VecToAngles( const vec3_t vector, vec3_t angles );
void VecToAnglesBuggy( const vec3_t vector, vec3_t angles );
void AnglesToAxis( const vec3_t angles, vec3_t axis[ 3 ] );

void AxisClear( vec3_t axis[ 3 ] );
void AxisCopy( const vec3_t in[ 3 ], vec3_t out[ 3 ] );

void RotateAroundDirection( vec3_t axis[ 3 ], float yaw );
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up );
// perpendicular vector could be replaced by this

void RotatePoint( vec3_t point, const vec3_t matrix[ 3 ] );
void TransposeMatrix( const vec3_t matrix[ 3 ], vec3_t transpose[ 3 ] );

extern const vec3_t axisDefault[ 3 ];

#define NUMVERTEXNORMALS    162

extern float bytedirs[ NUMVERTEXNORMALS ][ 3 ];

int DirToByte( const vec3_t dir );
void ByteToDir( int b, vec3_t dir );

float VectorDistance( const vec3_t v1, const vec3_t v2 );
float VectorDistanceSquared( const vec3_t p1, const vec3_t p2 );
int VectorBetweenVectors( const vec3_t v, const vec3_t v1, const vec3_t v2 );
void VectorMiddle( const vec3_t v1, const vec3_t v2, vec3_t middle );
float AngleDiff( float ang1, float ang2 );
void ProjectPointOntoVector( const vec3_t point, const vec3_t vStart, const vec3_t vDir, vec3_t vProj );
void ProjectPointOntoVectorFromPoints( const vec3_t point, const vec3_t vStart, const vec3_t vEnd, vec3_t vProj );
void ProjectPointOntoVectorBounded( const vec3_t point, const vec3_t vStart, const vec3_t vEnd, vec3_t vProj );
float DistanceFromLineSquaredDir( const vec3_t p, const vec3_t lp1, const vec3_t lp2, const vec3_t dir );
float DistanceFromLineSquared( const vec3_t p, const vec3_t lp1, const vec3_t lp2 );
float Q_random( int* seed );
extern "C" void Sys_SnapVector( float* v );

#endif
