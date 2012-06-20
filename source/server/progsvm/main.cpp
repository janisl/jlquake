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

#include "../server.h"
#include "progsvm.h"
#include "../quake_hexen/local.h"

dprograms_t* progs;
dfunction_t* pr_functions;
char* pr_strings;
ddef_t* pr_fielddefs;
ddef_t* pr_globaldefs;
dstatement_t* pr_statements;
float* pr_globals;			// same as pr_global_struct
int pr_edict_size;			// in bytes
progGlobalVars_t pr_globalVars;
unsigned short pr_crc;

static Array<const char*> pr_strtbl;

static void PR_ClearStringMap()
{
	pr_strtbl.Clear();
}

int PR_SetString(const char* s)
{
	if (s < pr_strings || s >= pr_strings + progs->numstrings)
	{
		for (int i = 0; i < pr_strtbl.Num(); i++)
		{
			if (pr_strtbl[i] == s)
			{
				return -i - 1;
			}
		}
		pr_strtbl.Append(s);
		return -pr_strtbl.Num();
	}
	return (int)(s - pr_strings);
}

const char* PR_GetString(int number)
{
	if (number < 0)
	{
		return pr_strtbl[-number - 1];
	}
	return pr_strings + number;
}

const char* ED_NewString(const char* string)
{
	char* news, * new_p;
	int i,l;

	l = String::Length(string) + 1;
	news = (char*)Mem_Alloc(l);
	new_p = news;

	for (i = 0; i < l; i++)
	{
		if (string[i] == '\\' && i < l - 1)
		{
			i++;
			if (string[i] == 'n')
			{
				*new_p++ = '\n';
			}
			else
			{
				*new_p++ = '\\';
			}
		}
		else
		{
			*new_p++ = string[i];
		}
	}

	return news;
}

static ddef_t* ED_GlobalAtOfs(int offset)
{
	for (int i = 0; i < progs->numglobaldefs; i++)
	{
		ddef_t* def = &pr_globaldefs[i];
		if (def->ofs == offset)
		{
			return def;
		}
	}
	return NULL;
}

ddef_t* ED_FieldAtOfs(int offset)
{
	for (int i = 0; i < progs->numfielddefs; i++)
	{
		ddef_t* def = &pr_fielddefs[i];
		if (def->ofs == offset)
		{
			return def;
		}
	}
	return NULL;
}

ddef_t* ED_FindField(const char* name)
{
	for (int i = 0; i < progs->numfielddefs; i++)
	{
		ddef_t* def = &pr_fielddefs[i];
		if (!String::Cmp(PR_GetString(def->s_name), name))
		{
			return def;
		}
	}
	return NULL;
}

ddef_t* ED_FindGlobal(const char* name)
{
	for (int i = 0; i < progs->numglobaldefs; i++)
	{
		ddef_t* def = &pr_globaldefs[i];
		if (!String::Cmp(PR_GetString(def->s_name), name))
		{
			return def;
		}
	}
	return NULL;
}

dfunction_t* ED_FindFunction(const char* name)
{
	for (int i = 0; i < progs->numfunctions; i++)
	{
		dfunction_t* func = &pr_functions[i];
		if (!String::Cmp(PR_GetString(func->s_name), name))
		{
			return func;
		}
	}
	return NULL;
}

dfunction_t* ED_FindFunctioni(const char* name)
{
	for (int i = 0; i < progs->numfunctions; i++)
	{
		dfunction_t* func = &pr_functions[i];
		if (!String::ICmp(PR_GetString(func->s_name), name))
		{
			return func;
		}
	}
	return NULL;
}

