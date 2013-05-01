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

#include "translate.h"
#include "client_main.h"
#include "public.h"
#include "../common/Common.h"
#include "../common/common_defs.h"
#include "../common/strings.h"
#include "../common/command_buffer.h"

#define MAX_TRANS_STRING    4096
#define MAX_VA_STRING       32000
#define TRANSLATION_HASH_SIZE      1024

struct trans_t {
	char original[ MAX_TRANS_STRING ];
	char translated[ MAX_LANGUAGES ][ MAX_TRANS_STRING ];
	trans_t* next;
	float x_offset;
	float y_offset;
	bool fromFile;
};

Cvar* cl_language;
static Cvar* cl_debugTranslation;

static trans_t* transTable[ TRANSLATION_HASH_SIZE ];

static trans_t* AllocTrans( const char* original, const char* translated[ MAX_LANGUAGES ] ) {
	trans_t* t = new trans_t;
	Com_Memset( t, 0, sizeof ( trans_t ) );

	if ( original ) {
		String::NCpy( t->original, original, MAX_TRANS_STRING );
	}

	if ( translated ) {
		for ( int i = 0; i < MAX_LANGUAGES; i++ ) {
			String::NCpy( t->translated[ i ], translated[ i ], MAX_TRANS_STRING );
		}
	}

	return t;
}

