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

#include "Vec3.h"

const idVec3 vec3_origin( 0.0f, 0.0f, 0.0f );

#if 0
/*
=============
idVec3::ToYaw
=============
*/
float idVec3::ToYaw( void ) const {
	float yaw;

	if ( ( y == 0.0f ) && ( x == 0.0f ) ) {
		yaw = 0.0f;
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}
	}

	return yaw;
}

/*
=============
idVec3::ToPitch
=============
*/
float idVec3::ToPitch( void ) const {
	float forward;
	float pitch;

	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return pitch;
}

/*
=============
idVec3::ToAngles
=============
*/
idAngles idVec3::ToAngles( void ) const {
	float forward;
	float yaw;
	float pitch;

	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	return idAngles( -pitch, yaw, 0.0f );
}

/*
=============
idVec3::ToPolar
=============
*/
idPolar3 idVec3::ToPolar( void ) const {
	float forward;
	float yaw;
	float pitch;

	if ( ( x == 0.0f ) && ( y == 0.0f ) ) {
		yaw = 0.0f;
		if ( z > 0.0f ) {
			pitch = 90.0f;
		} else {
			pitch = 270.0f;
		}
	} else {
		yaw = RAD2DEG( atan2( y, x ) );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = ( float )idMath::Sqrt( x * x + y * y );
		pitch = RAD2DEG( atan2( z, forward ) );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}
	return idPolar3( idMath::Sqrt( x * x + y * y + z * z ), yaw, -pitch );
}

/*
=============
idVec3::ToMat3
=============
*/
idMat3 idVec3::ToMat3( void ) const {
	idMat3 mat;
	float d;

	mat[ 0 ] = *this;
	d = x * x + y * y;
	if ( !d ) {
		mat[ 1 ][ 0 ] = 1.0f;
		mat[ 1 ][ 1 ] = 0.0f;
		mat[ 1 ][ 2 ] = 0.0f;
	} else {
		d = idMath::InvSqrt( d );
		mat[ 1 ][ 0 ] = -y * d;
		mat[ 1 ][ 1 ] = x * d;
		mat[ 1 ][ 2 ] = 0.0f;
	}
	mat[ 2 ] = Cross( mat[ 1 ] );

	return mat;
}

/*
=============
idVec3::ToString
=============
*/
const char* idVec3::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void idVec3::Lerp( const idVec3& v1, const idVec3& v2, const float l ) {
	if ( l <= 0.0f ) {
		( *this ) = v1;
	} else if ( l >= 1.0f ) {
		( *this ) = v2;
	} else {
		( *this ) = v1 + l * ( v2 - v1 );
	}
}

/*
=============
SLerp

Spherical linear interpolation from v1 to v2.
Vectors are expected to be normalized.
=============
*/
#define LERP_DELTA 1e-6

void idVec3::SLerp( const idVec3& v1, const idVec3& v2, const float t ) {
	float omega, cosom, sinom, scale0, scale1;

	if ( t <= 0.0f ) {
		( *this ) = v1;
		return;
	} else if ( t >= 1.0f ) {
		( *this ) = v2;
		return;
	}

	cosom = v1 * v2;
	if ( ( 1.0f - cosom ) > LERP_DELTA ) {
		omega = acos( cosom );
		sinom = sin( omega );
		scale0 = sin( ( 1.0f - t ) * omega ) / sinom;
		scale1 = sin( t * omega ) / sinom;
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}

	( *this ) = ( v1 * scale0 + v2 * scale1 );
}

/*
=============
ProjectSelfOntoSphere

Projects the z component onto a sphere.
=============
*/
void idVec3::ProjectSelfOntoSphere( const float radius ) {
	float rsqr = radius * radius;
	float len = Length();
	if ( len  < rsqr * 0.5f ) {
		z = sqrt( rsqr - len );
	} else {
		z = rsqr / ( 2.0f * sqrt( len ) );
	}
}
#endif
