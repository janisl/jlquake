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

#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__

#include "../../common/mathlib.h"
#include "../../common/math/Vec3.h"

#ifndef EQUAL_EPSILON
#define EQUAL_EPSILON   0.001
#endif

class idVec3Splines : public idVec3
{
public:
	idVec3Splines()
	{
	}
	idVec3Splines(const float x, const float y, const float z);

	operator float*();

	float operator[](const int index) const;
	float& operator[](const int index);

	void set(const float x, const float y, const float z);

	idVec3Splines operator-() const;

	idVec3Splines& operator=(const idVec3Splines& a);

	float operator*(const idVec3Splines& a) const;
	idVec3Splines operator*(const float a) const;
	friend idVec3Splines operator*(float a, idVec3Splines b);

	idVec3Splines operator+(const idVec3Splines& a) const;
	idVec3Splines operator-(const idVec3Splines& a) const;

	idVec3Splines& operator+=(const idVec3Splines& a);
	idVec3Splines& operator-=(const idVec3Splines& a);
	idVec3Splines& operator*=(const float a);

	int operator==(const idVec3Splines& a) const;
	int operator!=(const idVec3Splines& a) const;

	idVec3Splines Cross(const idVec3Splines& a) const;
	idVec3Splines& Cross(const idVec3Splines& a, const idVec3Splines& b);

	float Length() const;
	float Normalize();

	void Zero();
	void Snap();
};

inline idVec3Splines::idVec3Splines(const float x, const float y, const float z)
: idVec3(x, y, z)
{
}

inline float idVec3Splines::operator[](const int index) const
{
	return (&x)[index];
}

inline float& idVec3Splines::operator[](const int index)
{
	return (&x)[index];
}

inline idVec3Splines::operator float*()
{
	return &x;
}

inline idVec3Splines idVec3Splines::operator-() const
{
	return idVec3Splines(-x, -y, -z);
}

inline idVec3Splines& idVec3Splines::operator=(const idVec3Splines& a)
{
	x = a.x;
	y = a.y;
	z = a.z;

	return *this;
}

inline void idVec3Splines::set(const float x, const float y, const float z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

inline idVec3Splines idVec3Splines::operator-(const idVec3Splines& a) const
{
	return idVec3Splines(x - a.x, y - a.y, z - a.z);
}

inline float idVec3Splines::operator*(const idVec3Splines& a) const
{
	return x * a.x + y * a.y + z * a.z;
}

inline idVec3Splines idVec3Splines::operator*(const float a) const
{
	return idVec3Splines(x * a, y * a, z * a);
}

inline idVec3Splines operator*(const float a, const idVec3Splines b)
{
	return idVec3Splines(b.x * a, b.y * a, b.z * a);
}

inline idVec3Splines idVec3Splines::operator+(const idVec3Splines& a) const
{
	return idVec3Splines(x + a.x, y + a.y, z + a.z);
}

inline idVec3Splines& idVec3Splines::operator+=(const idVec3Splines& a)
{
	x += a.x;
	y += a.y;
	z += a.z;

	return *this;
}

inline idVec3Splines& idVec3Splines::operator-=(const idVec3Splines& a)
{
	x -= a.x;
	y -= a.y;
	z -= a.z;

	return *this;
}

inline idVec3Splines& idVec3Splines::operator*=(const float a)
{
	x *= a;
	y *= a;
	z *= a;

	return *this;
}

inline int idVec3Splines::operator==(const idVec3Splines& a) const
{
	if (idMath::Fabs(x - a.x) > EQUAL_EPSILON)
	{
		return false;
	}

	if (idMath::Fabs(y - a.y) > EQUAL_EPSILON)
	{
		return false;
	}

	if (idMath::Fabs(z - a.z) > EQUAL_EPSILON)
	{
		return false;
	}

	return true;
}

inline int idVec3Splines::operator!=(const idVec3Splines& a) const
{
	if (idMath::Fabs(x - a.x) > EQUAL_EPSILON)
	{
		return true;
	}

	if (idMath::Fabs(y - a.y) > EQUAL_EPSILON)
	{
		return true;
	}

	if (idMath::Fabs(z - a.z) > EQUAL_EPSILON)
	{
		return true;
	}

	return false;
}

inline idVec3Splines idVec3Splines::Cross(const idVec3Splines& a) const
{
	return idVec3Splines(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x);
}

inline idVec3Splines& idVec3Splines::Cross(const idVec3Splines& a, const idVec3Splines& b)
{
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;

	return *this;
}

inline float idVec3Splines::Length() const
{
	float length;

	length = x * x + y * y + z * z;
	return (float)sqrt(length);
}

inline float idVec3Splines::Normalize()
{
	float length;
	float ilength;

	length = this->Length();
	if (length)
	{
		ilength = 1.0f / length;
		x *= ilength;
		y *= ilength;
		z *= ilength;
	}

	return length;
}

inline void idVec3Splines::Zero()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

inline void idVec3Splines::Snap()
{
	x = float(int(x));
	y = float(int(y));
	z = float(int(z));
}

#endif
