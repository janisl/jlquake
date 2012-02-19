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

class String
{
private:
	char*		Str;

	void Resize(int NewLen);

public:
	//	Constructors.
	String()
	: Str(NULL)
	{}
	String(const char* InStr)
	: Str(NULL)
	{
		if (*InStr)
		{
			Resize(Length(InStr));
			Cpy(Str, InStr);
		}
	}
	String(const String& InStr)
	: Str(NULL)
	{
		if (InStr.Str)
		{
			Resize(InStr.Length());
			Cpy(Str, InStr.Str);
		}
	}
	String(const String& InStr, int Start, int Len);
	explicit String(char InChr)
	: Str(NULL)
	{
		Resize(1);
		Str[0] = InChr;
	}
	explicit String(bool InBool)
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
	explicit String(int InInt)
	: Str(NULL)
	{
		char Buf[64];

		sprintf(Buf, "%d", InInt);
		Resize(Length(Buf));
		Cpy(Str, Buf);
	}
	explicit String(unsigned InInt)
	: Str(NULL)
	{
		char Buf[64];

		sprintf(Buf, "%u", InInt);
		Resize(Length(Buf));
		Cpy(Str, Buf);
	}
	explicit String(float InFloat)
	: Str(NULL)
	{
		char Buf[64];

		sprintf(Buf, "%f", InFloat);
		Resize(Length(Buf));
		Cpy(Str, Buf);
	}

	//	Destructor.
	~String()
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
	String& operator=(const char* InStr)
	{
		Resize(Length(InStr));
		if (*InStr)
			Cpy(Str, InStr);
		return *this;
	}
	String& operator=(const String& InStr)
	{
		Resize(InStr.Length());
		if (InStr.Str)
			Cpy(Str, InStr.Str);
		return *this;
	}

	//	Concatenation operators.
	String& operator+=(const char* InStr)
	{
		if (*InStr)
		{
			int l = Length();
			Resize(l + Length(InStr));
			Cpy(Str + l, InStr);
		}
		return *this;
	}
	String& operator+=(const String& InStr)
	{
		if (InStr.Length())
		{
			int l = Length();
			Resize(l + InStr.Length());
			Cpy(Str + l, *InStr);
		}
		return *this;
	}
	String& operator+=(char InChr)
	{
		int l = Length();
		Resize(l + 1);
		Str[l] = InChr;
		return *this;
	}
	String& operator+=(bool InBool)
	{
		return operator+=(InBool ? "true" : "false");
	}
	String& operator+=(int InInt)
	{
		char Buf[64];

		sprintf(Buf, "%d", InInt);
		return operator+=(Buf);
	}
	String& operator+=(unsigned InInt)
	{
		char Buf[64];

		sprintf(Buf, "%u", InInt);
		return operator+=(Buf);
	}
	String& operator+=(float InFloat)
	{
		char Buf[64];

		sprintf(Buf, "%f", InFloat);
		return operator+=(Buf);
	}
	friend String operator+(const String& S1, const char* S2)
	{
		String Ret(S1);
		Ret += S2;
		return Ret;
	}
	friend String operator+(const String& S1, const String& S2)
	{
		String Ret(S1);
		Ret += S2;
		return Ret;
	}
	friend String operator+(const String& S1, char S2)
	{
		String Ret(S1);
		Ret += S2;
		return Ret;
	}
	friend String operator+(const String& S1, bool InBool)
	{
		String Ret(S1);
		Ret += InBool;
		return Ret;
	}
	friend String operator+(const String& S1, int InInt)
	{
		String Ret(S1);
		Ret += InInt;
		return Ret;
	}
	friend String operator+(const String& S1, unsigned InInt)
	{
		String Ret(S1);
		Ret += InInt;
		return Ret;
	}
	friend String operator+(const String& S1, float InFloat)
	{
		String Ret(S1);
		Ret += InFloat;
		return Ret;
	}

