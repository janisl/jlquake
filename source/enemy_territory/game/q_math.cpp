/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// q_math.c -- stateless support routines that are included in each code module
#include "q_shared.h"


vec4_t colorBlack      =   {0, 0, 0, 1};
vec4_t colorRed        =   {1, 0, 0, 1};
vec4_t colorGreen      =   {0, 1, 0, 1};
vec4_t colorBlue       =   {0, 0, 1, 1};
vec4_t colorYellow     =   {1, 1, 0, 1};
vec4_t colorOrange     =   {1, 0.5, 0, 1};
vec4_t colorMagenta    =   {1, 0, 1, 1};
vec4_t colorCyan       =   {0, 1, 1, 1};
vec4_t colorWhite      =   {1, 1, 1, 1};
vec4_t colorLtGrey     =   {0.75, 0.75, 0.75, 1};
vec4_t colorMdGrey     =   {0.5, 0.5, 0.5, 1};
vec4_t colorDkGrey     =   {0.25, 0.25, 0.25, 1};
vec4_t colorMdRed      =   {0.5, 0, 0, 1};
vec4_t colorMdGreen    =   {0, 0.5, 0, 1};
vec4_t colorDkGreen    =   {0, 0.20, 0, 1};
vec4_t colorMdCyan     =   {0, 0.5, 0.5, 1};
vec4_t colorMdYellow   =   {0.5, 0.5, 0, 1};
vec4_t colorMdOrange   =   {0.5, 0.25, 0, 1};
vec4_t colorMdBlue     =   {0, 0, 0.5, 1};

vec4_t clrBrown =          {0.68f,         0.68f,          0.56f,          1.f};
vec4_t clrBrownDk =        {0.58f * 0.75f, 0.58f * 0.75f,  0.46f * 0.75f,  1.f};
vec4_t clrBrownLine =      {0.0525f,       0.05f,          0.025f,         0.2f};
vec4_t clrBrownLineFull =  {0.0525f,       0.05f,          0.025f,         1.f};

vec4_t clrBrownTextLt2 =   {108 * 1.8 / 255.f,     88 * 1.8 / 255.f,   62 * 1.8 / 255.f,   1.f};
vec4_t clrBrownTextLt =    {108 * 1.3 / 255.f,     88 * 1.3 / 255.f,   62 * 1.3 / 255.f,   1.f};
vec4_t clrBrownText =      {108 / 255.f,         88 / 255.f,       62 / 255.f,       1.f};
vec4_t clrBrownTextDk =    {20 / 255.f,          2 / 255.f,        0 / 255.f,        1.f};
vec4_t clrBrownTextDk2 =   {108 * 0.75 / 255.f,    88 * 0.75 / 255.f,  62 * 0.75 / 255.f,  1.f};

vec4_t g_color_table[32] =
{
	{ 0.0,  0.0,    0.0,    1.0 },      // 0 - black		0
	{ 1.0,  0.0,    0.0,    1.0 },      // 1 - red			1
	{ 0.0,  1.0,    0.0,    1.0 },      // 2 - green		2
	{ 1.0,  1.0,    0.0,    1.0 },      // 3 - yellow		3
	{ 0.0,  0.0,    1.0,    1.0 },      // 4 - blue			4
	{ 0.0,  1.0,    1.0,    1.0 },      // 5 - cyan			5
	{ 1.0,  0.0,    1.0,    1.0 },      // 6 - purple		6
	{ 1.0,  1.0,    1.0,    1.0 },      // 7 - white		7
	{ 1.0,  0.5,    0.0,    1.0 },      // 8 - orange		8
	{ 0.5,  0.5,    0.5,    1.0 },      // 9 - md.grey		9
	{ 0.75, 0.75,   0.75,   1.0 },      // : - lt.grey		10		// lt grey for names
	{ 0.75, 0.75,   0.75,   1.0 },      // ; - lt.grey		11
	{ 0.0,  0.5,    0.0,    1.0 },      // < - md.green		12
	{ 0.5,  0.5,    0.0,    1.0 },      // = - md.yellow	13
	{ 0.0,  0.0,    0.5,    1.0 },      // > - md.blue		14
	{ 0.5,  0.0,    0.0,    1.0 },      // ? - md.red		15
	{ 0.5,  0.25,   0.0,    1.0 },      // @ - md.orange	16
	{ 1.0,  0.6f,   0.1f,   1.0 },      // A - lt.orange	17
	{ 0.0,  0.5,    0.5,    1.0 },      // B - md.cyan		18
	{ 0.5,  0.0,    0.5,    1.0 },      // C - md.purple	19
	{ 0.0,  0.5,    1.0,    1.0 },      // D				20
	{ 0.5,  0.0,    1.0,    1.0 },      // E				21
	{ 0.2f, 0.6f,   0.8f,   1.0 },      // F				22
	{ 0.8f, 1.0,    0.8f,   1.0 },      // G				23
	{ 0.0,  0.4,    0.2f,   1.0 },      // H				24
	{ 1.0,  0.0,    0.2f,   1.0 },      // I				25
	{ 0.7f, 0.1f,   0.1f,   1.0 },      // J				26
	{ 0.6f, 0.2f,   0.0,    1.0 },      // K				27
	{ 0.8f, 0.6f,   0.2f,   1.0 },      // L				28
	{ 0.6f, 0.6f,   0.2f,   1.0 },      // M				29
	{ 1.0,  1.0,    0.75,   1.0 },      // N				30
	{ 1.0,  1.0,    0.5,    1.0 },      // O				31
};

//==============================================================

