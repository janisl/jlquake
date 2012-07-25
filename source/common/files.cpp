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
/*
QUAKE3 FILESYSTEM

All of Quake's data access is through a hierarchical file system, but the contents of
the file system can be transparently merged from several sources.

A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
references outside the quake directory system.

The "base path" is the path to the directory holding all the game directories and usually
the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
command line to allow code debugging in a different directory.  Basepath cannot
be modified at all after startup.  Any files that are created (demos, screenshots,
etc) will be created reletive to the base path, so base path should usually be writable.

The "cd path" is the path to an alternate hierarchy that will be searched if a file
is not located in the base path.  A user can do a partial install that copies some
data to a base path created on their hard drive and leave the rest on the cd.  Files
are never writen to the cd path.  It defaults to a value set by the installer, like
"e:\quake3", but it can be overridden with "+set ds_cdpath g:\quake3".

If a user runs the game directly from a CD, the base path would be on the CD.  This
should still function correctly, but all file writes will fail (harmlessly).

The "home path" is the path used for all write access. On win32 systems we have "base path"
== "home path", but on *nix systems the base installation is usually readonly, and
"home path" points to ~/.q3a or similar

The user can also install custom mods and content in "home path", so it should be searched
along with "home path" and "cd path" for game content.


The "base game" is the directory under the paths where data comes from by default, and
can be either "baseq3" or "demoq3".

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base game.
This is the basis for addons.

Clients automatically set the game directory after receiving a gamestate from a server,
so only servers need to worry about +set fs_game.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

To save disk space and speed loading, directory trees can be collapsed into zip files.
The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
otherwise the are simply normal uncompressed zip files.  A game directory can have multiple
zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk3 distributed as a patch to override all existing data.

Because we will have updated executables freely available online, there is no point to
trying to restrict demo / oem versions of the game with code changes.  Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

If not running in restricted mode, and a file is not found in any local filesystem,
an attempt will be made to download it and save it under the base path.

File search order: when FS_FOpenFileRead gets called it will go through the fs_searchpaths
structure and stop on the first successful hit. fs_searchpaths is built with successive
calls to FS_AddGameDirectory

Additionaly, we search in several subdirectories:
current game is the current mode
base game is a variable to allow mods based on other mods
(such as baseq3 + missionpack content combination in a mod for instance)
BASEGAME is the hardcoded base game ("baseq3")

e.g. the qpath "sound/newstuff/test.wav" would be searched for in the following places:

home path + current game's zip files
home path + current game's directory
base path + current game's zip files
base path + current game's directory
cd path + current game's zip files
cd path + current game's directory

home path + base game's zip file
home path + base game's directory
base path + base game's zip file
base path + base game's directory
cd path + base game's zip file
cd path + base game's directory

home path + BASEGAME's zip file
home path + BASEGAME's directory
base path + BASEGAME's zip file
base path + BASEGAME's directory
cd path + BASEGAME's zip file
cd path + BASEGAME's directory

server download, to be written to home path + current game's directory


The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
load the file with a request to cache.  Only one file will be kept cached at a time,
so any models that are going to be referenced by both subsystems should alternate
between the CM_ load function and the ref load function.

TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
game is currently active.  This allows character models, skins, and sounds to be downloaded
to a common directory no matter which game is active.

How to prevent downloading zip files?
Pass pk3 file names in systeminfo, and download before FS_Restart()?

Aborting a download disconnects the client from the server.

How to mark files as downloadable?  Commercial add-ons won't be downloadable.

Non-commercial downloads will want to download the entire zip file.
the game would have to be reset to actually read the zip in

Auto-update information

Path separators

Casing

  separate server gamedir and client gamedir, so if the user starts
  a local game after having connected to a network game, it won't stick
  with the network game.

  allow menu options for game selection?

Read / write config to floppy option.

Different version coexistance?

When building a pak file, make sure a q3config.cfg isn't present in it,
or configs will never get loaded from disk!

  todo:

  downloading (outside fs?)
  game directory passing and restarting
*/
//**************************************************************************

#include "qcommon.h"
#include "unzip.h"

// number of id paks that will never be autodownloaded from baseq3
#define NUM_ID_PAKS         9

#define PAK3_COUNT          245
#define PAK3_CRC            1478

#define MAX_FILE_HANDLES    64

#define MAX_SEARCH_PATHS    4096

#define MAX_FILES_IN_PACK   4096

#define MAX_ZPATH           256
#define MAX_FILEHASH_SIZE   1024

#define IDPAKHEADER         (('K' << 24) + ('C' << 16) + ('A' << 8) + 'P')

#define MAX_PAKFILES        1024

//
// in memory
//

struct packfile_t
{
	char name[MAX_QPATH];
	int filepos;
	int filelen;
};

struct pack_t
{
	char filename[MAX_OSPATH];
	FILE* handle;
	int numfiles;
	packfile_t* files;
};

//
// on disk
//
struct dpackheader_t
{
	qint32 ident;			// == IDPAKHEADER
	qint32 dirofs;
	qint32 dirlen;
};

struct dpackfile_t
{
	char name[56];
	qint32 filepos;
	qint32 filelen;
};

struct fileInPack_t
{
	char* name;					// name of the file
	unsigned long pos;			// file info position in zip
	fileInPack_t* next;			// next file in the hash
};

struct pack3_t
{
	char pakFilename[MAX_OSPATH];				// c:\quake3\baseq3\pak0.pk3
	char pakBasename[MAX_OSPATH];				// pak0
	char pakGamename[MAX_OSPATH];				// baseq3
	unzFile handle;								// handle to zip file
	int checksum;								// regular checksum
	int pure_checksum;							// checksum for pure
	int numfiles;								// number of files in pk3
	int referenced;								// referenced file flags
	int hashSize;								// hash table size (power of 2)
	fileInPack_t** hashTable;					// hash table
	fileInPack_t* buildBuffer;					// buffer with the filenames etc.
};

struct directory_t
{
	char path[MAX_OSPATH];					// c:\quake3
	char gamedir[MAX_OSPATH];				// baseq3
	char fullname[MAX_OSPATH];
};

struct searchpath_t
{
	searchpath_t* next;

	pack_t* pack;
	pack3_t* pack3;				// only one of pack / pack3 / dir will be non NULL
	directory_t* dir;

	searchpath_t()
	: next(NULL)
	, pack(NULL)
	, pack3(NULL)
	, dir(NULL)
	{}
};

struct filelink_t
{
	filelink_t* next;
	char* from;
	int fromlength;
	char* to;
};

union qfile_gut
{
	FILE* o;
	unzFile z;
};

struct qfile_ut
{
	qfile_gut file;
	qboolean unique;
};

struct fileHandleData_t
{
	qfile_ut handleFiles;
	qboolean handleSync;
	int baseOffset;
	int fileSize;
	int zipFilePos;
	qboolean zipFile;
	bool pakFile;
	char name[MAX_ZPATH];
};

struct officialpak_t
{
	char pakname[MAX_QPATH];
	bool ok;
};

void S_ClearSoundBuffer(bool killStreaming);
void FS_Restart(int checksumFeed);
bool CL_WWWBadChecksum(const char* pakname);

//	For HexenWorld
bool com_portals = false;
const char* fs_PrimaryBaseGame;
bool fs_ProtectKeyFile;

static fileHandleData_t fsh[MAX_FILE_HANDLES];

int fs_packFiles;								// total number of files in packs
int fs_checksumFeed;
static searchpath_t* fs_searchpaths;
static searchpath_t* fs_base_searchpaths;		// without gamedirs

char fs_gamedir[MAX_OSPATH];					// this will be a single file name with no separators
Cvar* fs_homepath;
Cvar* fs_basepath;
static Cvar* fs_debug;

static filelink_t* fs_links;

// never load anything from pk3 files that are not present at the server when pure
static int fs_numServerPaks;
static int fs_serverPaks[MAX_SEARCH_PATHS];								// checksums
static char* fs_serverPakNames[MAX_SEARCH_PATHS];						// pk3 names

static int fs_fakeChkSum;

// only used for autodownload, to make sure the client has at least
// all the pk3 files that are referenced at the server side
static int fs_numServerReferencedPaks;
static int fs_serverReferencedPaks[MAX_SEARCH_PATHS];						// checksums
static char* fs_serverReferencedPakNames[MAX_SEARCH_PATHS];					// pk3 names

// TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
// wether we did a reorder on the current search path when joining the server
static bool fs_reordered;

static int fs_filter_flag = 0;

//**************************************************************************
//
//	Internal utilities
//
//**************************************************************************

//	Return a hash value for the filename
static int FS_HashFileName(const char* fname, int hashSize)
{
	int hash = 0;
	int i = 0;
	while (fname[i] != '\0')
	{
		char letter = String::ToLower(fname[i]);
		if (letter == '.')
		{
			break;				// don't include extension
		}
		if (letter == '\\')
		{
			letter = '/';		// damn path names
		}
		if (letter == PATH_SEP)
		{
			letter = '/';		// damn path names
		}
		hash += (int)(letter) * (i + 119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize - 1);
	return hash;
}

//	Ignore case and seprator char distinctions
static int FS_FilenameCompare(const char* s1, const char* s2)
{
	int c1;
	do
	{
		c1 = *s1++;
		int c2 = *s2++;

		if (c1 >= 'a' && c1 <= 'z')
		{
			c1 -= ('a' - 'A');
		}
		if (c2 >= 'a' && c2 <= 'z')
		{
			c2 -= ('a' - 'A');
		}

		if (c1 == '\\' || c1 == ':')
		{
			c1 = '/';
		}
		if (c2 == '\\' || c2 == ':')
		{
			c2 = '/';
		}

		if (c1 < c2)
		{
			return -1;		// strings not equal
		}
		if (c1 > c2)
		{
			return 1;
		}
	}
	while (c1);

	return 0;		// strings are equal
}

static int paksort(const void* a, const void* b)
{
	char* aa = *(char**)a;
	char* bb = *(char**)b;

	return FS_FilenameCompare(aa, bb);
}

//	Fix things up differently for win/unix/mac
static void FS_ReplaceSeparators(char* path)
{
	for (char* s = path; *s; s++)
	{
		if (*s == '/' || *s == '\\')
		{
			*s = PATH_SEP;
		}
	}
}

//	Qpath may have either forward or backwards slashes
char* FS_BuildOSPath(const char* base, const char* game, const char* qpath)
{
	char temp[MAX_OSPATH];
	static char ospath[2][MAX_OSPATH];
	static int toggle;

	toggle ^= 1;		// flip-flop to allow two returns without clash

	if (!game || !game[0])
	{
		game = fs_gamedir;
	}

	String::Sprintf(temp, sizeof(temp), "/%s/%s", game, qpath);
	FS_ReplaceSeparators(temp);
	String::Sprintf(ospath[toggle], sizeof(ospath[0]), "%s%s", base, temp);

	return ospath[toggle];
}

//	Creates any directories needed to store the given filename
bool FS_CreatePath(const char* OSPath_)
{
	// use va() to have a clean const char* prototype
	char* OSPath = va("%s", OSPath_);
	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if (strstr(OSPath, "..") || strstr(OSPath, "::"))
	{
		common->Printf("WARNING: refusing to create relative path \"%s\"\n", OSPath);
		return true;
	}

	for (char* ofs = OSPath + 1; *ofs; ofs++)
	{
		if (*ofs == PATH_SEP)
		{
			// create the directory
			*ofs = 0;
			Sys_Mkdir(OSPath);
			*ofs = PATH_SEP;
		}
	}
	return false;
}

//	Copy a fully specified file from one place to another
void FS_CopyFile(const char* fromOSPath, const char* toOSPath)
{
	if (fs_debug->integer)
	{
		common->Printf("copy %s to %s\n", fromOSPath, toOSPath);
	}

	if (strstr(fromOSPath, "journal.dat") || strstr(fromOSPath, "journaldata.dat"))
	{
		common->Printf("Ignoring journal files\n");
		return;
	}

	FILE* f = fopen(fromOSPath, "rb");
	if (!f)
	{
		return;
	}
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);

	void* buf = Mem_Alloc(len);
	if ((int)fread(buf, 1, len, f) != len)
	{
		throw Exception("Short read in FS_Copyfiles()\n");
	}
	fclose(f);

	if (FS_CreatePath(toOSPath))
	{
		Mem_Free(buf);
		return;
	}

	f = fopen(toOSPath, "wb");
	if (!f)
	{
		Mem_Free(buf);
		return;
	}
	if ((int)fwrite(buf, 1, len, f) != len)
	{
		throw Exception("Short write in FS_Copyfiles()\n");
	}
	fclose(f);
	Mem_Free(buf);
}

