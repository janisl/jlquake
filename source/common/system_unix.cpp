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

// HEADER FILES ------------------------------------------------------------

#include "qcommon.h"
#include "system_unix.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>

#define MAX_FOUND_FILES		0x1000

char* __CopyString(const char* in);

/* base time in seconds, that's our origin
   timeval:tv_sec is an int: 
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */
unsigned long	sys_timeBase = 0;

bool			stdin_active = true;

static char		HomePathSuffix[MAX_OSPATH];

//	Test an file given OS path:
//	returns -1 if not found
//	returns 1 if directory
//	returns 0 otherwise
int Sys_StatFile(const char *ospath)
{
	struct stat stat_buf;
	if (stat(ospath, &stat_buf) == -1)
	{
		return -1;
	}
	if (S_ISDIR(stat_buf.st_mode))
	{
		return 1;
	}
	return 0;
}

void Sys_Mkdir(const char* path)
{
	mkdir(path, 0777);
}

int Sys_Rmdir(const char* path)
{
	return rmdir(path);
}

const char* Sys_Cwd() 
{
	static char cwd[MAX_OSPATH];

	getcwd(cwd, sizeof(cwd) - 1);
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

void Sys_SetHomePathSuffix(const char* Name)
{
	String::NCpyZ(HomePathSuffix, Name, sizeof(HomePathSuffix));
}

const char* Sys_DefaultHomePath()
{
	static char homePath[MAX_OSPATH];
	const char* p = getenv("HOME");
	if (p)
	{
		String::NCpyZ(homePath, p, sizeof(homePath));
#ifdef MACOS_X
		String::Cat(homePath, sizeof(homePath), "/Library/Application Support/");
#else
		String::Cat(homePath, sizeof(homePath), "/.");
#endif
		String::Cat(homePath, sizeof(homePath), HomePathSuffix);
		if (mkdir(homePath, 0777))
		{
			if (errno != EEXIST) 
			{
				throw Exception(va("Unable to create directory \"%s\", error is %s(%d)\n",
					homePath, strerror(errno), errno));
			}
		}
		return homePath;
	}
	return ""; // assume current dir
}

static void Sys_ListFilteredFiles(const char* basedir, const char* subdirs, const char* filter,
	char** list, int* numfiles)
{
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	DIR*		fdir;
	dirent*		d;
	struct stat	st;

	if (*numfiles >= MAX_FOUND_FILES - 1)
	{
		return;
	}

	if (String::Length(subdirs))
	{
		String::Sprintf(search, sizeof(search), "%s/%s", basedir, subdirs);
	}
	else
	{
		String::Sprintf(search, sizeof(search), "%s", basedir);
	}

	if ((fdir = opendir(search)) == NULL)
	{
		return;
	}

	while ((d = readdir(fdir)) != NULL)
	{
		String::Sprintf(filename, sizeof(filename), "%s/%s", search, d->d_name);
		if (stat(filename, &st) == -1)
		{
			continue;
		}

		if (st.st_mode & S_IFDIR)
		{
			if (String::ICmp(d->d_name, ".") && String::ICmp(d->d_name, ".."))
			{
				if (String::Length(subdirs))
				{
					String::Sprintf(newsubdirs, sizeof(newsubdirs), "%s/%s", subdirs, d->d_name);
				}
				else
				{
					String::Sprintf(newsubdirs, sizeof(newsubdirs), "%s", d->d_name);
				}
				Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
			}
		}
		if (*numfiles >= MAX_FOUND_FILES - 1)
		{
			break;
		}
		String::Sprintf(filename, sizeof(filename), "%s/%s", subdirs, d->d_name);
		if (!String::FilterPath(filter, filename, false))
		{
			continue;
		}
		list[*numfiles] = __CopyString(filename);
		(*numfiles)++;
	}

	closedir(fdir);
}

char** Sys_ListFiles(const char *directory, const char *extension, const char *filter,
	int *numfiles, bool wantsubs)
{
	dirent*		d;
	DIR*		fdir;
	bool		dironly = wantsubs;
	char		search[MAX_OSPATH];
	int			nfiles;
	char**		listCopy;
	char*		list[MAX_FOUND_FILES];
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

	extLen = String::Length(extension);

	// search
	nfiles = 0;

	if ((fdir = opendir(directory)) == NULL)
	{
		*numfiles = 0;
		return NULL;
	}

	while ((d = readdir(fdir)) != NULL)
	{
		String::Sprintf(search, sizeof(search), "%s/%s", directory, d->d_name);
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
			if (String::Length(d->d_name) < String::Length(extension) ||
				String::ICmp( 
					d->d_name + String::Length(d->d_name) - String::Length(extension),
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

//	current time in ms, using sys_timeBase as origin
//	NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
//	  0x7fffffff ms - ~24 days
//	although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
//	  (which would affect the wrap period)
int Sys_Milliseconds()
{
	timeval tp;
	gettimeofday(&tp, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}

	return (tp.tv_sec - sys_timeBase) * 1000 + tp.tv_usec / 1000;
}

double Sys_DoubleTime()
{
	timeval tp;
	gettimeofday(&tp, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000000.0;
	}

	return (tp.tv_sec - sys_timeBase) + tp.tv_usec / 1000000.0;
}
