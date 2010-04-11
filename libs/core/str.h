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

class QStr
{
private:
	char*		Str;

	void Resize(int NewLen);

public:
	//	Constructors.
	QStr()
	: Str(NULL)
	{}
	QStr(const char* InStr)
	: Str(NULL)
	{
		if (*InStr)
		{
			Resize(Length(InStr));
			Cpy(Str, InStr);
		}
	}
	QStr(const QStr& InStr)
	: Str(NULL)
	{
		if (InStr.Str)
		{
			Resize(InStr.Length());
			Cpy(Str, InStr.Str);
		}
	}
	QStr(const QStr& InStr, int Start, int Len);
	explicit QStr(char InChr)
	: Str(NULL)
	{
		Resize(1);
		Str[0] = InChr;
	}
	explicit QStr(bool InBool)
	: Str(NULL)
	{
		if (InBool)
		{
			Resize(4);
			Cpy(Str, "true");
		}
		else
		{
			Resize(5);
			Cpy(Str, "false");
		}
	}
	explicit QStr(int InInt)
	: Str(NULL)
	{
		char Buf[64];

		sprintf(Buf, "%d", InInt);
		Resize(Length(Buf));
		Cpy(Str, Buf);
	}
	explicit QStr(unsigned InInt)
	: Str(NULL)
	{
		char Buf[64];

		sprintf(Buf, "%u", InInt);
		Resize(Length(Buf));
		Cpy(Str, Buf);
	}
	explicit QStr(float InFloat)
	: Str(NULL)
	{
		char Buf[64];

		sprintf(Buf, "%f", InFloat);
		Resize(Length(Buf));
		Cpy(Str, Buf);
	}

	//	Destructor.
	~QStr()
	{
		Clean();
	}

	//	Clears the string.
	void Clean()
	{
		Resize(0);
	}
	//	Return length of the string.
	int Length() const
	{
		return Str ? ((int*)Str)[-1] : 0;
	}
	//	Return number of characters in a UTF-8 string.
	int Utf8Length() const
	{
		return Str ? Utf8Length(Str) : 0;
	}
	//	Return C string.
	const char* operator*() const
	{
		return Str ? Str : "";
	}
	//	Check if string is empty.
	bool IsEmpty() const
	{
		return !Str;
	}
	bool IsNotEmpty() const
	{
		return !!Str;
	}

	//	Character ancestors.
	char operator[](int Idx) const
	{
		return Str[Idx];
	}
	char& operator[](int Idx)
	{
		return Str[Idx];
	}

	//	Assignement operators.
	QStr& operator=(const char* InStr)
	{
		Resize(Length(InStr));
		if (*InStr)
			Cpy(Str, InStr);
		return *this;
	}
	QStr& operator=(const QStr& InStr)
	{
		Resize(InStr.Length());
		if (InStr.Str)
			Cpy(Str, InStr.Str);
		return *this;
	}

	//	Concatenation operators.
	QStr& operator+=(const char* InStr)
	{
		if (*InStr)
		{
			int l = Length();
			Resize(l + Length(InStr));
			Cpy(Str + l, InStr);
		}
		return *this;
	}
	QStr& operator+=(const QStr& InStr)
	{
		if (InStr.Length())
		{
			int l = Length();
			Resize(l + InStr.Length());
			Cpy(Str + l, *InStr);
		}
		return *this;
	}
	QStr& operator+=(char InChr)
	{
		int l = Length();
		Resize(l + 1);
		Str[l] = InChr;
		return *this;
	}
	QStr& operator+=(bool InBool)
	{
		return operator+=(InBool ? "true" : "false");
	}
	QStr& operator+=(int InInt)
	{
		char Buf[64];

		sprintf(Buf, "%d", InInt);
		return operator+=(Buf);
	}
	QStr& operator+=(unsigned InInt)
	{
		char Buf[64];

		sprintf(Buf, "%u", InInt);
		return operator+=(Buf);
	}
	QStr& operator+=(float InFloat)
	{
		char Buf[64];

		sprintf(Buf, "%f", InFloat);
		return operator+=(Buf);
	}
	friend QStr operator+(const QStr& S1, const char* S2)
	{
		QStr Ret(S1);
		Ret += S2;
		return Ret;
	}
	friend QStr operator+(const QStr& S1, const QStr& S2)
	{
		QStr Ret(S1);
		Ret += S2;
		return Ret;
	}
	friend QStr operator+(const QStr& S1, char S2)
	{
		QStr Ret(S1);
		Ret += S2;
		return Ret;
	}
	friend QStr operator+(const QStr& S1, bool InBool)
	{
		QStr Ret(S1);
		Ret += InBool;
		return Ret;
	}
	friend QStr operator+(const QStr& S1, int InInt)
	{
		QStr Ret(S1);
		Ret += InInt;
		return Ret;
	}
	friend QStr operator+(const QStr& S1, unsigned InInt)
	{
		QStr Ret(S1);
		Ret += InInt;
		return Ret;
	}
	friend QStr operator+(const QStr& S1, float InFloat)
	{
		QStr Ret(S1);
		Ret += InFloat;
		return Ret;
	}

