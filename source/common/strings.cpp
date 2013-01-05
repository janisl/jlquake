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
//**
//**	Dynamic string class.
//**
//**************************************************************************

#include "strings.h"
#include "Common.h"
#include "common_defs.h"

#if !defined _WIN32
#define stricmp     strcasecmp
#define strnicmp    strncasecmp
#endif

static char com_token[1024];

String::String(const String& InStr, int Start, int Len)
	: Str(NULL)
{
	assert(Start >= 0);
	assert(Start <= (int)InStr.Length());
	assert(Len >= 0);
	assert(Start + Len <= (int)InStr.Length());
	if (Len)
	{
		Resize(Len);
		NCpy(Str, InStr.Str + Start, Len);
	}
}

void String::Resize(int NewLen)
{
	assert(NewLen >= 0);
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

bool String::StartsWith(const char* S) const
{
	int l = Length(S);
	if (l > Length())
	{
		return false;
	}
	return NCmp(**this, S, l) == 0;
}

bool String::StartsWith(const String& S) const
{
	int l = S.Length();
	if (l > Length())
	{
		return false;
	}
	return NCmp(**this, *S, l) == 0;
}

bool String::EndsWith(const char* S) const
{
	int l = Length(S);
	if (l > Length())
	{
		return false;
	}
	return NCmp(**this + Length() - l, S, l) == 0;
}

bool String::EndsWith(const String& S) const
{
	int l = S.Length();
	if (l > Length())
	{
		return false;
	}
	return NCmp(**this + Length() - l, *S, l) == 0;
}

String String::ToLower() const
{
	if (!Str)
	{
		return String();
	}
	String Ret;
	int l = Length();
	Ret.Resize(l);
	for (int i = 0; i < l; i++)
	{
		Ret.Str[i] = ToLower(Str[i]);
	}
	return Ret;
}

String String::ToUpper() const
{
	if (!Str)
	{
		return String();
	}
	String Ret;
	int l = Length();
	Ret.Resize(l);
	for (int i = 0; i < l; i++)
	{
		Ret.Str[i] = ToUpper(Str[i]);
	}
	return Ret;
}

int String::IndexOf(char C) const
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

int String::IndexOf(const char* S) const
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

int String::IndexOf(const String& S) const
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

int String::LastIndexOf(char C) const
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

int String::LastIndexOf(const char* S) const
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

int String::LastIndexOf(const String& S) const
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

String String::Replace(const char* Search, const char* Replacement) const
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

	String Ret = *this;
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
				Ret = String(Ret, 0, i) + Replacement +
					  String(Ret, i + SLen, Ret.Length() - i - SLen);
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

String String::Replace(const String& Search, const String& Replacement) const
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

	String Ret = *this;
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
				Ret = String(Ret, 0, i) + Replacement +
					  String(Ret, i + SLen, Ret.Length() - i - SLen);
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

String String::Utf8Substring(int Start, int Len) const
{
	assert(Start >= 0);
	assert(Start <= (int)Utf8Length());
	assert(Len >= 0);
	assert(Start + Len <= (int)Utf8Length());
	if (!Len)
	{
		return String();
	}
	int RealStart = ByteLengthForUtf8(Str, Start);
	int RealLen = ByteLengthForUtf8(Str, Start + Len) - RealStart;
	return String(*this, RealStart, RealLen);
}

void String::Split(char C, Array<String>& A) const
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
				A.Append(String(*this, Start, i - Start));
			}
			Start = i + 1;
		}
	}
}

void String::Split(const char* Chars, Array<String>& A) const
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
				A.Append(String(*this, Start, i - Start));
			}
			Start = i + 1;
		}
	}
}

bool String::IsValidUtf8() const
{
	if (!Str)
	{
		return true;
	}
	for (const char* c = Str; *c; )
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

String String::Latin1ToUtf8() const
{
	String Ret;
	for (int i = 0; i < Length(); i++)
	{
		Ret += FromChar((quint8)Str[i]);
	}
	return Ret;
}

int String::Utf8Length(const char* S)
{
	int Count = 0;
	for (const char* c = S; *c; c++)
		if ((*c & 0xc0) != 0x80)
		{
			Count++;
		}
	return Count;
}

int String::ByteLengthForUtf8(const char* S, int N)
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
	assert(N == Count);
	return c - S;
}