void FS_CopyFileOS(const char* from, const char* to)
{
	char* fromOSPath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, from);
	char* toOSPath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, to);
	FS_CopyFile(fromOSPath, toOSPath);
}

void FS_Remove(const char* osPath)
{
	remove(osPath);
}

static bool FS_AllowDeletion(const char* filename)
{
	// for safety, only allow deletion from the save, profiles and demo directory
	if (String::NCmp(filename, "save/", 5) != 0 &&
		String::NCmp(filename, "profiles/", 9) != 0 &&
		String::NCmp(filename, "demos/", 6) != 0)
	{
		return false;
	}

	return true;
}

static int FS_DeleteDir(const char* dirname, bool nonEmpty, bool recursive)
{
	if (!fs_searchpaths)
	{
		common->FatalError("Filesystem call made without initialization\n");
	}

	if (!dirname || dirname[0] == 0)
	{
		return 0;
	}

	if (!FS_AllowDeletion(dirname))
	{
		return 0;
	}

	if (recursive)
	{
		char* ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, dirname);
		int nFiles = 0;
		char** pFiles = Sys_ListFiles(ospath, "/", NULL, &nFiles, false);
		for (int i = 0; i < nFiles; i++)
		{
			if (!String::ICmp(pFiles[i], "..") || !String::ICmp(pFiles[i], "."))
			{
				continue;
			}

			char temp[MAX_OSPATH];
			String::Sprintf(temp, sizeof(temp), "%s/%s", dirname, pFiles[i]);

			if (!FS_DeleteDir(temp, nonEmpty, recursive))
			{
				return 0;
			}
		}
		Sys_FreeFileList(pFiles);
	}

	if (nonEmpty)
	{
		char* ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, dirname);
		int nFiles = 0;
		char** pFiles = Sys_ListFiles(ospath, NULL, NULL, &nFiles, false);
		for (int i = 0; i < nFiles; i++)
		{
			ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s/%s", dirname, pFiles[i]));

			if (remove(ospath) == -1)
			{
				// failure
				return 0;
			}
		}
		Sys_FreeFileList(pFiles);
	}

	char* ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, dirname);

	if (Sys_Rmdir(ospath) == 0)
	{
		return 1;
	}

	return 0;
}

//	using fs_homepath for the file to remove
int FS_Delete(const char* filename)
{
	if (!fs_searchpaths)
	{
		common->FatalError("Filesystem call made without initialization\n");
	}

	if (!filename || filename[0] == 0)
	{
		return 0;
	}

	if (!FS_AllowDeletion(filename))
	{
		return 0;
	}

	char* ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, filename);

	int stat = Sys_StatFile(ospath);
	if (stat == -1)
	{
		return 0;
	}

	if (stat == 1)
	{
		return FS_DeleteDir(filename, true, true);
	}
	else
	{
		if (remove(ospath) != -1)
		{
			// success
			return 1;
		}
	}

	return 0;
}

//**************************************************************************
//
//	Building the search path.
//
//**************************************************************************

//	Takes an explicit (not game tree related) path to a pak file.
//
//	Loads the header and directory, adding the files at the beginning
// of the list so they override previous pack files.
static pack_t* FS_LoadPackFile(const char* packfile)
{
	dpackheader_t header;
	int i;
	packfile_t* newfiles;
	int numpackfiles;
	pack_t* pack;
	FILE* packhandle;
	dpackfile_t info[MAX_FILES_IN_PACK];
	unsigned short crc;

	packhandle = fopen(packfile, "rb");
	if (!packhandle)
	{
		return NULL;
	}

	fread(&header, 1, sizeof(header), packhandle);
	if (LittleLong(header.ident) != IDPAKHEADER)
	{
		throw Exception(va("%s is not a packfile", packfile));
	}
	header.dirofs = LittleLong(header.dirofs);
	header.dirlen = LittleLong(header.dirlen);

	numpackfiles = header.dirlen / sizeof(dpackfile_t);

	if (numpackfiles > MAX_FILES_IN_PACK)
	{
		throw Exception(va("%s has %i files", packfile, numpackfiles));
	}

	newfiles = new packfile_t[numpackfiles];

	fseek(packhandle, header.dirofs, SEEK_SET);
	fread(info, 1, header.dirlen, packhandle);

	// crc the directory to check for modifications
	CRC_Init(&crc);
	for (i = 0; i < header.dirlen; i++)
	{
		CRC_ProcessByte(&crc, ((byte*)info)[i]);
	}

	if ((GGameType & GAME_HexenWorld) && numpackfiles == PAK3_COUNT && crc == PAK3_CRC)
	{
		com_portals = true;
	}

	// parse the directory
	for (i = 0; i < numpackfiles; i++)
	{
		String::Cpy(newfiles[i].name, info[i].name);
		newfiles[i].filepos = LittleLong(info[i].filepos);
		newfiles[i].filelen = LittleLong(info[i].filelen);
	}

	pack = new pack_t;
	String::Cpy(pack->filename, packfile);
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;

	common->Printf("Added packfile %s (%i files)\n", packfile, numpackfiles);
	return pack;
}

//	Creates a new pak_t in the search chain for the contents of a zip file.
static pack3_t* FS_LoadZipFile(const char* zipfile, const char* basename)
{
	fileInPack_t* buildBuffer;
	pack3_t* pack;
	unzFile uf;
	int err;
	unz_global_info gi;
	char filename_inzip[MAX_ZPATH];
	unz_file_info file_info;
	int i, len;
	int hash;
	int fs_numHeaderLongs;
	int* fs_headerLongs;
	char* namePtr;

	fs_numHeaderLongs = 0;

	uf = unzOpen(zipfile);
	err = unzGetGlobalInfo(uf, &gi);

	if (err != UNZ_OK)
	{
		return NULL;
	}

	fs_packFiles += gi.number_entry;

	len = 0;
	unzGoToFirstFile(uf);
	for (i = 0; i < (int)gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK)
		{
			break;
		}
		len += String::Length(filename_inzip) + 1;
		unzGoToNextFile(uf);
	}

	buildBuffer = (fileInPack_t*)Mem_Alloc((gi.number_entry * sizeof(fileInPack_t)) + len);
	namePtr = ((char*)buildBuffer) + gi.number_entry * sizeof(fileInPack_t);
	fs_headerLongs = (int*)Mem_Alloc(gi.number_entry * sizeof(int));

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for (i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1)
	{
		if (i > (int)gi.number_entry)
		{
			break;
		}
	}

	pack = (pack3_t*)Mem_Alloc(sizeof(pack3_t) + i * sizeof(fileInPack_t*));
	pack->hashSize = i;
	pack->hashTable = (fileInPack_t**)(((char*)pack) + sizeof(pack3_t));
	for (i = 0; i < pack->hashSize; i++)
	{
		pack->hashTable[i] = NULL;
	}

	String::NCpyZ(pack->pakFilename, zipfile, sizeof(pack->pakFilename));
	String::NCpyZ(pack->pakBasename, basename, sizeof(pack->pakBasename));

	// strip .pk3 if needed
	if (String::Length(pack->pakBasename) > 4 && !String::ICmp(pack->pakBasename +
			String::Length(pack->pakBasename) - 4, ".pk3"))
	{
		pack->pakBasename[String::Length(pack->pakBasename) - 4] = 0;
	}

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile(uf);

	for (i = 0; i < (int)gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK)
		{
			break;
		}
		if (file_info.uncompressed_size > 0)
		{
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong(file_info.crc);
		}
		String::ToLower(filename_inzip);
		hash = FS_HashFileName(filename_inzip, pack->hashSize);
		buildBuffer[i].name = namePtr;
		String::Cpy(buildBuffer[i].name, filename_inzip);
		namePtr += String::Length(filename_inzip) + 1;
		// store the file position in the zip
		unzGetCurrentFileInfoPosition(uf, &buildBuffer[i].pos);
		//
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum = Com_BlockChecksum(fs_headerLongs, 4 * fs_numHeaderLongs);
	pack->pure_checksum = Com_BlockChecksumKey(fs_headerLongs, 4 * fs_numHeaderLongs, LittleLong(fs_checksumFeed));
	pack->checksum = LittleLong(pack->checksum);
	pack->pure_checksum = LittleLong(pack->pure_checksum);

	Mem_Free(fs_headerLongs);

	pack->buildBuffer = buildBuffer;
	return pack;
}

//	Sets fs_gamedir, adds the directory to the head of the path,
// then loads the pak1.pak pak2.pak ... and zip headers
void FS_AddGameDirectory(const char* path, const char* dir, int AddPacks)
{
	for (searchpath_t* sp = fs_searchpaths; sp; sp = sp->next)
	{
		if (sp->dir && !String::ICmp(sp->dir->path, path) && !String::ICmp(sp->dir->gamedir, dir))
		{
			return;			// we've already got this one
		}
	}

	String::NCpyZ(fs_gamedir, dir, sizeof(fs_gamedir));

	//
	// add the directory to the search path
	//
	searchpath_t* search = new searchpath_t;
	search->dir = new directory_t;
	String::NCpyZ(search->dir->path, path, sizeof(search->dir->path));
	String::NCpyZ(search->dir->gamedir, dir, sizeof(search->dir->gamedir));
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	//
	// add any pak files in the format pak0.pak pak1.pak, ...
	//
	if (AddPacks != ADDPACKS_None)
	{
		for (int i = 0; AddPacks != ADDPACKS_First10 || i < 10; i++)
		{
			char* pakfile = FS_BuildOSPath(path, dir, va("pak%i.pak", i));
			pack_t* pak = FS_LoadPackFile(pakfile);
			if (!pak)
			{
				if (AddPacks == ADDPACKS_UntilMissing)
				{
					break;
				}
				continue;
			}
			search = new searchpath_t;
			search->pack = pak;
			search->next = fs_searchpaths;
			fs_searchpaths = search;
		}
	}

	// find all .pk3 files in this directory
	char* pakfile = FS_BuildOSPath(path, dir, "");
	pakfile[String::Length(pakfile) - 1] = 0;	// strip the trailing slash

	int numfiles;
	char** pakfiles = Sys_ListFiles(pakfile, ".pk3", NULL, &numfiles, false);

	// sort them so that later alphabetic matches override
	// earlier ones.  This makes pak1.pk3 override pak0.pk3
	if (numfiles > MAX_PAKFILES)
	{
		numfiles = MAX_PAKFILES;
	}
	char* sorted[MAX_PAKFILES];
	for (int i = 0; i < numfiles; i++)
	{
		sorted[i] = pakfiles[i];
		if (((GGameType & GAME_WolfSP) && !String::NCmp(sorted[i], "sp_", 3)) ||
			((GGameType & GAME_WolfMP) && !String::NCmp(sorted[i], "mp_", 3)))
		{
			Com_Memcpy(sorted[i], "zz", 2);
		}
	}

	qsort(sorted, numfiles, sizeof(char*), paksort);

	for (int i = 0; i < numfiles; i++)
	{
		if ((GGameType & GAME_WolfSP) && !String::NCmp(sorted[i], "mp_", 3))
		{
			continue;
		}
		if ((GGameType & GAME_WolfMP) && !String::NCmp(sorted[i], "sp_", 3))
		{
			continue;
		}

		//	Fix filenames broken in mp/sp/pak sort above
		if (!String::NCmp(sorted[i], "zz_", 3))
		{
			if (GGameType & GAME_WolfSP)
			{
				Com_Memcpy(sorted[i], "sp", 2);
			}
			if (GGameType & GAME_WolfMP)
			{
				Com_Memcpy(sorted[i], "mp", 2);
			}
		}

		pakfile = FS_BuildOSPath(path, dir, sorted[i]);
		pack3_t* pak = FS_LoadZipFile(pakfile, sorted[i]);
		if (!pak)
		{
			continue;
		}
		// store the game name for downloading
		String::Cpy(pak->pakGamename, dir);

		search = new searchpath_t;
		search->pack3 = pak;
		search->next = fs_searchpaths;
		fs_searchpaths = search;
	}

	// done
	Sys_FreeFileList(pakfiles);
}

