//**************************************************************************
//**
//**	$Id: exception.cpp 105 2010-04-04 19:24:08Z dj_jl $
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

// MACROS ------------------------------------------------------------------

#define MAX_FOUND_FILES		0x1000

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

char* __CopyString(const char* in);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

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
	_mkdir(path);
}

//==========================================================================
//
//	Sys_Cwd
//
//==========================================================================

const char* Sys_Cwd()
{
	static char cwd[MAX_OSPATH];

	_getcwd(cwd, sizeof(cwd) - 1);
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
	return NULL;
}

//==========================================================================
//
//	Sys_ListFilteredFiles
//
//==========================================================================

static void Sys_ListFilteredFiles(const char* basedir, char* subdirs, char* filter,
	char** list, int* numfiles)
{
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	int			findhandle;
	struct _finddata_t findinfo;

	if (*numfiles >= MAX_FOUND_FILES - 1)
	{
		return;
	}

	if (QStr::Length(subdirs))
	{
		QStr::Sprintf(search, sizeof(search), "%s\\%s\\*", basedir, subdirs);
	}
	else
	{
		QStr::Sprintf(search, sizeof(search), "%s\\*", basedir);
	}

	findhandle = _findfirst(search, &findinfo);
	if (findhandle == -1)
	{
		return;
	}

	do
	{
		if (findinfo.attrib & _A_SUBDIR)
		{
			if (QStr::ICmp(findinfo.name, ".") && QStr::ICmp(findinfo.name, ".."))
			{
				if (QStr::Length(subdirs))
				{
					QStr::Sprintf(newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else
				{
					QStr::Sprintf(newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
			}
		}
		if (*numfiles >= MAX_FOUND_FILES - 1)
		{
			break;
		}
		QStr::Sprintf(filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name);
		if (!Com_FilterPath(filter, filename, false))
		{
			continue;
		}
		list[*numfiles] = __CopyString(filename);
		(*numfiles)++;
	} while (_findnext(findhandle, &findinfo) != -1);

	_findclose(findhandle);
}

//==========================================================================
//
//	Sys_ListFiles
//
//==========================================================================

char** Sys_ListFiles(const char* directory, const char* extension, char* filter,
	int* numfiles, bool wantsubs)
{
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	int			findhandle;
	int			flag;
	int			i;

	if (filter)
	{
		nfiles = 0;
		Sys_ListFilteredFiles(directory, "", filter, list, &nfiles);

		list[nfiles] = 0;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

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

	// passing a slash as extension will find directories
	if (extension[0] == '/' && extension[1] == 0)
	{
		extension = "";
		flag = 0;
	}
	else
	{
		flag = _A_SUBDIR;
	}

	QStr::Sprintf(search, sizeof(search), "%s\\*%s", directory, extension);

	// search
	nfiles = 0;

	findhandle = _findfirst(search, &findinfo);
	if (findhandle == -1)
	{
		*numfiles = 0;
		return NULL;
	}

	do
	{
		if ((!wantsubs && flag ^ (findinfo.attrib & _A_SUBDIR)) || (wantsubs && findinfo.attrib & _A_SUBDIR) )
		{
			if (nfiles == MAX_FOUND_FILES - 1)
			{
				break;
			}
			list[nfiles] = __CopyString(findinfo.name);
			nfiles++;
		}
	} while (_findnext(findhandle, &findinfo) != -1);

	list[nfiles] = 0;

	_findclose(findhandle);

	// return a copy of the list
	*numfiles = nfiles;

	if (!nfiles)
	{
		return NULL;
	}

	listCopy = (char**)Mem_Alloc((nfiles + 1 ) * sizeof(*listCopy));
	for (i = 0; i < nfiles; i++)
	{
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	do
	{
		flag = 0;
		for (i = 1; i < nfiles; i++)
		{
			if (strgtr(listCopy[i - 1], listCopy[i]))
			{
				char *temp = listCopy[i];
				listCopy[i] = listCopy[i-1];
				listCopy[i-1] = temp;
				flag = 1;
			}
		}
	} while(flag);

	return listCopy;
}

//==========================================================================
//
//	Sys_ListFiles
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
