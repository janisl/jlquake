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

// HEADER FILES ------------------------------------------------------------

#include "core.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>

// MACROS ------------------------------------------------------------------

#define MAX_FOUND_FILES		0x1000

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

char* __CopyString(const char* in);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

/* base time in seconds, that's our origin
   timeval:tv_sec is an int: 
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */
unsigned long	sys_timeBase = 0;

bool			stdin_active = true;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static char		HomePathSuffix[MAX_OSPATH];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Sys_Mkdir
//
//==========================================================================

void Sys_Mkdir(const char* path)
{
	mkdir(path, 0777);
}

//==========================================================================
//
//	Sys_Cwd
//
//==========================================================================

const char* Sys_Cwd() 
{
	static char cwd[MAX_OSPATH];

	getcwd(cwd, sizeof(cwd) - 1);
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

//==========================================================================
//
//	Sys_SetHomePathSuffix
//
//==========================================================================

void Sys_SetHomePathSuffix(const char* Name)
{
	QStr::NCpyZ(HomePathSuffix, Name, sizeof(HomePathSuffix));
}

//==========================================================================
//
//	Sys_DefaultHomePath
//
//==========================================================================

const char* Sys_DefaultHomePath()
{
	static char homePath[MAX_OSPATH];
	const char* p = getenv("HOME");
	if (p)
	{
		QStr::NCpyZ(homePath, p, sizeof(homePath));
#ifdef MACOS_X
		QStr::Cat(homePath, sizeof(homePath), "/Library/Application Support/");
#else
		QStr::Cat(homePath, sizeof(homePath), "/.");
#endif
		QStr::Cat(homePath, sizeof(homePath), HomePathSuffix);
		if (mkdir(homePath, 0777))
		{
			if (errno != EEXIST) 
			{
				throw QException(va("Unable to create directory \"%s\", error is %s(%d)\n",
					homePath, strerror(errno), errno));
			}
		}
		return homePath;
	}
	return ""; // assume current dir
}

//==========================================================================
//
//	Sys_DefaultHomePath
//
//==========================================================================

static void Sys_ListFilteredFiles(const char* basedir, const char* subdirs, const char* filter,
	char** list, int* numfiles)
{
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	DIR			*fdir;
	struct dirent *d;
	struct stat st;

	if (*numfiles >= MAX_FOUND_FILES - 1)
	{
		return;
	}

	if (QStr::Length(subdirs))
	{
		QStr::Sprintf(search, sizeof(search), "%s/%s", basedir, subdirs);
	}
	else
	{
		QStr::Sprintf(search, sizeof(search), "%s", basedir);
	}

	if ((fdir = opendir(search)) == NULL)
	{
		return;
	}

	while ((d = readdir(fdir)) != NULL)
	{
		QStr::Sprintf(filename, sizeof(filename), "%s/%s", search, d->d_name);
		if (stat(filename, &st) == -1)
		{
			continue;
		}

		if (st.st_mode & S_IFDIR)
		{
			if (QStr::ICmp(d->d_name, ".") && QStr::ICmp(d->d_name, ".."))
			{
				if (QStr::Length(subdirs))
				{
					QStr::Sprintf(newsubdirs, sizeof(newsubdirs), "%s/%s", subdirs, d->d_name);
				}
				else
				{
					QStr::Sprintf(newsubdirs, sizeof(newsubdirs), "%s", d->d_name);
				}
				Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
			}
		}
		if (*numfiles >= MAX_FOUND_FILES - 1)
		{
			break;
		}
		QStr::Sprintf(filename, sizeof(filename), "%s/%s", subdirs, d->d_name);
		if (!QStr::FilterPath(filter, filename, false))
		{
			continue;
		}
		list[*numfiles] = __CopyString(filename);
		(*numfiles)++;
	}

	closedir(fdir);
}

//==========================================================================
//
//	Sys_ListFiles
//
//==========================================================================

char** Sys_ListFiles(const char *directory, const char *extension, const char *filter,
	int *numfiles, bool wantsubs)
{
	struct dirent *d;
	DIR*		fdir;
	bool		dironly = wantsubs;
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	//int			flag; // bk001204 - unused
	int			i;
	struct stat st;

	int			extLen;

	if (filter)
	{
		nfiles = 0;
		Sys_ListFilteredFiles(directory, "", filter, list, &nfiles);

		list[nfiles] = 0;
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

	if (!extension)
	{
		extension = "";
	}

	if (extension[0] == '/' && extension[1] == 0)
	{
		extension = "";
		dironly = true;
	}

	extLen = QStr::Length(extension);

	// search
	nfiles = 0;

	if ((fdir = opendir(directory)) == NULL)
	{
		*numfiles = 0;
		return NULL;
	}

	while ((d = readdir(fdir)) != NULL)
	{
		QStr::Sprintf(search, sizeof(search), "%s/%s", directory, d->d_name);
		if (stat(search, &st) == -1)
		{
			continue;
		}
		if ((dironly && !(st.st_mode & S_IFDIR)) ||
			(!dironly && (st.st_mode & S_IFDIR)))
		{
			continue;
		}

		if (*extension)
		{
			if (QStr::Length(d->d_name) < QStr::Length(extension) ||
				QStr::ICmp( 
					d->d_name + QStr::Length(d->d_name) - QStr::Length(extension),
					extension))
			{
				continue; // didn't match
			}
		}

		if (nfiles == MAX_FOUND_FILES - 1)
		{
			break;
		}
		list[nfiles] = __CopyString(d->d_name);
		nfiles++;
	}

	list[nfiles] = 0;

	closedir(fdir);

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

//==========================================================================
//
//	Sys_FreeFileList
//
//==========================================================================

void Sys_FreeFileList(char** list)
{
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

//==========================================================================
//
//	Sys_Milliseconds
//
//	current time in ms, using sys_timeBase as origin
//	NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
//	  0x7fffffff ms - ~24 days
//	although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
//	  (which would affect the wrap period)
//
//==========================================================================

int Sys_Milliseconds()
{
	struct timeval tp;

	gettimeofday(&tp, NULL);
	
	if (!sys_timeBase)
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}

	return (tp.tv_sec - sys_timeBase) * 1000 + tp.tv_usec / 1000;
}

//==========================================================================
//
//	Sys_CommonConsoleInput
//
//==========================================================================

char text[256];
char* Sys_CommonConsoleInput()
{
	if (!com_dedicated || !com_dedicated->value)
	{
		return NULL;
	}

	if (!stdin_active)
	{
		return NULL;
	}

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select(1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
	{
		return NULL;
	}

	int len = read(0, text, sizeof(text));
	if (len == 0)
	{
		// eof!
		stdin_active = false;
		return NULL;
	}

	if (len < 1)
	{
		return NULL;
	}
	text[len - 1] = 0;    // rip off the /n and terminate

	return text;
}