//**************************************************************************
//
//	Reading and writing files.
//
//**************************************************************************

static fileHandle_t FS_HandleForFile()
{
	for (int i = 1; i < MAX_FILE_HANDLES; i++)
	{
		if (fsh[i].handleFiles.file.o == NULL)
		{
			return i;
		}
	}
	throw DropException("FS_HandleForFile: none free");
}

static FILE* FS_FileForHandle(fileHandle_t f)
{
	if (f < 0 || f > MAX_FILE_HANDLES)
	{
		throw DropException("FS_FileForHandle: out of reange");
	}
	if (fsh[f].zipFile == true)
	{
		throw DropException("FS_FileForHandle: can't get FILE on zip file");
	}
	if (!fsh[f].handleFiles.file.o)
	{
		throw DropException("FS_FileForHandle: NULL");
	}

	return fsh[f].handleFiles.file.o;
}

static bool FS_PakIsPure(pack3_t* pack)
{
	if (fs_numServerPaks)
	{
		for (int i = 0; i < fs_numServerPaks; i++)
		{
			// FIXME: also use hashed file names
			// NOTE TTimo: a pk3 with same checksum but different name would be validated too
			//   I don't see this as allowing for any exploit, it would only happen if the client does manips of it's file names 'not a bug'
			if (pack->checksum == fs_serverPaks[i])
			{
				return true;		// on the aproved list
			}
		}
		return false;	// not on the pure server pak list
	}
	return true;
}

//	If this is called on a non-unique FILE (from a pak file), it will return
// the size of the pak file, not the expected size of the file.
static int FS_filelength(fileHandle_t f)
{
	FILE* h = FS_FileForHandle(f);
	int pos = ftell(h);
	fseek(h, 0, SEEK_END);
	int end = ftell(h);
	fseek(h, pos, SEEK_SET);

	return end;
}

//	Finds the file in the search path. Returns filesize and an open file
// handle. Used for streaming data out of either a separate file or a ZIP
// file.
int FS_FOpenFileRead(const char* filename, fileHandle_t* file, bool uniqueFILE)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!file)
	{
		// just wants to see if file is there
		for (searchpath_t* search = fs_searchpaths; search; search = search->next)
		{
			// is the element a pak file?
			if (search->pack3)
			{
				if (fs_filter_flag & FS_EXCLUDE_PK3)
				{
					continue;
				}

				int hash = FS_HashFileName(filename, search->pack3->hashSize);
				if (!search->pack3->hashTable[hash])
				{
					continue;
				}
				// look through all the pak file elements
				pack3_t* pak = search->pack3;
				fileInPack_t* pakFile = pak->hashTable[hash];
				do
				{
					// case and separator insensitive comparisons
					if (!FS_FilenameCompare(pakFile->name, filename))
					{
						// found it!
						return true;
					}
					pakFile = pakFile->next;
				}
				while (pakFile != NULL);
			}
			else if (search->pack)
			{
				if (fs_filter_flag & FS_EXCLUDE_PK3)
				{
					continue;
				}

				// look through all the pak file elements
				pack_t* pak = search->pack;
				for (int i = 0; i < pak->numfiles; i++)
				{
					if (!FS_FilenameCompare(pak->files[i].name, filename))
					{
						// found it!
						return true;
					}
				}
			}
			else if (search->dir)
			{
				if (fs_filter_flag & FS_EXCLUDE_DIR)
				{
					continue;
				}

				directory_t* dir = search->dir;

				char* netpath = FS_BuildOSPath(dir->path, dir->gamedir, filename);
				FILE* temp = fopen(netpath, "rb");
				if (!temp)
				{
					continue;
				}
				fclose(temp);
				return true;
			}
		}
		return false;
	}

	if (!filename)
	{
		throw Exception("FS_FOpenFileRead: NULL 'filename' parameter passed\n");
	}

	// qpaths are not supposed to have a leading slash
	if (filename[0] == '/' || filename[0] == '\\')
	{
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if (strstr(filename, "..") || strstr(filename, "::"))
	{
		*file = 0;
		return -1;
	}

	// make sure the q3key file is only readable at initialization, any other
	// time the key should only be accessed in memory using the provided functions
	if (fs_ProtectKeyFile && (strstr(filename, "q3key") || strstr(filename, "rtcwkey") || strstr(filename, "etkey")))
	{
		*file = 0;
		return -1;
	}

	*file = FS_HandleForFile();
	fsh[*file].handleFiles.unique = uniqueFILE;

	if (fs_numServerPaks)
	{
		// check for links first
		for (filelink_t* link = fs_links; link; link = link->next)
		{
			if (!String::NCmp(filename, link->from, link->fromlength))
			{
				char netpath[MAX_OSPATH];
				String::Sprintf(netpath, sizeof(netpath), "%s%s", link->to, filename + link->fromlength);
				fsh[*file].handleFiles.file.o = fopen(netpath, "rb");
				if (fsh[*file].handleFiles.file.o)
				{
					if (fs_debug->integer)
					{
						common->Printf("link file: %s\n", netpath);
					}
					return FS_filelength(*file);
				}
				return -1;
			}
		}
	}

	//
	// search through the path, one element at a time
	//
	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pk3 file?
		if (search->pack3)
		{
			if (fs_filter_flag & FS_EXCLUDE_PK3)
			{
				continue;
			}

			int hash = FS_HashFileName(filename, search->pack3->hashSize);
			if (!search->pack3->hashTable[hash])
			{
				continue;
			}
			// disregard if it doesn't match one of the allowed pure pak files
			if (!FS_PakIsPure(search->pack3))
			{
				continue;
			}

			// look through all the pak file elements
			pack3_t* pak = search->pack3;
			fileInPack_t* pakFile = pak->hashTable[hash];
			do
			{
				// case and separator insensitive comparisons
				if (!FS_FilenameCompare(pakFile->name, filename))
				{
					// found it!

					// mark the pak as having been referenced and mark specifics on cgame and ui
					// shaders, txt, arena files  by themselves do not count as a reference as
					// these are loaded from all pk3s
					// from every pk3 file..
					int l = String::Length(filename);
					if (!(pak->referenced & FS_GENERAL_REF))
					{
						if (String::ICmp(filename + l - 7, ".shader") != 0 &&
							String::ICmp(filename + l - 4, ".txt") != 0 &&
							String::ICmp(filename + l - 4, ".cfg") != 0 &&
							String::ICmp(filename + l - 7, ".config") != 0 &&
							strstr(filename, "levelshots") == NULL &&
							String::ICmp(filename + l - 4, ".bot") != 0 &&
							String::ICmp(filename + l - 6, ".arena") != 0 &&
							String::ICmp(filename + l - 5, ".menu") != 0)
						{
							pak->referenced |= FS_GENERAL_REF;
						}
					}

					if (!(pak->referenced & FS_QAGAME_REF) && (strstr(filename, "qagame.qvm") ||
															   strstr(filename, "qagame_mp_x86.dll") || strstr(filename, "qagame.mp.i386.so") ||
															   strstr(filename, "qagame_mp_x86_64.dll") || strstr(filename, "qagame.mp.x86_64.so")))
					{
						pak->referenced |= FS_QAGAME_REF;
					}
					if (!(pak->referenced & FS_CGAME_REF) && (strstr(filename, "cgame.qvm") ||
															  strstr(filename, "cgame_mp_x86.dll") || strstr(filename, "cgame.mp.i386.so") ||
															  strstr(filename, "cgame_mp_x86_64.dll") || strstr(filename, "cgame.mp.x86_64.so")))
					{
						pak->referenced |= FS_CGAME_REF;
					}
					if (!(pak->referenced & FS_UI_REF) && (strstr(filename, "ui.qvm") ||
														   strstr(filename, "ui_mp_x86.dll") || strstr(filename, "ui.mp.i386.so") ||
														   strstr(filename, "ui_mp_x86_64.dll") || strstr(filename, "ui.mp.x86_64.so")))
					{
						pak->referenced |= FS_UI_REF;
					}

					// Don't allow maps to be loaded from pak0 (singleplayer)
					if ((GGameType & GAME_WolfMP) &&
						String::ICmp(filename + l - 4, ".bsp") == 0 &&
						String::ICmp(pak->pakBasename, "pak0") == 0)
					{
						*file = 0;
						return -1;
					}

					if (uniqueFILE)
					{
						// open a new file on the pakfile
						fsh[*file].handleFiles.file.z = unzReOpen(pak->pakFilename, pak->handle);
						if (fsh[*file].handleFiles.file.z == NULL)
						{
							throw Exception(va("Couldn't reopen %s", pak->pakFilename));
						}
					}
					else
					{
						fsh[*file].handleFiles.file.z = pak->handle;
					}
					String::NCpyZ(fsh[*file].name, filename, sizeof(fsh[*file].name));
					fsh[*file].zipFile = true;
					fsh[*file].pakFile = false;
					unz_s* zfi = (unz_s*)fsh[*file].handleFiles.file.z;
					// in case the file was new
					FILE* temp = zfi->file;
					// set the file position in the zip file (also sets the current file info)
					unzSetCurrentFileInfoPosition(pak->handle, pakFile->pos);
					// copy the file info into the unzip structure
					Com_Memcpy(zfi, pak->handle, sizeof(unz_s));
					// we copy this back into the structure
					zfi->file = temp;
					// open the file in the zip
					unzOpenCurrentFile(fsh[*file].handleFiles.file.z);
					fsh[*file].zipFilePos = pakFile->pos;

					if (fs_debug->integer)
					{
						common->Printf("FS_FOpenFileRead: %s (found in '%s')\n",
							filename, pak->pakFilename);
					}
					return zfi->cur_file_info.uncompressed_size;
				}
				pakFile = pakFile->next;
			}
			while (pakFile != NULL);
		}
		// is the element a pak file?
		else if (search->pack)
		{
			// look through all the pak file elements
			pack_t* pak = search->pack;
			for (int i = 0; i < pak->numfiles; i++)
			{
				if (!FS_FilenameCompare(pak->files[i].name, filename))
				{
					// found it!
					if (uniqueFILE)
					{
						// open a new file on the pakfile
						fsh[*file].handleFiles.file.o = fopen(pak->filename, "rb");
						if (!fsh[*file].handleFiles.file.o)
						{
							throw Exception(va("Couldn't reopen %s", pak->filename));
						}
					}
					else
					{
						fsh[*file].handleFiles.file.o = pak->handle;
					}
					String::NCpyZ(fsh[*file].name, filename, sizeof(fsh[*file].name));
					fsh[*file].zipFile = false;
					fsh[*file].pakFile = true;
					fsh[*file].baseOffset = pak->files[i].filepos;
					fsh[*file].fileSize = pak->files[i].filelen;
					fseek(fsh[*file].handleFiles.file.o, pak->files[i].filepos, SEEK_SET);
					if (fs_debug->integer)
					{
						common->Printf("FS_FOpenFileRead: %s (found in '%s')\n",
							filename, pak->filename);
					}
					return pak->files[i].filelen;
				}
			}
		}
		else if (search->dir)
		{
			if (fs_filter_flag & FS_EXCLUDE_DIR)
			{
				continue;
			}

			// check a file in the directory tree

			// if we are running restricted, the only files we
			// will allow to come from the directory are .cfg files
			int l = String::Length(filename);
			// if you are using FS_ReadFile to find out if a file exists,
			//   this test can make the search fail although the file is in the directory
			// I had the problem on https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=8
			// turned out I used FS_FileExists instead
			if (fs_numServerPaks)
			{
				if (String::ICmp(filename + l - 4, ".cfg") &&		// for config files
					String::ICmp(filename + l - 5, ".menu") &&	// menu files
					String::ICmp(filename + l - 4, ".svg") &&	// savegames
					String::ICmp(filename + l - 5, ".game") &&	// menu files
					String::ICmp(filename + l - 6, ".dm_49") &&	// menu files
					String::ICmp(filename + l - 6, ".dm_60") &&	// menu files
					String::ICmp(filename + l - 6, ".dm_68") &&	// menu files
					String::ICmp(filename + l - 6, ".dm_84") &&	// menu files
					String::ICmp(filename + l - 4, ".dat") &&		// for journal files
					String::ICmp(filename + l - 8, "bots.txt") &&
					String::ICmp(filename + l - 8, ".botents"))
				{
					continue;
				}
			}

			directory_t* dir = search->dir;

			char* netpath = FS_BuildOSPath(dir->path, dir->gamedir, filename);
			fsh[*file].handleFiles.file.o = fopen(netpath, "rb");
			if (!fsh[*file].handleFiles.file.o)
			{
				continue;
			}

			if (String::ICmp(filename + l - 4, ".cfg") &&		// for config files
				String::ICmp(filename + l - 5, ".menu") &&	// menu files
				String::ICmp(filename + l - 5, ".game") &&	// menu files
				String::ICmp(filename + l - 6, ".dm_49") &&	// menu files
				String::ICmp(filename + l - 6, ".dm_60") &&	// menu files
				String::ICmp(filename + l - 6, ".dm_68") &&	// menu files
				String::ICmp(filename + l - 6, ".dm_84") &&	// menu files
				String::ICmp(filename + l - 4, ".dat") &&	// for journal files
				String::ICmp(filename + l - 8, ".botents"))
			{
				fs_fakeChkSum = random();
			}

			String::NCpyZ(fsh[*file].name, filename, sizeof(fsh[*file].name));
			fsh[*file].zipFile = false;
			fsh[*file].pakFile = false;
			if (fs_debug->integer)
			{
				common->Printf("FS_FOpenFileRead: %s (found in '%s/%s')\n", filename,
					dir->path, dir->gamedir);
			}

			return FS_filelength(*file);
		}
	}

	common->DPrintf("Can't find %s\n", filename);

	*file = 0;
	return -1;
}