int String::GetChar(const char*& S)
{
	if ((quint8) * S < 128)
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
		common->FatalError("Not a valid UTF-8");
	}
	S++;

	do
	{
		if ((*S & 0xc0) != 0x80)
		{
			common->FatalError("Not a valid UTF-8");
		}
		Val = (Val << 6) | (*S & 0x3f);
		S++;
	}
	while (--Cnt);
	return Val;
}

String String::FromChar(int C)
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

int String::Length(const char* str)
{
#if 1
	return (int)strlen(str);
#else
	//	Quake implementation.
	int count = 0;
	while (str[count])
	{
		count++;
	}
	return count;
#endif
}

int String::Cmp(const char* s1, const char* s2)
{
#if 1
	return strcmp(s1, s2);
#else
	//	Quake implementation.
	while (1)
	{
		if (*s1 != *s2)
		{
			return -1;				// strings not equal
		}
		if (!*s1)
		{
			return 0;				// strings are equal
		}
		s1++;
		s2++;
	}
	return -1;
#endif
}

int String::NCmp(const char* s1, const char* s2, size_t n)
{
#if 1
	return strncmp(s1, s2, n);
#elif 1
	//	Quake 3 implementation.
	int c1, c2;
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
	}
	while (c1);
	return 0;		// strings are equal
#else
	//	Quake implementation.
	while (1)
	{
		if (!count--)
		{
			return 0;
		}
		if (*s1 != *s2)
		{
			return -1;				// strings not equal
		}
		if (!*s1)
		{
			return 0;				// strings are equal
		}
		s1++;
		s2++;
	}
	return -1;
#endif
}

int String::ICmp(const char* s1, const char* s2)
{
#if 1
	return stricmp(s1, s2);
#elif 0
	//	Quake 3 version
	return (s1 && s2) ? NICmp(s1, s2, 99999) : -1;
#else
	//	Quake implementation.
	return NICmp(s1, s2, 99999);
#endif
}

int String::NICmp(const char* s1, const char* s2, size_t n)
{
#if 1
	return strnicmp(s1, s2, n);
#elif 0
	//	Quake 3 implementation.
	int c1, c2;

	// bk001129 - moved in 1.17 fix not in id codebase
	if (s1 == NULL)
	{
		if (s2 == NULL)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else if (s2 == NULL)
	{
		return 1;
	}

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
			if (c1 >= 'a' && c1 <= 'z')
			{
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z')
			{
				c2 -= ('a' - 'A');
			}
			if (c1 != c2)
			{
				return c1 < c2 ? -1 : 1;
			}
		}
	}
	while (c1);
	return 0;		// strings are equal
#elif 0
	//	Quake 2 implementation.
	int c1, c2;
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
			if (c1 >= 'a' && c1 <= 'z')
			{
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z')
			{
				c2 -= ('a' - 'A');
			}
			if (c1 != c2)
			{
				return -1;		// strings not equal
			}
		}
	}
	while (c1);
	return 0;		// strings are equal
#else
	//	Quake implementation.
	int c1, c2;
	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
		{
			return 0;				// strings are equal until end point
		}

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
			{
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z')
			{
				c2 -= ('a' - 'A');
			}
			if (c1 != c2)
			{
				return -1;				// strings not equal
			}
		}
		if (!c1)
		{
			return 0;				// strings are equal
		}
	}
	return -1;
#endif
}

void String::Cpy(char* dst, const char* src)
{
#if 1
	strcpy(dst, src);
#else
	//	Quake implementation.
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
#endif
}

void String::NCpy(char* dst, const char* src, size_t count)
{
#if 1
	strncpy(dst, src, count);
#else
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
	{
		*dest++ = 0;
	}
#endif
}

