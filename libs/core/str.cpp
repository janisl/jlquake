//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	Dynamic string class.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"

// MACROS ------------------------------------------------------------------

#if !defined _WIN32
#define stricmp		strcasecmp
#define strnicmp	strncasecmp
#endif

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QStr::QStr
//
//==========================================================================

QStr::QStr(const QStr& InStr, int Start, int Len)
: Str(NULL)
{
	qassert(Start >= 0);
	qassert(Start <= (int)InStr.Length());
	qassert(Len >= 0);
	qassert(Start + Len <= (int)InStr.Length());
	if (Len)
	{
		Resize(Len);
		NCpy(Str, InStr.Str + Start, Len);
	}
}

//==========================================================================
//
//	QStr::Resize
//
//==========================================================================

void QStr::Resize(int NewLen)
{
	qassert(NewLen >= 0);
	if (NewLen == Length())
	{
		//	Same length, use existing buffer.
		return;
	}
	if (!NewLen)
	{
		//	Free string.
		if (Str)
		{
			delete[] (Str - sizeof(int));
			Str = NULL;
		}
	}
	else
	{
		//	Allocate memory.
		int AllocLen = sizeof(int) + NewLen + 1;
		char* NewStr = (new char[AllocLen]) + sizeof(int);
		if (Str)
		{
			int Len = Min(Length(), NewLen);
			NCpy(NewStr, Str, Len);
			delete[] (Str - sizeof(int));
		}
		Str = NewStr;
		//	Set length.
		((int*)Str)[-1] = NewLen;
		//	Set terminator.
		Str[NewLen] = 0;
	}
}

//==========================================================================
//
//	QStr::StartsWith
//
//==========================================================================

bool QStr::StartsWith(const char* S) const
{
	int l = Length(S);
	if (l > Length())
	{
		return false;
	}
	return NCmp(**this, S, l) == 0;
}

//==========================================================================
//
//	QStr::StartsWith
//
//==========================================================================

bool QStr::StartsWith(const QStr& S) const
{
	int l = S.Length();
	if (l > Length())
	{
		return false;
	}
	return NCmp(**this, *S, l) == 0;
}

//==========================================================================
//
//	QStr::EndsWith
//
//==========================================================================

bool QStr::EndsWith(const char* S) const
{
	int l = Length(S);
	if (l > Length())
	{
		return false;
	}
	return NCmp(**this + Length() - l, S, l) == 0;
}

//==========================================================================
//
//	QStr::EndsWith
//
//==========================================================================

bool QStr::EndsWith(const QStr& S) const
{
	int l = S.Length();
	if (l > Length())
	{
		return false;
	}
	return NCmp(**this + Length() - l, *S, l) == 0;
}

//==========================================================================
//
//	QStr::ToLower
//
//==========================================================================

QStr QStr::ToLower() const
{
	if (!Str)
	{
		return QStr();
	}
	QStr Ret;
	int l = Length();
	Ret.Resize(l);
	for (int i = 0; i < l; i++)
	{
		Ret.Str[i] = ToLower(Str[i]);
	}
	return Ret;
}

//==========================================================================
//
//	QStr::ToUpper
//
//==========================================================================

QStr QStr::ToUpper() const
{
	if (!Str)
	{
		return QStr();
	}
	QStr Ret;
	int l = Length();
	Ret.Resize(l);
	for (int i = 0; i < l; i++)
	{
		Ret.Str[i] = ToUpper(Str[i]);
	}
	return Ret;
}

//==========================================================================
//
//	QStr::IndexOf
//
//==========================================================================

int QStr::IndexOf(char C) const
{
	int l = Length();
	for (int i = 0; i < l; i++)
	{
		if (Str[i] == C)
		{
			return i;
		}
	}
	return -1;
}

//==========================================================================
//
//	QStr::IndexOf
//
//==========================================================================