int     Q_rand( int *seed ) {
	*seed = ( 69069 * *seed + 1 );
	return *seed;
}

float   Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

float   Q_crandom( int *seed ) {
	return 2.0 * ( Q_random( seed ) - 0.5 );
}


//=======================================================

unsigned ColorBytes3( float r, float g, float b ) {
	unsigned i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;

	return i;
}

unsigned ColorBytes4( float r, float g, float b, float a ) {
	unsigned i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float max;

	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}


void vectoangles( const vec3_t value1, vec3_t angles ) {
	float forward;
	float yaw, pitch;

	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		} else {
			pitch = 270;
		}
	} else {
		if ( value1[0] ) {
			yaw = ( atan2( value1[1], value1[0] ) * 180 / M_PI );
		} else if ( value1[1] > 0 )   {
			yaw = 90;
		} else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch = ( atan2( value1[2], forward ) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

//============================================================================

#if id386 && !( ( defined __linux__ || defined __FreeBSD__ || defined __GNUC__ ) && ( defined __i386__ ) ) // rb010123
long myftol( float f ) {
	static int tmp;
	__asm fld f
	__asm fistp tmp
	__asm mov eax, tmp
}
#endif

//============================================================

/*
================
ProjectPointOntoVector
================
*/
void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj ) {
	vec3_t pVec, vec;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
}

/*
================
ProjectPointOntoVectorBounded
================
*/
void ProjectPointOntoVectorBounded( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj ) {
	vec3_t pVec, vec;
	int j;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
	// check bounds
	for ( j = 0; j < 3; j++ )
		if ( ( vProj[j] > vStart[j] && vProj[j] > vEnd[j] ) ||
			 ( vProj[j] < vStart[j] && vProj[j] < vEnd[j] ) ) {
			break;
		}
	if ( j < 3 ) {
		if ( Q_fabs( vProj[j] - vStart[j] ) < Q_fabs( vProj[j] - vEnd[j] ) ) {
			VectorCopy( vStart, vProj );
		} else {
			VectorCopy( vEnd, vProj );
		}
	}
}

/*
================
DistanceFromLineSquared
================
*/
float DistanceFromLineSquared( vec3_t p, vec3_t lp1, vec3_t lp2 ) {
	vec3_t proj, t;
	int j;

	ProjectPointOntoVector( p, lp1, lp2, proj );
	for ( j = 0; j < 3; j++ )
		if ( ( proj[j] > lp1[j] && proj[j] > lp2[j] ) ||
			 ( proj[j] < lp1[j] && proj[j] < lp2[j] ) ) {
			break;
		}
	if ( j < 3 ) {
		if ( Q_fabs( proj[j] - lp1[j] ) < Q_fabs( proj[j] - lp2[j] ) ) {
			VectorSubtract( p, lp1, t );
		} else {
			VectorSubtract( p, lp2, t );
		}
		return VectorLengthSquared( t );
	}
	VectorSubtract( p, proj, t );
	return VectorLengthSquared( t );
}

/*
================
DistanceFromVectorSquared
================
*/
float DistanceFromVectorSquared( vec3_t p, vec3_t lp1, vec3_t lp2 ) {
	vec3_t proj, t;

	ProjectPointOntoVector( p, lp1, lp2, proj );
	VectorSubtract( p, proj, t );
	return VectorLengthSquared( t );
}

float vectoyaw( const vec3_t vec ) {
	float yaw;

	if ( vec[YAW] == 0 && vec[PITCH] == 0 ) {
		yaw = 0;
	} else {
		if ( vec[PITCH] ) {
			yaw = ( atan2( vec[YAW], vec[PITCH] ) * 180 / M_PI );
		} else if ( vec[YAW] > 0 ) {
			yaw = 90;
		} else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}
	}

	return yaw;
}

/*
=================
AxisToAngles

  Used to convert the MD3 tag axis to MDC tag angles, which are much smaller

  This doesn't have to be fast, since it's only used for conversion in utils, try to avoid
  using this during gameplay
=================
*/
void AxisToAngles( vec3_t axis[3], vec3_t angles ) {
	vec3_t right, roll_angles, tvec;

	// first get the pitch and yaw from the forward vector
	vectoangles( axis[0], angles );

	// now get the roll from the right vector
	VectorCopy( axis[1], right );
	// get the angle difference between the tmpAxis[2] and axis[2] after they have been reverse-rotated
	RotatePointAroundVector( tvec, axisDefault[2], right, -angles[YAW] );
	RotatePointAroundVector( right, axisDefault[1], tvec, -angles[PITCH] );
	// now find the angles, the PITCH is effectively our ROLL
	vectoangles( right, roll_angles );
	roll_angles[PITCH] = AngleNormalize180( roll_angles[PITCH] );
	// if the yaw is more than 90 degrees difference, we should adjust the pitch
	if ( DotProduct( right, axisDefault[1] ) < 0 ) {
		if ( roll_angles[PITCH] < 0 ) {
			roll_angles[PITCH] = -90 + ( -90 - roll_angles[PITCH] );
		} else {
			roll_angles[PITCH] =  90 + ( 90 - roll_angles[PITCH] );
		}
	}

	angles[ROLL] = -roll_angles[PITCH];
}

float VectorDistance( vec3_t v1, vec3_t v2 ) {
	vec3_t dir;

	VectorSubtract( v2, v1, dir );
	return VectorLength( dir );
}

float VectorDistanceSquared( vec3_t v1, vec3_t v2 ) {
	vec3_t dir;

	VectorSubtract( v2, v1, dir );
	return VectorLengthSquared( dir );
}
// done.
