// sv_edict.c -- entity dictionary

#include "qwsvdef.h"

qboolean ignore_precache = false;

func_t SpectatorConnect;
func_t SpectatorThink;
func_t SpectatorDisconnect;

/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs(void)
{
	char num[32];
	dfunction_t* f;
	int i;
	char finalprogname[MAX_OSPATH];

	ED_ClearGEFVCache();

	String::Cpy(finalprogname, "hwprogs.dat");

	progs = (dprograms_t*)COM_LoadHunkFile(finalprogname);
	if (!progs)
	{
		progs = (dprograms_t*)COM_LoadHunkFile("progs.dat");
	}
	if (!progs)
	{
		SV_Error("PR_LoadProgs: couldn't load progs.dat");
	}
	Con_DPrintf("Programs occupy %iK.\n", com_filesize / 1024);

// add prog crc to the serverinfo
	sprintf(num, "%i", CRC_Block((byte*)progs, com_filesize));
	Info_SetValueForKey(svs.qh_info, "*progs", num, MAX_SERVERINFO_STRING, 64, 64, !sv_highchars->value);

// byte swap the header
	for (i = 0; i < (int)sizeof(*progs) / 4; i++)
		((int*)progs)[i] = LittleLong(((int*)progs)[i]);

	if (progs->version != PROG_VERSION)
	{
		SV_Error("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
	}
//	if (progs->crc != PROGHEADER_CRC)
//		SV_Error ("You must have the progs.dat from HexenWorld installed");

	pr_functions = (dfunction_t*)((byte*)progs + progs->ofs_functions);
	pr_strings = (char*)progs + progs->ofs_strings;
	pr_globaldefs = (ddef_t*)((byte*)progs + progs->ofs_globaldefs);
	pr_fielddefs = (ddef_t*)((byte*)progs + progs->ofs_fielddefs);
	pr_statements = (dstatement_t*)((byte*)progs + progs->ofs_statements);
	pr_globals = (float*)((byte*)progs + progs->ofs_globals);

	pr_edict_size = progs->entityfields * 4 + sizeof(qhedict_t) - sizeof(entvars_t);

// byte swap the lumps
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
			SV_Error("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
		}
		pr_fielddefs[i].ofs = LittleShort(pr_fielddefs[i].ofs);
		pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
	}

	for (i = 0; i < progs->numglobals; i++)
		((int*)pr_globals)[i] = LittleLong(((int*)pr_globals)[i]);

	// Zoid, find the spectator functions
	SpectatorConnect = SpectatorThink = SpectatorDisconnect = 0;

	if ((f = ED_FindFunction("SpectatorConnect")) != NULL)
	{
		SpectatorConnect = (func_t)(f - pr_functions);
	}
	if ((f = ED_FindFunction("SpectatorThink")) != NULL)
	{
		SpectatorThink = (func_t)(f - pr_functions);
	}
	if ((f = ED_FindFunction("SpectatorDisconnect")) != NULL)
	{
		SpectatorDisconnect = (func_t)(f - pr_functions);
	}

	ED_InitEntityFields();
	PR_InitGlobals();
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
	max_temp_edicts = Cvar_Get("max_temp_edicts", "30", CVAR_ARCHIVE);
}
