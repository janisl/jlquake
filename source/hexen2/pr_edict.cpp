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
Cvar* max_temp_edicts;

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
qhedict_t* ED_Alloc(void)
{
	int i;
	qhedict_t* e;

	for (i = svs.qh_maxclients + 1 + max_temp_edicts->value; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && (e->freetime < 2 || sv.qh_time - e->freetime > 0.5))
		{
			ED_ClearEdict(e);
			return e;
		}
	}

	if (i == MAX_EDICTS_QH)
	{
		SV_Edicts("edicts.txt");
		Sys_Error("ED_Alloc: no free edicts");
	}

	sv.qh_num_edicts++;
	e = QH_EDICT_NUM(i);
	ED_ClearEdict(e);

	return e;
}

qhedict_t* ED_Alloc_Temp(void)
{
	int i,j,Found;
	qhedict_t* e,* Least;
	float LeastTime;
	qboolean LeastSet;

	LeastTime = -1;
	LeastSet = false;
	for (i = svs.qh_maxclients + 1,j = 0; j < max_temp_edicts->value; i++,j++)
	{
		e = QH_EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && (e->freetime < 2 || sv.qh_time - e->freetime > 0.5))
		{
			ED_ClearEdict(e);
			e->alloctime = sv.qh_time;

			return e;
		}
		else if (e->alloctime < LeastTime || !LeastSet)
		{
			Least = e;
			LeastTime = e->alloctime;
			Found = j;
			LeastSet = true;
		}
	}

	ED_Free(Least);
	ED_ClearEdict(Least);
	Least->alloctime = sv.qh_time;

	return Least;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free(qhedict_t* ed)
{
	SV_UnlinkEdict(ed);			// unlink from world bsp

	ed->free = true;
	ed->SetModel(0);
	ed->SetTakeDamage(0);
	ed->v.modelindex = 0;
	ed->SetColorMap(0);
	ed->SetSkin(0);
	ed->SetFrame(0);
	VectorCopy(vec3_origin, ed->GetOrigin());
	ed->SetAngles(vec3_origin);
	ed->SetNextThink(-1);
	ed->SetSolid(0);

	ed->freetime = sv.qh_time;
	ed->alloctime = -1;
}

//============================================================================


/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile(const char* data)
{
	qhedict_t* ent;
	int inhibit,skip;
	dfunction_t* func;
	const char* orig;
	int start_amount;

	ent = NULL;
	inhibit = 0;
	*pr_globalVars.time = sv.qh_time;
	orig = data;
	int entity_file_size = String::Length(data);

	start_amount = current_loading_size;
// parse ents
	while (1)
	{
// parse the opening brace
		char* token = String::Parse1(&data);
		if (!data)
		{
			break;
		}

		if (entity_file_size)
		{
			current_loading_size = start_amount + ((data - orig) * 80 / entity_file_size);
			SCR_UpdateScreen();
		}

		if (token[0] != '{')
		{
			Sys_Error("ED_LoadFromFile: found %s when expecting {",token);
		}

		if (!ent)
		{
			ent = QH_EDICT_NUM(0);
		}
		else
		{
			ent = ED_Alloc();
		}
		data = ED_ParseEdict(data, ent);

#if 0
		//jfm fuckup test
		//remove for final release
		if ((ent->v.spawnflags > 1) && !String::Cmp("worldspawn",PR_GetString(ent->v.classname)))
		{
			Host_Error("invalid SpawnFlags on World!!!\n");
		}
#endif

// remove things from different skill levels or deathmatch
		if (deathmatch->value)
		{
			if (((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_DEATHMATCH))
			{
				ED_Free(ent);
				inhibit++;
				continue;
			}
		}
		else if (coop->value)
		{
			if (((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_COOP))
			{
				ED_Free(ent);
				inhibit++;
				continue;
			}
		}
		else
		{	// Gotta be single player
			if (((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_SINGLE))
			{
				ED_Free(ent);
				inhibit++;
				continue;
			}

			skip = 0;

			switch ((int)clh2_playerclass->value)
			{
			case CLASS_PALADIN:
				if ((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_PALADIN)
				{
					skip = 1;
				}
				break;

			case CLASS_CLERIC:
				if ((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_CLERIC)
				{
					skip = 1;
				}
				break;

			case CLASS_DEMON:
			case CLASS_NECROMANCER:
				if ((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_NECROMANCER)
				{
					skip = 1;
				}
				break;

			case CLASS_THEIF:
				if ((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_THEIF)
				{
					skip = 1;
				}
				break;
			}

			if (skip)
			{
				ED_Free(ent);
				inhibit++;
				continue;
			}
		}

		if ((current_skill == 0 && ((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_EASY)) ||
			(current_skill == 1 && ((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_MEDIUM)) ||
			(current_skill >= 2 && ((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_HARD)))
		{
			ED_Free(ent);
			inhibit++;
			continue;
		}

//
// immediately call spawn function
//
		if (!ent->GetClassName())
		{
			Con_Printf("No classname for:\n");
			ED_Print(ent);
			ED_Free(ent);
			continue;
		}

		// look for the spawn function
		func = ED_FindFunction(PR_GetString(ent->GetClassName()));

		if (!func)
		{
			Con_Printf("No spawn function for:\n");
			ED_Print(ent);
			ED_Free(ent);
			continue;
		}

		*pr_globalVars.self = EDICT_TO_PROG(ent);
		PR_ExecuteProgram(func - pr_functions);
	}

	Con_DPrintf("%i entities inhibited\n", inhibit);
}

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