int QStr::IndexOf(const char* S) const
{
	int sl = Length(S);
	if (!sl)
	{
		return -1;
	}
	int l = Length();
	for (int i = 0; i <= l - sl; i++)
	{
		if (NCmp(Str + i, S, sl) == 0)
		{
			return i;
		}
	}
	return -1;
}

//==========================================================================
//
//	QStr::IndexOf
//
//==========================================================================

int QStr::IndexOf(const QStr& S) const
{
	int sl = S.Length();
	if (!sl)
	{
		return -1;
	}
	int l = Length();
	for (int i = 0; i <= l - sl; i++)
	{
		if (NCmp(Str + i, *S, sl) == 0)
		{
			return i;
		}
	}
	return -1;
}

//==========================================================================
//
//	QStr::LastIndexOf
//
//==========================================================================

int QStr::LastIndexOf(char C) const
{
	for (int i = Length() - 1; i >= 0; i--)
	{
		if (Str[i] == C)
		{
			return i;
		}
	}
	return -1;
}

//==========================================================================
//
//	QStr::LastIndexOf
//
//==========================================================================

int QStr::LastIndexOf(const char* S) const
{
	int sl = Length(S);
	if (!sl)
	{
		return -1;
	}
	int l = Length();
	for (int i = l - sl; i >= 0; i--)
	{
		if (NCmp(Str + i, S, sl) == 0)
		{
			return i;
		}
	}
	return -1;
}

//==========================================================================
//
//	QStr::LastIndexOf
//
//==========================================================================

int QStr::LastIndexOf(const QStr& S) const
{
	int sl = S.Length();
	if (!sl)
	{
		return -1;
	}
	int l = Length();
	for (int i = l - sl; i >= 0; i--)
	{
		if (NCmp(Str + i, *S, sl) == 0)
		{
			return i;
		}
	}
	return -1;
}

//==========================================================================
//
//	QStr::Replace
//
//==========================================================================

QStr QStr::Replace(const char* Search, const char* Replacement) const
{
	if (!Length())
	{
		//	Nothing to replace in an empty string.
		return *this;
	}

	int SLen = Length(Search);
	int RLen = Length(Replacement);
	if (!SLen)
	{
		//	Nothing to search for.
		return *this;
	}

	QStr Ret = *this;
	int i = 0;
	while (i <= Ret.Length() - SLen)
	{
		if (!NCmp(Ret.Str + i, Search, SLen))
		{
			//	If search and replace strings are of the same size, we can
			// just copy the data and avoid memory allocations.
			if (SLen == RLen)
			{
				Com_Memcpy(Ret.Str + i, Replacement, RLen);
			}
			else
			{
				Ret = QStr(Ret, 0, i) + Replacement +
					QStr(Ret, i + SLen, Ret.Length() - i - SLen);
			}
			i += RLen;
		}
		else
		{
			i++;
		}
	}
	return Ret;
}

//==========================================================================
//
//	QStr::Replace
//
//==========================================================================

QStr QStr::Replace(const QStr& Search, const QStr& Replacement) const
{
	if (!Length())
	{
		//	Nothing to replace in an empty string.
		return *this;
	}

	int SLen = Search.Length();
	int RLen = Replacement.Length();
	if (!SLen)
	{
		//	Nothing to search for.
		return *this;
	}

	QStr Ret = *this;
	int i = 0;
	while (i <= Ret.Length() - SLen)
	{
		if (!NCmp(Ret.Str + i, *Search, SLen))
		{
			//	If search and replace strings are of the same size, we can
			// just copy the data and avoid memory allocations.
			if (SLen == RLen)
			{
				Com_Memcpy(Ret.Str + i, *Replacement, RLen);
			}
			else
			{
				Ret = QStr(Ret, 0, i) + Replacement +
					QStr(Ret, i + SLen, Ret.Length() - i - SLen);
			}
			i += RLen;
		}
		else
		{
			i++;
		}
	}
	return Ret;
}

//==========================================================================
//
//	QStr::Utf8Substring
//
//==========================================================================

