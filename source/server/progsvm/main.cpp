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

#include "progsvm.h"

dprograms_t* progs;
dfunction_t* pr_functions;
char* pr_strings;
ddef_t* pr_fielddefs;
ddef_t* pr_globaldefs;
dstatement_t* pr_statements;
float* pr_globals;			// same as pr_global_struct
int pr_edict_size;			// in bytes

static Array<const char*> pr_strtbl;

void PR_ClearStringMap()
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
