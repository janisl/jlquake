// sv_edict.c -- entity dictionary

/*
 * $Header: /H2 Mission Pack/Pr_edict.c 11    3/27/98 2:12p Jmonroe $
 */

#include "quakedef.h"

globalvars_t	*pr_global_struct;
int				pr_edict_size;	// in bytes

// For international stuff
int             *pr_string_index = NULL;
char			*pr_global_strings = NULL;
int				pr_string_count = 0;

#ifdef MISSIONPACK
int             *pr_info_string_index = NULL;
char			*pr_global_info_strings = NULL;
int				pr_info_string_count = 0;
#endif

qboolean		ignore_precache = false;


unsigned short		pr_crc;

int		type_size[8] = {1,sizeof(string_t)/4,1,3,1,1,sizeof(func_t)/4,sizeof(void *)/4};

qboolean	ED_ParseEpair (void *base, ddef_t *key, char *s);

Cvar*	nomonsters;
Cvar*	gamecfg;
Cvar*	scratch1;
Cvar*	scratch2;
Cvar*	scratch3;
Cvar*	scratch4;
Cvar*	savedgamecfg;
Cvar*	saved1;
Cvar*	saved2;
Cvar*	saved3;
Cvar*	saved4;
Cvar*	max_temp_edicts;

static char field_name[256], class_name[256];
static qboolean RemoveBadReferences;

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict (qhedict_t *e)
{
	Com_Memset(&e->v, 0, progs->entityfields * 4);
	Com_Memset(&e->baseline, 0, sizeof(e->baseline));
	e->free = false;
}

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
qhedict_t *ED_Alloc (void)
{
	int			i;
	qhedict_t		*e;

	for ( i=svs.maxclients+1+max_temp_edicts->value ; i<sv.num_edicts ; i++)
	{
		e = EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && ( e->freetime < 2 || sv.time - e->freetime > 0.5 ) )
		{
			ED_ClearEdict (e);
			return e;
		}
	}
	
	if (i == MAX_EDICTS_H2)
	{
		SV_Edicts("edicts.txt");
		Sys_Error ("ED_Alloc: no free edicts");
	}
		
	sv.num_edicts++;
	e = EDICT_NUM(i);
	ED_ClearEdict (e);

	return e;
}