int FS_FOpenFileRead_Filtered(const char* qpath, fileHandle_t* file, bool uniqueFILE, int filter_flag)
{
	fs_filter_flag = filter_flag;
	int ret = FS_FOpenFileRead(qpath, file, uniqueFILE);
	fs_filter_flag = 0;
	return ret;
}

fileHandle_t FS_FOpenFileWrite(const char* filename)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	fileHandle_t f = FS_HandleForFile();
	fsh[f].zipFile = false;
	fsh[f].pakFile = false;

	char* ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, filename);

	if (fs_debug->integer)
	{
		common->Printf("FS_FOpenFileWrite: %s\n", ospath);
	}

	if (FS_CreatePath(ospath))
	{
		return 0;
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//common->DPrintf("writing to: %s\n", ospath);
	fsh[f].handleFiles.file.o = fopen(ospath, "wb");

	String::NCpyZ(fsh[f].name, filename, sizeof(fsh[f].name));

	fsh[f].handleSync = false;
	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}
	return f;
}

static fileHandle_t FS_FOpenFileAppend(const char* filename)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	fileHandle_t f = FS_HandleForFile();
	fsh[f].zipFile = false;
	fsh[f].pakFile = false;

	String::NCpyZ(fsh[f].name, filename, sizeof(fsh[f].name));

	// don't let sound stutter
	S_ClearSoundBuffer(false);

	char* ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, filename);

	if (fs_debug->integer)
	{
		common->Printf("FS_FOpenFileAppend: %s\n", ospath);
	}

	if (FS_CreatePath(ospath))
	{
		return 0;
	}

	fsh[f].handleFiles.file.o = fopen(ospath, "ab");
	fsh[f].handleSync = false;
	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}
	return f;
}

//	Handle based file calls for virtual machines
int FS_FOpenFileByMode(const char* qpath, fileHandle_t* f, fsMode_t mode)
{
	int r;

	bool sync = false;

	switch (mode)
	{
	case FS_READ:
		r = FS_FOpenFileRead(qpath, f, true);
		break;
	case FS_WRITE:
		*f = FS_FOpenFileWrite(qpath);
		r = 0;
		if (*f == 0)
		{
			r = -1;
		}
		break;
	case FS_APPEND_SYNC:
		sync = true;
	case FS_APPEND:
		*f = FS_FOpenFileAppend(qpath);
		r = 0;
		if (*f == 0)
		{
			r = -1;
		}
		break;
	default:
		throw Exception("FSH_FOpenFile: bad mode");
	}

	if (!f)
	{
		return r;
	}

	if (*f)
	{
		if (fsh[*f].zipFile == true)
		{
			fsh[*f].baseOffset = unztell(fsh[*f].handleFiles.file.z);
		}
		else
		{
			fsh[*f].baseOffset = ftell(fsh[*f].handleFiles.file.o);
		}
		fsh[*f].fileSize = r;
	}
	fsh[*f].handleSync = sync;

	return r;
}

//	search for a file somewhere below the home path, base path or cd path
// we search in that order, matching FS_FOpenFileRead order
int FS_SV_FOpenFileRead(const char* filename, fileHandle_t* fp)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	fileHandle_t f = FS_HandleForFile();
	fsh[f].zipFile = false;
	fsh[f].pakFile = false;

	String::NCpyZ(fsh[f].name, filename, sizeof(fsh[f].name));

	// don't let sound stutter
	S_ClearSoundBuffer(false);

	// search homepath
	char* ospath = FS_BuildOSPath(fs_homepath->string, filename, "");
	// remove trailing slash
	ospath[String::Length(ospath) - 1] = '\0';

	if (fs_debug->integer)
	{
		common->Printf("FS_SV_FOpenFileRead (fs_homepath): %s\n", ospath);
	}

	fsh[f].handleFiles.file.o = fopen(ospath, "rb");
	fsh[f].handleSync = false;
	if (!fsh[f].handleFiles.file.o)
	{
		// NOTE TTimo on non *nix systems, fs_homepath == fs_basepath, might want to avoid
		if (String::ICmp(fs_homepath->string, fs_basepath->string))
		{
			// search basepath
			ospath = FS_BuildOSPath(fs_basepath->string, filename, "");
			ospath[String::Length(ospath) - 1] = '\0';

			if (fs_debug->integer)
			{
				common->Printf("FS_SV_FOpenFileRead (fs_basepath): %s\n", ospath);
			}

			fsh[f].handleFiles.file.o = fopen(ospath, "rb");
			fsh[f].handleSync = false;
		}
	}

	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}

	*fp = f;
	if (f)
	{
		return FS_filelength(f);
	}
	return 0;
}

fileHandle_t FS_SV_FOpenFileWrite(const char* filename)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	char* ospath = FS_BuildOSPath(fs_homepath->string, filename, "");
	ospath[String::Length(ospath) - 1] = '\0';

	fileHandle_t f = FS_HandleForFile();
	fsh[f].zipFile = false;
	fsh[f].pakFile = false;

	if (fs_debug->integer)
	{
		common->Printf("FS_SV_FOpenFileWrite: %s\n", ospath);
	}

	if (FS_CreatePath(ospath))
	{
		return 0;
	}

	common->DPrintf("writing to: %s\n", ospath);
	fsh[f].handleFiles.file.o = fopen(ospath, "wb");

	String::NCpyZ(fsh[f].name, filename, sizeof(fsh[f].name));

	fsh[f].handleSync = false;
	if (!fsh[f].handleFiles.file.o)
	{
		f = 0;
	}
	return f;
}

//	If the FILE pointer is an open pak file, leave it open.
void FS_FCloseFile(fileHandle_t f)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (fsh[f].zipFile == true)
	{
		unzCloseCurrentFile(fsh[f].handleFiles.file.z);
		if (fsh[f].handleFiles.unique)
		{
			unzClose(fsh[f].handleFiles.file.z);
		}
		Com_Memset(&fsh[f], 0, sizeof(fsh[f]));
		return;
	}

	if (fsh[f].pakFile == true)
	{
		if (fsh[f].handleFiles.unique)
		{
			fclose(fsh[f].handleFiles.file.o);
		}
		Com_Memset(&fsh[f], 0, sizeof(fsh[f]));
		return;
	}

	// we didn't find it as a pak, so close it as a unique file
	if (fsh[f].handleFiles.file.o)
	{
		fclose(fsh[f].handleFiles.file.o);
	}
	Com_Memset(&fsh[f], 0, sizeof(fsh[f]));
}

//	Properly handles partial reads
int FS_Read(void* buffer, int len, fileHandle_t f)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!f)
	{
		return 0;
	}

	if (fsh[f].zipFile == false)
	{
		byte* buf = (byte*)buffer;

		int remaining = len;
		int tries = 0;
		while (remaining)
		{
			int block = remaining;
			int read = (int)fread(buf, 1, block, fsh[f].handleFiles.file.o);
			if (read == 0)
			{
				// we might have been trying to read from a CD, which
				// sometimes returns a 0 read on windows
				if (!tries)
				{
					tries = 1;
				}
				else
				{
					return len - remaining;	//Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
				}
			}

			if (read == -1)
			{
				throw Exception("FS_Read: -1 bytes read");
			}

			remaining -= read;
			buf += read;
		}
		return len;
	}
	else
	{
		return unzReadCurrentFile(fsh[f].handleFiles.file.z, buffer, len);
	}
}

//	Properly handles partial writes
int FS_Write(const void* buffer, int len, fileHandle_t h)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!h)
	{
		return 0;
	}

	FILE* f = FS_FileForHandle(h);
	const byte* buf = (const byte*)buffer;

	int remaining = len;
	int tries = 0;
	while (remaining)
	{
		int block = remaining;
		int written = (int)fwrite(buf, 1, block, f);
		if (written == 0)
		{
			if (!tries)
			{
				tries = 1;
			}
			else
			{
				common->Printf("FS_Write: 0 bytes written\n");
				return 0;
			}
		}

		if (written == -1)
		{
			common->Printf("FS_Write: -1 bytes written\n");
			return 0;
		}

		remaining -= written;
		buf += written;
	}
	if (fsh[h].handleSync)
	{
		fflush(f);
	}
	return len;
}

void FS_Printf(fileHandle_t h, const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	FS_Write(msg, String::Length(msg), h);
}

void FS_Flush(fileHandle_t f)
{
	fflush(fsh[f].handleFiles.file.o);
}

int FS_Seek(fileHandle_t f, int offset, int origin)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (fsh[f].zipFile == true)
	{
		if (offset == 0 && origin == FS_SEEK_SET)
		{
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition(fsh[f].handleFiles.file.z, fsh[f].zipFilePos);
			return unzOpenCurrentFile(fsh[f].handleFiles.file.z);
		}
		else if (offset < 65536 && origin == FS_SEEK_SET)
		{
			char foo[65536];
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition(fsh[f].handleFiles.file.z, fsh[f].zipFilePos);
			unzOpenCurrentFile(fsh[f].handleFiles.file.z);
			return FS_Read(foo, offset, f);
		}
		else
		{
			throw Exception("ZIP FILE FSEEK NOT YET IMPLEMENTED\n");
			return -1;
		}
	}
	else
	{
		int _origin;
		FILE* file = FS_FileForHandle(f);
		switch (origin)
		{
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			throw Exception("Bad origin in FS_Seek\n");
		}

		return fseek(file, offset, _origin);
	}
}

int FS_FTell(fileHandle_t f)
{
	int pos;
	if (fsh[f].zipFile == true)
	{
		pos = unztell(fsh[f].handleFiles.file.z);
	}
	else
	{
		pos = ftell(fsh[f].handleFiles.file.o);
	}
	return pos;
}

void FS_ForceFlush(fileHandle_t f)
{
	FILE* file = FS_FileForHandle(f);
	setvbuf(file, NULL, _IONBF, 0);
}