QStr QStr::Utf8Substring(int Start, int Len) const
{
	qassert(Start >= 0);
	qassert(Start <= (int)Utf8Length());
	qassert(Len >= 0);
	qassert(Start + Len <= (int)Utf8Length());
	if (!Len)
	{
		return QStr();
	}
	int RealStart = ByteLengthForUtf8(Str, Start);
	int RealLen = ByteLengthForUtf8(Str, Start + Len) - RealStart;
	return QStr(*this, RealStart, RealLen);
}

//==========================================================================
//
//	QStr::Split
//
//==========================================================================

void QStr::Split(char C, QArray<QStr>& A) const
{
	A.Clear();
	if (!Str)
	{
		return;
	}
	int Start = 0;
	int Len = Length();
	for (int i = 0; i <= Len; i++)
	{
		if (i == Len || Str[i] == C)
		{
			if (Start != i)
			{
				A.Append(QStr(*this, Start, i - Start));
			}
			Start = i + 1;
		}
	}
}

//==========================================================================
//
//	QStr::Split
//
//==========================================================================

void QStr::Split(const char* Chars, QArray<QStr>& A) const
{
	A.Clear();
	if (!Str)
	{
		return;
	}
	int Start = 0;
	int Len = Length();
	for (int i = 0; i <= Len; i++)
	{
		bool DoSplit = i == Len;
		for (const char* pChar = Chars; !DoSplit && *pChar; pChar++)
		{
			DoSplit = Str[i] == *pChar;
		}
		if (DoSplit)
		{
			if (Start != i)
			{
				A.Append(QStr(*this, Start, i - Start));
			}
			Start = i + 1;
		}
	}
}

//==========================================================================
//
//	QStr::IsValidUtf8
//
//==========================================================================

bool QStr::IsValidUtf8() const
{
	if (!Str)
	{
		return true;
	}
	for (const char* c = Str; *c;)
	{
		if ((*c & 0x80) == 0)
		{
			c++;
		}
		else if ((*c & 0xe0) == 0xc0)
		{
			if ((c[1] & 0xc0) != 0x80)
			{
				return false;
			}
			c += 2;
		}
		else if ((*c & 0xf0) == 0xe0)
		{
			if ((c[1] & 0xc0) != 0x80 || (c[2] & 0xc0) != 0x80)
			{
				return false;
			}
			c += 3;
		}
		else if ((*c & 0xf8) == 0xf0)
		{
			if ((c[1] & 0xc0) != 0x80 || (c[2] & 0xc0) != 0x80 ||
				(c[3] & 0xc0) != 0x80)
			{
				return false;
			}
			c += 4;
		}
		else
		{
			return false;
		}
	}
	return true;
}

//==========================================================================
//
//	QStr::Latin1ToUtf8
//
//==========================================================================

QStr QStr::Latin1ToUtf8() const
{
	QStr Ret;
	for (int i = 0; i < Length(); i++)
	{
		Ret += FromChar((quint8)Str[i]);
	}
	return Ret;
}

//==========================================================================
//
//	QStr::Utf8Length
//
//==========================================================================

int QStr::Utf8Length(const char* S)
{
	int Count = 0;
	for (const char* c = S; *c; c++)
		if ((*c & 0xc0) != 0x80)
			Count++;
	return Count;
}

//==========================================================================
//
//	QStr::ByteLengthForUtf8
//
//==========================================================================

int QStr::ByteLengthForUtf8(const char* S, size_t N)
{
	int Count = 0;
	const char* c;
	for (c = S; *c; c++)
	{
		if ((*c & 0xc0) != 0x80)
		{
			if (Count == N)
			{
				return c - S;
			}
			Count++;
		}
	}
	qassert(N == Count);
	return c - S;
}

//==========================================================================
//
//	QStr::GetChar
//
//==========================================================================

