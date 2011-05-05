//**************************************************************************
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

static char		com_token[1024];

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
//	QStr::Sprintf
//
//==========================================================================

void QStr::Sprintf(char* Dest, int Size, const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		BigBuffer[32000];	// big, but small enough to fit in PPC stack

	va_start(ArgPtr, Fmt);
	int Len = Q_vsnprintf(BigBuffer, 32000, Fmt, ArgPtr);
	va_end(ArgPtr);
	if (Len >= sizeof(BigBuffer))
	{
		throw QException("QStr::Sprintf: overflowed bigbuffer");
	}
	if (Len >= Size)
	{
		GLog.WriteLine("QStr::Sprintf: overflow of %i in %i", Len, Size);
#if defined _DEBUG && defined _MSC_VER
		__asm
		{
			int 3;
		}
#endif
	}
	NCpyZ(Dest, BigBuffer, Size);
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
//	QStr::SkipPath
//
//==========================================================================

char* QStr::SkipPath(char* PathName)
{
	char* Last = PathName;
	while (*PathName)
	{
		if (*PathName == '/')
		{
			Last = PathName + 1;
		}
		PathName++;
	}
	return Last;
}

//==========================================================================
//
//	QStr::SkipPath
//
//==========================================================================

const char* QStr::SkipPath(const char* PathName)
{
	const char* Last = PathName;
	while (*PathName)
	{
		if (*PathName == '/')
		{
			Last = PathName + 1;
		}
		PathName++;
	}
	return Last;
}

//==========================================================================
//
//	QStr::StripExtension
//
//==========================================================================

void QStr::StripExtension(const char* In, char* Out)
{
	while (*In && *In != '.')
	{
		*Out++ = *In++;
	}
	*Out = 0;
}

//==========================================================================
//
//	QStr::DefaultExtension
//
//==========================================================================

void QStr::DefaultExtension(char* Path, int MaxSize, const char* Extension)
{
	char	OldPath[MAX_QPATH];
	char*	Src;

	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
	Src = Path + Length(Path) - 1;

	while (*Src != '/' && Src != Path)
	{
		if (*Src == '.')
		{
			// it has an extension
			return;
		}
		Src--;
	}

	NCpyZ(OldPath, Path, sizeof(OldPath));
	Sprintf(Path, MaxSize, "%s%s", OldPath, Extension);
}

//==========================================================================
//
//	QStr::FilePath
//
//	Returns the path up to, but not including the last /
//
//==========================================================================

void QStr::FilePath(const char* In, char* Out)
{
	const char* S = In + Length(In) - 1;
	
	while (S != In && *S != '/')
		S--;

	NCpy(Out, In, S - In);
	Out[S - In] = 0;
}

//==========================================================================
//
//	QStr::FileBase
//
//==========================================================================

void QStr::FileBase(const char* In, char* Out)
{
	const char* S2;

	const char* S = In + Length(In) - 1;
	
	while (S != In && *S != '.')
		S--;

	for (S2 = S; S2 != In && *S2 != '/'; S2--)
	;

	if (S - S2 < 2)
	{
		Out[0] = 0;
	}
	else
	{
		S--;
		NCpy(Out, S2 + 1, S - S2);
		Out[S - S2] = 0;
	}
}

//==========================================================================
//
//	QStr::FileExtension
//
//==========================================================================

const char* QStr::FileExtension(const char* In)
{
	static char		Exten[8];

	while (*In && *In != '.')
	{
		In++;
	}
	if (!*In)
	{
		return "";
	}
	In++;
	int i;
	for (i = 0; i < 7 && *In; i++, In++)
	{
		Exten[i] = *In;
	}
	Exten[i] = 0;
	return Exten;
}

//==========================================================================
//
//	QStr::Parse1
//
//	Parse a token out of a string
//	data is an in/out parm, returns a parsed out token
//
//==========================================================================

char* QStr::Parse1(const char **data_p)
{
	int             c;
	int             len;
	const char*		data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*data_p = NULL;
		return com_token;
	}

	// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return com_token;
		}
		data++;
	}

	// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
		{
			data++;
		}
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
			if (len < sizeof(com_token))
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		*data_p = data+1;
		return com_token;
	}

	// parse a regular word
	do
	{
		if (len < sizeof(com_token))
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
		{
			break;
		}
	} while (c > 32);

	if (len == sizeof(com_token))
	{
		len = 0;
	}
	com_token[len] = 0;
	*data_p = data;
	return com_token;
}