//**************************************************************************
//
//	CONVENIENCE FUNCTIONS FOR ENTIRE FILES
//
//**************************************************************************

int FS_FileIsInPAK(const char* filename, int* pChecksum)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!filename)
	{
		throw Exception("FS_FOpenFileRead: NULL 'filename' parameter passed\n");
	}

	// qpaths are not supposed to have a leading slash
	if (filename[0] == '/' || filename[0] == '\\')
	{
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if (strstr(filename, "..") || strstr(filename, "::"))
	{
		return -1;
	}

	//
	// search through the path, one element at a time
	//

	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (search->pack3)
		{
			int hash = FS_HashFileName(filename, search->pack3->hashSize);
			if (!search->pack3->hashTable[hash])
			{
				continue;
			}
			// disregard if it doesn't match one of the allowed pure pak files
			if (!FS_PakIsPure(search->pack3))
			{
				continue;
			}

			// look through all the pak file elements
			pack3_t* pak = search->pack3;
			fileInPack_t* pakFile = pak->hashTable[hash];
			do
			{
				// case and separator insensitive comparisons
				if (!FS_FilenameCompare(pakFile->name, filename))
				{
					if (pChecksum)
					{
						*pChecksum = pak->pure_checksum;
					}
					return 1;
				}
				pakFile = pakFile->next;
			}
			while (pakFile != NULL);
		}
		else if (search->pack)
		{
			// look through all the pak file elements
			pack_t* pak = search->pack;
			for (int i = 0; i < pak->numfiles; i++)
			{
				if (!FS_FilenameCompare(pak->files[i].name, filename))
				{
					return 1;
				}
			}
		}
	}
	return -1;
}

//	Filename are relative to the quake search path a null buffer will just
// return the file length without loading
int FS_ReadFile(const char* qpath, void** buffer)
{
	fileHandle_t h;

	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!qpath || !qpath[0])
	{
		throw Exception("FS_ReadFile with empty name\n");
	}

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	bool isConfig;
	if (strstr(qpath, ".cfg"))
	{
		isConfig = true;
		if (com_journal && com_journal->integer == 2)
		{
			common->DPrintf("Loading %s from journal file.\n", qpath);
			int len;
			int r = FS_Read(&len, sizeof(len), com_journalDataFile);
			if (r != sizeof(len))
			{
				if (buffer != NULL)
				{
					*buffer = NULL;
				}
				return -1;
			}
			// if the file didn't exist when the journal was created
			if (!len)
			{
				if (buffer == NULL)
				{
					return 1;			// hack for old journal files
				}
				*buffer = NULL;
				return -1;
			}
			if (buffer == NULL)
			{
				return len;
			}

			byte* buf = (byte*)Mem_Alloc(len + 1);
			*buffer = buf;

			r = FS_Read(buf, len, com_journalDataFile);
			if (r != len)
			{
				throw Exception("Read from journalDataFile failed");
			}

			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;

			return len;
		}
	}
	else
	{
		isConfig = false;
	}

	// look for it in the filesystem or pack files
	int len = FS_FOpenFileRead(qpath, &h, false);
	if (h == 0)
	{
		if (buffer)
		{
			*buffer = NULL;
		}
		// if we are journalling and it is a config file, write a zero to the journal file
		if (isConfig && com_journal && com_journal->integer == 1)
		{
			common->DPrintf("Writing zero for %s to journal file.\n", qpath);
			len = 0;
			FS_Write(&len, sizeof(len), com_journalDataFile);
			FS_Flush(com_journalDataFile);
		}
		return -1;
	}

	if (!buffer)
	{
		if (isConfig && com_journal && com_journal->integer == 1)
		{
			common->DPrintf("Writing len for %s to journal file.\n", qpath);
			FS_Write(&len, sizeof(len), com_journalDataFile);
			FS_Flush(com_journalDataFile);
		}
		FS_FCloseFile(h);
		return len;
	}

	byte* buf = (byte*)Mem_Alloc(len + 1);
	*buffer = buf;

	FS_Read(buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile(h);

	// if we are journalling and it is a config file, write it to the journal file
	if (isConfig && com_journal && com_journal->integer == 1)
	{
		common->DPrintf("Writing %s to journal file.\n", qpath);
		FS_Write(&len, sizeof(len), com_journalDataFile);
		FS_Write(buf, len, com_journalDataFile);
		FS_Flush(com_journalDataFile);
	}
	return len;
}

//	Filename are relative to the quake search path
int FS_ReadFile(const char* qpath, Array<byte>& Buffer)
{
	fileHandle_t h;

	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!qpath || !qpath[0])
	{
		throw Exception("FS_ReadFile with empty name\n");
	}

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	bool isConfig;
	if (strstr(qpath, ".cfg"))
	{
		isConfig = true;
		if (com_journal && com_journal->integer == 2)
		{
			common->DPrintf("Loading %s from journal file.\n", qpath);
			int len;
			int r = FS_Read(&len, sizeof(len), com_journalDataFile);
			if (r != sizeof(len))
			{
				Buffer.Clear();
				return -1;
			}
			// if the file didn't exist when the journal was created
			if (!len)
			{
				Buffer.Clear();
				return -1;
			}

			Buffer.SetNum(len + 1);

			r = FS_Read(Buffer.Ptr(), len, com_journalDataFile);
			if (r != len)
			{
				throw Exception("Read from journalDataFile failed");
			}

			return len;
		}
	}
	else
	{
		isConfig = false;
	}

	// look for it in the filesystem or pack files
	int len = FS_FOpenFileRead(qpath, &h, false);
	if (h == 0)
	{
		Buffer.Clear();
		// if we are journalling and it is a config file, write a zero to the journal file
		if (isConfig && com_journal && com_journal->integer == 1)
		{
			common->DPrintf("Writing zero for %s to journal file.\n", qpath);
			len = 0;
			FS_Write(&len, sizeof(len), com_journalDataFile);
			FS_Flush(com_journalDataFile);
		}
		return -1;
	}

	Buffer.SetNum(len);

	FS_Read(Buffer.Ptr(), len, h);

	FS_FCloseFile(h);

	// if we are journalling and it is a config file, write it to the journal file
	if (isConfig && com_journal && com_journal->integer == 1)
	{
		common->DPrintf("Writing %s to journal file.\n", qpath);
		FS_Write(&len, sizeof(len), com_journalDataFile);
		FS_Write(Buffer.Ptr(), len, com_journalDataFile);
		FS_Flush(com_journalDataFile);
	}
	return len;
}

void FS_FreeFile(void* buffer)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}
	if (!buffer)
	{
		throw Exception("FS_FreeFile( NULL )");
	}

	Mem_Free(buffer);
}

//	Filename are reletive to the quake search path
void FS_WriteFile(const char* qpath, const void* buffer, int size)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!qpath || !buffer)
	{
		throw Exception("FS_WriteFile: NULL parameter");
	}

	fileHandle_t f = FS_FOpenFileWrite(qpath);
	if (!f)
	{
		common->Printf("Failed to open %s\n", qpath);
		return;
	}

	FS_Write(buffer, size, f);

	FS_FCloseFile(f);
}

void FS_Rename(const char* from, const char* to)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	// don't let sound stutter
	S_ClearSoundBuffer(false);

	char* from_ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, from);
	char* to_ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, to);

	if (fs_debug->integer)
	{
		common->Printf("FS_Rename: %s --> %s\n", from_ospath, to_ospath);
	}

	if (rename(from_ospath, to_ospath))
	{
		// Failed first attempt, try deleting destination, and renaming again
		FS_Remove(to_ospath);
		if (rename(from_ospath, to_ospath))
		{
			// Failed, try copying it and deleting the original
			FS_CopyFile(from_ospath, to_ospath);
			FS_Remove(from_ospath);
		}
	}
}

void FS_SV_Rename(const char* from, const char* to)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	// don't let sound stutter
	S_ClearSoundBuffer(false);

	char* from_ospath = FS_BuildOSPath(fs_homepath->string, from, "");
	char* to_ospath = FS_BuildOSPath(fs_homepath->string, to, "");
	from_ospath[String::Length(from_ospath) - 1] = '\0';
	to_ospath[String::Length(to_ospath) - 1] = '\0';

	if (fs_debug->integer)
	{
		common->Printf("FS_SV_Rename: %s --> %s\n", from_ospath, to_ospath);
	}

	if (rename(from_ospath, to_ospath))
	{
		// Failed, try copying it and deleting the original
		FS_CopyFile(from_ospath, to_ospath);
		FS_Remove(from_ospath);
	}
}

//	Tests if the file exists in the current gamedir, this DOES NOT search the
// paths. This is to determine if opening a file to write (which always goes
// into the current gamedir) will cause any overwrites.
//	NOTE TTimo: this goes with FS_FOpenFileWrite for opening the file afterwards
bool FS_FileExists(const char* file)
{
	char* testpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, file);
	FILE* f = fopen(testpath, "rb");
	if (f)
	{
		fclose(f);
		return true;
	}
	return false;
}

//	Tests if the file exists
static bool FS_SV_FileExists(const char* file)
{
	char* testpath = FS_BuildOSPath(fs_homepath->string, file, "");
	testpath[String::Length(testpath) - 1] = '\0';

	FILE* f = fopen(testpath, "rb");
	if (f)
	{
		fclose(f);
		return true;
	}
	return false;
}

//**************************************************************************
//
//	DIRECTORY SCANNING FUNCTIONS
//
//**************************************************************************

#define MAX_FOUND_FILES 0x1000

static int FS_ReturnPath(const char* zname, char* zpath, int* depth)
{
	int newdep = 0;
	zpath[0] = 0;
	int len = 0;
	int at = 0;

	while (zname[at] != 0)
	{
		if (zname[at] == '/' || zname[at] == '\\')
		{
			len = at;
			newdep++;
		}
		at++;
	}
	String::Cpy(zpath, zname);
	zpath[len] = 0;
	*depth = newdep;

	return len;
}

static int FS_AddFileToList(char* name, char* list[MAX_FOUND_FILES], int nfiles)
{
	if (nfiles == MAX_FOUND_FILES - 1)
	{
		return nfiles;
	}
	for (int i = 0; i < nfiles; i++)
	{
		if (!String::ICmp(name, list[i]))
		{
			return nfiles;		// allready in list
		}
	}
	list[nfiles] = __CopyString(name);
	nfiles++;

	return nfiles;
}

