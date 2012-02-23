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

#include "core.h"
#include "system_windows.h"
#include <direct.h>
#include <io.h>

// MACROS ------------------------------------------------------------------

#define MAX_FOUND_FILES		0x1000

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

char* __CopyString(const char* in);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

HINSTANCE		global_hInstance;
// when we get a windows message, we store the time off so keyboard processing
// can know the exact time of an event
unsigned		sysMsgTime;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static char		HomePathSuffix[MAX_OSPATH];

static double	lastcurtime = 0.0;
static double		curtime = 0.0;
static int			lowshift;
static double		pfreq;

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
	String::NCpyZ(HomePathSuffix, Name, sizeof(HomePathSuffix));
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

static void Sys_ListFilteredFiles(const char* basedir, const char* subdirs, const char* filter,
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

	if (String::Length(subdirs))
	{
		String::Sprintf(search, sizeof(search), "%s\\%s\\*", basedir, subdirs);
	}
	else
	{
		String::Sprintf(search, sizeof(search), "%s\\*", basedir);
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
			if (String::ICmp(findinfo.name, ".") && String::ICmp(findinfo.name, ".."))
			{
				if (String::Length(subdirs))
				{
					String::Sprintf(newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else
				{
					String::Sprintf(newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
			}
		}
		if (*numfiles >= MAX_FOUND_FILES - 1)
		{
			break;
		}
		String::Sprintf(filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name);
		if (!String::FilterPath(filter, filename, false))
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
//	strgtr
//
//==========================================================================

static bool strgtr(const char *s0, const char *s1)
{
	int l0 = String::Length(s0);
	int l1 = String::Length(s1);

	if (l1 < l0)
	{
		l0 = l1;
	}

	for (int i = 0; i < l0; i++)
	{
		if (s1[i] > s0[i])
		{
			return true;
		}
		if (s1[i] < s0[i])
		{
			return false;
		}
	}
	return false;
}

//==========================================================================
//
//	Sys_ListFiles
//
//==========================================================================

char** Sys_ListFiles(const char* directory, const char* extension, const char* filter,
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

	String::Sprintf(search, sizeof(search), "%s\\*%s", directory, extension);

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

//==========================================================================
//
//	Sys_Milliseconds
//
//==========================================================================

int Sys_Milliseconds()
{
	static int		base;
	static bool		initialized = false;

	if (!initialized)
	{
		base = timeGetTime();
		initialized = true;
	}
	return timeGetTime() - base;
}

//==========================================================================
//
//	Sys_Milliseconds
//
//==========================================================================

double Sys_DoubleTime()
{
#if 1
	//	Method used in Quake, Hexen 2 and HexenWorld client
	static int			sametimecount;
	static unsigned int	oldtime;
	static int			first = 1;
	LARGE_INTEGER		PerformanceCount;
	unsigned int		temp, t2;
	double				time;

	if (first)
	{
		LARGE_INTEGER	PerformanceFreq;
		unsigned int	lowpart, highpart;

		if (!QueryPerformanceFrequency (&PerformanceFreq))
			throw Exception("No hardware timer available");

		// get 32 out of the 64 time bits such that we have around
		// 1 microsecond resolution
		lowpart = (unsigned int)PerformanceFreq.LowPart;
		highpart = (unsigned int)PerformanceFreq.HighPart;
		lowshift = 0;

		while (highpart || (lowpart > 2000000.0))
		{
			lowshift++;
			lowpart >>= 1;
			lowpart |= (highpart & 1) << 31;
			highpart >>= 1;
		}

		pfreq = 1.0 / (double)lowpart;
	}

	QueryPerformanceCounter (&PerformanceCount);

	temp = ((unsigned int)PerformanceCount.LowPart >> lowshift) |
		   ((unsigned int)PerformanceCount.HighPart << (32 - lowshift));

	if (first)
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
		// check for turnover or backward time
		if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
		{
			oldtime = temp;	// so we can't get stuck
		}
		else
		{
			t2 = temp - oldtime;

			time = (double)t2 * pfreq;
			oldtime = temp;

			curtime += time;

			if (curtime == lastcurtime)
			{
				sametimecount++;

				if (sametimecount > 100000)
				{
					curtime += 1.0;
					sametimecount = 0;
				}
			}
			else
			{
				sametimecount = 0;
			}

			lastcurtime = curtime;
		}
	}

    return curtime;
#elif 0
	//	Method used in QuakeWorld client
	static DWORD starttime;
	static qboolean first = true;
	DWORD now;
	double t;

	now = timeGetTime();

	if (first) {
		first = false;
		starttime = now;
		return 0.0;
	}
	
	if (now < starttime) // wrapped?
		return (now / 1000.0) + (LONG_MAX - starttime / 1000.0);

	if (now - starttime == 0)
		return 0.0;

	return (now - starttime) / 1000.0;
#else
	//	Method used in QuakeWorld and HexenWorld servers.
	double t;
    struct _timeb tstruct;
	static int	starttime;

	_ftime( &tstruct );
 
	if (!starttime)
		starttime = tstruct.time;
	t = (tstruct.time-starttime) + tstruct.millitm*0.001;
	
	return t;
#endif
}
