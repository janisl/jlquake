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

#include "qcommon.h"

//	Should be const
vec3_t vec3_origin = {0, 0, 0};

const vec3_t axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

float bytedirs[NUMVERTEXNORMALS][3] =
{
	{-0.525731, 0.000000, 0.850651},
	{-0.442863, 0.238856, 0.864188},
	{-0.295242, 0.000000, 0.955423},
	{-0.309017, 0.500000, 0.809017},
	{-0.162460, 0.262866, 0.951056},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.850651, 0.525731},
	{-0.147621, 0.716567, 0.681718},
	{0.147621, 0.716567, 0.681718},
	{0.000000, 0.525731, 0.850651},
	{0.309017, 0.500000, 0.809017},
	{0.525731, 0.000000, 0.850651},
	{0.295242, 0.000000, 0.955423},
	{0.442863, 0.238856, 0.864188},
	{0.162460, 0.262866, 0.951056},
	{-0.681718, 0.147621, 0.716567},
	{-0.809017, 0.309017, 0.500000},
	{-0.587785, 0.425325, 0.688191},
	{-0.850651, 0.525731, 0.000000},
	{-0.864188, 0.442863, 0.238856},
	{-0.716567, 0.681718, 0.147621},
	{-0.688191, 0.587785, 0.425325},
	{-0.500000, 0.809017, 0.309017},
	{-0.238856, 0.864188, 0.442863},
	{-0.425325, 0.688191, 0.587785},
	{-0.716567, 0.681718, -0.147621},
	{-0.500000, 0.809017, -0.309017},
	{-0.525731, 0.850651, 0.000000},
	{0.000000, 0.850651, -0.525731},
	{-0.238856, 0.864188, -0.442863},
	{0.000000, 0.955423, -0.295242},
	{-0.262866, 0.951056, -0.162460},
	{0.000000, 1.000000, 0.000000},
	{0.000000, 0.955423, 0.295242},
	{-0.262866, 0.951056, 0.162460},
	{0.238856, 0.864188, 0.442863},
	{0.262866, 0.951056, 0.162460},
	{0.500000, 0.809017, 0.309017},
	{0.238856, 0.864188, -0.442863},
	{0.262866, 0.951056, -0.162460},
	{0.500000, 0.809017, -0.309017},
	{0.850651, 0.525731, 0.000000},
	{0.716567, 0.681718, 0.147621},
	{0.716567, 0.681718, -0.147621},
	{0.525731, 0.850651, 0.000000},
	{0.425325, 0.688191, 0.587785},
	{0.864188, 0.442863, 0.238856},
	{0.688191, 0.587785, 0.425325},
	{0.809017, 0.309017, 0.500000},
	{0.681718, 0.147621, 0.716567},
	{0.587785, 0.425325, 0.688191},
	{0.955423, 0.295242, 0.000000},
	{1.000000, 0.000000, 0.000000},
	{0.951056, 0.162460, 0.262866},
	{0.850651, -0.525731, 0.000000},
	{0.955423, -0.295242, 0.000000},
	{0.864188, -0.442863, 0.238856},
	{0.951056, -0.162460, 0.262866},
	{0.809017, -0.309017, 0.500000},
	{0.681718, -0.147621, 0.716567},
	{0.850651, 0.000000, 0.525731},
	{0.864188, 0.442863, -0.238856},
	{0.809017, 0.309017, -0.500000},
	{0.951056, 0.162460, -0.262866},
	{0.525731, 0.000000, -0.850651},
	{0.681718, 0.147621, -0.716567},
	{0.681718, -0.147621, -0.716567},
	{0.850651, 0.000000, -0.525731},
	{0.809017, -0.309017, -0.500000},
	{0.864188, -0.442863, -0.238856},
	{0.951056, -0.162460, -0.262866},
	{0.147621, 0.716567, -0.681718},
	{0.309017, 0.500000, -0.809017},
	{0.425325, 0.688191, -0.587785},
	{0.442863, 0.238856, -0.864188},
	{0.587785, 0.425325, -0.688191},
	{0.688191, 0.587785, -0.425325},
	{-0.147621, 0.716567, -0.681718},
	{-0.309017, 0.500000, -0.809017},
	{0.000000, 0.525731, -0.850651},
	{-0.525731, 0.000000, -0.850651},
	{-0.442863, 0.238856, -0.864188},
	{-0.295242, 0.000000, -0.955423},
	{-0.162460, 0.262866, -0.951056},
	{0.000000, 0.000000, -1.000000},
	{0.295242, 0.000000, -0.955423},
	{0.162460, 0.262866, -0.951056},
	{-0.442863, -0.238856, -0.864188},
	{-0.309017, -0.500000, -0.809017},
	{-0.162460, -0.262866, -0.951056},
	{0.000000, -0.850651, -0.525731},
	{-0.147621, -0.716567, -0.681718},
	{0.147621, -0.716567, -0.681718},
	{0.000000, -0.525731, -0.850651},
	{0.309017, -0.500000, -0.809017},
	{0.442863, -0.238856, -0.864188},
	{0.162460, -0.262866, -0.951056},
	{0.238856, -0.864188, -0.442863},
	{0.500000, -0.809017, -0.309017},
	{0.425325, -0.688191, -0.587785},
	{0.716567, -0.681718, -0.147621},
	{0.688191, -0.587785, -0.425325},
	{0.587785, -0.425325, -0.688191},
	{0.000000, -0.955423, -0.295242},
	{0.000000, -1.000000, 0.000000},
	{0.262866, -0.951056, -0.162460},
	{0.000000, -0.850651, 0.525731},
	{0.000000, -0.955423, 0.295242},
	{0.238856, -0.864188, 0.442863},
	{0.262866, -0.951056, 0.162460},
	{0.500000, -0.809017, 0.309017},
	{0.716567, -0.681718, 0.147621},
	{0.525731, -0.850651, 0.000000},
	{-0.238856, -0.864188, -0.442863},
	{-0.500000, -0.809017, -0.309017},
	{-0.262866, -0.951056, -0.162460},
	{-0.850651, -0.525731, 0.000000},
	{-0.716567, -0.681718, -0.147621},
	{-0.716567, -0.681718, 0.147621},
	{-0.525731, -0.850651, 0.000000},
	{-0.500000, -0.809017, 0.309017},
	{-0.238856, -0.864188, 0.442863},
	{-0.262866, -0.951056, 0.162460},
	{-0.864188, -0.442863, 0.238856},
	{-0.809017, -0.309017, 0.500000},
	{-0.688191, -0.587785, 0.425325},
	{-0.681718, -0.147621, 0.716567},
	{-0.442863, -0.238856, 0.864188},
	{-0.587785, -0.425325, 0.688191},
	{-0.309017, -0.500000, 0.809017},
	{-0.147621, -0.716567, 0.681718},
	{-0.425325, -0.688191, 0.587785},
	{-0.162460, -0.262866, 0.951056},
	{0.442863, -0.238856, 0.864188},
	{0.162460, -0.262866, 0.951056},
	{0.309017, -0.500000, 0.809017},
	{0.147621, -0.716567, 0.681718},
	{0.000000, -0.525731, 0.850651},
	{0.425325, -0.688191, 0.587785},
	{0.587785, -0.425325, 0.688191},
	{0.688191, -0.587785, 0.425325},
	{-0.955423, 0.295242, 0.000000},
	{-0.951056, 0.162460, 0.262866},
	{-1.000000, 0.000000, 0.000000},
	{-0.850651, 0.000000, 0.525731},
	{-0.955423, -0.295242, 0.000000},
	{-0.951056, -0.162460, 0.262866},
	{-0.864188, 0.442863, -0.238856},
	{-0.951056, 0.162460, -0.262866},
	{-0.809017, 0.309017, -0.500000},
	{-0.864188, -0.442863, -0.238856},
	{-0.951056, -0.162460, -0.262866},
	{-0.809017, -0.309017, -0.500000},
	{-0.681718, 0.147621, -0.716567},
	{-0.681718, -0.147621, -0.716567},
	{-0.850651, 0.000000, -0.525731},
	{-0.688191, 0.587785, -0.425325},
	{-0.587785, 0.425325, -0.688191},
	{-0.425325, 0.688191, -0.587785},
	{-0.425325, -0.688191, -0.587785},
	{-0.587785, -0.425325, -0.688191},
	{-0.688191, -0.587785, -0.425325},
};