//	Returns a uniqued list of files that match the given criteria
// from all search paths
static char** FS_ListFilteredFiles(const char* path, const char* extension, char* filter, int* numfiles)
{
	int nfiles;
	char** listCopy;
	char* list[MAX_FOUND_FILES];
	searchpath_t* search;
	int i;
	int pathLength;
	int extensionLength;
	int length, pathDepth, temp;
	pack3_t* pak;
	fileInPack_t* buildBuffer;
	char zpath[MAX_ZPATH];

	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!path)
	{
		*numfiles = 0;
		return NULL;
	}
	if (!extension)
	{
		extension = "";
	}

	pathLength = String::Length(path);
	if (path[pathLength - 1] == '\\' || path[pathLength - 1] == '/')
	{
		pathLength--;
	}
	extensionLength = String::Length(extension);
	nfiles = 0;
	FS_ReturnPath(path, zpath, &pathDepth);

	//
	// search through the path, one element at a time, adding to list
	//
	for (search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (search->pack3)
		{
			//ZOID:  If we are pure, don't search for files on paks that
			// aren't on the pure list
			if (!FS_PakIsPure(search->pack3))
			{
				continue;
			}

			// look through all the pak file elements
			pak = search->pack3;
			buildBuffer = pak->buildBuffer;
			for (i = 0; i < pak->numfiles; i++)
			{
				char* name;
				int zpathLen, depth;

				// check for directory match
				name = buildBuffer[i].name;
				//
				if (filter)
				{
					// case insensitive
					if (!String::FilterPath(filter, name, false))
					{
						continue;
					}
					// unique the match
					nfiles = FS_AddFileToList(name, list, nfiles);
				}
				else
				{
					zpathLen = FS_ReturnPath(name, zpath, &depth);

					if ((depth - pathDepth) > 2 || pathLength > zpathLen || String::NICmp(name, path, pathLength))
					{
						continue;
					}

					// check for extension match
					length = String::Length(name);
					if (length < extensionLength)
					{
						continue;
					}

					if (String::ICmp(name + length - extensionLength, extension))
					{
						continue;
					}
					// unique the match

					temp = pathLength;
					if (pathLength)
					{
						temp++;		// include the '/'
					}
					nfiles = FS_AddFileToList(name + temp, list, nfiles);
				}
			}
		}
		else if (search->pack)
		{
			// look through all the pak file elements
			pack_t* pak = search->pack;
			packfile_t* buildBuffer = pak->files;
			for (i = 0; i < pak->numfiles; i++)
			{
				char* name;
				int zpathLen, depth;

				// check for directory match
				name = buildBuffer[i].name;
				//
				if (filter)
				{
					// case insensitive
					if (!String::FilterPath(filter, name, false))
					{
						continue;
					}
					// unique the match
					nfiles = FS_AddFileToList(name, list, nfiles);
				}
				else
				{
					zpathLen = FS_ReturnPath(name, zpath, &depth);

					if ((depth - pathDepth) > 2 || pathLength > zpathLen || String::NICmp(name, path, pathLength))
					{
						continue;
					}

					// check for extension match
					length = String::Length(name);
					if (length < extensionLength)
					{
						continue;
					}

					if (String::ICmp(name + length - extensionLength, extension))
					{
						continue;
					}
					// unique the match

					temp = pathLength;
					if (pathLength)
					{
						temp++;		// include the '/'
					}
					nfiles = FS_AddFileToList(name + temp, list, nfiles);
				}
			}
		}
		else if (search->dir)
		{
			// scan for files in the filesystem
			char* netpath;
			int numSysFiles;
			char** sysFiles;
			char* name;

			// don't scan directories for files if we are pure or restricted
			if (fs_numServerPaks && (!(GGameType & GAME_WolfSP) || String::ICmp(extension, "svg")))
			{
				continue;
			}
			else
			{
				netpath = FS_BuildOSPath(search->dir->path, search->dir->gamedir, path);
				sysFiles = Sys_ListFiles(netpath, extension, filter, &numSysFiles, false);
				for (i = 0; i < numSysFiles; i++)
				{
					// unique the match
					name = sysFiles[i];
					nfiles = FS_AddFileToList(name, list, nfiles);
				}
				Sys_FreeFileList(sysFiles);
			}
		}
	}

	// return a copy of the list
	*numfiles = nfiles;

	if (!nfiles)
	{
		return NULL;
	}

	listCopy = (char**)Mem_Alloc((nfiles + 1) * sizeof(*listCopy));
	for (i = 0; i < nfiles; i++)
	{
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

char** FS_ListFiles(const char* path, const char* extension, int* numfiles)
{
	return FS_ListFilteredFiles(path, extension, NULL, numfiles);
}

void FS_FreeFileList(char** list)
{
	if (!fs_searchpaths)
	{
		throw Exception("Filesystem call made without initialization\n");
	}

	if (!list)
	{
		return;
	}

	for (int i = 0; list[i]; i++)
	{
		Mem_Free(list[i]);
	}

	Mem_Free(list);
}

int FS_GetFileList(const char* path, const char* extension, char* listbuf, int bufsize)
{
	int nFiles, i, nTotal, nLen;
	char** pFiles = NULL;

	*listbuf = 0;
	nFiles = 0;
	nTotal = 0;

	if (String::ICmp(path, "$modlist") == 0)
	{
		return FS_GetModList(listbuf, bufsize);
	}

	pFiles = FS_ListFiles(path, extension, &nFiles);

	for (i = 0; i < nFiles; i++)
	{
		nLen = String::Length(pFiles[i]) + 1;
		if (nTotal + nLen + 1 < bufsize)
		{
			String::Cpy(listbuf, pFiles[i]);
			listbuf += nLen;
			nTotal += nLen;
		}
		else
		{
			nFiles = i;
			break;
		}
	}

	FS_FreeFileList(pFiles);

	return nFiles;
}

//	mkv: Naive implementation. Concatenates three lists into a
//      new list, and frees the old lists from the heap.
static unsigned int Sys_CountFileList(char** list)
{
	int i = 0;

	if (list)
	{
		while (*list)
		{
			list++;
			i++;
		}
	}
	return i;
}

static char** Sys_ConcatenateFileLists(char** list0, char** list1, char** list2)
{
	int totalLength = 0;
	char** cat = NULL, ** dst, ** src;

	totalLength += Sys_CountFileList(list0);
	totalLength += Sys_CountFileList(list1);
	totalLength += Sys_CountFileList(list2);

	/* Create new list. */
	dst = cat = (char**)Mem_Alloc((totalLength + 1) * sizeof(char*));

	/* Copy over lists. */
	if (list0)
	{
		for (src = list0; *src; src++, dst++)
			*dst = *src;
	}
	if (list1)
	{
		for (src = list1; *src; src++, dst++)
			*dst = *src;
	}
	if (list2)
	{
		for (src = list2; *src; src++, dst++)
			*dst = *src;
	}

	// Terminate the list
	*dst = NULL;

	// Free our old lists.
	// NOTE: not freeing their content, it's been merged in dst and still being used
	if (list0)
	{
		Mem_Free(list0);
	}
	if (list1)
	{
		Mem_Free(list1);
	}
	if (list2)
	{
		Mem_Free(list2);
	}

	return cat;
}

//	Returns a list of mod directory names
//	A mod directory is a peer to baseq3 with a pk3 in it
//	The directories are searched in base path, cd path and home path
int FS_GetModList(char* listbuf, int bufsize)
{
	int nMods, i, j, nTotal, nLen, nPaks, nPotential, nDescLen;
	char** pFiles = NULL;
	char** pPaks = NULL;
	char* name, * path;
	char descPath[MAX_OSPATH];
	fileHandle_t descHandle;

	int dummy;
	char** pFiles0 = NULL;
	char** pFiles1 = NULL;
	qboolean bDrop = false;

	*listbuf = 0;
	nMods = nPotential = nTotal = 0;

	pFiles0 = Sys_ListFiles(fs_homepath->string, NULL, NULL, &dummy, true);
	pFiles1 = Sys_ListFiles(fs_basepath->string, NULL, NULL, &dummy, true);
	// we searched for mods in the three paths
	// it is likely that we have duplicate names now, which we will cleanup below
	pFiles = Sys_ConcatenateFileLists(pFiles0, pFiles1, NULL);
	nPotential = Sys_CountFileList(pFiles);

	for (i = 0; i < nPotential; i++)
	{
		name = pFiles[i];
		// NOTE: cleaner would involve more changes
		// ignore duplicate mod directories
		if (i != 0)
		{
			bDrop = false;
			for (j = 0; j < i; j++)
			{
				if (String::ICmp(pFiles[j],name) == 0)
				{
					// this one can be dropped
					bDrop = true;
					break;
				}
			}
		}
		if (bDrop)
		{
			continue;
		}
		// we drop "baseq3" "." and ".."
		if (!String::ICmp(name, fs_PrimaryBaseGame) || !String::NICmp(name, ".", 1))
		{
			continue;
		}

		// now we need to find some .pk3 files to validate the mod
		// NOTE TTimo: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
		// we didn't keep the information when we merged the directory names, as to what OS Path it was found under
		//   so it could be in base path, cd path or home path
		//   we will try each three of them here (yes, it's a bit messy)
		path = FS_BuildOSPath(fs_basepath->string, name, "");
		nPaks = 0;
		pPaks = Sys_ListFiles(path, ".pk3", NULL, &nPaks, false);
		Sys_FreeFileList(pPaks);// we only use Sys_ListFiles to check wether .pk3 files are present

		/* try on home path */
		if (nPaks <= 0)
		{
			path = FS_BuildOSPath(fs_homepath->string, name, "");
			nPaks = 0;
			pPaks = Sys_ListFiles(path, ".pk3", NULL, &nPaks, false);
			Sys_FreeFileList(pPaks);
		}

		if (nPaks > 0)
		{
			nLen = String::Length(name) + 1;
			// nLen is the length of the mod path
			// we need to see if there is a description available
			descPath[0] = '\0';
			String::Cpy(descPath, name);
			String::Cat(descPath, sizeof(descPath), "/description.txt");
			nDescLen = FS_SV_FOpenFileRead(descPath, &descHandle);
			if (nDescLen > 0 && descHandle)
			{
				FILE* file;
				file = FS_FileForHandle(descHandle);
				Com_Memset(descPath, 0, sizeof(descPath));
				nDescLen = (int)fread(descPath, 1, 48, file);
				if (nDescLen >= 0)
				{
					descPath[nDescLen] = '\0';
				}
				FS_FCloseFile(descHandle);
			}
			else
			{
				String::Cpy(descPath, name);
			}
			nDescLen = String::Length(descPath) + 1;

			if (nTotal + nLen + 1 + nDescLen + 1 < bufsize)
			{
				String::Cpy(listbuf, name);
				listbuf += nLen;
				String::Cpy(listbuf, descPath);
				listbuf += nDescLen;
				nTotal += nLen + nDescLen;
				nMods++;
			}
			else
			{
				break;
			}
		}
	}
	Sys_FreeFileList(pFiles);

	return nMods;
}

//**************************************************************************
//
//	Methods for pure servers
//
//**************************************************************************

//	Returns a space separated string containing the checksums of all loaded pk3 files.
//	Servers with sv_pure set will get this string and pass it to clients.
const char* FS_LoadedPakChecksums()
{
	static char info[BIG_INFO_STRING];

	info[0] = 0;

	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (!search->pack3)
		{
			continue;
		}

		String::Cat(info, sizeof(info), va("%i ", search->pack3->checksum));
	}

	return info;
}

//	Returns a space separated string containing the names of all loaded pk3 files.
//	Servers with sv_pure set will get this string and pass it to clients.
const char* FS_LoadedPakNames()
{
	static char info[BIG_INFO_STRING];

	info[0] = 0;

	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (!search->pack3)
		{
			continue;
		}

		if (*info)
		{
			String::Cat(info, sizeof(info), " ");
		}
		if (GGameType & GAME_ET)
		{
			// Changed to have the full path
			String::Cat(info, sizeof(info), search->pack3->pakGamename);
			String::Cat(info, sizeof(info), "/");
		}
		String::Cat(info, sizeof(info), search->pack3->pakBasename);
	}

	return info;
}

//	Returns a space separated string containing the pure checksums of all loaded pk3 files.
//	Servers with sv_pure use these checksums to compare with the checksums the clients send
// back to the server.
const char* FS_LoadedPakPureChecksums()
{
	static char info[BIG_INFO_STRING];

	info[0] = 0;

	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (!search->pack3)
		{
			continue;
		}

		String::Cat(info, sizeof(info), va("%i ", search->pack3->pure_checksum));
	}

	return info;
}

//	Returns a space separated string containing the checksums of all referenced pk3 files.
//	The server will send this to the clients so they can check which files should be auto-downloaded.
const char* FS_ReferencedPakChecksums()
{
	static char info[BIG_INFO_STRING];

	info[0] = 0;


	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (search->pack3)
		{
			if (search->pack3->referenced || String::NICmp(search->pack3->pakGamename, fs_PrimaryBaseGame, String::Length(fs_PrimaryBaseGame)))
			{
				String::Cat(info, sizeof(info), va("%i ", search->pack3->checksum));
			}
		}
	}

	return info;
}

