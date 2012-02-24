/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		files.c
 *
 * desc:		handle based filesystem for Quake III Arena
 *
 *
 *****************************************************************************/


#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../core/unzip.h"

// TTimo: moved to qcommon.h
// NOTE: could really do with a cvar
//#define	BASEGAME			"main"
#define DEMOGAME            "demomain"

// every time a new demo pk3 file is built, this checksum must be updated.
// the easiest way to get it is to just run the game and see what it spits out
//DHM - Nerve :: Wolf Multiplayer demo checksum
// NOTE TTimo: always needs the 'u' for unsigned int (gcc)
#define DEMO_PAK_CHECKSUM   2031778175u

#define MAX_SEARCH_PATHS    4096

struct packfile_t
{
	char		name[MAX_QPATH];
	int			filepos;
	int			filelen;
};

struct pack_t
{
	char		filename[MAX_OSPATH];
	FILE*		handle;
	int			numfiles;
	packfile_t*	files;
};

struct fileInPack_t
{
	char                    *name;      // name of the file
	unsigned long pos;                  // file info position in zip
	fileInPack_t*   next;       // next file in the hash
};

struct pack3_t
{
	char pakFilename[MAX_OSPATH];               // c:\quake3\baseq3\pak0.pk3
	char pakBasename[MAX_OSPATH];               // pak0
	char pakGamename[MAX_OSPATH];               // baseq3
	unzFile handle;                             // handle to zip file
	int checksum;                               // regular checksum
	int pure_checksum;                          // checksum for pure
	int numfiles;                               // number of files in pk3
	int referenced;                             // referenced file flags
	int hashSize;                               // hash table size (power of 2)
	fileInPack_t*   *hashTable;                 // hash table
	fileInPack_t*   buildBuffer;                // buffer with the filenames etc.
};

struct directory_t
{
	char path[MAX_OSPATH];              // c:\quake3
	char gamedir[MAX_OSPATH];           // baseq3
	char			fullname[MAX_OSPATH];
};

struct searchpath_t
{
	searchpath_t*next;

	pack_t*			pack;
	pack3_t      *pack3;      // only one of pack / dir will be non NULL
	directory_t *dir;
};

static Cvar      *fs_basegame;
static Cvar      *fs_gamedirvar;
extern searchpath_t    *fs_searchpaths;
static int fs_readCount;                    // total bytes read
static int fs_loadCount;                    // total files read
static int fs_loadStack;                    // total files in memory

// last valid game folder used
char lastValidBase[MAX_OSPATH];
char lastValidGame[MAX_OSPATH];

#ifdef FS_MISSING
FILE*       missingFiles = NULL;
#endif

bool CL_WWWBadChecksum(const char* pakname)
{
	return false;
}

/*
=================
FS_LoadStack
return load stack
=================
*/
int FS_LoadStack() {
	return fs_loadStack;
}

/*
==========
FS_ShiftStr
perform simple string shifting to avoid scanning from the exe
==========
*/
char *FS_ShiftStr( const char *string, int shift ) {
	static char buf[MAX_STRING_CHARS];
	int i,l;

	l = String::Length( string );
	for ( i = 0; i < l; i++ ) {
		buf[i] = string[i] + shift;
	}
	buf[i] = '\0';
	return buf;
}

// TTimo
// relevant to client only
#if !defined( DEDICATED )
/*
==================
FS_CL_ExtractFromPakFile

NERVE - SMF - Extracts the latest file from a pak file.

Compares packed file against extracted file. If no differences, does not copy.
This is necessary for exe/dlls which may or may not be locked.

NOTE TTimo:
  fullpath gives the full OS path to the dll that will potentially be loaded
	on win32 it's always in fs_basepath/<fs_game>/
	on linux it can be in fs_homepath/<fs_game>/ or fs_basepath/<fs_game>/
  the dll is extracted to fs_homepath (== fs_basepath on win32) if needed

  the return value doesn't tell wether file was extracted or not, it just says wether it's ok to continue
  (i.e. either the right file was extracted successfully, or it was already present)

  cvar_lastVersion is the optional name of a CVAR_ARCHIVE used to store the wolf version for the last extracted .so
  show_bug.cgi?id=463

==================
*/
qboolean FS_CL_ExtractFromPakFile( const char *fullpath, const char *gamedir, const char *filename, const char *cvar_lastVersion ) {
	int srcLength;
	int destLength;
	unsigned char   *srcData;
	unsigned char   *destData;
	qboolean needToCopy;
	FILE            *destHandle;

	needToCopy = qtrue;

	// read in compressed file
	srcLength = FS_ReadFile( filename, (void **)&srcData );

	// if its not in the pak, we bail
	if ( srcLength == -1 ) {
		return qfalse;
	}

	// read in local file
	destHandle = fopen( fullpath, "rb" );

	// if we have a local file, we need to compare the two
	if ( destHandle ) {
		fseek( destHandle, 0, SEEK_END );
		destLength = ftell( destHandle );
		fseek( destHandle, 0, SEEK_SET );

		if ( destLength > 0 ) {
			destData = (unsigned char*)Z_Malloc( destLength );

			fread( destData, 1, destLength, destHandle );

			// compare files
			if ( destLength == srcLength ) {
				int i;

				for ( i = 0; i < destLength; i++ ) {
					if ( destData[i] != srcData[i] ) {
						break;
					}
				}

				if ( i == destLength ) {
					needToCopy = qfalse;
				}
			}

			Z_Free( destData ); // TTimo
		}

		fclose( destHandle );
	}

	// write file
	if ( needToCopy ) {
		fileHandle_t f;

		// Com_DPrintf("FS_ExtractFromPakFile: FS_FOpenFileWrite '%s'\n", filename);
		f = FS_FOpenFileWrite( filename );
		if ( !f ) {
			Com_Printf( "Failed to open %s\n", filename );
			return qfalse;
		}

		FS_Write( srcData, srcLength, f );

		FS_FCloseFile( f );

#ifdef __linux__
		// show_bug.cgi?id=463
		// need to keep track of what versions we extract
		if ( cvar_lastVersion ) {
			Cvar_Set( cvar_lastVersion, Cvar_VariableString( "version" ) );
		}
#endif
	}

	FS_FreeFile( srcData );
	return qtrue;
}
#endif