//	Safe strncpy that ensures a trailing zero
void String::NCpyZ(char* dest, const char* src, int destSize)
{
	// bk001129 - also NULL dest
	if (!dest)
	{
		common->FatalError("Q_strncpyz: NULL dest");
	}
	if (!src)
	{
		common->FatalError("Q_strncpyz: NULL src");
	}
	if (destSize < 1)
	{
		common->FatalError("Q_strncpyz: destsize < 1");
	}

	NCpy(dest, src, destSize - 1);
	dest[destSize - 1] = 0;
}

//	Never goes past bounds or leaves without a terminating 0
void String::Cat(char* dest, int size, const char* src)
{
	int l1 = Length(dest);
	if (l1 >= size)
	{
		common->FatalError("Q_strcat: already overflowed");
	}
	NCpyZ(dest + l1, src, size - l1);
}

char* String::ToLower(char* s1)
{
	char* S = s1;
	while (*S)
	{
		*S = ToLower(*S);
		S++;
	}
	return s1;
}

char* String::ToUpper(char* s1)
{
	char* S = s1;
	while (*S)
	{
		*S = ToUpper(*S);
		S++;
	}
	return s1;
}

char* String::RChr(const char* string, int c)
{
	char cc = c;
	char* s;
	char* sp = NULL;

	s = (char*)string;

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

int String::Atoi(const char* Str)
{
	int Sign;
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
		if (C < '0' || C > '9')
		{
			return Val * Sign;
		}
		Val = Val * 10 + C - '0';
	}

	return 0;
}

float String::Atof(const char* Str)
{
	int Sign;
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

void String::Sprintf(char* Dest, int Size, const char* Fmt, ...)
{
#if 1
	va_list ArgPtr;
	char BigBuffer[32000];			// big, but small enough to fit in PPC stack

	va_start(ArgPtr, Fmt);
	int Len = Q_vsnprintf(BigBuffer, 32000, Fmt, ArgPtr);
	va_end(ArgPtr);
	if (Len >= (int)sizeof(BigBuffer))
	{
		common->FatalError("String::Sprintf: overflowed bigbuffer");
	}
	if (Len >= Size)
	{
		common->Printf("String::Sprintf: overflow of %i in %i\n", Len, Size);
#if defined _DEBUG && defined _MSC_VER
		__debugbreak();
#endif
	}
	NCpyZ(Dest, BigBuffer, Size);
#else
	//	Wolf multiplayer / Enemy territory version.
	/*
	C99 for vsnprintf:
	return the number of characters  (excluding  the  trailing  '\0')
	which would have been written to the final string if enough space had been available.
	*/
	va_list argptr;
	va_start(argptr,fmt);
	int len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end(argptr);

	if (len >= size)
	{
		common->Printf("String::Sprintf: overflow of %i in %i\n", len, size);
	}
#endif
}

int String::IsPrint(int c)
{
	if (c >= 0x20 && c <= 0x7E)
	{
		return 1;
	}
	return 0;
}

int String::IsLower(int c)
{
	if (c >= 'a' && c <= 'z')
	{
		return 1;
	}
	return 0;
}

int String::IsUpper(int c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return 1;
	}
	return 0;
}

int String::IsAlpha(int c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
	{
		return 1;
	}
	return 0;
}

int String::IsSpace(int c)
{
	if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f')
	{
		return 1;
	}
	return 0;
}

int String::IsDigit(int c)
{
	if (c >= '0' && c <= '9')
	{
		return 1;
	}
	return 0;
}

char String::ToUpper(char c)
{
	if (c >= 'a' && c <= 'z')
	{
		return c - ('a' - 'A');
	}
	return c;
}

char String::ToLower(char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return c + ('a' - 'A');
	}
	return c;
}

char* String::SkipPath(char* PathName)
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

const char* String::SkipPath(const char* PathName)
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

void String::StripExtension(const char* In, char* Out)
{
	while (*In && *In != '.')
	{
		*Out++ = *In++;
	}
	*Out = 0;
}

