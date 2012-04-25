#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "../linux/glob.h"

#include "../qcommon/qcommon.h"

int curtime;

//============================================

static char findbase[MAX_OSPATH];
static char findpath[MAX_OSPATH];
static char findpattern[MAX_OSPATH];
static DIR* fdir;

static qboolean CompareAttributes(char* path, char* name,
	unsigned musthave, unsigned canthave)
{
	struct stat st;
	char fn[MAX_OSPATH];

// . and .. never match
	if (String::Cmp(name, ".") == 0 || String::Cmp(name, "..") == 0)
	{
		return false;
	}

	sprintf(fn, "%s/%s", path, name);
	if (stat(fn, &st) == -1)
	{
		return false;	// shouldn't happen

	}
	if ((st.st_mode & S_IFDIR) && (canthave & SFF_SUBDIR))
	{
		return false;
	}

	if ((musthave & SFF_SUBDIR) && !(st.st_mode & S_IFDIR))
	{
		return false;
	}

	return true;
}

char* Sys_FindFirst(char* path, unsigned musthave, unsigned canhave)
{
	struct dirent* d;
	char* p;

	if (fdir)
	{
		Sys_Error("Sys_BeginFind without close");
	}

//	COM_FilePath (path, findbase);
	String::Cpy(findbase, path);

	if ((p = String::RChr(findbase, '/')) != NULL)
	{
		*p = 0;
		String::Cpy(findpattern, p + 1);
	}
	else
	{
		String::Cpy(findpattern, "*");
	}

	if (String::Cmp(findpattern, "*.*") == 0)
	{
		String::Cpy(findpattern, "*");
	}

	if ((fdir = opendir(findbase)) == NULL)
	{
		return NULL;
	}
	while ((d = readdir(fdir)) != NULL)
	{
		if (!*findpattern || glob_match(findpattern, d->d_name))
		{
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d->d_name, musthave, canhave))
			{
				sprintf(findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

char* Sys_FindNext(unsigned musthave, unsigned canhave)
{
	struct dirent* d;

	if (fdir == NULL)
	{
		return NULL;
	}
	while ((d = readdir(fdir)) != NULL)
	{
		if (!*findpattern || glob_match(findpattern, d->d_name))
		{
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d->d_name, musthave, canhave))
			{
				sprintf(findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

void Sys_FindClose(void)
{
	if (fdir != NULL)
	{
		closedir(fdir);
	}
	fdir = NULL;
}


//============================================