/*
==============
FS_Delete
TTimo - this was not in the 1.30 filesystem code
using fs_homepath for the file to remove
==============
*/
int FS_Delete( char *filename ) {
	char *ospath;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !filename || filename[0] == 0 ) {
		return 0;
	}

	// for safety, only allow deletion from the save directory
	if ( String::NCmp( filename, "save/", 5 ) != 0 ) {
		return 0;
	}

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( remove( ospath ) != -1 ) {  // success
		return 1;
	}


	return 0;
}

//============================================================================

/*
================
FS_Startup
================
*/
static void FS_Startup( const char *gameName ) {
	Cvar  *fs;

	Com_Printf( "----- FS_Startup -----\n" );

	FS_SharedStartup();
	fs_basegame = Cvar_Get( "fs_basegame", "", CVAR_INIT );
	fs_gamedirvar = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );

	// add search path elements in reverse priority order
	if ( fs_cdpath->string[0] ) {
		FS_AddGameDirectory( fs_cdpath->string, gameName, ADDPACKS_None );
	}
	if ( fs_basepath->string[0] ) {
		FS_AddGameDirectory( fs_basepath->string, gameName, ADDPACKS_None );
	}
	// fs_homepath is somewhat particular to *nix systems, only add if relevant
	// NOTE: same filtering below for mods and basegame
	if ( fs_basepath->string[0] && String::ICmp( fs_homepath->string,fs_basepath->string ) ) {
		FS_AddGameDirectory( fs_homepath->string, gameName, ADDPACKS_None );
	}

	// check for additional base game so mods can be based upon other mods
	if ( fs_basegame->string[0] && !String::ICmp( gameName, BASEGAME ) && String::ICmp( fs_basegame->string, gameName ) ) {
		if ( fs_cdpath->string[0] ) {
			FS_AddGameDirectory( fs_cdpath->string, fs_basegame->string, ADDPACKS_None );
		}
		if ( fs_basepath->string[0] ) {
			FS_AddGameDirectory( fs_basepath->string, fs_basegame->string, ADDPACKS_None );
		}
		if ( fs_homepath->string[0] && String::ICmp( fs_homepath->string,fs_basepath->string ) ) {
			FS_AddGameDirectory( fs_homepath->string, fs_basegame->string, ADDPACKS_None );
		}
	}

	// check for additional game folder for mods
	if ( fs_gamedirvar->string[0] && !String::ICmp( gameName, BASEGAME ) && String::ICmp( fs_gamedirvar->string, gameName ) ) {
		if ( fs_cdpath->string[0] ) {
			FS_AddGameDirectory( fs_cdpath->string, fs_gamedirvar->string, ADDPACKS_None );
		}
		if ( fs_basepath->string[0] ) {
			FS_AddGameDirectory( fs_basepath->string, fs_gamedirvar->string, ADDPACKS_None );
		}
		if ( fs_homepath->string[0] && String::ICmp( fs_homepath->string,fs_basepath->string ) ) {
			FS_AddGameDirectory( fs_homepath->string, fs_gamedirvar->string, ADDPACKS_None );
		}
	}

	Com_ReadCDKey( BASEGAME );
	fs = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );
	if ( fs && fs->string[0] != 0 ) {
		Com_AppendCDKey( fs->string );
	}

	// show_bug.cgi?id=506
	// reorder the pure pk3 files according to server order
	FS_ReorderPurePaks();

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = qfalse; // We just loaded, it's not modified

	Com_Printf( "----------------------\n" );

#ifdef FS_MISSING
	if ( missingFiles == NULL ) {
		missingFiles = fopen( "\\missing.txt", "ab" );
	}
