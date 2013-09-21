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

#ifndef __idVec3__
#define __idVec3__

#include "Math.h"
#include "../mathlib.h"

#if 0
#define VECTOR_EPSILON      0.001f
#endif

class idVec3 {
public:
	float x;
	float y;
	float z;

	idVec3();
	explicit idVec3( const float x, const float y, const float z );

	void Set( const float x, const float y, const float z );
	void Zero();

	float operator[]( const int index ) const;
	float& operator[]( const int index );
#if 0
	idVec3 operator-() const;
#endif
	idVec3& operator=( const idVec3& a );
	idVec3 operator+( const idVec3& a ) const;
	idVec3 operator-( const idVec3& a ) const;
	float operator*( const idVec3& a ) const;
	idVec3 operator*( const float a ) const;
	idVec3 operator/( const float a ) const;
	idVec3& operator+=( const idVec3& a );
	idVec3& operator-=( const idVec3& a );
#if 0
	idVec3&        operator/=( const idVec3& a );
	idVec3&        operator/=( const float a );
#endif
	idVec3& operator*=( const float a );

	friend idVec3 operator*( const float a, const idVec3& b );

	bool            Compare( const idVec3& a ) const;								// exact compare, no epsilon
#if 0
	bool            Compare( const idVec3& a, const float epsilon ) const;			// compare with epsilon
#endif
	bool operator==( const idVec3& a ) const;										// exact compare, no epsilon
	bool operator!=( const idVec3& a ) const;										// exact compare, no epsilon

#if 0
	bool            FixDegenerateNormal( void );		// fix degenerate axial cases
	bool            FixDenormals( void );				// change tiny numbers to zero

	idVec3          Cross( const idVec3& a ) const;
	idVec3&        Cross( const idVec3& a, const idVec3& b );
#endif
	float Length() const;
	float LengthSqr() const;
#if 0
	float           LengthFast( void ) const;
#endif
	float Normalize();				// returns length
#if 0
	float           NormalizeFast( void );				// returns length
	idVec3&        Truncate( float length );			// cap length
	void            Clamp( const idVec3& min, const idVec3& max );
	void            Snap( void );						// snap to closest integer value
	void            SnapInt( void );					// snap towards integer (floor)

	int             GetDimension( void ) const;

	float           ToYaw( void ) const;
	float           ToPitch( void ) const;
	idAngles        ToAngles( void ) const;
	idPolar3        ToPolar( void ) const;
	idMat3          ToMat3( void ) const;			// vector should be normalized
	const idVec2&  ToVec2( void ) const;
	idVec2&        ToVec2( void );
	const float* ToFloatPtr( void ) const;
#endif
	float* ToFloatPtr();
#if 0
	const char* ToString( int precision = 2 ) const;

	void            NormalVectors( idVec3& left, idVec3& down ) const;		// vector should be normalized
	void            OrthogonalBasis( idVec3& left, idVec3& up ) const;

	void            ProjectOntoPlane( const idVec3& normal, const float overBounce = 1.0f );
	bool            ProjectAlongPlane( const idVec3& normal, const float epsilon, const float overBounce = 1.0f );
	void            ProjectSelfOntoSphere( const float radius );

	void            Lerp( const idVec3& v1, const idVec3& v2, const float l );
	void            SLerp( const idVec3& v1, const idVec3& v2, const float l );
#endif

	void FromOldVec3( vec3_t vec );
	void ToOldVec3( vec3_t vec ) const;
};

extern const idVec3 vec3_origin;
#define vec3_zero vec3_origin

inline idVec3::idVec3() {
}