int QStr::GetChar(const char*& S)
{
	if ((quint8)*S < 128)
	{
		return *S++;
	}
	int Cnt;
	int Val;
	if ((*S & 0xe0) == 0xc0)
	{
		Val = *S & 0x1f;
		Cnt = 1;
	}
	else if ((*S & 0xf0) == 0xe0)
	{
		Val = *S & 0x0f;
		Cnt = 2;
	}
	else if ((*S & 0xf8) == 0xf0)
	{
		Val = *S & 0x07;
		Cnt = 3;
	}
	else
	{
		throw QException("Not a valid UTF-8");
	}
	S++;

	do
	{
		if ((*S & 0xc0) != 0x80)
		{
			throw QException("Not a valid UTF-8");
		}
		Val = (Val << 6) | (*S & 0x3f);
		S++;
	}
	while (--Cnt);
	return Val;
}

//==========================================================================
//
//	QStr::FromChar
//
//==========================================================================

QStr QStr::FromChar(int C)
{
	char Ret[8];
	if (C < 0x80)
	{
		Ret[0] = C;
		Ret[1] = 0;
	}
	else if (C < 0x800)
	{
		Ret[0] = 0xc0 | (C & 0x1f);
		Ret[1] = 0x80 | ((C >> 5) & 0x3f);
		Ret[2] = 0;
	}
	else if (C < 0x10000)
	{
		Ret[0] = 0xe0 | (C & 0x0f);
		Ret[1] = 0x80 | ((C >> 4) & 0x3f);
		Ret[2] = 0x80 | ((C >> 10) & 0x3f);
		Ret[3] = 0;
	}
	else
	{
		Ret[0] = 0xf0 | (C & 0x07);
		Ret[1] = 0x80 | ((C >> 3) & 0x3f);
		Ret[2] = 0x80 | ((C >> 9) & 0x3f);
		Ret[3] = 0x80 | ((C >> 15) & 0x3f);
		Ret[4] = 0;
	}
	return Ret;
}

//==========================================================================
//
//	QStr::Length
//
//==========================================================================

int QStr::Length(const char* S)
{
#if 1
	return (int)strlen(S);
#else
	//	Quake implementation.
	int             count;
	
	count = 0;
	while (str[count])
		count++;

	return count;
#endif
}

//==========================================================================
//
//	QStr::Cmp
//
//==========================================================================

int QStr::Cmp(const char* S1, const char* S2)
{
#if 1
	return strcmp(S1, S2);
#else
	//	Quake implementation.
	while (1)
	{
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}
	
	return -1;
#endif
}

//==========================================================================
//
//	QStr::NCmp
//
//==========================================================================

int QStr::NCmp(const char* S1, const char* S2, size_t N)
{
#if 1
	return strncmp(S1, S2, N);
#elif 1
	//	Quake 3 implementation.
	int		c1, c2;
	do
	{
		c1 = *s1++;
		c2 = *s2++;
		if (!n--)
		{
			return 0;		// strings are equal until end point
		}
		if (c1 != c2)
		{
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);
	return 0;		// strings are equal
#else
	//	Quake implementation.
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;              // strings not equal    
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}
	
	return -1;
#endif
}

//==========================================================================
//
//	QStr::ICmp
//
//==========================================================================

int QStr::ICmp(const char* S1, const char* S2)
{
#if 1
	return stricmp(S1, S2);
#else
	//	Quake implementation.
	return NICmp(s1, s2, 99999);
#endif
}

//==========================================================================
//
//	QStr::NICmp
//
//==========================================================================

int QStr::NICmp(const char* S1, const char* S2, size_t N)
{
#if 1
	return strnicmp(S1, S2, N);
#elif 0
	//	Quake 3 implementation.
	int		c1, c2;

	// bk001129 - moved in 1.17 fix not in id codebase
        if ( s1 == NULL ) {
           if ( s2 == NULL )
             return 0;
           else
             return -1;
        }
        else if ( s2==NULL )
          return 1;


	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);
	
	return 0;		// strings are equal