static int GenerateTranslationHashValue( const char* fname ) {
	int hash = 0;
	int i = 0;
	while ( fname[ i ] != '\0' ) {
		char letter = String::ToLower( fname[ i ] );
		hash += ( long )( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( TRANSLATION_HASH_SIZE - 1 );
	return hash;
}

static void CL_SaveTransTable( const char* fileName, bool newOnly ) {
	if ( cl.wm_corruptedTranslationFile ) {
		common->Printf( S_COLOR_YELLOW "WARNING: Cannot save corrupted translation file. Please reload first." );
		return;
	}

	fileHandle_t f;
	FS_FOpenFileByMode( fileName, &f, FS_WRITE );

	int bucketnum = 0;
	int maxbucketlen = 0;
	int avebucketlen = 0;
	int transnum = 0;
	int untransnum = 0;

	// write out version, if one
	const char* buf;
	if ( String::Length( cl.wm_translationVersion ) ) {
		buf = va( "#version\t\t\"%s\"\n", cl.wm_translationVersion );
	} else {
		buf = va( "#version\t\t\"1.0 01/01/01\"\n" );
	}

	int len = String::Length( buf );
	FS_Write( buf, len, f );

	// write out translated strings
	for ( int j = 0; j < 2; j++ ) {
		for ( int i = 0; i < TRANSLATION_HASH_SIZE; i++ ) {
			trans_t* t = transTable[ i ];

			if ( !t || ( newOnly && t->fromFile ) ) {
				continue;
			}

			int bucketlen = 0;

			for (; t; t = t->next ) {
				bucketlen++;

				if ( String::Length( t->translated[ 0 ] ) ) {
					if ( j ) {
						continue;
					}
					transnum++;
				} else {
					if ( !j ) {
						continue;
					}
					untransnum++;
				}

				buf = va( "{\n\tenglish\t\t\"%s\"\n", t->original );
				len = String::Length( buf );
				FS_Write( buf, len, f );

				buf = va( "\tfrench\t\t\"%s\"\n", t->translated[ LANGUAGE_FRENCH ] );
				len = String::Length( buf );
				FS_Write( buf, len, f );

				buf = va( "\tgerman\t\t\"%s\"\n", t->translated[ LANGUAGE_GERMAN ] );
				len = String::Length( buf );
				FS_Write( buf, len, f );

				buf = va( "\titalian\t\t\"%s\"\n", t->translated[ LANGUAGE_ITALIAN ] );
				len = String::Length( buf );
				FS_Write( buf, len, f );

				buf = va( "\tspanish\t\t\"%s\"\n", t->translated[ LANGUAGE_SPANISH ] );
				len = String::Length( buf );
				FS_Write( buf, len, f );

				buf = "}\n";
				len = String::Length( buf );
				FS_Write( buf, len, f );
			}

			if ( bucketlen > maxbucketlen ) {
				maxbucketlen = bucketlen;
			}

			if ( bucketlen ) {
				bucketnum++;
				avebucketlen += bucketlen;
			}
		}
	}

	common->Printf( "Saved translation table.\nTotal = %i, Translated = %i, Untranslated = %i, aveblen = %2.2f, maxblen = %i\n",
		transnum + untransnum, transnum, untransnum, ( float )avebucketlen / bucketnum, maxbucketlen );

	FS_FCloseFile( f );
}

static trans_t* LookupTrans( const char* original, const char* translated[ MAX_LANGUAGES ], bool isLoading ) {
	int hash = GenerateTranslationHashValue( original );

	trans_t* prev = NULL;
	for ( trans_t* t = transTable[ hash ]; t; prev = t, t = t->next ) {
		if ( !String::ICmp( original, t->original ) ) {
			if ( isLoading ) {
				common->DPrintf( S_COLOR_YELLOW "WARNING: Duplicate string found: \"%s\"\n", original );
			}
			return t;
		}
	}

	trans_t* newt = AllocTrans( original, translated );

	if ( prev ) {
		prev->next = newt;
	} else {
		transTable[ hash ] = newt;
	}

	if ( cl_debugTranslation->integer >= 1 && !isLoading ) {
		common->Printf( "Missing translation: \'%s\'\n", original );
	}

	// see if we want to save out the translation table everytime a string is added
	if ( cl_debugTranslation->integer == 2 && !isLoading ) {
		CL_SaveTransTable( "scripts/translation.cfg", false );
	}

	return newt;
}

//	compare formatting characters
static bool CL_CheckTranslationString( const char* original, const char* translated ) {
	char format_org[ 128 ], format_trans[ 128 ];
	Com_Memset( format_org, 0, 128 );
	Com_Memset( format_trans, 0, 128 );

	// generate formatting string for original
	int len = String::Length( original );

	for ( int i = 0; i < len; i++ ) {
		if ( original[ i ] != '%' ) {
			continue;
		}

		String::Cat( format_org, sizeof ( format_org ), va( "%c%c ", '%', original[ i + 1 ] ) );
	}

	// generate formatting string for translated
	len = String::Length( translated );
	if ( !len ) {
		return true;
	}

	for ( int i = 0; i < len; i++ ) {
		if ( translated[ i ] != '%' ) {
			continue;
		}

		String::Cat( format_trans, sizeof ( format_trans ), va( "%c%c ", '%', translated[ i + 1 ] ) );
	}

	// compare
	len = String::Length( format_org );

	if ( len != String::Length( format_trans ) ) {
		return false;
	}

	for ( int i = 0; i < len; i++ ) {
		if ( format_org[ i ] != format_trans[ i ] ) {
			return false;
		}
	}

	return true;
}

static void CL_LoadTransTable( const char* fileName ) {
	int count = 0;
	bool aborted = false;
	cl.wm_corruptedTranslationFile = false;

	fileHandle_t f;
	int len = FS_FOpenFileByMode( fileName, &f, FS_READ );
	if ( len <= 0 ) {
		return;
	}

	char* text = ( char* )Mem_Alloc( len + 1 );

	FS_Read( text, len, f );
	text[ len ] = 0;
	FS_FCloseFile( f );

	// parse the text
	const char* text_p = text;

	const char* token;
	char translated[ MAX_LANGUAGES ][ MAX_VA_STRING ];
	char original[ MAX_VA_STRING ];
	do {
		token = String::Parse3( &text_p );
		if ( String::ICmp( "{", token ) ) {
			// parse version number
			if ( !String::ICmp( "#version", token ) ) {
				token = String::Parse3( &text_p );
				String::Cpy( cl.wm_translationVersion, token );
				continue;
			}

			break;
		}

		// english
		token = String::Parse3( &text_p );
		if ( String::ICmp( "english", token ) ) {
			aborted = true;
			break;
		}

		token = String::Parse3( &text_p );
		String::Cpy( original, token );

		if ( cl_debugTranslation->integer == 3 ) {
			common->Printf( "%i Loading: \"%s\"\n", count, original );
		}

		// french
		token = String::Parse3( &text_p );
		if ( String::ICmp( "french", token ) ) {
			aborted = true;
			break;
		}

		token = String::Parse3( &text_p );
		String::Cpy( translated[ LANGUAGE_FRENCH ], token );
		if ( !CL_CheckTranslationString( original, translated[ LANGUAGE_FRENCH ] ) ) {
			common->Printf( S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n" );
			aborted = true;
			break;
		}

		// german
		token = String::Parse3( &text_p );
		if ( String::ICmp( "german", token ) ) {
			aborted = true;
			break;
		}

		token = String::Parse3( &text_p );
		String::Cpy( translated[ LANGUAGE_GERMAN ], token );
		if ( !CL_CheckTranslationString( original, translated[ LANGUAGE_GERMAN ] ) ) {
			common->Printf( S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n" );
			aborted = true;
			break;
		}

		// italian
		token = String::Parse3( &text_p );
		if ( String::ICmp( "italian", token ) ) {
			aborted = true;
			break;
		}

		token = String::Parse3( &text_p );
		String::Cpy( translated[ LANGUAGE_ITALIAN ], token );
		if ( !CL_CheckTranslationString( original, translated[ LANGUAGE_ITALIAN ] ) ) {
			common->Printf( S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n" );
			aborted = true;
			break;
		}

		// spanish
		token = String::Parse3( &text_p );
		if ( String::ICmp( "spanish", token ) ) {
			aborted = true;
			break;
		}

		token = String::Parse3( &text_p );
		String::Cpy( translated[ LANGUAGE_SPANISH ], token );
		if ( !CL_CheckTranslationString( original, translated[ LANGUAGE_SPANISH ] ) ) {
			common->Printf( S_COLOR_YELLOW "WARNING: Translation formatting doesn't match up with English version!\n" );
			aborted = true;
			break;
		}

		// do lookup
		trans_t* t = LookupTrans( original, NULL, true );

		if ( t ) {
			t->fromFile = true;

			for ( int i = 0; i < MAX_LANGUAGES; i++ ) {
				String::NCpy( t->translated[ i ], translated[ i ], MAX_TRANS_STRING );
			}
		}

		token = String::Parse3( &text_p );

		// set offset if we have one
		if ( !String::ICmp( "offset", token ) ) {
			token = String::Parse3( &text_p );
			t->x_offset = String::Atof( token );

			token = String::Parse3( &text_p );
			t->y_offset = String::Atof( token );

			token = String::Parse3( &text_p );
		}

		if ( String::ICmp( "}", token ) ) {
			aborted = true;
			break;
		}

		count++;
	} while ( token );

	if ( aborted ) {
		int i, line = 1;

		for ( i = 0; i < len && ( text + i ) < text_p; i++ ) {
			if ( text[ i ] == '\n' ) {
				line++;
			}
		}

		common->Printf( S_COLOR_YELLOW "WARNING: Problem loading %s on line %i\n", fileName, line );
		cl.wm_corruptedTranslationFile = true;
	} else {
		common->Printf( "Loaded %i translation strings from %s\n", count, fileName );
	}

	// cleanup
	Mem_Free( text );
}

static void CL_LoadTranslations() {
	Com_Memset( transTable, 0, sizeof ( trans_t* ) * TRANSLATION_HASH_SIZE );
	CL_LoadTransTable( "scripts/translation.cfg" );

	int numFiles;
	char** fileList = FS_ListFiles( "translations", ".cfg", &numFiles );

	for ( int i = 0; i < numFiles; i++ ) {
		CL_LoadTransTable( va( "translations/%s", fileList[ i ] ) );
	}

	FS_FreeFileList( fileList );
}

static void CL_ReloadTranslation() {
	for ( int i = 0; i < TRANSLATION_HASH_SIZE; i++ ) {
		trans_t* trans = transTable[ i ];
		while ( trans ) {
			trans_t* next = trans->next;
			delete trans;
			trans = next;
		}
	}

	CL_LoadTranslations();
}

static void CL_SaveTranslations_f() {
	CL_SaveTransTable( "scripts/translation.cfg", false );
}

static void CL_SaveNewTranslations_f() {
	if ( Cmd_Argc() != 2 ) {
		common->Printf( "usage: SaveNewTranslations <filename>\n" );
		return;
	}

	char fileName[ 512 ];
	String::Cpy( fileName, va( "translations/%s.cfg", Cmd_Argv( 1 ) ) );

	CL_SaveTransTable( fileName, true );
}

static void CL_LoadTranslations_f() {
	CL_ReloadTranslation();
}

void CL_InitTranslation() {
	cl_language = Cvar_Get( "cl_language", "0", CVAR_ARCHIVE );
	if ( !( GGameType & ( GAME_WolfMP | GAME_ET ) ) ) {
		return;
	}
	cl_debugTranslation = Cvar_Get( "cl_debugTranslation", "0", 0 );

	Cmd_AddCommand( "SaveTranslations", CL_SaveTranslations_f );
	Cmd_AddCommand( "SaveNewTranslations", CL_SaveNewTranslations_f );
	Cmd_AddCommand( "LoadTranslations", CL_LoadTranslations_f );

	CL_LoadTranslations();
}

void CL_TranslateString( const char* string, char* destBuffer ) {
	if ( !string ) {
		String::Cpy( destBuffer, "(null)" );
		return;
	}

	if ( !( GGameType & ( GAME_WolfMP | GAME_ET ) ) ) {
		String::Cpy( destBuffer, string );
		return;
	}

	int currentLanguage = cl_language->integer - 1;

	// early bail if we only want english or bad language type
	if ( currentLanguage < 0 || currentLanguage >= MAX_LANGUAGES || !String::Length( string ) ) {
		String::Cpy( destBuffer, string );
		return;
	}

	// ignore newlines
	bool newline = false;
	if ( string[ String::Length( string ) - 1 ] == '\n' ) {
		newline = true;
	}

	int count = 0;
	for ( int i = 0; string[ i ] != '\0'; i++ ) {
		if ( string[ i ] != '\n' ) {
			destBuffer[ count++ ] = string[ i ];
		}
	}
	destBuffer[ count ] = '\0';

	trans_t* t = LookupTrans( destBuffer, NULL, false );

	if ( t && String::Length( t->translated[ currentLanguage ] ) ) {
		int offset = 0;

		if ( cl_debugTranslation->integer >= 1 ) {
			destBuffer[ 0 ] = '^';
			destBuffer[ 1 ] = '1';
			destBuffer[ 2 ] = '[';
			offset = 3;
		}

		String::Cpy( destBuffer + offset, t->translated[ currentLanguage ] );

		if ( cl_debugTranslation->integer >= 1 ) {
			int len2 = String::Length( destBuffer );

			destBuffer[ len2 ] = ']';
			destBuffer[ len2 + 1 ] = '^';
			destBuffer[ len2 + 2 ] = '7';
			destBuffer[ len2 + 3 ] = '\0';
		}

		if ( newline ) {
			int len2 = String::Length( destBuffer );

			destBuffer[ len2 ] = '\n';
			destBuffer[ len2 + 1 ] = '\0';
		}
	} else {
		int offset = 0;

		if ( cl_debugTranslation->integer >= 1 ) {
			destBuffer[ 0 ] = '^';
			destBuffer[ 1 ] = '1';
			destBuffer[ 2 ] = '[';
			offset = 3;
		}

		String::Cpy( destBuffer + offset, string );

		if ( cl_debugTranslation->integer >= 1 ) {
			int len2 = String::Length( destBuffer );
			bool addnewline = false;

			if ( destBuffer[ len2 - 1 ] == '\n' ) {
				len2--;
				addnewline = true;
			}

			destBuffer[ len2 ] = ']';
			destBuffer[ len2 + 1 ] = '^';
			destBuffer[ len2 + 2 ] = '7';
			destBuffer[ len2 + 3 ] = '\0';

			if ( addnewline ) {
				destBuffer[ len2 + 3 ] = '\n';
				destBuffer[ len2 + 4 ] = '\0';
			}
		}
	}
}

//	TTimo - handy, stores in a static buf, converts \n to chr(13)
const char* CL_TranslateStringBuf( const char* string ) {
	char* p;
	int i,l;
	static char buf[ MAX_VA_STRING ];
	CL_TranslateString( string, buf );
	while ( ( p = strstr( buf, "\\n" ) ) ) {
		*p = '\n';
		p++;
		// Com_Memcpy(p, p+1, String::Length(p) ); b0rks on win32
		l = String::Length( p );
		for ( i = 0; i < l; i++ ) {
			*p = *( p + 1 );
			p++;
		}
	}
	return buf;
}
