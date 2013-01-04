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

#ifndef EQUAL_EPSILON
#define EQUAL_EPSILON   0.001
#endif

class idVec3
{
public:
	float x, y, z;

	idVec3()
	{
	}
	idVec3(const float x, const float y, const float z);

	operator float*();

	float operator[](const int index) const;
	float& operator[](const int index);

	void set(const float x, const float y, const float z);

	idVec3 operator-() const;

	idVec3& operator=(const idVec3& a);

	float operator*(const idVec3& a) const;
	idVec3 operator*(const float a) const;
	friend idVec3 operator*(float a, idVec3 b);

	idVec3 operator+(const idVec3& a) const;
	idVec3 operator-(const idVec3& a) const;

	idVec3& operator+=(const idVec3& a);
	idVec3& operator-=(const idVec3& a);
	idVec3& operator*=(const float a);

	int operator==(const idVec3& a) const;
	int operator!=(const idVec3& a) const;

	idVec3 Cross(const idVec3& a) const;
	idVec3& Cross(const idVec3& a, const idVec3& b);

	float Length() const;
	float Normalize();

	void Zero();
	void Snap();
};

inline idVec3::idVec3(const float x, const float y, const float z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

inline float idVec3::operator[](const int index) const
{
	return (&x)[index];
}

inline float& idVec3::operator[](const int index)
{
	return (&x)[index];
}

inline idVec3::operator float*()
{
	return &x;
}

inline idVec3 idVec3::operator-() const
{
	return idVec3(-x, -y, -z);
}

inline idVec3& idVec3::operator=(const idVec3& a)
{
	x = a.x;
	y = a.y;
	z = a.z;

	return *this;
}

inline void idVec3::set(const float x, const float y, const float z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

inline idVec3 idVec3::operator-(const idVec3& a) const
{
	return idVec3(x - a.x, y - a.y, z - a.z);
}

inline float idVec3::operator*(const idVec3& a) const
{
	return x * a.x + y * a.y + z * a.z;
}

inline idVec3 idVec3::operator*(const float a) const
{
	return idVec3(x * a, y * a, z * a);
}

inline idVec3 operator*(const float a, const idVec3 b)
{
	return idVec3(b.x * a, b.y * a, b.z * a);
}

inline idVec3 idVec3::operator+(const idVec3& a) const
{
	return idVec3(x + a.x, y + a.y, z + a.z);
}

inline idVec3& idVec3::operator+=(const idVec3& a)
{
	x += a.x;
	y += a.y;
	z += a.z;

	return *this;
}

inline idVec3& idVec3::operator-=(const idVec3& a)
{
	x -= a.x;
	y -= a.y;
	z -= a.z;

	return *this;
}

inline idVec3& idVec3::operator*=(const float a)
{
	x *= a;
	y *= a;
	z *= a;

	return *this;
}

inline int idVec3::operator==(const idVec3& a) const
{
	if (Q_fabs(x - a.x) > EQUAL_EPSILON)
	{
		return false;
	}

	if (Q_fabs(y - a.y) > EQUAL_EPSILON)
	{
		return false;
	}

	if (Q_fabs(z - a.z) > EQUAL_EPSILON)
	{
		return false;
	}

	return true;
}

inline int idVec3::operator!=(const idVec3& a) const
{
	if (Q_fabs(x - a.x) > EQUAL_EPSILON)
	{
		return true;
	}

	if (Q_fabs(y - a.y) > EQUAL_EPSILON)
	{
		return true;
	}

	if (Q_fabs(z - a.z) > EQUAL_EPSILON)
	{
		return true;
	}

	return false;
}

inline idVec3 idVec3::Cross(const idVec3& a) const
{
	return idVec3(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x);
}

inline idVec3& idVec3::Cross(const idVec3& a, const idVec3& b)
{
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;

	return *this;
}

inline float idVec3::Length() const
{
	float length;

	length = x * x + y * y + z * z;
	return (float)sqrt(length);
}

inline float idVec3::Normalize()
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

inline void idVec3::Zero()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

inline void idVec3::Snap()
{
	x = float(int(x));
	y = float(int(y));
	z = float(int(z));
}

#endif
