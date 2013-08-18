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

#ifndef __idBounds__
#define __idBounds__

#include "../math/Vec3.h"
#include "Sphere.h"

/*
===============================================================================

	Axis Aligned Bounding Box

===============================================================================
*/

class idBounds {
public:
	idBounds();
	explicit idBounds( const idVec3 &mins, const idVec3 &maxs );
#if 0
					explicit idBounds( const idVec3 &point );
#endif

	const idVec3& operator[]( const int index ) const;
	idVec3& operator[]( const int index );
	idBounds operator+( const idVec3 &t ) const;				// returns translated bounds
#if 0
	idBounds &		operator+=( const idVec3 &t );					// translate the bounds
	idBounds		operator*( const idMat3 &r ) const;				// returns rotated bounds
	idBounds &		operator*=( const idMat3 &r );					// rotate the bounds
	idBounds		operator+( const idBounds &a ) const;
	idBounds &		operator+=( const idBounds &a );
	idBounds		operator-( const idBounds &a ) const;
	idBounds &		operator-=( const idBounds &a );

	bool			Compare( const idBounds &a ) const;							// exact compare, no epsilon
	bool			Compare( const idBounds &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idBounds &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idBounds &a ) const;						// exact compare, no epsilon
#endif

	void Clear();									// inside out bounds
	void Zero();									// single point at origin

#if 0
	idVec3			GetCenter( void ) const;						// returns center of bounds
	float			GetRadius( void ) const;						// returns the radius relative to the bounds origin
	float			GetRadius( const idVec3 &center ) const;		// returns the radius relative to the given center
	float			GetVolume( void ) const;						// returns the volume of the bounds
	bool			IsCleared( void ) const;						// returns true if bounds are inside out
#endif

	bool AddPoint( const idVec3 &v );				// add the point, returns true if the bounds expanded
	bool AddBounds( const idBounds &a );			// add the bounds, returns true if the bounds expanded
#if 0
	idBounds		Intersect( const idBounds &a ) const;			// return intersection of this bounds with the given bounds
	idBounds &		IntersectSelf( const idBounds &a );				// intersect this bounds with the given bounds
	idBounds		Expand( const float d ) const;					// return bounds expanded in all directions with the given value
	idBounds &		ExpandSelf( const float d );					// expand bounds in all directions with the given value
	idBounds		Translate( const idVec3 &translation ) const;	// return translated bounds
	idBounds &		TranslateSelf( const idVec3 &translation );		// translate this bounds
	idBounds		Rotate( const idMat3 &rotation ) const;			// return rotated bounds
	idBounds &		RotateSelf( const idMat3 &rotation );			// rotate this bounds

	float			PlaneDistance( const idPlane &plane ) const;
	int				PlaneSide( const idPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const idVec3 &p ) const;			// includes touching
	bool			IntersectsBounds( const idBounds &a ) const;	// includes touching
	bool			LineIntersection( const idVec3 &start, const idVec3 &end ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const idVec3 &start, const idVec3 &dir, float &scale ) const;

					// most tight bounds for the given transformed bounds
	void			FromTransformedBounds( const idBounds &bounds, const idVec3 &origin, const idMat3 &axis );
					// most tight bounds for a point set
	void			FromPoints( const idVec3 *points, const int numPoints );
					// most tight bounds for a translation
	void			FromPointTranslation( const idVec3 &point, const idVec3 &translation );
	void			FromBoundsTranslation( const idBounds &bounds, const idVec3 &origin, const idMat3 &axis, const idVec3 &translation );
					// most tight bounds for a rotation
	void			FromPointRotation( const idVec3 &point, const idRotation &rotation );
	void			FromBoundsRotation( const idBounds &bounds, const idVec3 &origin, const idMat3 &axis, const idRotation &rotation );

	void			ToPoints( idVec3 points[8] ) const;
#endif
	idSphere ToSphere() const;

#if 0
	void			AxisProjection( const idVec3 &dir, float &min, float &max ) const;
	void			AxisProjection( const idVec3 &origin, const idMat3 &axis, const idVec3 &dir, float &min, float &max ) const;
#endif

private:
	idVec3 b[ 2 ];
};

#if 0
extern idBounds	bounds_zero;
#endif

inline idBounds::idBounds() {
}

inline idBounds::idBounds( const idVec3 &mins, const idVec3 &maxs ) {
	b[ 0 ] = mins;
	b[ 1 ] = maxs;
}

#if 0
inline idBounds::idBounds( const idVec3 &point ) {
	b[0] = point;
	b[1] = point;
}
#endif

inline const idVec3& idBounds::operator[]( const int index ) const {
	return b[ index ];
}

inline idVec3& idBounds::operator[]( const int index ) {
	return b[ index ];
}

inline idBounds idBounds::operator+( const idVec3 &t ) const {
	return idBounds( b[ 0 ] + t, b[ 1 ] + t );
}

#if 0
inline idBounds &idBounds::operator+=( const idVec3 &t ) {
	b[0] += t;
	b[1] += t;
	return *this;
}

