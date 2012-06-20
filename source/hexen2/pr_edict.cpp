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


unsigned short pr_crc;

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

static void GetProgsName(char* finalprogname)
{
	String::Cpy(finalprogname, "progs.dat");
	Array<byte> MapList;
	FS_ReadFile("maplist.txt", MapList);
	MapList.Append(0);
	const char* p = (char*)MapList.Ptr();
	const char* token = String::Parse2(&p);
	int NumMaps = String::Atoi(token);
	for (int i = 0; i < NumMaps; i++)
	{
		token = String::Parse2(&p);
		if (!String::ICmp(token, sv.name))
		{
			token = String::Parse2(&p);
			String::Cpy(finalprogname, token);
		}
		else
		{
			token = String::Parse2(&p);
		}
	}
}

/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs(void)
{
	int i;
	char finalprogname[MAX_OSPATH];

	ED_ClearGEFVCache();

	CRC_Init(&pr_crc);

	GetProgsName(finalprogname);

	progs = (dprograms_t*)COM_LoadHunkFile(finalprogname);
	if (!progs)
	{
		Sys_Error("PR_LoadProgs: couldn't load %s",finalprogname);
	}
	Con_DPrintf("Programs occupy %iK.\n", com_filesize / 1024);

	for (i = 0; i < com_filesize; i++)
		CRC_ProcessByte(&pr_crc, ((byte*)progs)[i]);

// byte swap the header
	for (i = 0; i < (int)sizeof(*progs) / 4; i++)
		((int*)progs)[i] = LittleLong(((int*)progs)[i]);

	if (progs->version != PROG_VERSION)
	{
		Sys_Error("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
	}
	if (progs->crc != (GGameType & GAME_H2Portals ? H2MPPROGHEADER_CRC : H2PROGHEADER_CRC))
	{
		Sys_Error("progs.dat system vars have been modified, progdefs.h is out of date");
	}

	pr_functions = (dfunction_t*)((byte*)progs + progs->ofs_functions);
	pr_strings = (char*)progs + progs->ofs_strings;
	pr_globaldefs = (ddef_t*)((byte*)progs + progs->ofs_globaldefs);
	pr_fielddefs = (ddef_t*)((byte*)progs + progs->ofs_fielddefs);
	pr_statements = (dstatement_t*)((byte*)progs + progs->ofs_statements);

	pr_globals = (float*)((byte*)progs + progs->ofs_globals);

	pr_edict_size = progs->entityfields * 4 + sizeof(qhedict_t) - sizeof(entvars_t);

	if (GBigEndian)	// byte swap the lumps
	{
		for (i = 0; i < progs->numstatements; i++)
		{
			pr_statements[i].op = LittleShort(pr_statements[i].op);
			pr_statements[i].a = LittleShort(pr_statements[i].a);
			pr_statements[i].b = LittleShort(pr_statements[i].b);
			pr_statements[i].c = LittleShort(pr_statements[i].c);
		}

		for (i = 0; i < progs->numfunctions; i++)
		{
			pr_functions[i].first_statement = LittleLong(pr_functions[i].first_statement);
			pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
			pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
			pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
			pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
			pr_functions[i].locals = LittleLong(pr_functions[i].locals);
		}

		for (i = 0; i < progs->numglobaldefs; i++)
		{
			pr_globaldefs[i].type = LittleShort(pr_globaldefs[i].type);
			pr_globaldefs[i].ofs = LittleShort(pr_globaldefs[i].ofs);
			pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
		}

		for (i = 0; i < progs->numfielddefs; i++)
		{
			pr_fielddefs[i].type = LittleShort(pr_fielddefs[i].type);
			if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
			{
				Sys_Error("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
			}
			pr_fielddefs[i].ofs = LittleShort(pr_fielddefs[i].ofs);
			pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
		}

		for (i = 0; i < progs->numglobals; i++)
			((int*)pr_globals)[i] = LittleLong(((int*)pr_globals)[i]);
	}

	ED_InitEntityFields();
	PR_InitGlobals();
#ifdef MISSIONPACK
	// set the cl_playerclass value after pr_global_struct has been created
	*pr_globalVars.cl_playerclass = clh2_playerclass->value;
#endif
}


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