//	a safer version
void String::StripExtension2(const char* in, char* out, int destsize)
{
	int len = 0;
	while (len < destsize - 1 && *in && *in != '.')
	{
		*out++ = *in++;
		len++;
	}
	*out = 0;
}

void String::DefaultExtension(char* Path, int MaxSize, const char* Extension)
{
	char OldPath[MAX_QPATH];
	char* Src;

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

//	Returns the path up to, but not including the last /
void String::FilePath(const char* In, char* Out)
{
	const char* S = In + Length(In) - 1;

	while (S != In && *S != '/')
		S--;

	NCpy(Out, In, S - In);
	Out[S - In] = 0;
}

void String::FileBase(const char* In, char* Out)
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

const char* String::FileExtension(const char* In)
{
	static char Exten[8];

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

//	unixifies a pathname
void String::FixPath(char* pathname)
{
	while (*pathname)
	{
		if (*pathname == '\\')
		{
			*pathname = '/';
		}
		pathname++;
	}
}

//	Parse a token out of a string
//	data is an in/out parm, returns a parsed out token
char* String::Parse1(const char** data_p)
{
	int c;
	int len;
	const char* data;

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
	while ((c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return com_token;
		}
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
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
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < (int)sizeof(com_token))
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse single characters
	if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		*data_p = data + 1;
		return com_token;
	}

	// parse a regular word
	do
	{
		if (len < (int)sizeof(com_token))
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':')
		{
			break;
		}
	}
	while (c > 32);

	if (len == sizeof(com_token))
	{
		len = 0;
	}
	com_token[len] = 0;
	*data_p = data;
	return com_token;
}

//	Parse a token out of a string
char* String::Parse2(const char** data_p)
{
	int c;
	int len;
	const char* data;

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
	while ((c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return com_token;
		}
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
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
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < (int)sizeof(com_token))
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < (int)sizeof(com_token))
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	}
	while (c > 32);

	if (len == sizeof(com_token))
	{
//		common->Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

//	Parse a token out of a string
//	Will never return NULL, just empty strings
//
//	If "allowLineBreaks" is true then an empty string will be returned if
// the next token is a newline.
char* String::Parse3(const char** data_p)
{
	return ParseExt(data_p, true);
}

static const char* SkipWhitespace(const char* data, bool* hasNewLines)
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

char* String::ParseExt(const char** data_p, bool allowLineBreaks)
{
	int c = 0, len;
	bool hasNewLines = false;
	const char* data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return com_token;
	}

	while (1)
	{
		// skip whitespace
		data = SkipWhitespace(data, &hasNewLines);
		if (!data)
		{
			*data_p = NULL;
			return com_token;
		}
		if (hasNewLines && !allowLineBreaks)
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			data += 2;
			while (*data && *data != '\n')
			{
				data++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			data += 2;
			while (*data && (*data != '*' || data[1] != '/'))
			{
				data++;
			}
			if (*data)
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
			if ((GGameType & GAME_ET) && c == '\\' && *data == '\"')
			{
				// Arnout: string-in-string
				if (len < MAX_TOKEN_CHARS_Q3)
				{
					com_token[len] = '\"';
					len++;
				}
				data++;

				while (1)
				{
					c = *data++;

					if (!c)
					{
						com_token[len] = 0;
						*data_p = data;
						break;
					}
					if ((c == '\\' && *data == '\"'))
					{
						if (len < MAX_TOKEN_CHARS_Q3)
						{
							com_token[len] = '\"';
							len++;
						}
						data++;
						c = *data++;
						break;
					}
					if (len < MAX_TOKEN_CHARS_Q3)
					{
						com_token[len] = c;
						len++;
					}
				}
			}
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p = (char*)data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS_Q3)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS_Q3)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	}
	while (c > 32);

	if (len == MAX_TOKEN_CHARS_Q3)
	{
//		common->Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = (char*)data;
	return com_token;
}

