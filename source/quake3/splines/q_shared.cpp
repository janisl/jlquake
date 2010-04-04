/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.hpp"

/*
============================================================================

GROWLISTS

============================================================================
*/

// malloc / free all in one place for debugging
extern "C" void *Com_Allocate( int bytes );
extern "C" void Com_Dealloc( void *ptr );

void Com_InitGrowList( growList_t *list, int maxElements ) {
	list->maxElements = maxElements;
	list->currentElements = 0;
	list->elements = (void **)Com_Allocate( list->maxElements * sizeof( void * ) );
}

int Com_AddToGrowList( growList_t *list, void *data ) {
	void	**old;

	if ( list->currentElements != list->maxElements ) {
		list->elements[list->currentElements] = data;
		return list->currentElements++;
	}

	// grow, reallocate and move
	old = list->elements;

	if ( list->maxElements < 0 ) {
		Com_Error( ERR_FATAL, "Com_AddToGrowList: maxElements = %i", list->maxElements );
	}

	if ( list->maxElements == 0 ) {
		// initialize the list to hold 100 elements
		Com_InitGrowList( list, 100 );
		return Com_AddToGrowList( list, data );
	}

	list->maxElements *= 2;

	Com_DPrintf( "Resizing growlist to %i maxElements\n", list->maxElements );

	list->elements = (void **)Com_Allocate( list->maxElements * sizeof( void * ) );

	if ( !list->elements ) {
		Com_Error( ERR_DROP, "Growlist alloc failed" );
	}

	memcpy( list->elements, old, list->currentElements * sizeof( void * ) );

	Com_Dealloc( old );

	return Com_AddToGrowList( list, data );
}

void *Com_GrowListElement( const growList_t *list, int index ) {
	if ( index < 0 || index >= list->currentElements ) {
		Com_Error( ERR_DROP, "Com_GrowListElement: %i out of range of %i", 
			index, list->currentElements );
	}
	return list->elements[index];
}

int Com_IndexForGrowListElement( const growList_t *list, const void *element ) {
	int		i;

	for ( i = 0 ; i < list->currentElements ; i++ ) {
		if ( list->elements[i] == element ) {
			return i;
		}
	}
	return -1;
}

//============================================================================