inline idBounds idBounds::operator*( const idMat3 &r ) const {
	idBounds bounds;
	bounds.FromTransformedBounds( *this, vec3_origin, r );
	return bounds;
}

inline idBounds &idBounds::operator*=( const idMat3 &r ) {
	this->FromTransformedBounds( *this, vec3_origin, r );
	return *this;
}

inline idBounds idBounds::operator+( const idBounds &a ) const {
	idBounds newBounds;
	newBounds = *this;
	newBounds.AddBounds( a );
	return newBounds;
}

inline idBounds &idBounds::operator+=( const idBounds &a ) {
	idBounds::AddBounds( a );
	return *this;
}

inline idBounds idBounds::operator-( const idBounds &a ) const {
	assert( b[1][0] - b[0][0] > a.b[1][0] - a.b[0][0] &&
				b[1][1] - b[0][1] > a.b[1][1] - a.b[0][1] &&
					b[1][2] - b[0][2] > a.b[1][2] - a.b[0][2] );
	return idBounds( idVec3( b[0][0] + a.b[1][0], b[0][1] + a.b[1][1], b[0][2] + a.b[1][2] ),
					idVec3( b[1][0] + a.b[0][0], b[1][1] + a.b[0][1], b[1][2] + a.b[0][2] ) );
}

inline idBounds &idBounds::operator-=( const idBounds &a ) {
	assert( b[1][0] - b[0][0] > a.b[1][0] - a.b[0][0] &&
				b[1][1] - b[0][1] > a.b[1][1] - a.b[0][1] &&
					b[1][2] - b[0][2] > a.b[1][2] - a.b[0][2] );
	b[0] += a.b[1];
	b[1] += a.b[0];
	return *this;
}

inline bool idBounds::Compare( const idBounds &a ) const {
	return ( b[0].Compare( a.b[0] ) && b[1].Compare( a.b[1] ) );
}

inline bool idBounds::Compare( const idBounds &a, const float epsilon ) const {
	return ( b[0].Compare( a.b[0], epsilon ) && b[1].Compare( a.b[1], epsilon ) );
}

inline bool idBounds::operator==( const idBounds &a ) const {
	return Compare( a );
}

inline bool idBounds::operator!=( const idBounds &a ) const {
	return !Compare( a );
}
#endif

inline void idBounds::Clear() {
	b[0][0] = b[0][1] = b[0][2] = idMath::INFINITY;
	b[1][0] = b[1][1] = b[1][2] = -idMath::INFINITY;
}

inline void idBounds::Zero() {
	b[0][0] = b[0][1] = b[0][2] =
	b[1][0] = b[1][1] = b[1][2] = 0;
}

#if 0
inline idVec3 idBounds::GetCenter( void ) const {
	return idVec3( ( b[1][0] + b[0][0] ) * 0.5f, ( b[1][1] + b[0][1] ) * 0.5f, ( b[1][2] + b[0][2] ) * 0.5f );
}

inline float idBounds::GetVolume( void ) const {
	if ( b[0][0] >= b[1][0] || b[0][1] >= b[1][1] || b[0][2] >= b[1][2] ) {
		return 0.0f;
	}
	return ( ( b[1][0] - b[0][0] ) * ( b[1][1] - b[0][1] ) * ( b[1][2] - b[0][2] ) );
}

inline bool idBounds::IsCleared( void ) const {
	return b[0][0] > b[1][0];
}
#endif

inline bool idBounds::AddPoint( const idVec3 &v ) {
	bool expanded = false;
	if ( v.x < b[ 0 ].x) {
		b[ 0 ].x = v.x;
		expanded = true;
	}
	if ( v.x > b[ 1 ].x) {
		b[ 1 ].x = v.x;
		expanded = true;
	}
	if ( v.y < b[ 0 ].y ) {
		b[ 0 ].y = v.y;
		expanded = true;
	}
	if ( v.y > b[ 1 ].y) {
		b[ 1 ].y = v.y;
		expanded = true;
	}
	if ( v.z < b[ 0 ].z ) {
		b[ 0 ].z = v.z;
		expanded = true;
	}
	if ( v.z > b[ 1 ].z) {
		b[ 1 ].z = v.z;
		expanded = true;
	}
	return expanded;
}

inline bool idBounds::AddBounds( const idBounds &a ) {
	bool expanded = false;
	if ( a.b[ 0 ].x < b[ 0 ].x ) {
		b[ 0 ].x = a.b[ 0 ].x;
		expanded = true;
	}
	if ( a.b[ 0 ].y < b[ 0 ].y ) {
		b[ 0 ].y = a.b[ 0 ].y;
		expanded = true;
	}
	if ( a.b[ 0 ].z < b[ 0 ].z ) {
		b[ 0 ].z = a.b[ 0 ].z;
		expanded = true;
	}
	if ( a.b[ 1 ].x > b[ 1 ].x ) {
		b[ 1 ].x = a.b[ 1 ].x;
		expanded = true;
	}
	if ( a.b[ 1 ].y > b[ 1 ].y ) {
		b[ 1 ].y = a.b[ 1 ].y;
		expanded = true;
	}
	if ( a.b[ 1 ].z > b[ 1 ].z ) {
		b[ 1 ].z = a.b[ 1 ].z;
		expanded = true;
	}
	return expanded;
}