int String::Compress(char* data_p)
{
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		//	Version used in Wolf games, I don't have time now to deal with it.
		char* datai, * datao;
		int c, pc, size;
		qboolean ws = false;//Hmmmm, always false.

		size = 0;
		pc = 0;
		datai = datao = data_p;
		if (datai)
		{
			while ((c = *datai) != 0)
			{
				if (c == 13 || c == 10)
				{
					*datao = c;
					datao++;
					ws = false;
					pc = c;
					datai++;
					size++;
					// skip double slash comments
				}
				else if (c == '/' && datai[1] == '/')
				{
					while (*datai && *datai != '\n')
					{
						datai++;
					}
					ws = false;
					// skip /* */ comments
				}
				else if (c == '/' && datai[1] == '*')
				{
					if (GGameType & GAME_ET)
					{
						datai += 2;	// Arnout: skip over '/*'
					}
					while (*datai && (*datai != '*' || datai[1] != '/'))
					{
						datai++;
					}
					if (*datai)
					{
						datai += 2;
					}
					ws = false;
				}
				else
				{
					if (ws)
					{
						*datao = ' ';
						datao++;
					}
					*datao = c;
					datao++;
					datai++;
					ws = false;
					pc = c;
					size++;
				}
			}
		}
		*datao = 0;
		return size;
	}
	char* in, * out;
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
				while (*in && (*in != '*' || in[1] != '/'))
					in++;
				if (*in)
				{
					in += 2;
				}
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

//	The next token should be an open brace. Skips until a matching close
// brace is found. Internal brace depths are properly skipped.
void String::SkipBracedSection(const char** program)
{
	const char* token;
	int depth;

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
	}
	while (depth && *program);
}