float Com_Clamp( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

/*
============
Com_StringContains
============
*/
const char *Com_StringContains( const char *str1, const char *str2, int casesensitive) {
	int len, i, j;

	len = QStr::Length(str1) - QStr::Length(str2);
	for (i = 0; i <= len; i++, str1++) {
		for (j = 0; str2[j]; j++) {
			if (casesensitive) {
				if (str1[j] != str2[j]) {
					break;
				}
			}
			else {
				if (QStr::ToUpper(str1[j]) != QStr::ToUpper(str2[j])) {
					break;
				}
			}
		}
		if (!str2[j]) {
			return str1;
		}
	}
	return NULL;
}

/*
============
Com_Filter
============
*/
int Com_Filter( const char *filter, const char *name, int casesensitive)
{
	char buf[MAX_TOKEN_CHARS];
	const char *ptr;
	int i, found;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			for (i = 0; *filter; i++) {
				if (*filter == '*' || *filter == '?') break;
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if (QStr::Length(buf)) {
				ptr = Com_StringContains(name, buf, casesensitive);
				if (!ptr) return qfalse;
				name = ptr + QStr::Length(buf);
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else if (*filter == '[' && *(filter+1) == '[') {
			filter++;
		}
		else if (*filter == '[') {
			filter++;
			found = qfalse;
			while(*filter && !found) {
				if (*filter == ']' && *(filter+1) != ']') break;
				if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
					if (casesensitive) {
						if (*name >= *filter && *name <= *(filter+2)) found = qtrue;
					}
					else {
						if (QStr::ToUpper(*name) >= QStr::ToUpper(*filter) &&
							QStr::ToUpper(*name) <= QStr::ToUpper(*(filter+2))) found = qtrue;
					}
					filter += 3;
				}
				else {
					if (casesensitive) {
						if (*filter == *name) found = qtrue;
					}
					else {
						if (QStr::ToUpper(*filter) == QStr::ToUpper(*name)) found = qtrue;
					}
					filter++;
				}
			}
			if (!found) return qfalse;
			while(*filter) {
				if (*filter == ']' && *(filter+1) != ']') break;
				filter++;
			}
			filter++;
			name++;
		}
		else {
			if (casesensitive) {
				if (*filter != *name) return qfalse;
			}
			else {
				if (QStr::ToUpper(*filter) != QStr::ToUpper(*name)) return qfalse;
			}
			filter++;
			name++;
		}
	}
	return qtrue;
}


/*
================
Com_HashString

================
*/
int Com_HashString( const char *fname ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = QStr::ToLower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}


/*
============
Com_SkipPath
============
*/
char *Com_SkipPath (char *pathname)
{
	char	*last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
Com_StripExtension
============
*/
void Com_StripExtension( const char *in, char *out ) {
	while ( *in && *in != '.' ) {
		*out++ = *in++;
	}
	*out = 0;
}


/*
==================
Com_DefaultExtension
==================
*/
void Com_DefaultExtension (char *path, int maxSize, const char *extension ) {
	char	oldPath[MAX_QPATH];
	char    *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + QStr::Length(path) - 1;

	while (*src != '/' && src != path) {
		if ( *src == '.' ) {
			return;                 // it has an extension
		}
		src--;
	}

	QStr::NCpyZ( oldPath, path, sizeof( oldPath ) );
	Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}

/*
===============
Com_ParseInfos
===============
*/
int Com_ParseInfos( const char *buf, int max, char infos[][MAX_INFO_STRING] ) {
	const char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];

	count = 0;

	while ( 1 ) {
		token = Com_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( QStr::Cmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		infos[count][0] = 0;
		while ( 1 ) {
			token = Com_Parse( &buf );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !QStr::Cmp( token, "}" ) ) {
				break;
			}
			QStr::NCpyZ( key, token, sizeof( key ) );

			token = Com_ParseOnLine( &buf );
			if ( !token[0] ) {
				token = "<NULL>";
			}
			Info_SetValueForKey( infos[count], key, token );
		}
		count++;
	}

	return count;
}



/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_PrintStrlen( const char *string ) {
	int			len;
	const char	*p;

	if( !string ) {
		return 0;
	}

	len = 0;
	p = string;
	while( *p ) {
		if( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++;
		}		
		else if ( c >= 0x20 && c <= 0x7E ) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}


void QDECL Com_sprintf( char *dest, int size, const char *fmt, ...) {
	int		len;
	va_list		argptr;
	char	bigbuffer[32000];	// big, but small enough to fit in PPC stack

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);
	if ( len >= sizeof( bigbuffer ) ) {
		Com_Error( ERR_FATAL, "Com_sprintf: overflowed bigbuffer" );
	}
	if (len >= size) {
		Com_Printf ("Com_sprintf: overflow of %i in %i\n", len, size);
	}
	QStr::NCpyZ(dest, bigbuffer, size );
}


/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
	char	pkey[MAX_INFO_KEY];
	static	char value[2][MAX_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;
	
	if ( !s || !key ) {
		return "";
	}

	if ( QStr::Length( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );
	}

	valueindex ^= 1;
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

		if (!QStr::ICmp(key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}


/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char *(*head), char key[MAX_INFO_KEY], char value[MAX_INFO_VALUE] ) {
	char	*o;
	const char	*s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char	*o;

	if ( QStr::Length( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
	}

	if (strchr (key, '\\')) {
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

		if (!QStr::Cmp(key, pkey) )
		{
			QStr::Cpy(start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[MAX_INFO_STRING];

	if ( QStr::Length( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	if (strchr (key, '\\') || strchr (value, '\\'))
	{
		Com_Printf ("Can't use keys or values with a \\\n");
		return;
	}

	if (strchr (key, ';') || strchr (value, ';'))
	{
		Com_Printf ("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr (key, '\"') || strchr (value, '\"'))
	{
		Com_Printf ("Can't use keys or values with a \"\n");
		return;
	}

	Info_RemoveKey (s, key);
	if (!value || !QStr::Length(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (QStr::Length(newi) + QStr::Length(s) > MAX_INFO_STRING)
	{
		Com_Printf ("Info string length exceeded\n");
		return;
	}

	QStr::Cat(s, MAX_INFO_STRING, newi);
}

//====================================================================


/*
===============
ParseHex
===============
*/
int	ParseHex( const char *text ) {
	int		value;
	int		c;

	value = 0;
	while ( ( c = *text++ ) != 0 ) {
		if ( c >= '0' && c <= '9' ) {
			value = value * 16 + c - '0';
			continue;
		}
		if ( c >= 'a' && c <= 'f' ) {
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if ( c >= 'A' && c <= 'F' ) {
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}