#endif
	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}

/*
=====================
FS_GamePureChecksum
Returns the checksum of the pk3 from which the server loaded the qagame.qvm
NOTE TTimo: this is not used in RTCW so far
=====================
*/
const char *FS_GamePureChecksum( void ) {
	static char info[MAX_STRING_TOKENS];
	searchpath_t *search;

	info[0] = 0;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( search->pack3 ) {
			if ( search->pack3->referenced & FS_QAGAME_REF ) {
				String::Sprintf( info, sizeof( info ), "%d", search->pack3->checksum );
			}
		}
	}

	return info;
}

#if defined( DO_LIGHT_DEDICATED )
static const int feeds[5] = {
	0xd6009839, 0x636bb1d5, 0x198df4c9, 0x7ffa631b, 0x8f89a69e
};

// counter to walk through the randomized list
static int feed_index = -1;

static int lookup_randomized[5] = { 0, 1, 2, 3, 4 };

/*
=====================
randomize the order of the 5 checksums we rely on
5 random swaps of the table
=====================
*/
void FS_InitRandomFeed() {
	int i, swap, aux;
	for ( i = 0; i < 5; i++ )
	{
		swap = (int)( 5.0 * rand() / ( RAND_MAX + 1.0 ) );
		aux = lookup_randomized[i]; lookup_randomized[i] = lookup_randomized[swap]; lookup_randomized[swap] = aux;
	}
}

/*
=====================
FS_RandChecksumFeed

Return a random checksum feed among our list
we keep the seed and use it when requested for the pure checksum
=====================
*/
int FS_RandChecksumFeed() {
	if ( feed_index == -1 ) {
		FS_InitRandomFeed();
	}
	feed_index = ( feed_index + 1 ) % 5;
	return feeds[lookup_randomized[feed_index]];
}

#endif

/*
================
FS_InitFilesystem

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void FS_InitFilesystem( void ) {
	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	fs_PrimaryBaseGame = BASEGAME;
	Com_StartupVariable( "fs_cdpath" );
	Com_StartupVariable( "fs_basepath" );
	Com_StartupVariable( "fs_homepath" );
	Com_StartupVariable( "fs_game" );
	Com_StartupVariable( "fs_copyfiles" );
	Com_StartupVariable( "fs_restrict" );

	// try to start up normally
	FS_Startup( BASEGAME );

#ifndef UPDATE_SERVER
	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
		// TTimo - added some verbosity, 'couldn't load default.cfg' confuses the hell out of users
		Com_Error( ERR_FATAL, "Couldn't load default.cfg - I am missing essential files - verify your installation?" );
	}
#endif

	String::NCpyZ( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	String::NCpyZ( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
}


/*
================
FS_Restart
================
*/
void FS_Restart( int checksumFeed ) {

	// free anything we currently have loaded
	FS_Shutdown();

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// clear pak references
	FS_ClearPakReferences( 0 );

	// try to start up normally
	FS_Startup( BASEGAME );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// (for instance a TA demo server)
		if ( lastValidBase[0] ) {
			FS_PureServerSetLoadedPaks( "", "" );
			Cvar_Set( "fs_basepath", lastValidBase );
			Cvar_Set( "fs_gamedirvar", lastValidGame );
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			Cvar_Set( "fs_restrict", "0" );
			FS_Restart( checksumFeed );
			Com_Error( ERR_DROP, "Invalid game folder\n" );
			return;
		}
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}

	// bk010116 - new check before safeMode
	if ( String::ICmp( fs_gamedirvar->string, lastValidGame ) ) {
		// skip the wolfconfig.cfg if "safe" is on the command line
		if ( !Com_SafeMode() ) {
			Cbuf_AddText( "exec wolfconfig_mp.cfg\n" );
		}
	}

	String::NCpyZ( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	String::NCpyZ( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );

}

/*
=================
FS_ConditionalRestart
restart if necessary

 FIXME TTimo
this doesn't catch all cases where an FS_Restart is necessary
see show_bug.cgi?id=478
=================
*/
qboolean FS_ConditionalRestart( int checksumFeed ) {
	if ( fs_gamedirvar->modified || checksumFeed != fs_checksumFeed ) {
		FS_Restart( checksumFeed );
		return qtrue;
	}
	return qfalse;
}

// CVE-2006-2082
// compared requested pak against the names as we built them in FS_ReferencedPakNames
qboolean FS_VerifyPak( const char *pak ) {
	char teststring[ BIG_INFO_STRING ];
	searchpath_t    *search;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		if ( search->pack3 ) {
			String::NCpyZ( teststring, search->pack3->pakGamename, sizeof( teststring ) );
			String::Cat( teststring, sizeof( teststring ), "/" );
			String::Cat( teststring, sizeof( teststring ), search->pack3->pakBasename );
			String::Cat( teststring, sizeof( teststring ), ".pk3" );
			if ( !String::ICmp( teststring, pak ) ) {
				return qtrue;
			}
		}
	}
	return qfalse;
}
