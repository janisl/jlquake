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

#ifndef __idVec2__
#define __idVec2__

class idVec2 {
public:
	float			x;
	float			y;

#if 0
					idVec2( void );
					explicit idVec2( const float x, const float y );

	void			Set( const float x, const float y );
	void			Zero( void );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	idVec2			operator-() const;
	float			operator*( const idVec2 &a ) const;
	idVec2			operator*( const float a ) const;
	idVec2			operator/( const float a ) const;
	idVec2			operator+( const idVec2 &a ) const;
	idVec2			operator-( const idVec2 &a ) const;
	idVec2 &		operator+=( const idVec2 &a );
	idVec2 &		operator-=( const idVec2 &a );
	idVec2 &		operator/=( const idVec2 &a );
	idVec2 &		operator/=( const float a );
	idVec2 &		operator*=( const float a );

	friend idVec2	operator*( const float a, const idVec2 b );

	bool			Compare( const idVec2 &a ) const;							// exact compare, no epsilon
	bool			Compare( const idVec2 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec2 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec2 &a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthFast( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length
	idVec2 &		Truncate( float length );	// cap length
	void			Clamp( const idVec2 &min, const idVec2 &max );
	void			Snap( void );				// snap to closest integer value
	void			SnapInt( void );			// snap towards integer (floor)

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const idVec2 &v1, const idVec2 &v2, const float l );
#endif
};

#if 0
extern idVec2 vec2_origin;
#define vec2_zero vec2_origin

ID_INLINE idVec2::idVec2( void ) {
}

ID_INLINE idVec2::idVec2( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ID_INLINE void idVec2::Set( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ID_INLINE void idVec2::Zero( void ) {
	x = y = 0.0f;
}

ID_INLINE bool idVec2::Compare( const idVec2 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) );
}

ID_INLINE bool idVec2::Compare( const idVec2 &a, const float epsilon ) const {
	if ( idMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec2::operator==( const idVec2 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec2::operator!=( const idVec2 &a ) const {
	return !Compare( a );
}

ID_INLINE float idVec2::operator[]( int index ) const {
	return ( &x )[ index ];
}

ID_INLINE float& idVec2::operator[]( int index ) {
	return ( &x )[ index ];
}

ID_INLINE float idVec2::Length( void ) const {
	return ( float )idMath::Sqrt( x * x + y * y );
}

ID_INLINE float idVec2::LengthFast( void ) const {
	float sqrLength;

	sqrLength = x * x + y * y;
	return sqrLength * idMath::RSqrt( sqrLength );
}

ID_INLINE float idVec2::LengthSqr( void ) const {
	return ( x * x + y * y );
}

ID_INLINE float idVec2::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y;
	invLength = idMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	return invLength * sqrLength;
}

ID_INLINE float idVec2::NormalizeFast( void ) {
	float lengthSqr, invLength;

	lengthSqr = x * x + y * y;
	invLength = idMath::RSqrt( lengthSqr );
	x *= invLength;
	y *= invLength;
	return invLength * lengthSqr;
}

ID_INLINE idVec2 &idVec2::Truncate( float length ) {
	float length2;
	float ilength;

	if ( !length ) {
		Zero();
	}
	else {
		length2 = LengthSqr();
		if ( length2 > length * length ) {
			ilength = length * idMath::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
		}
	}

	return *this;
}

ID_INLINE void idVec2::Clamp( const idVec2 &min, const idVec2 &max ) {
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
}

ID_INLINE void idVec2::Snap( void ) {
	x = floor( x + 0.5f );
	y = floor( y + 0.5f );
}

ID_INLINE void idVec2::SnapInt( void ) {
	x = float( int( x ) );
	y = float( int( y ) );
}

ID_INLINE idVec2 idVec2::operator-() const {
	return idVec2( -x, -y );
}

ID_INLINE idVec2 idVec2::operator-( const idVec2 &a ) const {
	return idVec2( x - a.x, y - a.y );
}

ID_INLINE float idVec2::operator*( const idVec2 &a ) const {
	return x * a.x + y * a.y;
}

ID_INLINE idVec2 idVec2::operator*( const float a ) const {
	return idVec2( x * a, y * a );
}

ID_INLINE idVec2 idVec2::operator/( const float a ) const {
	float inva = 1.0f / a;
	return idVec2( x * inva, y * inva );
}

ID_INLINE idVec2 operator*( const float a, const idVec2 b ) {
	return idVec2( b.x * a, b.y * a );
}

ID_INLINE idVec2 idVec2::operator+( const idVec2 &a ) const {
	return idVec2( x + a.x, y + a.y );
}

ID_INLINE idVec2 &idVec2::operator+=( const idVec2 &a ) {
	x += a.x;
	y += a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator/=( const idVec2 &a ) {
	x /= a.x;
	y /= a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator-=( const idVec2 &a ) {
	x -= a.x;
	y -= a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator*=( const float a ) {
	x *= a;
	y *= a;

	return *this;
}

ID_INLINE int idVec2::GetDimension( void ) const {
	return 2;
}

ID_INLINE const float *idVec2::ToFloatPtr( void ) const {
	return &x;
}

ID_INLINE float *idVec2::ToFloatPtr( void ) {
	return &x;
}
#endif

#endif
