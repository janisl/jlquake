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

#ifndef __STRINGS_H__
#define __STRINGS_H__

#include "Str.h"

class String {
public:
	//	Static UTF-8 related methods.
	static int Utf8Length( const char* );
	static int ByteLengthForUtf8( const char*, int );
	static int GetChar( const char*& );
	static idStr FromChar( int );

	static int Length( const char* text );
	static int Cmp( const char* s1, const char* s2 );
	static int NCmp( const char* s1, const char* s2, size_t n );
	static int ICmp( const char* s1, const char* s2 );
	static int NICmp( const char* s1, const char* s2, size_t n );
	static void Cpy( char* dst, const char* src );
	static void NCpy( char* dst, const char* src, size_t count );
	static void NCpyZ( char* dest, const char* src, int destSize );
	static void Cat( char* dest, int size, const char* src );
	static char* ToLower( char* s1 );
	static char* ToUpper( char* s1 );
	static char* RChr( const char* string, int c );
	static int Atoi( const char* Str );
	static float Atof( const char* Str );
	static void Sprintf( char* Dest, int Size, const char* Fmt, ... );

	//	Replacements for standard character functions.
	static int IsPrint( int c );
	static int IsLower( int c );
	static int IsUpper( int c );
	static int IsAlpha( int c );
	static int IsSpace( int c );
	static int IsDigit( int c );
	static char ToUpper( char c );
	static char ToLower( char c );

	static char* SkipPath( char* PathName );
	static const char* SkipPath( const char* PathName );
	static void StripExtension( const char* In, char* Out );
	static void StripExtension2( const char* in, char* out, int destsize );
	static void DefaultExtension( char* Path, int MaxSize, const char* Extension );
	static void FilePath( const char* In, char* Out );
	static void FileBase( const char* In, char* Out );
	static const char* FileExtension( const char* In );
	static void FixPath( char* pathname );

	static char* Parse1( const char** Data_p );
	static char* Parse2( const char** Data_p );
	static char* Parse3( const char** Data_p );
	static char* ParseExt( const char** Data_p, bool AllowLineBreak );
	static int Compress( char* data_p );
	static void SkipBracedSection( const char** program );
	static void SkipRestOfLine( const char** data );

	static bool Filter( const char* Filter, const char* Name, bool CaseSensitive );
	static bool FilterPath( const char* Filter, const char* Name, bool CaseSensitive );

	static int LengthWithoutColours( const char* string );
	static char* CleanStr( char* string );
};

char* va( const char* Format, ... ) id_attribute( ( format( printf, 1, 2 ) ) );

#define Q_COLOR_ESCAPE  '^'
#define Q_IsColorString( p )  ( p && *( p ) == Q_COLOR_ESCAPE && *( ( p ) + 1 ) && *( ( p ) + 1 ) != Q_COLOR_ESCAPE )

#define COLOR_BLACK     '0'
#define COLOR_RED       '1'
#define COLOR_GREEN     '2'
#define COLOR_YELLOW    '3'
#define COLOR_BLUE      '4'
#define COLOR_CYAN      '5'
#define COLOR_MAGENTA   '6'
#define COLOR_WHITE     '7'
//	New colours from Enemy territory
#define COLOR_ORANGE    '8'
#define COLOR_MDGREY    '9'
#define COLOR_LTGREY    ':'
//#define COLOR_LTGREY	';'
#define COLOR_MDGREEN   '<'
#define COLOR_MDYELLOW  '='
#define COLOR_MDBLUE    '>'
#define COLOR_MDRED     '?'
#define COLOR_LTORANGE  'A'
#define COLOR_MDCYAN    'B'
#define COLOR_MDPURPLE  'C'
#define COLOR_NULL      '*'

#define COLOR_BITS  31
#define ColorIndex( c )   ( ( ( c ) - '0' ) & COLOR_BITS )

#define S_COLOR_BLACK   "^0"
#define S_COLOR_RED     "^1"
#define S_COLOR_GREEN   "^2"
#define S_COLOR_YELLOW  "^3"
#define S_COLOR_BLUE    "^4"
#define S_COLOR_CYAN    "^5"
#define S_COLOR_MAGENTA "^6"
#define S_COLOR_WHITE   "^7"
//	New colours from Enemy territory
#define S_COLOR_ORANGE      "^8"
#define S_COLOR_MDGREY      "^9"
#define S_COLOR_LTGREY      "^:"
//#define S_COLOR_LTGREY		"^;"
#define S_COLOR_MDGREEN     "^<"
#define S_COLOR_MDYELLOW    "^="
#define S_COLOR_MDBLUE      "^>"
#define S_COLOR_MDRED       "^?"
#define S_COLOR_LTORANGE    "^A"
#define S_COLOR_MDCYAN      "^B"
#define S_COLOR_MDPURPLE    "^C"
#define S_COLOR_NULL        "^*"

#define _vsnprintf use_Q_vsnprintf
#define vsnprintf use_Q_vsnprintf
int Q_vsnprintf( char* dest, int size, const char* fmt, va_list argptr );

#endif
