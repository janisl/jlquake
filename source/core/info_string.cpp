//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	INFO STRINGS
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Info_ValueForKey
//
//	Searches the string for the given key and returns the associated value,
// or an empty string.
//
//==========================================================================

const char* Info_ValueForKey(const char* s, const char* key)
{
	char	pkey[BIG_INFO_KEY];
	static	char value[4][BIG_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;
	
	if (!s || !key)
	{
		return "";
	}

	if (String::Length( s ) >= BIG_INFO_STRING)
	{
		throw QDropException("Info_ValueForKey: oversize infostring");
	}

	valueindex = (valueindex + 1) % 4;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!String::ICmp(key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}

//==========================================================================
//
//	Info_RemoveKey
//
//==========================================================================

void Info_RemoveKey(char* s, const char* key, int MaxSize)
{
	char	*start;
	char	pkey[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char	*o;

	if (String::Length(s) >= MaxSize)
	{
		throw QDropException("Info_RemoveKey: oversize infostring");
	}

	if (strchr(key, '\\'))
	{
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!String::Cmp (key, pkey) )
		{
			String::Cpy(start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}
}

//==========================================================================
//
//	Info_RemovePrefixedKeys
//
//==========================================================================

void Info_RemovePrefixedKeys(char *start, char prefix, int MaxSize)
{
	char	*s;
	char	pkey[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	char	*o;

	s = start;

	while (1)
	{
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (pkey[0] == prefix)
		{
			Info_RemoveKey (start, pkey, MaxSize);
			s = start;
		}

		if (!*s)
			return;
	}
}

//==========================================================================
//
//	Info_DoSetValueForKey
//
//	Changes or adds a key/value pair
//
//==========================================================================

void Info_SetValueForKey(char *s, const char* key, const char* value,
	int MaxSize, int MaxKeySize, int MaxValSize, bool NoHighChars, bool LowerCaseVal)
{
	char	newi[BIG_INFO_STRING];

	if (String::Length(s) >= MaxSize)
	{
		throw QDropException("Info_SetValueForKey: oversize infostring");
	}

	if (strchr(key, '\\') || strchr(value, '\\'))
	{
		gLog.write("Can't use keys or values with a \\\n");
		return;
	}

	if (strchr(key, ';') || strchr(value, ';'))
	{
		gLog.write("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr(key, '\"') || strchr(value, '\"'))
	{
		gLog.write("Can't use keys or values with a \"\n");
		return;
	}

	if (String::Length(key) > MaxKeySize - 1 || String::Length(value) > MaxValSize - 1)
	{
		gLog.write("Keys and values must be < %d characters.\n", MaxKeySize);
		return;
	}

	Info_RemoveKey(s, key, MaxSize);
	if (!value || !String::Length(value))
		return;

	String::Sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

	if (String::Length(newi) + String::Length(s) > MaxSize)
	{
		gLog.write("Info string length exceeded\n");
		return;
	}

	if (NoHighChars || LowerCaseVal)
	{
		// only copy ascii values
		s += String::Length(s);
		char* v = newi;
		while (*v)
		{
			int c = (unsigned char)*v++;
			if (NoHighChars)
			{
				c &= 127;		// strip high bits
				if (c < 32 || c >= 127)
					continue;
				if (LowerCaseVal)
					c = String::ToLower(c);
			}
			if (c > 13)
				*s++ = c;
		}
		*s = 0;
	}
	else
	{
		String::Cat(newi, sizeof(newi), s);
		String::Cpy(s, newi);
	}
}

//==========================================================================
//
//	Info_Validate
//
//	Some characters are illegal in info strings because they can mess up the
// server's parsing
//
//==========================================================================

bool Info_Validate(const char* s)
{
	if (strchr(s, '\"'))
	{
		return false;
	}
	if (strchr(s, ';'))
	{
		return false;
	}
	return true;
}

//==========================================================================
//
//	Info_NextPair
//
//	Used to itterate through all the key/value pairs in an info string
//
//==========================================================================

void Info_NextPair(const char** head, char* key, char* value)
{
	char*		o;
	const char*	s;

	s = *head;

	if (*s == '\\')
	{
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while (*s != '\\')
	{
		if (!*s)
		{
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while (*s != '\\' && *s)
	{
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}

//==========================================================================
//
//	Info_Print
//
//==========================================================================

void Info_Print(const char* s)
{
	char	key[512];
	char	value[512];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			Com_Memset(o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		gLog.write("%s", key);

		if (!*s)
		{
			gLog.write("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		gLog.write("%s\n", value);
	}
}