//	Returns a space separated string containing the pure checksums of all referenced pk3 files.
//	Servers with sv_pure set will get this string back from clients for pure validation
//
//	The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."
const char* FS_ReferencedPakPureChecksums()
{
	static char info[BIG_INFO_STRING];
	searchpath_t* search;
	int nFlags, numPaks, checksum;

	info[0] = 0;

	checksum = fs_checksumFeed;
	numPaks = 0;
	for (nFlags = FS_CGAME_REF; nFlags; nFlags = nFlags >> 1)
	{
		if (nFlags & FS_GENERAL_REF)
		{
			// add a delimter between must haves and general refs
			//String::Cat(info, sizeof(info), "@ ");
			info[String::Length(info) + 1] = '\0';
			info[String::Length(info) + 2] = '\0';
			info[String::Length(info)] = '@';
			info[String::Length(info)] = ' ';
		}
		for (search = fs_searchpaths; search; search = search->next)
		{
			// is the element a pak file and has it been referenced based on flag?
			if (search->pack3 && (search->pack3->referenced & nFlags))
			{
				String::Cat(info, sizeof(info), va("%i ", search->pack3->pure_checksum));
				if (nFlags & (FS_CGAME_REF | FS_UI_REF))
				{
					break;
				}
				checksum ^= search->pack3->pure_checksum;
				numPaks++;
			}
		}
		if (fs_fakeChkSum != 0)
		{
			// only added if a non-pure file is referenced
			String::Cat(info, sizeof(info), va("%i ", fs_fakeChkSum));
		}
	}
	// last checksum is the encoded number of referenced pk3s
	checksum ^= numPaks;
	String::Cat(info, sizeof(info), va("%i ", checksum));

	return info;
}

//	Returns a space separated string containing the names of all referenced pk3 files.
//	The server will send this to the clients so they can check which files should be auto-downloaded.
const char* FS_ReferencedPakNames()
{
	static char info[BIG_INFO_STRING];
	searchpath_t* search;

	info[0] = 0;

	// we want to return ALL pk3's from the fs_game path
	// and referenced one's from baseq3
	for (search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file?
		if (search->pack3)
		{
			if (*info)
			{
				String::Cat(info, sizeof(info), " ");
			}
			if (search->pack3->referenced || String::NICmp(search->pack3->pakGamename, fs_PrimaryBaseGame, String::Length(fs_PrimaryBaseGame)))
			{
				String::Cat(info, sizeof(info), search->pack3->pakGamename);
				String::Cat(info, sizeof(info), "/");
				String::Cat(info, sizeof(info), search->pack3->pakBasename);
			}
		}
	}

	return info;
}

void FS_ClearPakReferences(int flags)
{
	if (!flags)
	{
		flags = -1;
	}
	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		// is the element a pak file and has it been referenced?
		if (search->pack3)
		{
			search->pack3->referenced &= ~flags;
		}
	}
}

//	If the string is empty, all data sources will be allowed.
//	If not empty, only pk3 files that match one of the space separated
// checksums will be checked for files, with the exception of .cfg and .dat files.
void FS_PureServerSetLoadedPaks(const char* pakSums, const char* pakNames)
{
	int i, c, d;

	Cmd_TokenizeString(pakSums);

	c = Cmd_Argc();
	if (c > MAX_SEARCH_PATHS)
	{
		c = MAX_SEARCH_PATHS;
	}

	fs_numServerPaks = c;

	for (i = 0; i < c; i++)
	{
		fs_serverPaks[i] = String::Atoi(Cmd_Argv(i));
	}

	if (fs_numServerPaks)
	{
		common->DPrintf("Connected to a pure server.\n");
	}
	else if (!(GGameType & GAME_WolfSP))
	{
		if (fs_reordered)
		{
			// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
			// force a restart to make sure the search order will be correct
			common->DPrintf("FS search reorder is required\n");
			FS_Restart(fs_checksumFeed);
			return;
		}
	}

	for (i = 0; i < c; i++)
	{
		if (fs_serverPakNames[i])
		{
			Mem_Free(fs_serverPakNames[i]);
		}
		fs_serverPakNames[i] = NULL;
	}
	if (pakNames && *pakNames)
	{
		Cmd_TokenizeString(pakNames);

		d = Cmd_Argc();
		if (d > MAX_SEARCH_PATHS)
		{
			d = MAX_SEARCH_PATHS;
		}

		for (i = 0; i < d; i++)
		{
			fs_serverPakNames[i] = __CopyString(Cmd_Argv(i));
		}
	}
}

//	The checksums and names of the pk3 files referenced at the server are
// sent to the client and stored here. The client will use these checksums
// to see if any pk3 files need to be auto-downloaded.
void FS_PureServerSetReferencedPaks(const char* pakSums, const char* pakNames)
{
	int i, c, d;

	Cmd_TokenizeString(pakSums);

	c = Cmd_Argc();
	if (c > MAX_SEARCH_PATHS)
	{
		c = MAX_SEARCH_PATHS;
	}

	fs_numServerReferencedPaks = c;

	for (i = 0; i < c; i++)
	{
		fs_serverReferencedPaks[i] = String::Atoi(Cmd_Argv(i));
	}

	for (i = 0; i < c; i++)
	{
		if (fs_serverReferencedPakNames[i])
		{
			Mem_Free(fs_serverReferencedPakNames[i]);
		}
		fs_serverReferencedPakNames[i] = NULL;
	}
	if (pakNames && *pakNames)
	{
		Cmd_TokenizeString(pakNames);

		d = Cmd_Argc();
		if (d > MAX_SEARCH_PATHS)
		{
			d = MAX_SEARCH_PATHS;
		}

		for (i = 0; i < d; i++)
		{
			fs_serverReferencedPakNames[i] = __CopyString(Cmd_Argv(i));
		}
	}
}

bool FS_idPak(const char* pak, const char* base)
{
	if ((GGameType & (GAME_WolfMP | GAME_ET)) && !FS_FilenameCompare(pak, va("%s/mp_bin", base)))
	{
		return true;
	}

	int i;
	for (i = 0; i < NUM_ID_PAKS; i++)
	{
		if (!FS_FilenameCompare(pak, va("%s/pak%d", base, i)))
		{
			break;
		}
		if (GGameType & (GAME_WolfSP | GAME_WolfMP))
		{
			if (!FS_FilenameCompare(pak, va("%s/mp_pak%d", base, i)))
			{
				break;
			}
			if (!FS_FilenameCompare(pak, va("%s/sp_pak%d", base, i)))
			{
				break;
			}
		}
	}
	if (i < NUM_ID_PAKS)
	{
		return true;
	}
	return false;
}

//	----------------
//	dlstring == true
//
//	Returns a list of pak files that we should download from the server. They all get stored
//	in the current gamedir and an FS_Restart will be fired up after we download them all.
//
//	The string is the format:
//
//	@remotename@localname [repeat]
//
//	static int		fs_numServerReferencedPaks;
//	static int		fs_serverReferencedPaks[MAX_SEARCH_PATHS];
//	static char		*fs_serverReferencedPakNames[MAX_SEARCH_PATHS];
//
//	----------------
//	dlstring == false
//
//	we are not interested in a download string format, we want something human-readable
//	(this is used for diagnostics while connecting to a pure server)
bool FS_ComparePaks(char* neededpaks, int len, bool dlstring)
{
	searchpath_t* sp;
	qboolean havepak, badchecksum;
	int i;

	if (!fs_numServerReferencedPaks)
	{
		return false;	// Server didn't send any pack information along
	}

	*neededpaks = 0;

	for (i = 0; i < fs_numServerReferencedPaks; i++)
	{
		// Ok, see if we have this pak file
		badchecksum = false;
		havepak = false;

		// never autodownload any of the id paks
		if (FS_idPak(fs_serverReferencedPakNames[i], fs_PrimaryBaseGame) ||
			((GGameType & GAME_Quake3) && FS_idPak(fs_serverReferencedPakNames[i], "missionpack")))
		{
			continue;
		}

		for (sp = fs_searchpaths; sp; sp = sp->next)
		{
			if (sp->pack3 && sp->pack3->checksum == fs_serverReferencedPaks[i])
			{
				havepak = true;	// This is it!
				break;
			}
		}

		if (!havepak && fs_serverReferencedPakNames[i] && *fs_serverReferencedPakNames[i])
		{
			// Don't got it
			if (dlstring)
			{
				// Remote name
				String::Cat(neededpaks, len, "@");
				String::Cat(neededpaks, len, fs_serverReferencedPakNames[i]);
				String::Cat(neededpaks, len, ".pk3");

				// Local name
				String::Cat(neededpaks, len, "@");
				// Do we have one with the same name?
				if (FS_SV_FileExists(va("%s.pk3", fs_serverReferencedPakNames[i])))
				{
					char st[MAX_ZPATH];
					// We already have one called this, we need to download it to another name
					// Make something up with the checksum in it
					String::Sprintf(st, sizeof(st), "%s.%08x.pk3", fs_serverReferencedPakNames[i], fs_serverReferencedPaks[i]);
					String::Cat(neededpaks, len, st);
				}
				else
				{
					String::Cat(neededpaks, len, fs_serverReferencedPakNames[i]);
					String::Cat(neededpaks, len, ".pk3");
				}
			}
			else
			{
				String::Cat(neededpaks, len, fs_serverReferencedPakNames[i]);
				String::Cat(neededpaks, len, ".pk3");
				// Do we have one with the same name?
				if (FS_SV_FileExists(va("%s.pk3", fs_serverReferencedPakNames[i])))
				{
					String::Cat(neededpaks, len, " (local file exists with wrong checksum)");
					// let the client subsystem track bad download redirects (dl file with wrong checksums)
					// this is a bit ugly but the only other solution would have been callback passing..
					if (CL_WWWBadChecksum(va("%s.pk3", fs_serverReferencedPakNames[i])))
					{
						// remove a potentially malicious download file
						// (this is also intended to avoid expansion of the pk3 into a file with different checksum .. messes up wwwdl chkfail)
						char* rmv = FS_BuildOSPath(fs_homepath->string, va("%s.pk3", fs_serverReferencedPakNames[i]), "");
						rmv[String::Length(rmv) - 1] = '\0';
						FS_Remove(rmv);
					}
				}
				String::Cat(neededpaks, len, "\n");
			}
		}
	}

	if (*neededpaks)
	{
		common->Printf("Need paks: %s\n", neededpaks);
		return true;
	}

	return false;	// We have them all
}

//	NOTE TTimo: the reordering that happens here is not reflected in the cvars (\cvarlist *pak*)
// this can lead to misleading situations, see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
void FS_ReorderPurePaks()
{
	searchpath_t* s;
	int i;
	searchpath_t** p_insert_index,	// for linked list reordering
	** p_previous;		// when doing the scan

	// only relevant when connected to pure server
	if (!fs_numServerPaks)
	{
		return;
	}

	fs_reordered = false;

	p_insert_index = &fs_searchpaths;	// we insert in order at the beginning of the list
	for (i = 0; i < fs_numServerPaks; i++)
	{
		p_previous = p_insert_index;// track the pointer-to-current-item
		for (s = *p_insert_index; s; s = s->next)
		{
			// the part of the list before p_insert_index has been sorted already
			if (s->pack3 && fs_serverPaks[i] == s->pack3->checksum)
			{
				fs_reordered = true;
				// move this element to the insert list
				*p_previous = s->next;
				s->next = *p_insert_index;
				*p_insert_index = s;
				// increment insert list
				p_insert_index = &s->next;
				break;	// iterate to next server pack
			}
			p_previous = &s->next;
		}
	}
}

bool FS_VerifyOfficialPaks()
{

	if (!fs_numServerPaks)
	{
		return true;
	}

	int numOfficialPaksOnServer = 0;
	officialpak_t officialpaks[64];
	for (int i = 0; i < fs_numServerPaks; i++)
	{
		if (FS_idPak(fs_serverPakNames[i], fs_PrimaryBaseGame))
		{
			String::NCpyZ(officialpaks[numOfficialPaksOnServer].pakname, fs_serverPakNames[i], sizeof(officialpaks[0].pakname));
			officialpaks[numOfficialPaksOnServer].ok = false;
			numOfficialPaksOnServer++;
		}
	}

	int numOfficialPaksLocal = 0;
	for (int i = 0; i < fs_numServerPaks; i++)
	{
		for (searchpath_t* sp = fs_searchpaths; sp; sp = sp->next)
		{
			if (sp->pack3 && sp->pack3->checksum == fs_serverPaks[i])
			{
				char packPath[MAX_QPATH];
				String::Sprintf(packPath, sizeof(packPath), "%s/%s", sp->pack3->pakGamename, sp->pack3->pakBasename);

				if (FS_idPak(packPath, fs_PrimaryBaseGame))
				{
					for (int j = 0; j < numOfficialPaksOnServer; j++)
					{
						if (!String::ICmp(packPath, officialpaks[j].pakname))
						{
							officialpaks[j].ok = true;
						}
					}
					numOfficialPaksLocal++;
				}
				break;
			}
		}
	}

	if (numOfficialPaksOnServer == numOfficialPaksLocal)
	{
		return true;
	}
	for (int i = 0; i < numOfficialPaksOnServer; i++)
	{
		if (!officialpaks[i].ok)
		{
			common->Printf("ERROR: Missing/corrupt official pak file %s\n", officialpaks[i].pakname);
		}
	}
	return false;
}