inline idVec3::idVec3( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

inline void idVec3::Set( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

inline void idVec3::Zero() {
	x = y = z = 0.0f;
}

inline float idVec3::operator[]( const int index ) const {
	return ( &x )[ index ];
}

inline float& idVec3::operator[]( const int index ) {
	return ( &x )[ index ];
}

#if 0
inline idVec3 idVec3::operator-() const {
	return idVec3( -x, -y, -z );
}
#endif

inline idVec3& idVec3::operator=( const idVec3& a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	return *this;
}

inline idVec3 idVec3::operator+( const idVec3& a ) const {
	return idVec3( x + a.x, y + a.y, z + a.z );
}

inline idVec3 idVec3::operator-( const idVec3& a ) const {
	return idVec3( x - a.x, y - a.y, z - a.z );
}

inline float idVec3::operator*( const idVec3& a ) const {
	return x * a.x + y * a.y + z * a.z;
}

inline idVec3 idVec3::operator*( const float a ) const {
	return idVec3( x * a, y * a, z * a );
}

inline idVec3 idVec3::operator/( const float a ) const {
	float inva = 1.0f / a;
	return idVec3( x * inva, y * inva, z * inva );
}

inline idVec3& idVec3::operator+=( const idVec3& a ) {
	x += a.x;
	y += a.y;
	z += a.z;

	return *this;
}

inline idVec3& idVec3::operator-=( const idVec3& a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;

	return *this;
}

#if 0
inline idVec3& idVec3::operator/=( const idVec3& a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;

	return *this;
}

inline idVec3& idVec3::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;

	return *this;
}
#endif

inline idVec3& idVec3::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;

	return *this;
}

inline idVec3 operator*( const float a, const idVec3& b ) {
	return idVec3( b.x * a, b.y * a, b.z * a );
}