#if !idppc

float Q_rsqrt(float number)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = *(long*)&y;						// evil floating point bit level hacking
	i  = 0x5f3759df - (i >> 1);				// what the fuck?
	y  = *(float*)&i;
	y  = y * (threehalfs - (x2 * y * y));	// 1st iteration
//	y  = y * (threehalfs - (x2 * y * y));	// 2nd iteration, this can be removed

#ifdef __linux__
//	assert(!isnan(y)); // bk010122 - FPE?
#endif
	return y;
}

float Q_fabs(float f)
{
	int tmp = *(int*)&f;
	tmp &= 0x7FFFFFFF;
	return *(float*)&tmp;
}

#endif

int Q_log2(int val)
{
	int answer = 0;
	while ((val >>= 1) != 0)
	{
		answer++;
	}
	return answer;
}

//	the msvc acos doesn't always return a value between -PI and PI:
//
//	int i;
//	i = 1065353246;
//	acos(*(float*) &i) == -1.#IND0
float Q_acos(float c)
{
	float angle;

	angle = acos(c);

	if (angle > M_PI)
	{
		return (float)M_PI;
	}
	if (angle < -M_PI)
	{
		return (float)M_PI;
	}
	return angle;
}

#if id386 && defined _MSC_VER
#pragma warning (disable:4035)
__declspec(naked) long Q_ftol(float f)
{
	static int tmp;
	__asm fld dword ptr [esp + 4]
	__asm fistp tmp
	__asm mov eax, tmp
	__asm ret
}
#pragma warning (default:4035)
#endif