//==========================================================================
//
//	QStr::Parse2
//
//	Parse a token out of a string
//
//==========================================================================

char* QStr::Parse2(const char** data_p)
{
	int				c;
	int				len;
	const char*		data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*data_p = NULL;
		return com_token;
	}

	// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return com_token;
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
			if (len < sizeof(com_token))
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < sizeof(com_token))
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c > 32);

	if (len == sizeof(com_token))
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

//==========================================================================
//
//	QStr::Parse3
//
//	Parse a token out of a string
//	Will never return NULL, just empty strings
//
//	If "allowLineBreaks" is qtrue then an empty string will be returned if
// the next token is a newline.
//
//==========================================================================

char* QStr::Parse3(const char** data_p)
{
	return ParseExt(data_p, true);
}

//==========================================================================
//
//	SkipWhitespace
//
//==========================================================================

static const char* SkipWhitespace(const char *data, bool* hasNewLines)
{
	int c;

	while ((c = *data) <= ' ')
	{
		if (!c)
		{
			return NULL;
		}
		if (c == '\n')
		{
			*hasNewLines = true;
		}
		data++;
	}

	return data;
}

//==========================================================================
//
//	QStr::ParseExt
//
//==========================================================================

char* QStr::ParseExt(const char** data_p, bool allowLineBreaks)
{
	int c = 0, len;
	bool hasNewLines = false;
	const char *data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data )
		{
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < sizeof(com_token))
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < sizeof(com_token))
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	if (len == sizeof(com_token))
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

//==========================================================================
//
//	QStr::Compress
//
//==========================================================================

int QStr::Compress(char *data_p)
{
	char *in, *out;
	int c;
	bool newline = false, whitespace = false;

	in = out = data_p;
	if (in)
	{
		while ((c = *in) != 0)
		{
			// skip double slash comments
			if (c == '/' && in[1] == '/')
			{
				while (*in && *in != '\n')
				{
					in++;
				}
			}
			// skip /* */ comments
			else if (c == '/' && in[1] == '*')
			{
				while (*in && ( *in != '*' || in[1] != '/')) 
					in++;
				if (*in) 
					in += 2;
			}
			// record when we hit a newline
			else if (c == '\n' || c == '\r')
			{
				newline = true;
				in++;
			}
			// record when we hit whitespace
			else if (c == ' ' || c == '\t')
			{
				whitespace = true;
				in++;
			}
			// an actual token
			else
			{
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline)
				{
					*out++ = '\n';
					newline = false;
					whitespace = false;
				}
				if (whitespace)
				{
					*out++ = ' ';
					whitespace = false;
				}

				// copy quoted strings unmolested
				if (c == '"')
				{
					*out++ = c;
					in++;
					while (1)
					{
						c = *in;
						if (c && c != '"')
						{
							*out++ = c;
							in++;
						}
						else
						{
							break;
						}
					}
					if (c == '"')
					{
						*out++ = c;
						in++;
					}
				}
				else
				{
					*out = c;
					out++;
					in++;
				}
			}
		}
	}
	*out = 0;
	return out - data_p;
}

//==========================================================================
//
//	QStr::SkipBracedSection
//
//	The next token should be an open brace. Skips until a matching close
// brace is found. Internal brace depths are properly skipped.
//
//==========================================================================