#if 0
inline idBounds idBounds::Intersect( const idBounds &a ) const {
	idBounds n;
	n.b[0][0] = ( a.b[0][0] > b[0][0] ) ? a.b[0][0] : b[0][0];
	n.b[0][1] = ( a.b[0][1] > b[0][1] ) ? a.b[0][1] : b[0][1];
	n.b[0][2] = ( a.b[0][2] > b[0][2] ) ? a.b[0][2] : b[0][2];
	n.b[1][0] = ( a.b[1][0] < b[1][0] ) ? a.b[1][0] : b[1][0];
	n.b[1][1] = ( a.b[1][1] < b[1][1] ) ? a.b[1][1] : b[1][1];
	n.b[1][2] = ( a.b[1][2] < b[1][2] ) ? a.b[1][2] : b[1][2];
	return n;
}

inline idBounds &idBounds::IntersectSelf( const idBounds &a ) {
	if ( a.b[0][0] > b[0][0] ) {
		b[0][0] = a.b[0][0];
	}
	if ( a.b[0][1] > b[0][1] ) {
		b[0][1] = a.b[0][1];
	}
	if ( a.b[0][2] > b[0][2] ) {
		b[0][2] = a.b[0][2];
	}
	if ( a.b[1][0] < b[1][0] ) {
		b[1][0] = a.b[1][0];
	}
	if ( a.b[1][1] < b[1][1] ) {
		b[1][1] = a.b[1][1];
	}
	if ( a.b[1][2] < b[1][2] ) {
		b[1][2] = a.b[1][2];
	}
	return *this;
}

inline idBounds idBounds::Expand( const float d ) const {
	return idBounds( idVec3( b[0][0] - d, b[0][1] - d, b[0][2] - d ),
						idVec3( b[1][0] + d, b[1][1] + d, b[1][2] + d ) );
}

inline idBounds &idBounds::ExpandSelf( const float d ) {
	b[0][0] -= d;
	b[0][1] -= d;
	b[0][2] -= d;
	b[1][0] += d;
	b[1][1] += d;
	b[1][2] += d;
	return *this;
}

inline idBounds idBounds::Translate( const idVec3 &translation ) const {
	return idBounds( b[0] + translation, b[1] + translation );
}

inline idBounds &idBounds::TranslateSelf( const idVec3 &translation ) {
	b[0] += translation;
	b[1] += translation;
	return *this;
}

inline idBounds idBounds::Rotate( const idMat3 &rotation ) const {
	idBounds bounds;
	bounds.FromTransformedBounds( *this, vec3_origin, rotation );
	return bounds;
}

inline idBounds &idBounds::RotateSelf( const idMat3 &rotation ) {
	FromTransformedBounds( *this, vec3_origin, rotation );
	return *this;
}

inline bool idBounds::ContainsPoint( const idVec3 &p ) const {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2]
		|| p[0] > b[1][0] || p[1] > b[1][1] || p[2] > b[1][2] ) {
		return false;
	}
	return true;
}

inline bool idBounds::IntersectsBounds( const idBounds &a ) const {
	if ( a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[1][2] < b[0][2]
		|| a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] || a.b[0][2] > b[1][2] ) {
		return false;
	}
	return true;
}
#endif

inline idSphere idBounds::ToSphere() const {
	idSphere sphere;
	sphere.SetOrigin( ( b[ 0 ] + b[ 1 ] ) * 0.5f );
	sphere.SetRadius( ( b[ 1 ] - sphere.GetOrigin() ).Length() );
	return sphere;
}

#if 0
inline void idBounds::AxisProjection( const idVec3 &dir, float &min, float &max ) const {
	float d1, d2;
	idVec3 center, extents;

	center = ( b[0] + b[1] ) * 0.5f;
	extents = b[1] - center;

	d1 = dir * center;
	d2 = idMath::Fabs( extents[0] * dir[0] ) +
			idMath::Fabs( extents[1] * dir[1] ) +
				idMath::Fabs( extents[2] * dir[2] );

	min = d1 - d2;
	max = d1 + d2;
}

inline void idBounds::AxisProjection( const idVec3 &origin, const idMat3 &axis, const idVec3 &dir, float &min, float &max ) const {
	float d1, d2;
	idVec3 center, extents;

	center = ( b[0] + b[1] ) * 0.5f;
	extents = b[1] - center;
	center = origin + center * axis;

	d1 = dir * center;
	d2 = idMath::Fabs( extents[0] * ( dir * axis[0] ) ) +
			idMath::Fabs( extents[1] * ( dir * axis[1] ) ) +
				idMath::Fabs( extents[2] * ( dir * axis[2] ) );

	min = d1 - d2;
	max = d1 + d2;
}
#endif

#endif