// compared requested pak against the names as we built them in FS_ReferencedPakNames
bool FS_VerifyPak(const char* pak)
{
	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		if (search->pack3)
		{
			char teststring[BIG_INFO_STRING];
			String::NCpyZ(teststring, search->pack3->pakGamename, sizeof(teststring));
			String::Cat(teststring, sizeof(teststring), "/");
			String::Cat(teststring, sizeof(teststring), search->pack3->pakBasename);
			String::Cat(teststring, sizeof(teststring), ".pk3");
			if (!String::ICmp(teststring, pak))
			{
				return true;
			}
		}
	}
	return false;
}

//**************************************************************************
//
//	Console commands
//
//**************************************************************************

void FS_Path_f()
{
	common->Printf("Current search path:\n");
	for (searchpath_t* s = fs_searchpaths; s; s = s->next)
	{
		if (s == fs_base_searchpaths)
		{
			common->Printf("----------\n");
		}
		if (s->pack3)
		{
			common->Printf("%s (%i files)\n", s->pack3->pakFilename, s->pack3->numfiles);
			if (fs_numServerPaks)
			{
				if (!FS_PakIsPure(s->pack3))
				{
					common->Printf("    not on the pure list\n");
				}
				else
				{
					common->Printf("    on the pure list\n");
				}
			}
		}
		else if (s->pack)
		{
			common->Printf("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		}
		else
		{
			common->Printf("%s/%s\n", s->dir->path, s->dir->gamedir);
		}
	}

	if (fs_links)
	{
		common->Printf("\nLinks:\n");
		for (filelink_t* l = fs_links; l; l = l->next)
		{
			common->Printf("%s : %s\n", l->from, l->to);
		}
	}

	common->Printf("\n");
	for (int i = 1; i < MAX_FILE_HANDLES; i++)
	{
		if (fsh[i].handleFiles.file.o)
		{
			common->Printf("handle %i: %s\n", i, fsh[i].name);
		}
	}
}

static void FS_Dir_f()
{
	if (Cmd_Argc() < 2 || Cmd_Argc() > 3)
	{
		common->Printf("usage: dir <directory> [extension]\n");
		return;
	}

	const char* path = Cmd_Argv(1);
	const char* extension = Cmd_Argc() == 2 ? "" : Cmd_Argv(2);

	common->Printf("Directory of %s %s\n", path, extension);
	common->Printf("---------------\n");

	int ndirs;
	char** dirnames = FS_ListFiles(path, extension, &ndirs);

	for (int i = 0; i < ndirs; i++)
	{
		common->Printf("%s\n", dirnames[i]);
	}
	FS_FreeFileList(dirnames);
}

static void FS_ConvertPath(char* s)
{
	while (*s)
	{
		if (*s == '\\' || *s == ':')
		{
			*s = '/';
		}
		s++;
	}
}

static void FS_SortFileList(char** filelist, int numfiles)
{
	int i, j, k, numsortedfiles;
	char** sortedlist;

	sortedlist = (char**)Mem_Alloc((numfiles + 1) * sizeof(*sortedlist));
	sortedlist[0] = NULL;
	numsortedfiles = 0;
	for (i = 0; i < numfiles; i++)
	{
		for (j = 0; j < numsortedfiles; j++)
		{
			if (FS_FilenameCompare(filelist[i], sortedlist[j]) < 0)
			{
				break;
			}
		}
		for (k = numsortedfiles; k > j; k--)
		{
			sortedlist[k] = sortedlist[k - 1];
		}
		sortedlist[j] = filelist[i];
		numsortedfiles++;
	}
	Com_Memcpy(filelist, sortedlist, numfiles * sizeof(*filelist));
	Mem_Free(sortedlist);
}

static void FS_NewDir_f()
{
	char* filter;
	char** dirnames;
	int ndirs;
	int i;

	if (Cmd_Argc() < 2)
	{
		common->Printf("usage: fdir <filter>\n");
		common->Printf("example: fdir *q3dm*.bsp\n");
		return;
	}

	filter = Cmd_Argv(1);

	common->Printf("---------------\n");

	dirnames = FS_ListFilteredFiles("", "", filter, &ndirs);

	FS_SortFileList(dirnames, ndirs);

	for (i = 0; i < ndirs; i++)
	{
		FS_ConvertPath(dirnames[i]);
		common->Printf("%s\n", dirnames[i]);
	}
	common->Printf("%d files listed\n", ndirs);
	FS_FreeFileList(dirnames);
}

//	Creates a filelink_t
static void FS_Link_f()
{
	if (Cmd_Argc() != 3)
	{
		common->Printf("USAGE: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	filelink_t** prev = &fs_links;
	for (filelink_t* l = fs_links; l; l = l->next)
	{
		if (!String::Cmp(l->from, Cmd_Argv(1)))
		{
			Mem_Free(l->to);
			if (!String::Length(Cmd_Argv(2)))
			{
				// delete it
				*prev = l->next;
				Mem_Free(l->from);
				delete l;
				return;
			}
			l->to = __CopyString(Cmd_Argv(2));
			return;
		}
		prev = &l->next;
	}

	// create a new link
	filelink_t* l = new filelink_t;
	l->next = fs_links;
	fs_links = l;
	l->from = __CopyString(Cmd_Argv(1));
	l->fromlength = String::Length(l->from);
	l->to = __CopyString(Cmd_Argv(2));
}

//**************************************************************************
//
//	Other
//
//**************************************************************************

bool FS_Initialized()
{
	return fs_searchpaths != NULL;
}

void FS_SharedStartup()
{
	fs_debug = Cvar_Get("fs_debug", "0", 0);
	fs_basepath = Cvar_Get("fs_basepath", Sys_Cwd(), CVAR_INIT);
	const char* homePath = Sys_DefaultHomePath();
	if (!homePath || !homePath[0])
	{
		homePath = fs_basepath->string;
	}
	fs_homepath = Cvar_Get("fs_homepath", homePath, CVAR_INIT);

	// add our commands
	Cmd_AddCommand("path", FS_Path_f);
	Cmd_AddCommand("dir", FS_Dir_f);
	Cmd_AddCommand("fdir", FS_NewDir_f);
	if (GGameType & GAME_Quake2)
	{
		Cmd_AddCommand("link", FS_Link_f);
	}
}

//	Frees all resources and closes all files
void FS_Shutdown()
{
	for (int i = 0; i < MAX_FILE_HANDLES; i++)
	{
		if (fsh[i].fileSize)
		{
			FS_FCloseFile(i);
		}
	}

	// free everything
	for (searchpath_t* p = fs_searchpaths; p; )
	{
		if (p->pack)
		{
			fclose(p->pack->handle);
			delete[] p->pack->files;
			delete p->pack;
		}
		if (p->pack3)
		{
			unzClose(p->pack3->handle);
			Mem_Free(p->pack3->buildBuffer);
			Mem_Free(p->pack3);
		}
		if (p->dir)
		{
			delete p->dir;
		}
		searchpath_t* next = p->next;
		delete p;
		p = next;
	}

	// any FS_ calls will now be an error until reinitialized
	fs_searchpaths = NULL;

	Cmd_RemoveCommand("path");
	Cmd_RemoveCommand("dir");
	Cmd_RemoveCommand("fdir");
	Cmd_RemoveCommand("touchFile");
}

void FS_SetSearchPathBase()
{
	fs_base_searchpaths = fs_searchpaths;
}

void FS_ResetSearchPathToBase()
{
	while (fs_searchpaths != fs_base_searchpaths)
	{
		if (fs_searchpaths->pack)
		{
			fclose(fs_searchpaths->pack->handle);
			delete[] fs_searchpaths->pack->files;
			delete fs_searchpaths->pack;
		}
		if (fs_searchpaths->pack3)
		{
			unzClose(fs_searchpaths->pack3->handle);
			Mem_Free(fs_searchpaths->pack3->buildBuffer);
			Mem_Free(fs_searchpaths->pack3);
		}
		if (fs_searchpaths->dir)
		{
			delete fs_searchpaths->dir;
		}
		searchpath_t* next = fs_searchpaths->next;
		delete fs_searchpaths;
		fs_searchpaths = next;
	}
}

//	Allows enumerating all of the directories in the search path
char* FS_NextPath(char* prevpath)
{
	if (!prevpath)
	{
		return fs_gamedir;
	}

	char* prev = fs_gamedir;
	for (searchpath_t* s = fs_searchpaths; s; s = s->next)
	{
		if (s->pack)
		{
			continue;
		}
		sprintf(s->dir->fullname, "%s/%s", s->dir->path, s->dir->gamedir);
		if (prevpath == prev)
		{
			return s->dir->fullname;
		}
		prev = s->dir->fullname;
	}

	return NULL;
}

int FS_GetQuake2GameType()
{
	for (searchpath_t* search = fs_searchpaths; search; search = search->next)
	{
		if (!search->dir)
		{
			continue;
		}

		if (strstr(search->dir->gamedir, "xatrix"))
		{
			return 1;
		}

		if (strstr(search->dir->gamedir, "rogue"))
		{
			return 2;
		}
	}
	return 0;
}

/*
NERVE - SMF - Extracts the latest file from a pak file.

Compares packed file against extracted file. If no differences, does not copy.
This is necessary for exe/dlls which may or may not be locked.

NOTE TTimo:
  fullpath gives the full OS path to the dll that will potentially be loaded
  the dll is extracted to fs_homepath if needed

  the return value doesn't tell wether file was extracted or not, it just says wether it's ok to continue
  (i.e. either the right file was extracted successfully, or it was already present)
*/
bool FS_CL_ExtractFromPakFile(const char* fullpath, const char* gamedir, const char* filename)
{
	bool needToCopy = true;

	// read in compressed file
	unsigned char* srcData;
	int srcLength = FS_ReadFile(filename, (void**)&srcData);

	// if its not in the pak, we bail
	if (srcLength == -1)
	{
		return false;
	}

	// read in local file
	FILE* destHandle = fopen(fullpath, "rb");

	// if we have a local file, we need to compare the two
	if (destHandle)
	{
		fseek(destHandle, 0, SEEK_END);
		int destLength = ftell(destHandle);
		fseek(destHandle, 0, SEEK_SET);

		if (destLength > 0)
		{
			unsigned char* destData = (unsigned char*)Mem_Alloc(destLength);

			fread(destData, 1, destLength, destHandle);

			// compare files
			if (destLength == srcLength)
			{
				int i;
				for (i = 0; i < destLength; i++)
				{
					if (destData[i] != srcData[i])
					{
						break;
					}
				}

				if (i == destLength)
				{
					needToCopy = false;
				}
			}

			Mem_Free(destData);	// TTimo
		}

		fclose(destHandle);
	}

	// write file
	if (needToCopy)
	{
		fileHandle_t f = FS_FOpenFileWrite(filename);
		if (!f)
		{
			common->Printf("Failed to open %s\n", filename);
			return false;
		}

		FS_Write(srcData, srcLength, f);

		FS_FCloseFile(f);
	}

	FS_FreeFile(srcData);
	return true;
}

//	Called to find where to write a file (demos, savegames, etc)
const char* FS_Gamedir()
{
	return fs_gamedir;
}