void QStr::SkipBracedSection(const char** program)
{
	const char*		token;
	int				depth;

	depth = 0;
	do
	{
		token = ParseExt(program, true);
		if (token[1] == 0)
		{
			if (token[0] == '{')
			{
				depth++;
			}
			else if (token[0] == '}')
			{
				depth--;
			}
		}
	} while(depth && *program);
}

//==========================================================================
//
//	QStr::SkipRestOfLine
//
//==========================================================================

void QStr::SkipRestOfLine(const char **data)
{
	const char	*p;
	int			c;

	p = *data;
	while ((c = *p++) != 0)
	{
		if (c == '\n')
		{
			break;
		}
	}

	*data = p;
}

/*
============
Com_StringContains
============
*/
static const char *Com_StringContains(const char *str1, const char *str2, bool casesensitive) {
	int len, i, j;

	len = QStr::Length(str1) - QStr::Length(str2);
	for (i = 0; i <= len; i++, str1++) {
		for (j = 0; str2[j]; j++) {
			if (casesensitive) {
				if (str1[j] != str2[j]) {
					break;
				}
			}
			else {
				if (QStr::ToUpper(str1[j]) != QStr::ToUpper(str2[j])) {
					break;
				}
			}
		}
		if (!str2[j]) {
			return str1;
		}
	}
	return NULL;
}

/*
============
Com_Filter
============
*/
bool QStr::Filter(const char *filter, const char *name, bool casesensitive)
{
	char buf[1024];
	const char *ptr;
	int i, found;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			for (i = 0; *filter; i++) {
				if (*filter == '*' || *filter == '?') break;
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if (QStr::Length(buf)) {
				ptr = Com_StringContains(name, buf, casesensitive);
				if (!ptr) return false;
				name = ptr + QStr::Length(buf);
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else if (*filter == '[' && *(filter+1) == '[') {
			filter++;
		}
		else if (*filter == '[') {
			filter++;
			found = false;
			while(*filter && !found) {
				if (*filter == ']' && *(filter+1) != ']') break;
				if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
					if (casesensitive) {
						if (*name >= *filter && *name <= *(filter+2)) found = true;
					}
					else {
						if (QStr::ToUpper(*name) >= QStr::ToUpper(*filter) &&
							QStr::ToUpper(*name) <= QStr::ToUpper(*(filter+2))) found = true;
					}
					filter += 3;
				}
				else {
					if (casesensitive) {
						if (*filter == *name) found = true;
					}
					else {
						if (QStr::ToUpper(*filter) == QStr::ToUpper(*name)) found = true;
					}
					filter++;
				}
			}
			if (!found) return false;
			while(*filter) {
				if (*filter == ']' && *(filter+1) != ']') break;
				filter++;
			}
			filter++;
			name++;
		}
		else {
			if (casesensitive) {
				if (*filter != *name) return false;
			}
			else {
				if (QStr::ToUpper(*filter) != QStr::ToUpper(*name)) return false;
			}
			filter++;
			name++;
		}
	}
	return true;
}

/*
============
Com_FilterPath
============
*/
bool QStr::FilterPath(const char *filter, const char *name, bool casesensitive)
{
	int i;
	char new_filter[MAX_QPATH];
	char new_name[MAX_QPATH];

	for (i = 0; i < MAX_QPATH-1 && filter[i]; i++) {
		if ( filter[i] == '\\' || filter[i] == ':' ) {
			new_filter[i] = '/';
		}
		else {
			new_filter[i] = filter[i];
		}
	}
	new_filter[i] = '\0';
	for (i = 0; i < MAX_QPATH-1 && name[i]; i++) {
		if ( name[i] == '\\' || name[i] == ':' ) {
			new_name[i] = '/';
		}
		else {
			new_name[i] = name[i];
		}
	}
	new_name[i] = '\0';
	return Filter(new_filter, new_name, casesensitive);
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
	Q_vsnprintf(Buf, 32000, Format, ArgPtr);
	va_end(ArgPtr);

	return Buf;  
}