#elif 0
	//	Quake 2 implementation.
	int		c1, c2;
	
	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point
		
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;		// strings not equal
		}
	} while (c1);
	
	return 0;		// strings are equal
#else
	//	Quake implementation.
	int             c1, c2;
	
	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;               // strings are equal until end point
		
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;              // strings not equal
		}
		if (!c1)
			return 0;               // strings are equal
//              s1++;
//              s2++;
	}
	
	return -1;
#endif
}

//==========================================================================
//
//	QStr::Cpy
//
//==========================================================================

void QStr::Cpy(char* Dst, const char* Src)
{
#if 1
	strcpy(Dst, Src);
#else
	//	Quake implementation.
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
#endif
}

//==========================================================================
//
//	QStr::NCpy
//
//==========================================================================

void QStr::NCpy(char* Dst, const char* Src, size_t N)
{
#if 1
	strncpy(Dst, Src, N);
#else
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
#endif
}

//==========================================================================
//
//	QStr::NCpy
//
//	Safe strncpy that ensures a trailing zero
//
//==========================================================================

void QStr::NCpyZ(char* Dest, const char* Src, int DestSize)
{
	// bk001129 - also NULL dest
	if (!Dest)
	{
		throw QException("Q_strncpyz: NULL dest");
	}
	if (!Src)
	{
		throw QException("Q_strncpyz: NULL src");
	}
	if (DestSize < 1)
	{
		throw QException("Q_strncpyz: destsize < 1"); 
	}

	NCpy(Dest, Src, DestSize - 1);
	Dest[DestSize - 1] = 0;
}

//==========================================================================
//
//	QStr::ToLower
//
//	Never goes past bounds or leaves without a terminating 0
//
//==========================================================================

void QStr::Cat(char* Dest, int Size, const char* Src)
{
	int L1 = Length(Dest);
	if (L1 >= Size)
	{
		throw QException("Q_strcat: already overflowed");
	}
	NCpyZ(Dest + L1, Src, Size - L1);
}

//==========================================================================
//
//	QStr::ToLower
//
//==========================================================================

char* QStr::ToLower(char* S1)
{
	char* S = S1;
	while (*S)
	{
		*S = ToLower(*S);
		S++;
	}
    return S1;
}

//==========================================================================
//
//	QStr::ToUpper
//
//==========================================================================

char* QStr::ToUpper(char* S1)
{
	char* S = S1;
	while (*S)
	{
		*S = ToUpper(*S);
		S++;
	}
    return S1;
}

//==========================================================================
//
//	QStr::RChr
//
//==========================================================================

char* QStr::RChr(const char* String, int C)
{
	char cc = C;
	char *s;
	char *sp = NULL;

	s = (char*)String;

	while (*s)
	{
		if (*s == cc)
		{
			sp = s;
		}
		s++;
	}
	if (cc == 0)
	{
		sp = s;
	}

	return sp;
}

//==========================================================================
//
//	QStr::Atoi
//
//==========================================================================

int QStr::Atoi(const char* Str)
{
	int			Sign;
	if (*Str == '-')
	{
		Sign = -1;
		Str++;
	}
	else
	{
		Sign = 1;
	}

	int Val = 0;

	//
	// check for hex
	//
	if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
	{
		Str += 2;
		while (1)
		{
			int C = *Str++;
			if (C >= '0' && C <= '9')
			{
				Val = (Val << 4) + C - '0';
			}
			else if (C >= 'a' && C <= 'f')
			{
				Val = (Val << 4) + C - 'a' + 10;
			}
			else if (C >= 'A' && C <= 'F')
			{
				Val = (Val << 4) + C - 'A' + 10;
			}
			else
			{
				return Val * Sign;
			}
		}
	}

	//
	// check for character
	//
	if (Str[0] == '\'')
	{
		return Sign * Str[1];
	}

	//
	// assume decimal
	//
	while (1)
	{
		int C = *Str++;
		if (C <'0' || C > '9')
		{
			return Val * Sign;
		}
		Val = Val * 10 + C - '0';
	}

	return 0;
}

