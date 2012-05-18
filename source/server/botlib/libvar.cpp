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
#include "local.h"

//list with library variables
static libvar_t* libvarlist;

static float LibVarStringValue(const char* string)
{
	int dotfound = 0;
	float value = 0;
	while (*string)
	{
		if (*string < '0' || *string > '9')
		{
			if (dotfound || *string != '.')
			{
				return 0;
			}
			else
			{
				dotfound = 10;
				string++;
			}
		}
		if (dotfound)
		{
			value = value + (float)(*string - '0') / (float)dotfound;
			dotfound *= 10;
		}
		else
		{
			value = value * 10.0 + (float)(*string - '0');
		}
		string++;
	}
	return value;
}

static libvar_t* LibVarAlloc(const char* var_name)
{
	libvar_t* v = (libvar_t*)Mem_Alloc(sizeof(libvar_t) + String::Length(var_name) + 1);
	Com_Memset(v, 0, sizeof(libvar_t));
	v->name = (char*)v + sizeof(libvar_t);
	String::Cpy(v->name, var_name);
	//add the variable in the list
	v->next = libvarlist;
	libvarlist = v;
	return v;
}

static void LibVarDeAlloc(libvar_t* v)
{
	if (v->string)
	{
		Mem_Free(v->string);
	}
	Mem_Free(v);
}

void LibVarDeAllocAll()
{
	for (libvar_t* v = libvarlist; v; v = libvarlist)
	{
		libvarlist = libvarlist->next;
		LibVarDeAlloc(v);
	}
	libvarlist = NULL;
}

static libvar_t* LibVarGet(const char* var_name)
{
	for (libvar_t* v = libvarlist; v; v = v->next)
	{
		if (!String::ICmp(v->name, var_name))
		{
			return v;
		}
	}
	return NULL;
}

const char* LibVarGetString(const char* var_name)
{
	libvar_t* v = LibVarGet(var_name);
	if (v)
	{
		return v->string;
	}
	else
	{
		return "";
	}
}

float LibVarGetValue(const char* var_name)
{
	libvar_t* v = LibVarGet(var_name);
	if (v)
	{
		return v->value;
	}
	else
	{
		return 0;
	}
}

libvar_t* LibVar(const char* var_name, const char* value)
{
	libvar_t* v = LibVarGet(var_name);
	if (v)
	{
		return v;
	}
	//create new variable
	v = LibVarAlloc(var_name);
	//variable string
	v->string = (char*)Mem_Alloc(String::Length(value) + 1);
	String::Cpy(v->string, value);
	//the value
	v->value = LibVarStringValue(v->string);

	return v;
}

const char* LibVarString(const char* var_name, const char* value)
{
	libvar_t* v = LibVar(var_name, value);
	return v->string;
}

float LibVarValue(const char* var_name, const char* value)
{
	libvar_t* v = LibVar(var_name, value);
	return v->value;
}

void LibVarSet(const char* var_name, const char* value)
{
	libvar_t* v = LibVarGet(var_name);
	if (v)
	{
		Mem_Free(v->string);
	}
	else
	{
		v = LibVarAlloc(var_name);
	}
	//variable string
	v->string = (char*)Mem_Alloc(String::Length(value) + 1);
	String::Cpy(v->string, value);
	//the value
	v->value = LibVarStringValue(v->string);
}