qhedict_t *ED_Alloc_Temp (void)
{
	int			i,j,Found;
	qhedict_t		*e,*Least;
	float		LeastTime;
	qboolean	LeastSet;

	LeastTime = -1;
	LeastSet = false;
	for ( i=svs.maxclients+1,j=0 ; j < max_temp_edicts->value ; i++,j++)
	{
		e = EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && ( e->freetime < 2 || sv.time - e->freetime > 0.5 ) )
		{
			ED_ClearEdict (e);
			e->alloctime = sv.time;

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
	ED_ClearEdict (Least);
	Least->alloctime = sv.time;

	return Least;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free (qhedict_t *ed)
{
	SV_UnlinkEdict (ed);		// unlink from world bsp

	ed->free = true;
	ed->v.model = 0;
	ed->v.takedamage = 0;
	ed->v.modelindex = 0;
	ed->v.colormap = 0;
	ed->v.skin = 0;
	ed->v.frame = 0;
	VectorCopy (vec3_origin, ed->v.origin);
	VectorCopy (vec3_origin, ed->v.angles);
	ed->v.nextthink = -1;
	ed->v.solid = 0;
	
	ed->freetime = sv.time;
	ed->alloctime = -1;
}

//===========================================================================

#define	MAX_FIELD_LEN	64
#define GEFV_CACHESIZE	2

typedef struct {
	ddef_t	*pcache;
	char	field[MAX_FIELD_LEN];
} gefv_cache;

static gefv_cache	gefvCache[GEFV_CACHESIZE] = {{NULL, ""}, {NULL, ""}};

eval_t *GetEdictFieldValue(qhedict_t *ed, const char *field)
{
	ddef_t			*def = NULL;
	int				i;
	static int		rep = 0;

	for (i=0 ; i<GEFV_CACHESIZE ; i++)
	{
		if (!String::Cmp(field, gefvCache[i].field))
		{
			def = gefvCache[i].pcache;
			goto Done;
		}
	}

	def = ED_FindField (field);

	if (String::Length(field) < MAX_FIELD_LEN)
	{
		gefvCache[rep].pcache = def;
		String::Cpy(gefvCache[rep].field, field);
		rep ^= 1;
	}

Done:
	if (!def)
		return NULL;

	return (eval_t *)((char *)&ed->v + def->ofs*4);
}


/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char *PR_ValueString (etype_t type, eval_t *val)
{
	static char	line[256];
	ddef_t		*def;
	dfunction_t	*f;
	
	type = (etype_t)(type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		sprintf (line, "%s", PR_GetString(val->string));
		break;
	case ev_entity:	
		sprintf (line, "entity %i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)) );
		break;
	case ev_function:
		f = pr_functions + val->function;
		sprintf (line, "%s()", PR_GetString(f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs ( val->_int );
		sprintf (line, ".%s", PR_GetString(def->s_name));
		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		sprintf (line, "%5.1f", val->_float);
		break;
	case ev_vector:
		sprintf (line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		sprintf (line, "pointer");
		break;
	default:
		sprintf (line, "bad type %i", type);
		break;
	}
	
	return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
char *PR_UglyValueString (etype_t type, eval_t *val)
{
	static char	line[256];
	ddef_t		*def;
	dfunction_t	*f;
	
	type = (etype_t)(type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		sprintf (line, "%s", PR_GetString(val->string));
		break;
	case ev_entity:	
		sprintf (line, "%i", NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;
	case ev_function:
		f = pr_functions + val->function;
		sprintf (line, "%s", PR_GetString(f->s_name));
		break;
	case ev_field:
		def = ED_FieldAtOfs ( val->_int );
		sprintf (line, "%s", PR_GetString(def->s_name));
		break;
	case ev_void:
		sprintf (line, "void");
		break;
	case ev_float:
		sprintf (line, "%f", val->_float);
		break;
	case ev_vector:
		sprintf (line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
		break;
	default:
		sprintf (line, "bad type %i", type);
		break;
	}
	
	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char *PR_GlobalString (int ofs)
{
	char	*s;
	int		i;
	ddef_t	*def;
	void	*val;
	static char	line[128];
	
	val = (void *)&pr_globals[ofs];
	def = ED_GlobalAtOfs(ofs);
	if (!def)
		sprintf (line,"%i(?)", ofs);
	else
	{
		s = PR_ValueString ((etype_t)def->type, (eval_t*)val);
		sprintf (line,"%i(%s)%s", ofs, PR_GetString(def->s_name), s);
	}
	
	i = String::Length(line);
	for ( ; i<20 ; i++)
		String::Cat(line, sizeof(line)," ");
	String::Cat(line, sizeof(line)," ");
		
	return line;
}

char *PR_GlobalStringNoContents (int ofs)
{
	int		i;
	ddef_t	*def;
	static char	line[128];
	
	def = ED_GlobalAtOfs(ofs);
	if (!def)
		sprintf (line,"%i(?)", ofs);
	else
		sprintf (line,"%i(%s)", ofs, PR_GetString(def->s_name));
	
	i = String::Length(line);
	for ( ; i<20 ; i++)
		String::Cat(line, sizeof(line)," ");
	String::Cat(line, sizeof(line)," ");
		
	return line;
}


/*
=============
ED_Print

For debugging
=============
*/
void ED_Print (qhedict_t *ed)
{
	int		l;
	ddef_t	*d;
	int		*v;
	int		i, j;
	const char	*name;
	int		type;

	if (ed->free)
	{
		Con_Printf ("FREE\n");
		return;
	}

	Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
	for (i=1 ; i<progs->numfielddefs ; i++)
	{
		d = &pr_fielddefs[i];
		name = PR_GetString(d->s_name);
		if ((name[String::Length(name)-2] == '_') && 
			((name[String::Length(name)-1] == 'x') || (name[String::Length(name)-1] == 'y') || (name[String::Length(name)-1] == 'z')))
			continue;	// skip _x, _y, _z vars
			
		v = (int *)((char *)&ed->v + d->ofs*4);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		
		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;
	
		Con_Printf ("%s",name);
		l = String::Length(name);
		while (l++ < 15)
			Con_Printf (" ");

		Con_Printf ("%s\n", PR_ValueString((etype_t)d->type, (eval_t *)v));		
	}
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write (fileHandle_t f, qhedict_t *ed)
{
	ddef_t	*d;
	int		*v;
	int		i, j;
	const char	*name;
	int		type;
	int		length;

	FS_Printf(f, "{\n");

	if (ed->free)
	{
		FS_Printf(f, "}\n");
		return;
	}
	
	RemoveBadReferences = true;
	
	if (ed->v.classname)
		String::Cpy(class_name,PR_GetString(ed->v.classname));
	else
		class_name[0] = 0;

	for (i=1 ; i<progs->numfielddefs ; i++)
	{
		d = &pr_fielddefs[i];
		name = PR_GetString(d->s_name);
		length = String::Length(name);
		if (name[length-2] == '_' && name[length-1] >= 'x' && name[length-1] <= 'z')
			continue;	// skip _x, _y, _z vars
			
		v = (int *)((char *)&ed->v + d->ofs*4);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;

		String::Cpy(field_name,name);
		FS_Printf(f,"\"%s\" ",name);
		FS_Printf(f,"\"%s\"\n", PR_UglyValueString((etype_t)d->type, (eval_t *)v));		
	}

	field_name[0] = 0;
	class_name[0] = 0;

	FS_Printf(f, "}\n");

	RemoveBadReferences = false;
}

void ED_PrintNum (int ent)
{
	ED_Print (EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts (void)
{
	int		i;
	
	Con_Printf ("%i entities\n", sv.num_edicts);
	for (i=0 ; i<sv.num_edicts ; i++)
		ED_PrintNum (i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f (void)
{
	int		i;
	
	i = String::Atoi(Cmd_Argv(1));
	if (i >= sv.num_edicts)
	{
		Con_Printf("Bad edict number\n");
		return;
	}
	ED_PrintNum (i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count (void)
{
	int		i;
	qhedict_t	*ent;
	int		active, models, solid, step;

	active = models = solid = step = 0;
	for (i=0 ; i<sv.num_edicts ; i++)
	{
		ent = EDICT_NUM(i);
		if (ent->free)
			continue;
		active++;
		if (ent->v.solid)
			solid++;
		if (ent->v.model)
			models++;
		if (ent->v.movetype == MOVETYPE_STEP)
			step++;
	}

	Con_Printf ("num_edicts:%3i\n", sv.num_edicts);
	Con_Printf ("active    :%3i\n", active);
	Con_Printf ("view      :%3i\n", models);
	Con_Printf ("touch     :%3i\n", solid);
	Con_Printf ("step      :%3i\n", step);

}

/*
==============================================================================

					ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals (fileHandle_t f)
{
	ddef_t		*def;
	int			i;
	const char		*name;
	int			type;

	FS_Printf(f,"{\n");
	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];
		type = def->type;
		if ( !(def->type & DEF_SAVEGLOBAL) )
			continue;
		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string
		&& type != ev_float
		&& type != ev_entity)
			continue;

		name = PR_GetString(def->s_name);
		FS_Printf(f,"\"%s\" ", name);
		FS_Printf(f,"\"%s\"\n", PR_UglyValueString((etype_t)type, (eval_t *)&pr_globals[def->ofs]));		
	}
	FS_Printf(f,"}\n");
}

/*
=============
ED_ParseGlobals
=============
*/
const char* ED_ParseGlobals(const char* data)
{
	char	keyname[64];
	ddef_t	*key;

	while (1)
	{	
	// parse key
		char* token = String::Parse1(&data);
		if (token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		String::Cpy(keyname, token);

	// parse value	
		token = String::Parse1(&data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		key = ED_FindGlobal (keyname);
		if (!key)
		{
			Con_Printf ("'%s' is not a global\n", keyname);
			continue;
		}

		if (!ED_ParseEpair ((void *)pr_globals, key, token))
			Host_Error ("ED_ParseGlobals: parse error");
	}
	return data;
}

//============================================================================


/*
=============
ED_NewString
=============
*/
char *ED_NewString (const char *string)
{
	char	*news, *new_p;
	int		i,l;
	
	l = String::Length(string) + 1;
	news = (char*)Hunk_Alloc (l);
	new_p = news;

	for (i=0 ; i< l ; i++)
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}
	
	return news;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
qboolean	ED_ParseEpair (void *base, ddef_t *key, char *s)
{
	int		i;
	char	string[128];
	ddef_t	*def;
	char	*v, *w;
	void	*d;
	dfunction_t	*func;
	
	d = (void *)((int *)base + key->ofs);
	
	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(string_t *)d = PR_SetString(ED_NewString(s));
		break;
		
	case ev_float:
		*(float *)d = String::Atof(s);
		break;
		
	case ev_vector:
		String::Cpy(string, s);
		v = string;
		w = string;
		for (i=0 ; i<3 ; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float *)d)[i] = String::Atof(w);
			w = v = v+1;
		}
		break;
		
	case ev_entity:
		*(int *)d = EDICT_TO_PROG(EDICT_NUM(String::Atoi(s)));
		break;
		
	case ev_field:
		def = ED_FindField (s);
		if (!def)
		{
			Con_Printf ("Can't find field %s\n", s);
			return false;
		}
		*(int *)d = G_INT(def->ofs);
		break;
	
	case ev_function:
		func = ED_FindFunction (s);
		if (!func)
		{
			Con_Printf ("Can't find function %s\n", s);
			return false;
		}
		*(func_t *)d = func - pr_functions;
		break;
		
	default:
		break;
	}
	return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
const char *ED_ParseEdict (const char *data, qhedict_t *ent)
{
	ddef_t		*key;
	qboolean	anglehack;
	qboolean	init;
	char		keyname[256];
	int			n;

	init = false;

// clear it
	if (ent != sv.edicts)	// hack
		Com_Memset(&ent->v, 0, progs->entityfields * 4);

// go through all the dictionary pairs
	while (1)
	{	
	// parse key
		char* token = String::Parse1(&data);
		if (token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");
		
// anglehack is to allow QuakeEd to write single scalar angles
// and allow them to be turned into vectors. (FIXME...)
if (!String::Cmp(token, "angle"))
{
	String::Cpy(token, "angles");
	anglehack = true;
}
else
	anglehack = false;

// FIXME: change light to _light to get rid of this hack
if (!String::Cmp(token, "light"))
	String::Cpy(token, "light_lev");	// hack for single light def

		String::Cpy(keyname, token);

		// another hack to fix heynames with trailing spaces
		n = String::Length(keyname);
		while (n && keyname[n-1] == ' ')
		{
			keyname[n-1] = 0;
			n--;
		}

	// parse value	
		token = String::Parse1(&data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		init = true;	

// keynames with a leading underscore are used for utility comments,
// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;

		if (String::ICmp(keyname,"MIDI") == 0)
		{
			String::Cpy(sv.midi_name,token);
			continue;
		}
		else if (String::ICmp(keyname,"CD") == 0)
		{
			sv.cd_track = (byte)atol(token);
			continue;
		}

		key = ED_FindField (keyname);
		if (!key)
		{
			Con_Printf ("'%s' is not a field\n", keyname);
			continue;
		}

if (anglehack)
{
char	temp[32];
String::Cpy(temp, token);
sprintf (token, "0 %s 0", temp);
}

		if (!ED_ParseEpair ((void *)&ent->v, key, token))
			Host_Error ("ED_ParseEdict: parse error");
	}

	if (!init)
		ent->free = true;

	return data;
}


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
void ED_LoadFromFile (const char *data)
{	
	qhedict_t		*ent;
	int			inhibit,skip;
	dfunction_t	*func;
	const char		*orig;
	int			start_amount;
	
	ent = NULL;
	inhibit = 0;
	pr_global_struct->time = sv.time;
	orig = data;
	int entity_file_size = String::Length(data);
	
	start_amount = current_loading_size;
// parse ents
	while (1)
	{
// parse the opening brace	
		char* token = String::Parse1(&data);
		if (!data)
			break;

		if (entity_file_size)
		{
			current_loading_size = start_amount + ((data-orig)*80/entity_file_size);
			SCR_UpdateScreen();
		}

		if (token[0] != '{')
			Sys_Error ("ED_LoadFromFile: found %s when expecting {",token);

		if (!ent)
			ent = EDICT_NUM(0);
		else
			ent = ED_Alloc ();
		data = ED_ParseEdict (data, ent);

#if 0
		//jfm fuckup test
		//remove for final release
		if ((ent->v.spawnflags >1) && !String::Cmp("worldspawn",PR_GetString(ent->v.classname)) )
		{
			Host_Error ("invalid SpawnFlags on World!!!\n");
		}
#endif

// remove things from different skill levels or deathmatch
		if (deathmatch->value)
		{
			if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}
		}
		else if (coop->value)
		{
			if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_COOP))
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}
		}
		else
		{ // Gotta be single player
			if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_SINGLE))
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}

			skip = 0;

			switch ((int)cl_playerclass->value)
			{		
			case CLASS_PALADIN:
				if ((int)ent->v.spawnflags & SPAWNFLAG_NOT_PALADIN)
				{
					skip = 1;
				}
				break;
				
			case CLASS_CLERIC:
				if ((int)ent->v.spawnflags & SPAWNFLAG_NOT_CLERIC)
				{
					skip = 1;
				}
				break;
				
			case CLASS_DEMON:
			case CLASS_NECROMANCER:
				if ((int)ent->v.spawnflags & SPAWNFLAG_NOT_NECROMANCER)
				{
					skip = 1;
				}
				break;
				
			case CLASS_THEIF:
				if ((int)ent->v.spawnflags & SPAWNFLAG_NOT_THEIF)
				{
					skip = 1;
				}
				break;				
			}

			if (skip)
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}	
		}
		
		if ((current_skill == 0 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY))
			|| (current_skill == 1 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM))
			|| (current_skill >= 2 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD)) )
		{
			ED_Free (ent);	
			inhibit++;
			continue;
		}

//
// immediately call spawn function
//
		if (!ent->v.classname)
		{
			Con_Printf ("No classname for:\n");
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}
		
	// look for the spawn function
		func = ED_FindFunction ( PR_GetString(ent->v.classname) );

		if (!func)
		{
			Con_Printf ("No spawn function for:\n");
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}

		pr_global_struct->self = EDICT_TO_PROG(ent);
		PR_ExecuteProgram (func - pr_functions);
	}	

	Con_DPrintf ("%i entities inhibited\n", inhibit);
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
void PR_LoadProgs (void)
{
	int		i;
	char	finalprogname[MAX_OSPATH];

// flush the non-C variable lookup cache
	for (i=0 ; i<GEFV_CACHESIZE ; i++)
		gefvCache[i].field[0] = 0;

	CRC_Init (&pr_crc);

	GetProgsName(finalprogname);

	progs = (dprograms_t *)COM_LoadHunkFile (finalprogname);
	if (!progs)
		Sys_Error ("PR_LoadProgs: couldn't load %s",finalprogname);
	Con_DPrintf ("Programs occupy %iK.\n", com_filesize/1024);

	for (i=0 ; i<com_filesize ; i++)
		CRC_ProcessByte (&pr_crc, ((byte *)progs)[i]);

// byte swap the header
	for (i=0 ; i<(int)sizeof(*progs)/4 ; i++)
		((int *)progs)[i] = LittleLong ( ((int *)progs)[i] );		

	if (progs->version != PROG_VERSION)
		Sys_Error ("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
	if (progs->crc != PROGHEADER_CRC)
		Sys_Error ("progs.dat system vars have been modified, progdefs.h is out of date");

	pr_functions = (dfunction_t *)((byte *)progs + progs->ofs_functions);
	pr_strings = (char *)progs + progs->ofs_strings;
	pr_globaldefs = (ddef_t *)((byte *)progs + progs->ofs_globaldefs);
	pr_fielddefs = (ddef_t *)((byte *)progs + progs->ofs_fielddefs);
	pr_statements = (dstatement_t *)((byte *)progs + progs->ofs_statements);

	pr_global_struct = (globalvars_t *)((byte *)progs + progs->ofs_globals);
	pr_globals = (float *)pr_global_struct;
	
	pr_edict_size = progs->entityfields * 4 + sizeof (qhedict_t) - sizeof(entvars_t);

	if (GBigEndian)// byte swap the lumps
	{
		for (i=0 ; i<progs->numstatements ; i++)
		{
			pr_statements[i].op = LittleShort(pr_statements[i].op);
			pr_statements[i].a = LittleShort(pr_statements[i].a);
			pr_statements[i].b = LittleShort(pr_statements[i].b);
			pr_statements[i].c = LittleShort(pr_statements[i].c);
		}

		for (i=0 ; i<progs->numfunctions; i++)
		{
			pr_functions[i].first_statement = LittleLong (pr_functions[i].first_statement);
			pr_functions[i].parm_start = LittleLong (pr_functions[i].parm_start);
			pr_functions[i].s_name = LittleLong (pr_functions[i].s_name);
			pr_functions[i].s_file = LittleLong (pr_functions[i].s_file);
			pr_functions[i].numparms = LittleLong (pr_functions[i].numparms);
			pr_functions[i].locals = LittleLong (pr_functions[i].locals);
		}	

		for (i=0 ; i<progs->numglobaldefs ; i++)
		{
			pr_globaldefs[i].type = LittleShort (pr_globaldefs[i].type);
			pr_globaldefs[i].ofs = LittleShort (pr_globaldefs[i].ofs);
			pr_globaldefs[i].s_name = LittleLong (pr_globaldefs[i].s_name);
		}

		for (i=0 ; i<progs->numfielddefs ; i++)
		{
			pr_fielddefs[i].type = LittleShort (pr_fielddefs[i].type);
			if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
				Sys_Error ("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
			pr_fielddefs[i].ofs = LittleShort (pr_fielddefs[i].ofs);
			pr_fielddefs[i].s_name = LittleLong (pr_fielddefs[i].s_name);
		}
		
		for (i=0 ; i<progs->numglobals ; i++)
			((int *)pr_globals)[i] = LittleLong (((int *)pr_globals)[i]);
	}

	int entSize = ED_InitEntityFields();
	if (entSize != sizeof(entvars_t))
		throw Exception(va("Wrong entity size %d should be %d\n", entSize, sizeof(entvars_t)));
#ifdef MISSIONPACK
	// set the cl_playerclass value after pr_global_struct has been created
	pr_global_struct->cl_playerclass = cl_playerclass->value;
#endif
}


#ifdef MISSIONPACK
void PR_LoadInfoStrings(void)
{
	int i,count,start,Length;
	char NewLineChar;

	pr_global_info_strings = (char *)COM_LoadHunkFile ("infolist.txt");
	if (!pr_global_info_strings)
		Sys_Error ("PR_LoadInfoStrings: couldn't load infolist.txt");

	NewLineChar = -1;

	for(i=count=0; pr_global_info_strings[i] != 0; i++)
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
		Sys_Error ("PR_LoadInfoStrings: no string lines found");
	}

	pr_info_string_index = (int *)Hunk_AllocName ((count+1)*4, "info_string_index");

	for(i=count=start=0; pr_global_info_strings[i] != 0; i++)
	{
		if (pr_global_info_strings[i] == 13 || pr_global_info_strings[i] == 10)
		{
			if (NewLineChar == pr_global_info_strings[i]) 
			{
				pr_info_string_index[count] = start;
				start = i+1;
				count++;
			}
			else start++;

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

	pr_global_strings = (char *)COM_LoadHunkFile ("strings.txt");
	if (!pr_global_strings)
		Sys_Error ("PR_LoadStrings: couldn't load strings.txt");

	NewLineChar = -1;

	for(i=count=0; pr_global_strings[i] != 0; i++)
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
		Sys_Error ("PR_LoadStrings: no string lines found");
	}

	pr_string_index = (int *)Hunk_AllocName ((count+1)*4, "string_index");

	for(i=count=start=0; pr_global_strings[i] != 0; i++)
	{
		if (pr_global_strings[i] == 13 || pr_global_strings[i] == 10)
		{
			if (NewLineChar == pr_global_strings[i]) 
			{
				pr_string_index[count] = start;
				start = i+1;
				count++;
			}
			else start++;

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
void PR_Init (void)
{
	Cmd_AddCommand ("edict", ED_PrintEdict_f);
	Cmd_AddCommand ("edicts", ED_PrintEdicts);
	Cmd_AddCommand ("edictcount", ED_Count);
	Cmd_AddCommand ("profile", PR_Profile_f);
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



qhedict_t *EDICT_NUM(int n)
{
	if (n < 0 || n >= sv.max_edicts)
		Sys_Error ("EDICT_NUM: bad number %i", n);
	return (qhedict_t *)((byte *)sv.edicts+ (n)*pr_edict_size);
}

int NUM_FOR_EDICT(qhedict_t *e)
{
	int		b;
	
	b = (byte *)e - (byte *)sv.edicts;
	b = b / pr_edict_size;
	
	if (b < 0 || b >= sv.num_edicts)
	{
		if (!RemoveBadReferences)
			Con_DPrintf ("NUM_FOR_EDICT: bad pointer, Class: %s Field: %s, Index %d, Total %d",class_name,field_name,b,sv.num_edicts);
		return(0);
	}
	if (e->free && RemoveBadReferences)
	{
//		Con_Printf ("NUM_FOR_EDICT: freed edict, Class: %s Field: %s, Index %d, Total %d",class_name,field_name,b,sv.num_edicts);
		return(0);
	}
	return b;
}
