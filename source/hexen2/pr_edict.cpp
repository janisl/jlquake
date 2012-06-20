// sv_edict.c -- entity dictionary

/*
 * $Header: /H2 Mission Pack/Pr_edict.c 11    3/27/98 2:12p Jmonroe $
 */

#include "quakedef.h"

// For international stuff
int* pr_string_index = NULL;
char* pr_global_strings = NULL;
int pr_string_count = 0;

#ifdef MISSIONPACK
int* pr_info_string_index = NULL;
char* pr_global_info_strings = NULL;
int pr_info_string_count = 0;
#endif

qboolean ignore_precache = false;

#ifdef MISSIONPACK
void PR_LoadInfoStrings(void)
{
	int i,count,start,Length;
	char NewLineChar;

	pr_global_info_strings = (char*)COM_LoadHunkFile("infolist.txt");
	if (!pr_global_info_strings)
	{
		Sys_Error("PR_LoadInfoStrings: couldn't load infolist.txt");
	}

	NewLineChar = -1;

	for (i = count = 0; pr_global_info_strings[i] != 0; i++)
	{
		if (pr_global_info_strings[i] == 13 || pr_global_info_strings[i] == 10)
		{
			if (NewLineChar == pr_global_info_strings[i] || NewLineChar == -1)
			{
				NewLineChar = pr_global_info_strings[i];
				count++;
			}
		}
	}
	Length = i;

	if (!count)
	{
		Sys_Error("PR_LoadInfoStrings: no string lines found");
	}

	pr_info_string_index = (int*)Hunk_AllocName((count + 1) * 4, "info_string_index");

	for (i = count = start = 0; pr_global_info_strings[i] != 0; i++)
	{
		if (pr_global_info_strings[i] == 13 || pr_global_info_strings[i] == 10)
		{
			if (NewLineChar == pr_global_info_strings[i])
			{
				pr_info_string_index[count] = start;
				start = i + 1;
				count++;
			}
			else
			{
				start++;
			}

			pr_global_info_strings[i] = 0;
		}
	}

	pr_info_string_count = count;
	Con_Printf("Read in %d objectives\n",count);
}
#endif

void PR_LoadStrings(void)
{
	int i,count,start,Length;
	char NewLineChar;

	pr_global_strings = (char*)COM_LoadHunkFile("strings.txt");
	if (!pr_global_strings)
	{
		Sys_Error("PR_LoadStrings: couldn't load strings.txt");
	}

	NewLineChar = -1;

	for (i = count = 0; pr_global_strings[i] != 0; i++)
	{
		if (pr_global_strings[i] == 13 || pr_global_strings[i] == 10)
		{
			if (NewLineChar == pr_global_strings[i] || NewLineChar == -1)
			{
				NewLineChar = pr_global_strings[i];
				count++;
			}
		}
	}
	Length = i;

	if (!count)
	{
		Sys_Error("PR_LoadStrings: no string lines found");
	}

	pr_string_index = (int*)Hunk_AllocName((count + 1) * 4, "string_index");

	for (i = count = start = 0; pr_global_strings[i] != 0; i++)
	{
		if (pr_global_strings[i] == 13 || pr_global_strings[i] == 10)
		{
			if (NewLineChar == pr_global_strings[i])
			{
				pr_string_index[count] = start;
				start = i + 1;
				count++;
			}
			else
			{
				start++;
			}

			pr_global_strings[i] = 0;
		}
	}

	pr_string_count = count;
	Con_Printf("Read in %d string lines\n",count);
}