	//	Comparison operators.
	friend bool operator==(const String& S1, const char* S2)
	{
		return !Cmp(*S1, S2);
	}
	friend bool operator==(const String& S1, const String& S2)
	{
		return !Cmp(*S1, *S2);
	}
	friend bool operator!=(const String& S1, const char* S2)
	{
		return !!Cmp(*S1, S2);
	}
	friend bool operator!=(const String& S1, const String& S2)
	{
		return !!Cmp(*S1, *S2);
	}

	//	Comparison functions.
	int Cmp(const char* S2) const
	{
		return Cmp(**this, S2);
	}
	int Cmp(const String& S2) const
	{
		return Cmp(**this, *S2);
	}
	int ICmp(const char* S2) const
	{
		return ICmp(**this, S2);
	}
	int ICmp(const String& S2) const
	{
		return ICmp(**this, *S2);
	}

	bool StartsWith(const char*) const;
	bool StartsWith(const String&) const;
	bool EndsWith(const char*) const;
	bool EndsWith(const String&) const;

	String ToLower() const;
	String ToUpper() const;

	int IndexOf(char) const;
	int IndexOf(const char*) const;
	int IndexOf(const String&) const;
	int LastIndexOf(char) const;
	int LastIndexOf(const char*) const;
	int LastIndexOf(const String&) const;

	String Replace(const char*, const char*) const;
	String Replace(const String&, const String&) const;

	String Utf8Substring(int, int) const;

	void Split(char, Array<String>&) const;
	void Split(const char*, Array<String>&) const;

	bool IsValidUtf8() const;
	String Latin1ToUtf8() const;

	//	Static UTF-8 related methods.
	static int Utf8Length(const char*);
	static int ByteLengthForUtf8(const char*, int);
	static int GetChar(const char*&);
	static String FromChar(int);

	//	Replacements for standard string functions.
	static int Length(const char* str);
	static int Cmp(const char* s1, const char* s2);
	static int NCmp(const char* s1, const char* s2, size_t n);
	static int ICmp(const char* s1, const char* s2);
	static int NICmp(const char* s1, const char* s2, size_t n);
	static void Cpy(char* dst, const char* src);
	static void NCpy(char* dst, const char* src, size_t count);
	static void NCpyZ(char* dest, const char* src, int destSize);
	static void Cat(char* dest, int size, const char* src);
	static char* ToLower(char* s1);
	static char* ToUpper(char* s1);
	static char* RChr(const char* string, int c);
	static int Atoi(const char* Str);
	static float Atof(const char* Str);
	static void Sprintf(char* Dest, int Size, const char* Fmt, ...);

	//	Replacements for standard character functions.
	static int IsPrint(int c);
	static int IsLower(int c);
	static int IsUpper(int c);
	static int IsAlpha(int c);
	static int IsSpace(int c);
	static int IsDigit(int c);
	static char ToUpper(char c);
	static char ToLower(char c);

	static char* SkipPath(char* PathName);
	static const char* SkipPath(const char *PathName);
	static void StripExtension(const char* In, char* Out);
	static void DefaultExtension(char* Path, int MaxSize, const char* Extension);
	static void FilePath(const char* In, char* Out);
	static void FileBase(const char* In, char* Out);
	static const char* FileExtension(const char* In);

#if 0
	static char* Parse1(const char** Data_p);
	static char* Parse2(const char** Data_p);
	static char* Parse3(const char** Data_p);
	static char* ParseExt(const char** Data_p, bool AllowLineBreak);
	static int Compress(char* data_p);
	static void SkipBracedSection(const char **program);
	static void SkipRestOfLine(const char **data);

	static bool Filter(const char* Filter, const char* Name, bool CaseSensitive);
	static bool FilterPath(const char* Filter, const char* Name, bool CaseSensitive);
#endif
};

#if 0
char* va(const char* Format, ...) id_attribute((format(printf, 1, 2)));

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
#endif

#define _vsnprintf use_Q_vsnprintf
#define vsnprintf use_Q_vsnprintf
int Q_vsnprintf(char* dest, int size, const char* fmt, va_list argptr);