qint8 ClampChar(int i)
{
	if (i < MIN_QINT8)
	{
		return MIN_QINT8;
	}
	if (i > MAX_QINT8)
	{
		return MAX_QINT8;
	}
	return i;
}

qint16 ClampShort(int i)
{
	if (i < MIN_QINT16)
	{
		return MIN_QINT16;
	}
	if (i > MAX_QINT16)
	{
		return MAX_QINT16;
	}
	return i;
}

void _VectorMA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

vec_t _DotProduct(const vec3_t v1, const vec3_t v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

void _VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

void _VectorCopy(const vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale(const vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

vec_t VectorNormalize(vec3_t v)
{
	// NOTE: TTimo - Apple G4 altivec source uses double?
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt(length);

	if (length)
	{
		ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

vec_t VectorNormalize2(const vec3_t v, vec3_t out)
{
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt(length);

	if (length)
	{
//		assert( ((Q_fabs(v[0])!=0.0f) || (Q_fabs(v[1])!=0.0f) || (Q_fabs(v[2])!=0.0f)) );
		ilength = 1 / length;
		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	}
	else
	{
//		assert( ((Q_fabs(v[0])==0.0f) && (Q_fabs(v[1])==0.0f) && (Q_fabs(v[2])==0.0f)) );
		VectorClear(out);
	}

	return length;
}

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom =  DotProduct(normal, normal);
	if (Q_fabs(inv_denom) == 0.0f)
	{
		// bk010122 - zero vectors get here
		throw Exception("Zero vector");
	}
	inv_denom = 1.0f / inv_denom;

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

//	assumes "src" is normalized
void PerpendicularVector(vec3_t dst, const vec3_t src)
{
	int pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabs(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	VectorNormalize(dst);
}

//	This is not implemented very well...
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point,
	float degrees)
{
	float m[3][3];
	float im[3][3];
	float zrot[3][3];
	float tmpmat[3][3];
	float rot[3][3];
	int i;
	vec3_t vr, vup, vf;
	float rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	CrossProduct(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	Com_Memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	Com_Memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD(degrees);
	zrot[0][0] = cos(rad);
	zrot[0][1] = sin(rad);
	zrot[1][0] = -sin(rad);
	zrot[1][1] = cos(rad);

	MatrixMultiply(m, zrot, tmpmat);
	MatrixMultiply(tmpmat, im, rot);

	for (i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

void VectorRotate(const vec3_t in, const vec3_t matrix[3], vec3_t out)
{
	out[0] = DotProduct(in, matrix[0]);
	out[1] = DotProduct(in, matrix[1]);
	out[2] = DotProduct(in, matrix[2]);
}

void Vector4Scale(const vec4_t in, vec_t scale, vec4_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3] * scale;
}

void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs)
{
	if (v[0] < mins[0])
	{
		mins[0] = v[0];
	}
	if (v[0] > maxs[0])
	{
		maxs[0] = v[0];
	}

	if (v[1] < mins[1])
	{
		mins[1] = v[1];
	}
	if (v[1] > maxs[1])
	{
		maxs[1] = v[1];
	}

	if (v[2] < mins[2])
	{
		mins[2] = v[2];
	}
	if (v[2] > maxs[2])
	{
		maxs[2] = v[2];
	}
}

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs)
{
	vec3_t corner;

	for (int i = 0; i < 3; i++)
	{
		float a = fabs(mins[i]);
		float b = fabs(maxs[i]);
		corner[i] = a > b ? a : b;
	}

	return VectorLength(corner);
}

void MatrixMultiply(const float in1[3][3], const float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}

//	returns angle normalized to the range [0 <= angle < 360]
float AngleNormalize360(float angle)
{
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}

//	returns angle normalized to the range [-180 < angle <= 180]
float AngleNormalize180(float angle)
{
	angle = AngleNormalize360(angle);
	if (angle > 180.0)
	{
		angle -= 360.0;
	}
	return angle;
}

float LerpAngle(float from, float to, float frac)
{
	float a;

	if (to - from > 180)
	{
		to -= 360;
	}
	if (to - from < -180)
	{
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}

//	Always returns a value from -180 to 180
float AngleSubtract(float a1, float a2)
{
	float a = a1 - a2;
	while (a > 180)
	{
		a -= 360;
	}
	while (a < -180)
	{
		a += 360;
	}
	return a;
}

void AnglesSubtract(const vec3_t v1, const vec3_t v2, vec3_t v3)
{
	v3[0] = AngleSubtract(v1[0], v2[0]);
	v3[1] = AngleSubtract(v1[1], v2[1]);
	v3[2] = AngleSubtract(v1[2], v2[2]);
}

//	returns the normalized delta from angle1 to angle2
float AngleDelta(float angle1, float angle2)
{
	return AngleNormalize180(angle1 - angle2);
}

void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float angle;
	static float sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI * 2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI * 2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI * 2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
		right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
		right[2] = -1 * sr * cp;
	}
	if (up)
	{
		up[0] = (cr * sp * cy + -sr * -sy);
		up[1] = (cr * sp * sy + -sr * cy);
		up[2] = cr * cp;
	}
}

void SetPlaneSignbits(cplane_t* out)
{
	// for fast box on planeside test
	int bits = 0;
	for (int j = 0; j < 3; j++)
	{
		if (out->normal[j] < 0)
		{
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}

#if id386 && defined _MSC_VER
#pragma warning( disable: 4035 )
__declspec(naked) int BoxOnPlaneSide(const vec3_t emins, const vec3_t emaxs, const cplane_t* p)
{
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {

		push ebx

		cmp bops_initialized, 1
		je initialized
		mov bops_initialized, 1

		mov Ljmptab[0 * 4], offset Lcase0
		mov Ljmptab[1 * 4], offset Lcase1
		mov Ljmptab[2 * 4], offset Lcase2
		mov Ljmptab[3 * 4], offset Lcase3
		mov Ljmptab[4 * 4], offset Lcase4
		mov Ljmptab[5 * 4], offset Lcase5
		mov Ljmptab[6 * 4], offset Lcase6
		mov Ljmptab[7 * 4], offset Lcase7

initialized:

		mov edx,dword ptr[4 + 12 + esp]
		mov ecx,dword ptr[4 + 4 + esp]
		xor eax,eax
		mov ebx,dword ptr[4 + 8 + esp]
		mov al,byte ptr[17 + edx]
		cmp al,8
		jge Lerror
		fld dword ptr[0 + edx]
		fld st(0)
		jmp dword ptr[Ljmptab + eax * 4]
		Lcase0
			: fmul dword ptr[ebx]
			fld dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul dword ptr[ecx]
			fxch st(2)
			fld st(0)
			fmul dword ptr[4 + ebx]
			fld dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul dword ptr[4 + ecx]
			fxch st(2)
			fld st(0)
			fmul dword ptr[8 + ebx]
			fxch st(5)
			faddp st(3),st(0)
			fmul dword ptr[8 + ecx]
			fxch st(1)
			faddp st(3),st(0)
			fxch st(3)
			faddp st(2),st(0)
			jmp LSetSides
			Lcase1
				: fmul dword ptr[ecx]
				fld dword ptr[0 + 4 + edx]
				fxch st(2)
				fmul dword ptr[ebx]
				fxch st(2)
				fld st(0)
				fmul dword ptr[4 + ebx]
				fld dword ptr[0 + 8 + edx]
				fxch st(2)
				fmul dword ptr[4 + ecx]
				fxch st(2)
				fld st(0)
				fmul dword ptr[8 + ebx]
				fxch st(5)
				faddp st(3),st(0)
				fmul dword ptr[8 + ecx]
				fxch st(1)
				faddp st(3),st(0)
				fxch st(3)
				faddp st(2),st(0)
				jmp LSetSides
				Lcase2
					: fmul dword ptr[ebx]
					fld dword ptr[0 + 4 + edx]
					fxch st(2)
					fmul dword ptr[ecx]
					fxch st(2)
					fld st(0)
					fmul dword ptr[4 + ecx]
					fld dword ptr[0 + 8 + edx]
					fxch st(2)
					fmul dword ptr[4 + ebx]
					fxch st(2)
					fld st(0)
					fmul dword ptr[8 + ebx]
					fxch st(5)
					faddp st(3),st(0)
					fmul dword ptr[8 + ecx]
					fxch st(1)
					faddp st(3),st(0)
					fxch st(3)
					faddp st(2),st(0)
					jmp LSetSides
					Lcase3
						: fmul dword ptr[ecx]
						fld dword ptr[0 + 4 + edx]
						fxch st(2)
						fmul dword ptr[ebx]
						fxch st(2)
						fld st(0)
						fmul dword ptr[4 + ecx]
						fld dword ptr[0 + 8 + edx]
						fxch st(2)
						fmul dword ptr[4 + ebx]
						fxch st(2)
						fld st(0)
						fmul dword ptr[8 + ebx]
						fxch st(5)
						faddp st(3),st(0)
						fmul dword ptr[8 + ecx]
						fxch st(1)
						faddp st(3),st(0)
						fxch st(3)
						faddp st(2),st(0)
						jmp LSetSides
						Lcase4
							: fmul dword ptr[ebx]
							fld dword ptr[0 + 4 + edx]
							fxch st(2)
							fmul dword ptr[ecx]
							fxch st(2)
							fld st(0)
							fmul dword ptr[4 + ebx]
							fld dword ptr[0 + 8 + edx]
							fxch st(2)
							fmul dword ptr[4 + ecx]
							fxch st(2)
							fld st(0)
							fmul dword ptr[8 + ecx]
							fxch st(5)
							faddp st(3),st(0)
							fmul dword ptr[8 + ebx]
							fxch st(1)
							faddp st(3),st(0)
							fxch st(3)
							faddp st(2),st(0)
							jmp LSetSides
							Lcase5
								: fmul dword ptr[ecx]
								fld dword ptr[0 + 4 + edx]
								fxch st(2)
								fmul dword ptr[ebx]
								fxch st(2)
								fld st(0)
								fmul dword ptr[4 + ebx]
								fld dword ptr[0 + 8 + edx]
								fxch st(2)
								fmul dword ptr[4 + ecx]
								fxch st(2)
								fld st(0)
								fmul dword ptr[8 + ecx]
								fxch st(5)
								faddp st(3),st(0)
								fmul dword ptr[8 + ebx]
								fxch st(1)
								faddp st(3),st(0)
								fxch st(3)
								faddp st(2),st(0)
								jmp LSetSides
								Lcase6
									: fmul dword ptr[ebx]
									fld dword ptr[0 + 4 + edx]
									fxch st(2)
									fmul dword ptr[ecx]
									fxch st(2)
									fld st(0)
									fmul dword ptr[4 + ecx]
									fld dword ptr[0 + 8 + edx]
									fxch st(2)
									fmul dword ptr[4 + ebx]
									fxch st(2)
									fld st(0)
									fmul dword ptr[8 + ecx]
									fxch st(5)
									faddp st(3),st(0)
									fmul dword ptr[8 + ebx]
									fxch st(1)
									faddp st(3),st(0)
									fxch st(3)
									faddp st(2),st(0)
									jmp LSetSides
									Lcase7
										: fmul dword ptr[ecx]
										fld dword ptr[0 + 4 + edx]
										fxch st(2)
										fmul dword ptr[ebx]
										fxch st(2)
										fld st(0)
										fmul dword ptr[4 + ecx]
										fld dword ptr[0 + 8 + edx]
										fxch st(2)
										fmul dword ptr[4 + ebx]
										fxch st(2)
										fld st(0)
										fmul dword ptr[8 + ecx]
										fxch st(5)
										faddp st(3),st(0)
										fmul dword ptr[8 + ebx]
										fxch st(1)
										faddp st(3),st(0)
										fxch st(3)
										faddp st(2),st(0)
										LSetSides
											: faddp st(2),st(0)
											fcomp dword ptr[12 + edx]
											xor ecx,ecx
											fnstsw ax
											fcomp dword ptr[12 + edx]
											and ah,1
											xor ah,1
											add cl,ah
											fnstsw ax
											and ah,1
											add ah,ah
											add cl,ah
											pop ebx
											mov eax,ecx
											ret
											Lerror
												: int 3
	}
}
#pragma warning( default: 4035 )
#elif !id386
int BoxOnPlaneSide(const vec3_t emins, const vec3_t emaxs, const cplane_t* p)
{
	float dist1, dist2;
	int sides;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
		{
			return 1;
		}
		if (p->dist >= emaxs[p->type])
		{
			return 2;
		}
		return 3;
	}

	// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
	{
		sides = 1;
	}
	if (dist2 < p->dist)
	{
		sides |= 2;
	}

	return sides;
}
#endif

//	Returns false if the triangle is degenrate.
//	The normal will point out of the clock for clockwise ordered points
bool PlaneFromPoints(vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c)
{
	vec3_t d1, d2;

	VectorSubtract(b, a, d1);
	VectorSubtract(c, a, d2);
	CrossProduct(d2, d1, plane);
	if (VectorNormalize(plane) == 0)
	{
		return false;
	}

	plane[3] = DotProduct(a, plane);
	return true;
}

static float VecToYawNotAlongZ(const vec3_t vector)
{
	if (vector[0])
	{
		float yaw = atan2(vector[1], vector[0]) * 180 / M_PI;
		if (yaw < 0)
		{
			yaw += 360;
		}
		return yaw;
	}
	else if (vector[1] > 0)
	{
		return 90;
	}
	else
	{
		return 270;
	}
}

float VecToYaw(const vec3_t vector)
{
	if (vector[1] == 0 && vector[0] == 0)
	{
		return 0;
	}
	else
	{
		return VecToYawNotAlongZ(vector);
	}
}

static float VecToPitchNotAlongZ(const vec3_t vector)
{
	float forward = sqrt(vector[0] * vector[0] + vector[1] * vector[1]);
	float pitch = (atan2(vector[2], forward) * 180 / M_PI);
	if (pitch < 0)
	{
		pitch += 360;
	}
	return pitch;
}

void VecToAngles(const vec3_t vector, vec3_t angles)
{
	float yaw;
	float pitch;
	if (vector[1] == 0 && vector[0] == 0)
	{
		yaw = 0;
		pitch = vector[2] > 0 ? 90 : 270;
	}
	else
	{
		yaw = VecToYawNotAlongZ(vector);
		pitch = VecToPitchNotAlongZ(vector);
	}
	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

void VecToAnglesBuggy(const vec3_t vector, vec3_t angles)
{
	VecToAngles(vector, angles);
	angles[PITCH] = -angles[PITCH];
}

void AnglesToAxis(const vec3_t angles, vec3_t axis[3])
{
	vec3_t right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors(angles, axis[0], right, axis[2]);
	VectorSubtract(vec3_origin, right, axis[1]);
}

void AxisClear(vec3_t axis[3])
{
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy(const vec3_t in[3], vec3_t out[3])
{
	VectorCopy(in[0], out[0]);
	VectorCopy(in[1], out[1]);
	VectorCopy(in[2], out[2]);
}

void RotateAroundDirection(vec3_t axis[3], float yaw)
{
	// create an arbitrary axis[1]
	PerpendicularVector(axis[1], axis[0]);

	// rotate it around axis[0] by yaw
	if (yaw)
	{
		vec3_t temp;

		VectorCopy(axis[1], temp);
		RotatePointAroundVector(axis[1], axis[0], temp, yaw);
	}

	// cross to get axis[2]
	CrossProduct(axis[0], axis[1], axis[2]);
}

//	Given a normalized forward vector, create two
//	other perpendicular vectors
void MakeNormalVectors(const vec3_t forward, vec3_t right, vec3_t up)
{
	float d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}

void RotatePoint(vec3_t point, const vec3_t matrix[3])
{
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

void TransposeMatrix(const vec3_t matrix[3], vec3_t transpose[3])
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			transpose[i][j] = matrix[j][i];
		}
	}
}

// this isn't a real cheap function to call!
int DirToByte(vec3_t dir)
{
	if (!dir)
	{
		return 0;
	}

	float bestd = 0;
	int best = 0;
	for (int i = 0; i < NUMVERTEXNORMALS; i++)
	{
		float d = DotProduct(dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir(int b, vec3_t dir)
{
	if (b < 0 || b >= NUMVERTEXNORMALS)
	{
		if (GGameType & GAME_Quake2)
		{
			throw DropException("MSF_ReadDir: out of range");
		}
		VectorCopy(vec3_origin, dir);
		return;
	}
	VectorCopy(bytedirs[b], dir);
}