	//	Comparison operators.
	friend bool operator==(const QStr& S1, const char* S2)
	{
		return !Cmp(*S1, S2);
	}
	friend bool operator==(const QStr& S1, const QStr& S2)
	{
		return !Cmp(*S1, *S2);
	}
	friend bool operator!=(const QStr& S1, const char* S2)
	{
		return !!Cmp(*S1, S2);
	}
	friend bool operator!=(const QStr& S1, const QStr& S2)
	{
		return !!Cmp(*S1, *S2);
	}

	//	Comparison functions.
	int Cmp(const char* S2) const
	{
		return Cmp(**this, S2);
	}
	int Cmp(const QStr& S2) const
	{
		return Cmp(**this, *S2);
	}
	int ICmp(const char* S2) const
	{
		return ICmp(**this, S2);
	}
	int ICmp(const QStr& S2) const
	{
		return ICmp(**this, *S2);
	}

	bool StartsWith(const char*) const;
	bool StartsWith(const QStr&) const;
	bool EndsWith(const char*) const;
	bool EndsWith(const QStr&) const;

	QStr ToLower() const;
	QStr ToUpper() const;

	int IndexOf(char) const;
	int IndexOf(const char*) const;
	int IndexOf(const QStr&) const;
	int LastIndexOf(char) const;
	int LastIndexOf(const char*) const;
	int LastIndexOf(const QStr&) const;

	QStr Replace(const char*, const char*) const;
	QStr Replace(const QStr&, const QStr&) const;

	QStr Utf8Substring(int, int) const;

	void Split(char, QArray<QStr>&) const;
	void Split(const char*, QArray<QStr>&) const;

	bool IsValidUtf8() const;
	QStr Latin1ToUtf8() const;

	//	Static UTF-8 related methods.
	static int Utf8Length(const char*);
	static int ByteLengthForUtf8(const char*, size_t);
	static int GetChar(const char*&);
	static QStr FromChar(int);

	//	Replacements for standard string functions.
	static int Length(const char* Str);
	static int Cmp(const char* S1, const char* S2);
	static int NCmp(const char* S1, const char* S2, size_t N);
	static int ICmp(const char* S1, const char* S2);
	static int NICmp(const char* S1, const char* S2, size_t N);
	static void Cpy(char* Dst, const char* Src);
	static void NCpy(char* Dst, const char* Src, size_t N);
	static void NCpyZ(char* Dest, const char* Src, int DestSize);
	static void Cat(char* Dest, int Size, const char* Src);
	static char* ToLower(char* S1);
	static char* ToUpper(char* S1);
	static char* RChr(const char* String, int C);
	static int Atoi(const char* Str);
	static float Atof(const char* Str);
	static void Sprintf(char* Dest, int Size, const char* Fmt, ...);

	//	Replacements for standard character functions.
	static int IsPrint(int C);
	static int IsLower(int C);
	static int IsUpper(int C);
	static int IsAlpha(int C);
	static int IsSpace(int C);
	static int IsDigit(int C);
	static char ToUpper(char C);
	static char ToLower(char C);

	static char* SkipPath(char* PathName);
	static const char* SkipPath(const char *PathName);
	static void StripExtension(const char* In, char* Out);
	static void DefaultExtension(char* Path, int MaxSize, const char* Extension);
	static void FilePath(const char* In, char* Out);
	static void FileBase(const char* In, char* Out);
	static const char* FileExtension(const char* In);

	static char* Parse1(const char** Data_p);
	static char* Parse2(const char** Data_p);
	static char* Parse3(const char** Data_p);
	static char* ParseExt(const char** Data_p, bool AllowLineBreak);
	static int Compress(char* data_p);
	static void SkipBracedSection(const char **program);
	static void SkipRestOfLine(const char **data);
};

char* va(const char* Format, ...) __attribute__ ((format(printf, 1, 2)));

#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE )

#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define ColorIndex(c)	( ( (c) - '0' ) & 7 )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"