//	Returns a string describing *data in a type specific manner
const char* PR_ValueString(etype_t type, const eval_t* val)
{
	static char line[256];
	ddef_t* def;
	dfunction_t* f;

	type = (etype_t)(type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		sprintf(line, "%s", PR_GetString(val->string));
		break;
	case ev_entity:
		sprintf(line, "entity %i", QH_NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;
	case ev_function:
		f = pr_functions + val->function;
		sprintf(line, "%s()", PR_GetString(f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs(val->_int);
		sprintf(line, ".%s", PR_GetString(def->s_name));
		break;
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%5.1f", val->_float);
		break;
	case ev_vector:
		sprintf(line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		sprintf(line, "pointer");
		break;
	default:
		sprintf(line, "bad type %i", type);
		break;
	}

	return line;
}

//	Easier to parse than PR_ValueString
const char* PR_UglyValueString(etype_t type, const eval_t* val)
{
	static char line[256];
	ddef_t* def;
	dfunction_t* f;

	type = (etype_t)(type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		sprintf(line, "%s", PR_GetString(val->string));
		break;
	case ev_entity:
		sprintf(line, "%i", QH_NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;
	case ev_function:
		f = pr_functions + val->function;
		sprintf(line, "%s", PR_GetString(f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs(val->_int);
		sprintf(line, "%s", PR_GetString(def->s_name));
		break;
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%f", val->_float);
		break;
	case ev_vector:
		sprintf(line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
		break;
	default:
		sprintf(line, "bad type %i", type);
		break;
	}

	return line;
}

//	Returns a string with a description and the contents of a global,
// padded to 20 field width
const char* PR_GlobalString(int ofs)
{
	static char line[128];

	void* val = (void*)&pr_globals[ofs];
	ddef_t* def = ED_GlobalAtOfs(ofs);
	if (!def)
	{
		//	"" is to shut up trigraph warnings
		sprintf(line,"%i(??" "?)", ofs);
	}
	else
	{
		const char* s = PR_ValueString((etype_t)def->type, (eval_t*)val);
		sprintf(line,"%i(%s)%s", ofs, PR_GetString(def->s_name), s);
	}

	int i = String::Length(line);
	for (; i < 20; i++)
	{
		String::Cat(line, sizeof(line), " ");
	}
	String::Cat(line, sizeof(line), " ");

	return line;
}

const char* PR_GlobalStringNoContents(int ofs)
{
	static char line[128];

	ddef_t* def = ED_GlobalAtOfs(ofs);
	if (!def)
	{
		//	"" is to shut up trigraph warnings
		sprintf(line,"%i(??" "?)", ofs);
	}
	else
	{
		sprintf(line,"%i(%s)", ofs, PR_GetString(def->s_name));
	}

	int i = String::Length(line);
	for (; i < 20; i++)
	{
		String::Cat(line, sizeof(line), " ");
	}
	String::Cat(line, sizeof(line), " ");

	return line;
}

void ED_WriteGlobals(fileHandle_t f)
{
	FS_Printf(f, "{\n");
	for (int i = 0; i < progs->numglobaldefs; i++)
	{
		ddef_t* def = &pr_globaldefs[i];
		int type = def->type;
		if (!(def->type & DEF_SAVEGLOBAL))
		{
			continue;
		}
		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string &&
			type != ev_float &&
			type != ev_entity)
		{
			continue;
		}

		const char* name = PR_GetString(def->s_name);
		FS_Printf(f, "\"%s\" ", name);
		FS_Printf(f, "\"%s\"\n", PR_UglyValueString((etype_t)type, (eval_t*)&pr_globals[def->ofs]));
	}
	FS_Printf(f, "}\n");
}

//	Can parse either fields or globals
//	returns false if error
bool ED_ParseEpair(void* base, const ddef_t* key, const char* s)
{
	int i;
	char string[128];
	ddef_t* def;
	char* v, * w;
	void* d;
	dfunction_t* func;

	d = (void*)((int*)base + key->ofs);

	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(string_t*)d = PR_SetString(ED_NewString(s));
		break;

	case ev_float:
		*(float*)d = String::Atof(s);
		break;

	case ev_vector:
		String::Cpy(string, s);
		v = string;
		w = string;
		for (i = 0; i < 3; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float*)d)[i] = String::Atof(w);
			w = v = v + 1;
		}
		break;

	case ev_entity:
		*(int*)d = EDICT_TO_PROG(QH_EDICT_NUM(String::Atoi(s)));
		break;

	case ev_field:
		def = ED_FindField(s);
		if (!def)
		{
			common->Printf("Can't find field %s\n", s);
			return false;
		}
		*(int*)d = G_INT(def->ofs);
		break;

	case ev_function:
		func = ED_FindFunction(s);
		if (!func)
		{
			common->Printf("Can't find function %s\n", s);
			return false;
		}
		*(func_t*)d = func - pr_functions;
		break;

	default:
		break;
	}
	return true;
}

const char* ED_ParseGlobals(const char* data)
{
	while (1)
	{
		// parse key
		char* token = String::Parse2(&data);
		if (token[0] == '}')
		{
			break;
		}
		if (!data)
		{
			common->Error("ED_ParseEntity: EOF without closing brace");
		}

		char keyname[64];
		String::NCpyZ(keyname, token, sizeof(keyname));

		// parse value
		token = String::Parse2(&data);
		if (!data)
		{
			common->Error("ED_ParseEntity: EOF without closing brace");
		}

		if (token[0] == '}')
		{
			common->Error("ED_ParseEntity: closing brace without data");
		}

		ddef_t* key = ED_FindGlobal(keyname);
		if (!key)
		{
			common->Printf("'%s' is not a global\n", keyname);
			continue;
		}

		if (!ED_ParseEpair((void*)pr_globals, key, token))
		{
			common->Error("ED_ParseGlobals: parse error");
		}
	}
	return data;
}

void PR_InitGlobals()
{
	Com_Memset(&pr_globalVars, 0, sizeof(pr_globalVars));
	//pad
	int index = 28;
	pr_globalVars.self = reinterpret_cast<int*>(&pr_globals[index++]);
	pr_globalVars.other = reinterpret_cast<int*>(&pr_globals[index++]);
	//world
	index++;
	pr_globalVars.time = &pr_globals[index++];
	pr_globalVars.frametime = &pr_globals[index++];
	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		pr_globalVars.newmis = reinterpret_cast<int*>(&pr_globals[index++]);
	}
	pr_globalVars.force_retouch = &pr_globals[index++];
	pr_globalVars.mapname = reinterpret_cast<string_t*>(&pr_globals[index++]);
	if (GGameType & GAME_Hexen2)
	{
		pr_globalVars.startspot = reinterpret_cast<string_t*>(&pr_globals[index++]);
	}
	if (!(GGameType & GAME_QuakeWorld))
	{
		pr_globalVars.deathmatch = &pr_globals[index++];
	}
	if (GGameType & GAME_Hexen2)
	{
		pr_globalVars.randomclass = &pr_globals[index++];
	}
	if (GGameType & GAME_HexenWorld)
	{
		pr_globalVars.damageScale = &pr_globals[index++];
		pr_globalVars.meleeDamScale = &pr_globals[index++];
		pr_globalVars.shyRespawn = &pr_globals[index++];
		pr_globalVars.spartanPrint = &pr_globals[index++];
		pr_globalVars.manaScale = &pr_globals[index++];
		pr_globalVars.tomeMode = &pr_globals[index++];
		pr_globalVars.tomeRespawn = &pr_globals[index++];
		pr_globalVars.w2Respawn = &pr_globals[index++];
		pr_globalVars.altRespawn = &pr_globals[index++];
		pr_globalVars.fixedLevel = &pr_globals[index++];
		pr_globalVars.autoItems = &pr_globals[index++];
		pr_globalVars.dmMode = &pr_globals[index++];
		pr_globalVars.easyFourth = &pr_globals[index++];
		pr_globalVars.patternRunner = &pr_globals[index++];
	}
	if (!(GGameType & GAME_QuakeWorld))
	{
		pr_globalVars.coop = &pr_globals[index++];
		//teamplay
		index++;
	}
	if (GGameType & GAME_Hexen2 && GGameType & GAME_H2Portals)
	{
		pr_globalVars.cl_playerclass = &pr_globals[index++];
	}
	pr_globalVars.serverflags = &pr_globals[index++];
	pr_globalVars.total_secrets = &pr_globals[index++];
	pr_globalVars.total_monsters = &pr_globals[index++];
	pr_globalVars.found_secrets = &pr_globals[index++];
	pr_globalVars.killed_monsters = &pr_globals[index++];
	if (GGameType & GAME_Hexen2)
	{
		//chunk_cnt
		index++;
		//done_precache
		index++;
	}
	pr_globalVars.parm1 = &pr_globals[index++];
	//param2-param16
	index += 15;
	pr_globalVars.v_forward = &pr_globals[index];
	index += 3;
	pr_globalVars.v_up = &pr_globals[index];
	index += 3;
	pr_globalVars.v_right = &pr_globals[index];
	index += 3;
	pr_globalVars.trace_allsolid = &pr_globals[index++];
	pr_globalVars.trace_startsolid = &pr_globals[index++];
	pr_globalVars.trace_fraction = &pr_globals[index++];
	pr_globalVars.trace_endpos = &pr_globals[index];
	index += 3;
	pr_globalVars.trace_plane_normal = &pr_globals[index];
	index += 3;
	pr_globalVars.trace_plane_dist = &pr_globals[index++];
	pr_globalVars.trace_ent = reinterpret_cast<int*>(&pr_globals[index++]);
	pr_globalVars.trace_inopen = &pr_globals[index++];
	pr_globalVars.trace_inwater = &pr_globals[index++];
	pr_globalVars.msg_entity = reinterpret_cast<int*>(&pr_globals[index++]);
	if (GGameType & GAME_Hexen2)
	{
		pr_globalVars.cycle_wrapped = &pr_globals[index++];
		//crouch_cnt
		index++;
		if (!(GGameType & GAME_H2Portals) || GGameType & GAME_HexenWorld)
		{
			//modelindex_assassin
			index++;
			//modelindex_crusader
			index++;
			//modelindex_paladin
			index++;
			//modelindex_necromancer
			index++;
		}
		//modelindex_sheep
		index++;
		//num_players
		index++;
		//exp_mult
		index++;
	}
	if (GGameType & GAME_HexenWorld)
	{
		pr_globalVars.max_players = &pr_globals[index++];
		pr_globalVars.defLosses = &pr_globals[index++];
		pr_globalVars.attLosses = &pr_globals[index++];
	}
	pr_globalVars.main = reinterpret_cast<func_t*>(&pr_globals[index++]);
	pr_globalVars.StartFrame = reinterpret_cast<func_t*>(&pr_globals[index++]);
	pr_globalVars.PlayerPreThink = reinterpret_cast<func_t*>(&pr_globals[index++]);
	pr_globalVars.PlayerPostThink = reinterpret_cast<func_t*>(&pr_globals[index++]);
	pr_globalVars.ClientKill = reinterpret_cast<func_t*>(&pr_globals[index++]);
	pr_globalVars.ClientConnect = reinterpret_cast<func_t*>(&pr_globals[index++]);
	pr_globalVars.PutClientInServer = reinterpret_cast<func_t*>(&pr_globals[index++]);
	if (GGameType & GAME_Hexen2)
	{
		pr_globalVars.ClientReEnter = reinterpret_cast<func_t*>(&pr_globals[index++]);
	}
	pr_globalVars.ClientDisconnect = reinterpret_cast<func_t*>(&pr_globals[index++]);
	if (GGameType & GAME_Hexen2)
	{
		pr_globalVars.ClassChangeWeapon = reinterpret_cast<func_t*>(&pr_globals[index++]);
	}
	if (!(GGameType & GAME_Hexen2) || GGameType & GAME_HexenWorld)
	{
		pr_globalVars.SetNewParms = reinterpret_cast<func_t*>(&pr_globals[index++]);
		pr_globalVars.SetChangeParms = reinterpret_cast<func_t*>(&pr_globals[index++]);
	}
	if (GGameType & GAME_HexenWorld)
	{
		pr_globalVars.SmitePlayer = reinterpret_cast<func_t*>(&pr_globals[index++]);
	}
}

static void PR_LoadProgsFile(const char* name, int expectedHeaderCrc)
{
	ED_ClearGEFVCache();
	PR_ClearStringMap();

	int filesize = FS_ReadFile(name, (void**)&progs);
	if (!progs)
	{
		common->Error("PR_LoadProgs: couldn't load %s", name);
	}
	common->DPrintf("Programs occupy %iK.\n", filesize / 1024);
	pr_crc = CRC_Block((byte*)progs, filesize);

	// byte swap the header
	for (int i = 0; i < (int)sizeof(*progs) / 4; i++)
	{
		((int*)progs)[i] = LittleLong(((int*)progs)[i]);
	}

	if (progs->version != PROG_VERSION)
	{
		common->FatalError("%s has wrong version number (%i should be %i)", name, progs->version, PROG_VERSION);
	}
	if (progs->crc != expectedHeaderCrc)
	{
		common->FatalError("%s system vars have been modified.", name);
	}

	pr_functions = (dfunction_t*)((byte*)progs + progs->ofs_functions);
	pr_strings = (char*)progs + progs->ofs_strings;
	pr_globaldefs = (ddef_t*)((byte*)progs + progs->ofs_globaldefs);
	pr_fielddefs = (ddef_t*)((byte*)progs + progs->ofs_fielddefs);
	pr_statements = (dstatement_t*)((byte*)progs + progs->ofs_statements);
	pr_globals = (float*)((byte*)progs + progs->ofs_globals);

	pr_edict_size = progs->entityfields * 4 + sizeof(qhedict_t) - sizeof(entvars_t);

	// byte swap the lumps
	for (int i = 0; i < progs->numstatements; i++)
	{
		pr_statements[i].op = LittleShort(pr_statements[i].op);
		pr_statements[i].a = LittleShort(pr_statements[i].a);
		pr_statements[i].b = LittleShort(pr_statements[i].b);
		pr_statements[i].c = LittleShort(pr_statements[i].c);
	}

	for (int i = 0; i < progs->numfunctions; i++)
	{
		pr_functions[i].first_statement = LittleLong(pr_functions[i].first_statement);
		pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
		pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
		pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
		pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
		pr_functions[i].locals = LittleLong(pr_functions[i].locals);
	}

	for (int i = 0; i < progs->numglobaldefs; i++)
	{
		pr_globaldefs[i].type = LittleShort(pr_globaldefs[i].type);
		pr_globaldefs[i].ofs = LittleShort(pr_globaldefs[i].ofs);
		pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
	}

	for (int i = 0; i < progs->numfielddefs; i++)
	{
		pr_fielddefs[i].type = LittleShort(pr_fielddefs[i].type);
		if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
		{
			common->FatalError("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
		}
		pr_fielddefs[i].ofs = LittleShort(pr_fielddefs[i].ofs);
		pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
	}

	for (int i = 0; i < progs->numglobals; i++)
	{
		((int*)pr_globals)[i] = LittleLong(((int*)pr_globals)[i]);
	}

	ED_InitEntityFields();
	PR_InitGlobals();
}

static void GetHexen2ProgsName(char* finalprogname)
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

void PR_LoadProgs()
{
	if (GGameType & GAME_Hexen2)
	{
		if (GGameType & GAME_HexenWorld)
		{
			PR_LoadProgsFile("hwprogs.dat", HWPROGHEADER_CRC);
		}
		else
		{
			char finalprogname[MAX_OSPATH];
			GetHexen2ProgsName(finalprogname);

			PR_LoadProgsFile(finalprogname, (GGameType & GAME_H2Portals ? H2MPPROGHEADER_CRC : H2PROGHEADER_CRC));

			if (GGameType & GAME_H2Portals)
			{
				// set the cl_playerclass value after pr_global_struct has been created
				*pr_globalVars.cl_playerclass = Cvar_VariableValue("_cl_playerclass");
			}
		}
	}
	else
	{
		if (GGameType & GAME_QuakeWorld)
		{
			PR_LoadProgsFile("qwprogs.dat", QWPROGHEADER_CRC);
		}
		else
		{
			PR_LoadProgsFile("progs.dat", Q1PROGHEADER_CRC);
		}
	}
}

void PR_Init()
{
	Cmd_AddCommand("edict", ED_PrintEdict_f);
	Cmd_AddCommand("edicts", ED_PrintEdicts);
	Cmd_AddCommand("edictcount", ED_Count);
	Cmd_AddCommand("profile", PR_Profile_f);
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		Cvar_Get("nomonsters", "0", 0);
		Cvar_Get("gamecfg", "0", 0);
		Cvar_Get("scratch1", "0", 0);
		Cvar_Get("scratch2", "0", 0);
		Cvar_Get("scratch3", "0", 0);
		Cvar_Get("scratch4", "0", 0);
		Cvar_Get("savedgamecfg", "0", CVAR_ARCHIVE);
		Cvar_Get("saved1", "0", CVAR_ARCHIVE);
		Cvar_Get("saved2", "0", CVAR_ARCHIVE);
		Cvar_Get("saved3", "0", CVAR_ARCHIVE);
		Cvar_Get("saved4", "0", CVAR_ARCHIVE);
	}
	if (GGameType & GAME_Hexen2)
	{
		max_temp_edicts = Cvar_Get("max_temp_edicts", "30", CVAR_ARCHIVE);
	}
}