inline bool idVec3::Compare( const idVec3& a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

#if 0
inline bool idVec3::Compare( const idVec3& a, const float epsilon ) const {
	if ( idMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	return true;
}
#endif

inline bool idVec3::operator==( const idVec3& a ) const {
	return Compare( a );
}

inline bool idVec3::operator!=( const idVec3& a ) const {
	return !Compare( a );
}

#if 0
inline float idVec3::NormalizeFast( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z;
	invLength = idMath::RSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	return invLength * sqrLength;
}

inline bool idVec3::FixDegenerateNormal( void ) {
	if ( x == 0.0f ) {
		if ( y == 0.0f ) {
			if ( z > 0.0f ) {
				if ( z != 1.0f ) {
					z = 1.0f;
					return true;
				}
			} else {
				if ( z != -1.0f ) {
					z = -1.0f;
					return true;
				}
			}
			return false;
		} else if ( z == 0.0f ) {
			if ( y > 0.0f ) {
				if ( y != 1.0f ) {
					y = 1.0f;
					return true;
				}
			} else {
				if ( y != -1.0f ) {
					y = -1.0f;
					return true;
				}
			}
			return false;
		}
	} else if ( y == 0.0f ) {
		if ( z == 0.0f ) {
			if ( x > 0.0f ) {
				if ( x != 1.0f ) {
					x = 1.0f;
					return true;
				}
			} else {
				if ( x != -1.0f ) {
					x = -1.0f;
					return true;
				}
			}
			return false;
		}
	}
	if ( idMath::Fabs( x ) == 1.0f ) {
		if ( y != 0.0f || z != 0.0f ) {
			y = z = 0.0f;
			return true;
		}
		return false;
	} else if ( idMath::Fabs( y ) == 1.0f ) {
		if ( x != 0.0f || z != 0.0f ) {
			x = z = 0.0f;
			return true;
		}
		return false;
	} else if ( idMath::Fabs( z ) == 1.0f ) {
		if ( x != 0.0f || y != 0.0f ) {
			x = y = 0.0f;
			return true;
		}
		return false;
	}
	return false;
}

inline bool idVec3::FixDenormals( void ) {
	bool denormal = false;
	if ( fabs( x ) < 1e-30f ) {
		x = 0.0f;
		denormal = true;
	}
	if ( fabs( y ) < 1e-30f ) {
		y = 0.0f;
		denormal = true;
	}
	if ( fabs( z ) < 1e-30f ) {
		z = 0.0f;
		denormal = true;
	}
	return denormal;
}

inline idVec3 idVec3::Cross( const idVec3& a ) const {
	return idVec3( y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x );
}

inline idVec3& idVec3::Cross( const idVec3& a, const idVec3& b ) {
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;

	return *this;
}
#endif

inline float idVec3::Length() const {
	return ( float )idMath::Sqrt( x * x + y * y + z * z );
}

inline float idVec3::LengthSqr() const {
	return x * x + y * y + z * z;
}

#if 0
inline float idVec3::LengthFast( void ) const {
	float sqrLength;

	sqrLength = x * x + y * y + z * z;
	return sqrLength * idMath::RSqrt( sqrLength );
}
#endif

inline float idVec3::Normalize() {
	float sqrLength = x * x + y * y + z * z;
	float invLength = idMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	return invLength * sqrLength;
}

#if 0
inline idVec3& idVec3::Truncate( float length ) {
	float length2;
	float ilength;

	if ( !length ) {
		Zero();
	} else {
		length2 = LengthSqr();
		if ( length2 > length * length ) {
			ilength = length * idMath::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
			z *= ilength;
		}
	}

	return *this;
}

inline void idVec3::Clamp( const idVec3& min, const idVec3& max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
	if ( z < min.z ) {
		z = min.z;
	} else if ( z > max.z ) {
		z = max.z;
	}
}

inline void idVec3::Snap( void ) {
	x = floor( x + 0.5f );
	y = floor( y + 0.5f );
	z = floor( z + 0.5f );
}

inline void idVec3::SnapInt( void ) {
	x = float( int( x ) );
	y = float( int( y ) );
	z = float( int( z ) );
}

inline int idVec3::GetDimension( void ) const {
	return 3;
}

inline const idVec2& idVec3::ToVec2( void ) const {
	return *reinterpret_cast<const idVec2*>( this );
}

inline idVec2& idVec3::ToVec2( void ) {
	return *reinterpret_cast<idVec2*>( this );
}

inline const float* idVec3::ToFloatPtr( void ) const {
	return &x;
}
#endif

inline float* idVec3::ToFloatPtr() {
	return &x;
}

#if 0
inline void idVec3::NormalVectors( idVec3& left, idVec3& down ) const {
	float d;

	d = x * x + y * y;
	if ( !d ) {
		left[ 0 ] = 1;
		left[ 1 ] = 0;
		left[ 2 ] = 0;
	} else {
		d = idMath::InvSqrt( d );
		left[ 0 ] = -y * d;
		left[ 1 ] = x * d;
		left[ 2 ] = 0;
	}
	down = left.Cross( *this );
}

inline void idVec3::OrthogonalBasis( idVec3& left, idVec3& up ) const {
	float l, s;

	if ( idMath::Fabs( z ) > 0.7f ) {
		l = y * y + z * z;
		s = idMath::InvSqrt( l );
		up[ 0 ] = 0;
		up[ 1 ] = z * s;
		up[ 2 ] = -y * s;
		left[ 0 ] = l * s;
		left[ 1 ] = -x * up[ 2 ];
		left[ 2 ] = x * up[ 1 ];
	} else {
		l = x * x + y * y;
		s = idMath::InvSqrt( l );
		left[ 0 ] = -y * s;
		left[ 1 ] = x * s;
		left[ 2 ] = 0;
		up[ 0 ] = -z * left[ 1 ];
		up[ 1 ] = z * left[ 0 ];
		up[ 2 ] = l * s;
	}
}

inline void idVec3::ProjectOntoPlane( const idVec3& normal, const float overBounce ) {
	float backoff;

	backoff = *this * normal;

	if ( overBounce != 1.0 ) {
		if ( backoff < 0 ) {
			backoff *= overBounce;
		} else {
			backoff /= overBounce;
		}
	}

	*this -= backoff * normal;
}

inline bool idVec3::ProjectAlongPlane( const idVec3& normal, const float epsilon, const float overBounce ) {
	idVec3 cross;
	float len;

	cross = this->Cross( normal ).Cross( ( *this ) );
	// normalize so a fixed epsilon can be used
	cross.Normalize();
	len = normal * cross;
	if ( idMath::Fabs( len ) < epsilon ) {
		return false;
	}
	cross *= overBounce * ( normal * ( *this ) ) / len;
	( *this ) -= cross;
	return true;
}
#endif

inline void idVec3::FromOldVec3( vec3_t vec ) {
	x = vec[ 0 ];
	y = vec[ 1 ];
	z = vec[ 2 ];
}

inline void idVec3::ToOldVec3( vec3_t vec ) const {
	vec[ 0 ] = x;
	vec[ 1 ] = y;
	vec[ 2 ] = z;
}

#endif	/* !__MATH_VECTOR_H__ */