void String::SkipRestOfLine(const char** data)
{
	const char* p;
	int c;

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

static const char* Com_StringContains(const char* str1, const char* str2, bool casesensitive)
{
	int len, i, j;

	len = String::Length(str1) - String::Length(str2);
	for (i = 0; i <= len; i++, str1++)
	{
		for (j = 0; str2[j]; j++)
		{
			if (casesensitive)
			{
				if (str1[j] != str2[j])
				{
					break;
				}
			}
			else
			{
				if (String::ToUpper(str1[j]) != String::ToUpper(str2[j]))
				{
					break;
				}
			}
		}
		if (!str2[j])
		{
			return str1;
		}
	}
	return NULL;
}

bool String::Filter(const char* filter, const char* name, bool casesensitive)
{
	char buf[MAX_TOKEN_CHARS_Q3];
	const char* ptr;
	int i, found;

	while (*filter)
	{
		if (*filter == '*')
		{
			filter++;
			for (i = 0; *filter; i++)
			{
				if (*filter == '*' || *filter == '?')
				{
					break;
				}
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if (Length(buf))
			{
				ptr = Com_StringContains(name, buf, casesensitive);
				if (!ptr)
				{
					return false;
				}
				name = ptr + Length(buf);
			}
		}
		else if (*filter == '?')
		{
			filter++;
			name++;
		}
		else if (*filter == '[' && *(filter + 1) == '[')
		{
			filter++;
		}
		else if (*filter == '[')
		{
			filter++;
			found = false;
			while (*filter && !found)
			{
				if (*filter == ']' && *(filter + 1) != ']')
				{
					break;
				}
				if (*(filter + 1) == '-' && *(filter + 2) && (*(filter + 2) != ']' || *(filter + 3) == ']'))
				{
					if (casesensitive)
					{
						if (*name >= *filter && *name <= *(filter + 2))
						{
							found = true;
						}
					}
					else
					{
						if (ToUpper(*name) >= ToUpper(*filter) &&
							ToUpper(*name) <= ToUpper(*(filter + 2)))
						{
							found = true;
						}
					}
					filter += 3;
				}
				else
				{
					if (casesensitive)
					{
						if (*filter == *name)
						{
							found = true;
						}
					}
					else
					{
						if (ToUpper(*filter) == ToUpper(*name))
						{
							found = true;
						}
					}
					filter++;
				}
			}
			if (!found)
			{
				return false;
			}
			while (*filter)
			{
				if (*filter == ']' && *(filter + 1) != ']')
				{
					break;
				}
				filter++;
			}
			filter++;
			name++;
		}
		else
		{
			if (casesensitive)
			{
				if (*filter != *name)
				{
					return false;
				}
			}
			else
			{
				if (String::ToUpper(*filter) != String::ToUpper(*name))
				{
					return false;
				}
			}
			filter++;
			name++;
		}
	}
	return true;
}

bool String::FilterPath(const char* filter, const char* name, bool casesensitive)
{
	int i;
	char new_filter[MAX_QPATH];
	char new_name[MAX_QPATH];

	for (i = 0; i < MAX_QPATH - 1 && filter[i]; i++)
	{
		if (filter[i] == '\\' || filter[i] == ':')
		{
			new_filter[i] = '/';
		}
		else
		{
			new_filter[i] = filter[i];
		}
	}
	new_filter[i] = '\0';
	for (i = 0; i < MAX_QPATH - 1 && name[i]; i++)
	{
		if (name[i] == '\\' || name[i] == ':')
		{
			new_name[i] = '/';
		}
		else
		{
			new_name[i] = name[i];
		}
	}
	new_name[i] = '\0';
	return Filter(new_filter, new_name, casesensitive);
}

int String::LengthWithoutColours(const char* string)
{
	int len;
	const char* p;

	if (!string)
	{
		return 0;
	}

	len = 0;
	p = string;
	while (*p)
	{
		if (Q_IsColorString(p))
		{
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

char* String::CleanStr(char* string)
{
	char* d;
	char* s;
	int c;

	s = string;
	d = string;
	while ((c = *s) != 0)
	{
		if (Q_IsColorString(s))
		{
			s++;
		}
		else if (c >= 0x20 && c <= 0x7E)
		{
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

//	Does a varargs printf into a temp buffer, so I don't need to have
// varargs versions of all text functions.
//	FIXME: make this buffer size safe someday
char* va(const char* format, ...)
{
	enum { MAX_VA_STRING = 32000 };
#if 1
	va_list argptr;
	static char string[2][MAX_VA_STRING];	// in case va is called by nested functions
	static int index = 0;

	char* buf = string[index & 1];
	index++;

	va_start(argptr, format);
	Q_vsnprintf(buf, 32000, format, argptr);
	va_end(argptr);

	return buf;
#else
	//	Wolf games use this, looks interesting.
	va_list argptr;
	static char temp_buffer[MAX_VA_STRING];
	static char string[MAX_VA_STRING];		// in case va is called by nested functions
	static int index = 0;

	va_start(argptr, format);
	vsprintf(temp_buffer, format, argptr);
	va_end(argptr);

	int len = String::Length(temp_buffer);
	if (len >= MAX_VA_STRING)
	{
		common->Error("Attempted to overrun string in call to va()\n");
	}

	if (len + index >= MAX_VA_STRING - 1)
	{
		index = 0;
	}

	char* buf = &string[index];
	memcpy(buf, temp_buffer, len + 1);

	index += len + 1;

	return buf;
#endif
}

/*
vsnprintf portability:

C99 standard: vsnprintf returns the number of characters (excluding the trailing
'\0') which would have been written to the final string if enough space had been available
snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')

win32: _vsnprintf returns the number of characters written, not including the terminating null character,
or a negative value if an output error occurs. If the number of characters to write exceeds count,
then count characters are written and -1 is returned and no trailing '\0' is added.

Q_vsnPrintf: always append a trailing '\0', returns number of characters written or
returns -1 on failure or if the buffer would be overflowed.
*/
int Q_vsnprintf(char* dest, int size, const char* fmt, va_list argptr)
{
#ifdef _WIN32
#undef _vsnprintf
	int ret = _vsnprintf(dest, size - 1, fmt, argptr);
#define _vsnprintf  use_idStr_vsnPrintf
#else
#undef vsnprintf
	int ret = vsnprintf(dest, size, fmt, argptr);
#define vsnprintf   use_idStr_vsnPrintf
#endif
	dest[size - 1] = '\0';
	if (ret < 0 || ret >= size)
	{
		return -1;
	}
	return ret;
}
