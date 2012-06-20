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

Cvar* nomonsters;
Cvar* gamecfg;
Cvar* scratch1;
Cvar* scratch2;
Cvar* scratch3;
Cvar* scratch4;
Cvar* savedgamecfg;
Cvar* saved1;
Cvar* saved2;
Cvar* saved3;
Cvar* saved4;

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

/*
===============
PR_Init
===============
*/
void PR_Init(void)
{
	PR_InitBuiltins();
	Cmd_AddCommand("edict", ED_PrintEdict_f);
	Cmd_AddCommand("edicts", ED_PrintEdicts);
	Cmd_AddCommand("edictcount", ED_Count);
	Cmd_AddCommand("profile", PR_Profile_f);
	nomonsters = Cvar_Get("nomonsters", "0", 0);
	gamecfg = Cvar_Get("gamecfg", "0", 0);
	scratch1 = Cvar_Get("scratch1", "0", 0);
	scratch2 = Cvar_Get("scratch2", "0", 0);
	scratch3 = Cvar_Get("scratch3", "0", 0);
	scratch4 = Cvar_Get("scratch4", "0", 0);
	savedgamecfg = Cvar_Get("savedgamecfg", "0", CVAR_ARCHIVE);
	saved1 = Cvar_Get("saved1", "0", CVAR_ARCHIVE);
	saved2 = Cvar_Get("saved2", "0", CVAR_ARCHIVE);
	saved3 = Cvar_Get("saved3", "0", CVAR_ARCHIVE);
	saved4 = Cvar_Get("saved4", "0", CVAR_ARCHIVE);
	max_temp_edicts = Cvar_Get("max_temp_edicts", "30", CVAR_ARCHIVE);
}