//==========================================================================
//
//	QStr::Atof
//
//==========================================================================

float QStr::Atof(const char* Str)
{
	int				Sign;
	if (*Str == '-')
	{
		Sign = -1;
		Str++;
	}
	else
	{
		Sign = 1;
	}

	double Val = 0;

	//
	// check for hex
	//
	if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
	{
		Str += 2;
		while (1)
		{
			int C = *Str++;
			if (C >= '0' && C <= '9')
			{
				Val = (Val * 16) + C - '0';
			}
			else if (C >= 'a' && C <= 'f')
			{
				Val = (Val * 16) + C - 'a' + 10;
			}
			else if (C >= 'A' && C <= 'F')
			{
				Val = (Val * 16) + C - 'A' + 10;
			}
			else
			{
				return Val * Sign;
			}
		}
	}

	//
	// check for character
	//
	if (Str[0] == '\'')
	{
		return Sign * Str[1];
	}

	//
	// assume decimal
	//
	int Decimal = -1;
	int Total = 0;
	while (1)
	{
		int C = *Str++;
		if (C == '.')
		{
			Decimal = Total;
			continue;
		}
		if (C < '0' || C > '9')
		{
			break;
		}
		Val = Val * 10 + C - '0';
		Total++;
	}

	if (Decimal == -1)
	{
		return Val * Sign;
	}
	while (Total > Decimal)
	{
		Val /= 10;
		Total--;
	}
	
	return Val * Sign;
}

//==========================================================================
//
//	QStr::IsPrint
//
//==========================================================================

int QStr::IsPrint(int C)
{
	if (C >= 0x20 && C <= 0x7E)
	{
		return 1;
	}
	return 0;
}

//==========================================================================
//
//	QStr::IsLower
//
//==========================================================================

int QStr::IsLower(int C)
{
	if (C >= 'a' && C <= 'z')
	{
		return 1;
	}
	return 0;
}

//==========================================================================
//
//	QStr::IsUpper
//
//==========================================================================

int QStr::IsUpper(int C)
{
	if (C >= 'A' && C <= 'Z')
	{
		return 1;
	}
	return 0;
}

//==========================================================================
//
//	QStr::IsAlpha
//
//==========================================================================

int QStr::IsAlpha(int C)
{
	if ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z'))
	{
		return 1;
	}
	return 0;
}

//==========================================================================
//
//	QStr::ToUpper
//
//==========================================================================

int QStr::IsSpace(int C)
{
	if (C == ' ' || C == '\t' || C == '\n' || C =='\r' || C == '\f')
	{
		return 1;
	}
	return 0;
}

//==========================================================================
//
//	QStr::IsDigit
//
//==========================================================================

int QStr::IsDigit(int C)
{
	if (C >= '0' && C <= '9')
	{
		return 1;
	}
	return 0;
}

//==========================================================================
//
//	QStr::ToUpper
//
//==========================================================================

char QStr::ToUpper(char C)
{
	if (C >= 'a' && C <= 'z')
	{
		return C - ('a' - 'A');
	}
	return C;
}

//==========================================================================
//
//	QStr::ToLower
//
//==========================================================================

char QStr::ToLower(char C)
{
	if (C >= 'A' && C <= 'Z')
	{
		return C + ('a' - 'A');
	}
	return C;
}

//==========================================================================
//
//	va
//
//	Does a varargs printf into a temp buffer, so I don't need to have
// varargs versions of all text functions.
//	FIXME: make this buffer size safe someday
//
//==========================================================================

char* va(const char* Format, ...)
{
	va_list			ArgPtr;
	static char		String[2][32000];	// in case va is called by nested functions
	static int		Index = 0;

	char* Buf = String[Index & 1];
	Index++;

	va_start(ArgPtr, Format);
	vsprintf(Buf, Format, ArgPtr);
	va_end(ArgPtr);

	return Buf;  
}
