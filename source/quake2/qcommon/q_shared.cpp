/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "q_shared.h"

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

//============================================================================


#if defined _M_IX86 && !defined C_ONLY
#pragma warning (disable:4035)
__declspec( naked ) long Q_ftol( float f )
{
	static int tmp;
	__asm fld dword ptr [esp+4]
	__asm fistp tmp
	__asm mov eax, tmp
	__asm ret
}
#pragma warning (default:4035)
#endif

/*
===============
LerpAngle

===============
*/
float LerpAngle (float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}


	int		i;
	vec3_t	corners[2];


// this is the slow, general version
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
#if !id386 || defined __linux__ 
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;

// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}
	
// general case
	switch (p->signbits)
	{
	case 0:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		assert( 0 );
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	assert( sides != 0 );

	return sides;
}
#else
#pragma warning( disable: 4035 )

__declspec( naked ) int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {

		push ebx
			
		cmp bops_initialized, 1
		je  initialized
		mov bops_initialized, 1
		
		mov Ljmptab[0*4], offset Lcase0
		mov Ljmptab[1*4], offset Lcase1
		mov Ljmptab[2*4], offset Lcase2
		mov Ljmptab[3*4], offset Lcase3
		mov Ljmptab[4*4], offset Lcase4
		mov Ljmptab[5*4], offset Lcase5
		mov Ljmptab[6*4], offset Lcase6
		mov Ljmptab[7*4], offset Lcase7
			
initialized:

		mov edx,ds:dword ptr[4+12+esp]
		mov ecx,ds:dword ptr[4+4+esp]
		xor eax,eax
		mov ebx,ds:dword ptr[4+8+esp]
		mov al,ds:byte ptr[17+edx]
		cmp al,8
		jge Lerror
		fld ds:dword ptr[0+edx]
		fld st(0)
		jmp dword ptr[Ljmptab+eax*4]
Lcase0:
		fmul ds:dword ptr[ebx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ebx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase1:
		fmul ds:dword ptr[ecx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ebx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase2:
		fmul ds:dword ptr[ebx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ecx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase3:
		fmul ds:dword ptr[ecx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ecx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase4:
		fmul ds:dword ptr[ebx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ebx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase5:
		fmul ds:dword ptr[ecx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ebx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase6:
		fmul ds:dword ptr[ebx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ecx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase7:
		fmul ds:dword ptr[ecx]
		fld ds:dword ptr[0+4+edx]
		fxch st(2)
		fmul ds:dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[4+ecx]
		fld ds:dword ptr[0+8+edx]
		fxch st(2)
		fmul ds:dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul ds:dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul ds:dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
LSetSides:
		faddp st(2),st(0)
		fcomp ds:dword ptr[12+edx]
		xor ecx,ecx
		fnstsw ax
		fcomp ds:dword ptr[12+edx]
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
Lerror:
		int 3
	}
}
#pragma warning( default: 4035 )
#endif

void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}


//====================================================================================

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char	*last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension (char *in, char *out)
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}

/*
============
COM_FileExtension
============
*/
char *COM_FileExtension (char *in)
{
	static char exten[8];
	int		i;

	while (*in && *in != '.')
		in++;
	if (!*in)
		return "";
	in++;
	for (i=0 ; i<7 && *in ; i++,in++)
		exten[i] = *in;
	exten[i] = 0;
	return exten;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase (char *in, char *out)
{
	char *s, *s2;
	
	s = in + QStr::Length(in) - 1;
	
	while (s != in && *s != '.')
		s--;
	
	for (s2 = s ; s2 != in && *s2 != '/' ; s2--)
	;
	
	if (s-s2 < 2)
		out[0] = 0;
	else
	{
		s--;
		QStr::NCpy(out,s2+1, s-s2);
		out[s-s2] = 0;
	}
}

/*
============
COM_FilePath

Returns the path up to, but not including the last /
============
*/
void COM_FilePath (char *in, char *out)
{
	char *s;
	
	s = in + QStr::Length(in) - 1;
	
	while (s != in && *s != '/')
		s--;

	QStr::NCpy(out,in, s-in);
	out[s-in] = 0;
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + QStr::Length(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	QStr::Cat(path, MAX_OSPATH, extension);
}

char	com_token[MAX_TOKEN_CHARS];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char **data_p)
{
	int		c;
	int		len;
	char	*data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;
	
	if (!data)
	{
		*data_p = NULL;
		return "";
	}
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return "";
		}
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}


/*
===============
Com_PageInMemory

===============
*/
int	paged_total;

void Com_PageInMemory (byte *buffer, int size)
{
	int		i;

	for (i=size-1 ; i>0 ; i-=4096)
		paged_total += buffer[i];
}



/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

void Com_sprintf (char *dest, int size, char *fmt, ...)
{
	int		len;
	va_list		argptr;
	char	bigbuffer[0x10000];

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);
	if (len >= size)
		Com_Printf ("Com_sprintf: overflow of %i in %i\n", len, size);
	QStr::NCpy(dest, bigbuffer, size-1);
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
char *Info_ValueForKey (char *s, char *key)
{
	char	pkey[512];
	static	char value[2][512];	// use two buffers so compares
								// work without stomping on each other
	static	int	valueindex;
	char	*o;
	
	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if (!QStr::Cmp(key, pkey) )
			return value[valueindex];

		if (!*s)
			return "";
		s++;
	}
}

void Info_RemoveKey (char *s, char *key)
{
	char	*start;
	char	pkey[512];
	char	value[512];
	char	*o;

	if (strstr (key, "\\"))
	{
//		Com_Printf ("Can't use a key with a \\\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!QStr::Cmp(key, pkey) )
		{
			QStr::Cpy(start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate (char *s)
{
	if (strstr (s, "\""))
		return false;
	if (strstr (s, ";"))
		return false;
	return true;
}

void Info_SetValueForKey (char *s, char *key, char *value)
{
	char	newi[MAX_INFO_STRING], *v;
	int		c;
	int		maxsize = MAX_INFO_STRING;

	if (strstr (key, "\\") || strstr (value, "\\") )
	{
		Com_Printf ("Can't use keys or values with a \\\n");
		return;
	}

	if (strstr (key, ";") )
	{
		Com_Printf ("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"") )
	{
		Com_Printf ("Can't use keys or values with a \"\n");
		return;
	}

	if (QStr::Length(key) > MAX_INFO_KEY-1 || QStr::Length(value) > MAX_INFO_KEY-1)
	{
		Com_Printf ("Keys and values must be < 64 characters.\n");
		return;
	}
	Info_RemoveKey (s, key);
	if (!value || !QStr::Length(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (QStr::Length(newi) + QStr::Length(s) > maxsize)
	{
		Com_Printf ("Info string length exceeded\n");
		return;
	}

	// only copy ascii values
	s += QStr::Length(s);
	v = newi;
	while (*v)
	{
		c = *v++;
		c &= 127;		// strip high bits
		if (c >= 32 && c < 127)
			*s++ = c;
	}
	*s = 0;
}

//====================================================================


