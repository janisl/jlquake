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

static const fielddef_t* FindField(const fielddef_t* defs, const char* name)
{
	for (int i = 0; defs[i].name; i++)
	{
		if (!String::Cmp(defs[i].name, name))
		{
			return &defs[i];
		}
	}
	return NULL;
}

static bool ReadNumber(source_t* source, const fielddef_t* fd, void* p)
{
	token_t token;
	if (!PC_ExpectAnyToken(source, &token))
	{
		return false;
	}

	//check for minus sign
	bool negative = false;
	if (token.type == TT_PUNCTUATION)
	{
		if (fd->type & FT_UNSIGNED)
		{
			SourceError(source, "expected unsigned value, found %s", token.string);
			return false;
		}
		//if not a minus sign
		if (String::Cmp(token.string, "-"))
		{
			SourceError(source, "unexpected punctuation %s", token.string);
			return false;
		}
		negative = true;
		//read the number
		if (!PC_ExpectAnyToken(source, &token))
		{
			return false;
		}
	}
	//check if it is a number
	if (token.type != TT_NUMBER)
	{
		SourceError(source, "expected number, found %s", token.string);
		return false;
	}
	//check for a float value
	if (token.subtype & TT_FLOAT)
	{
		if ((fd->type & FT_TYPE) != FT_FLOAT)
		{
			SourceError(source, "unexpected float");
			return 0;
		}
		double floatval = token.floatvalue;
		if (negative)
		{
			floatval = -floatval;
		}
		if (fd->type & FT_BOUNDED)
		{
			if (floatval < fd->floatmin || floatval > fd->floatmax)
			{
				SourceError(source, "float out of range [%f, %f]", fd->floatmin, fd->floatmax);
				return false;
			}
		}
		*(float*)p = (float)floatval;
		return true;
	}

	int intval = token.intvalue;
	if (negative)
	{
		intval = -intval;
	}
	//check bounds
	int intmin = 0, intmax = 0;
	if ((fd->type & FT_TYPE) == FT_CHAR)
	{
		if (fd->type & FT_UNSIGNED)
		{
			intmin = 0; intmax = 255;
		}
		else
		{
			intmin = -128; intmax = 127;
		}
	}
	if ((fd->type & FT_TYPE) == FT_INT)
	{
		if (fd->type & FT_UNSIGNED)
		{
			intmin = 0; intmax = 65535;
		}
		else
		{
			intmin = -32768; intmax = 32767;
		}
	}
	if ((fd->type & FT_TYPE) == FT_CHAR || (fd->type & FT_TYPE) == FT_INT)
	{
		if (fd->type & FT_BOUNDED)
		{
			intmin = Max(intmin, (int)fd->floatmin);
			intmax = Min(intmax, (int)fd->floatmax);
		}
		if (intval < intmin || intval > intmax)
		{
			SourceError(source, "value %d out of range [%d, %d]", intval, intmin, intmax);
			return false;
		}
	}
	else if ((fd->type & FT_TYPE) == FT_FLOAT)
	{
		if (fd->type & FT_BOUNDED)
		{
			if (intval < fd->floatmin || intval > fd->floatmax)
			{
				SourceError(source, "value %d out of range [%f, %f]", intval, fd->floatmin, fd->floatmax);
				return false;
			}
		}
	}
	//store the value
	if ((fd->type & FT_TYPE) == FT_CHAR)
	{
		if (fd->type & FT_UNSIGNED)
		{
			*(unsigned char*)p = (unsigned char)intval;
		}
		else
		{
			*(char*)p = (char)intval;
		}
	}
	else if ((fd->type & FT_TYPE) == FT_INT)
	{
		if (fd->type & FT_UNSIGNED)
		{
			*(unsigned int*)p = (unsigned int)intval;
		}
		else
		{
			*(int*)p = (int)intval;
		}
	}
	else if ((fd->type & FT_TYPE) == FT_FLOAT)
	{
		*(float*)p = (float)intval;
	}
	return true;
}

static bool ReadChar(source_t* source, const fielddef_t* fd, void* p)
{
	token_t token;
	if (!PC_ExpectAnyToken(source, &token))
	{
		return false;
	}

	//take literals into account
	if (token.type == TT_LITERAL)
	{
		StripSingleQuotes(token.string);
		*(char*)p = token.string[0];
	}
	else
	{
		PC_UnreadLastToken(source);
		if (!ReadNumber(source, fd, p))
		{
			return false;
		}
	}
	return true;
}

static bool ReadString(source_t* source, const fielddef_t* fd, void* p)
{
	token_t token;
	if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
	{
		return true;
	}
	//remove the double quotes
	StripDoubleQuotes(token.string);
	//copy the string
	String::NCpyZ((char*)p, token.string, MAX_STRINGFIELD);
	return true;
}

bool ReadStructure(source_t* source, const structdef_t* def, char* structure)
{
	if (!PC_ExpectTokenString(source, "{"))
	{
		return false;
	}
	while (1)
	{
		token_t token;
		if (!PC_ExpectAnyToken(source, &token))
		{
			return false;
		}
		//if end of structure
		if (!String::Cmp(token.string, "}"))
		{
			break;
		}
		//find the field with the name
		const fielddef_t* fd = FindField(def->fields, token.string);
		if (!fd)
		{
			SourceError(source, "unknown structure field %s", token.string);
			return false;
		}
		int num;
		if (fd->type & FT_ARRAY)
		{
			num = fd->maxarray;
			if (!PC_ExpectTokenString(source, "{"))
			{
				return false;
			}
		}
		else
		{
			num = 1;
		}
		void* p = (void*)(structure + fd->offset);
		while (num-- > 0)
		{
			if (fd->type & FT_ARRAY)
			{
				if (PC_CheckTokenString(source, "}"))
				{
					break;
				}
			}
			switch (fd->type & FT_TYPE)
			{
			case FT_CHAR:
				if (!ReadChar(source, fd, p))
				{
					return false;
				}
				p = (char*)p + sizeof(char);
				break;
			case FT_INT:
				if (!ReadNumber(source, fd, p))
				{
					return false;
				}
				p = (char*)p + sizeof(int);
				break;
			case FT_FLOAT:
				if (!ReadNumber(source, fd, p))
				{
					return false;
				}
				p = (char*)p + sizeof(float);
				break;
			case FT_STRING:
				if (!ReadString(source, fd, p))
				{
					return false;
				}
				p = (char*)p + MAX_STRINGFIELD;
				break;
			case FT_STRUCT:
				if (!fd->substruct)
				{
					SourceError(source, "BUG: no sub structure defined");
					return false;
				}
				ReadStructure(source, fd->substruct, (char*)p);
				p = (char*)p + fd->substruct->size;
				break;
			}
			if (fd->type & FT_ARRAY)
			{
				if (!PC_ExpectAnyToken(source, &token))
				{
					return false;
				}
				if (!String::Cmp(token.string, "}"))
				{
					break;
				}
				if (String::Cmp(token.string, ","))
				{
					SourceError(source, "expected a comma, found %s", token.string);
					return false;
				}
			}
		}
	}
	return true;
}
