/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//need to rewrite this

#include "../../common/qcommon.h"
#include "util_str.h"
#include <ctype.h>

#if __MACOS__
// LBO 2/1/05. Apple's system headers define these as macros. D'oh!
#undef tolower
#undef toupper
#endif

#ifdef _WIN32
#ifndef __GNUC__
#pragma warning(disable : 4244)	// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable : 4710)	// function 'blah' not inlined
#endif
#endif

static const int STR_ALLOC_GRAN = 20;

char* idStr::tolower
(
	char* s1
)
{
	char* s;

	s = s1;
	while (*s)
	{
		*s = ::tolower(*s);
		s++;
	}

	return s1;
}

char* idStr::toupper
(
	char* s1
)
{
	char* s;

	s = s1;
	while (*s)
	{
		*s = ::String::ToUpper(*s);
		s++;
	}

	return s1;
}

int idStr::icmpn
(
	const char* s1,
	const char* s2,
	int n
)
{
	int c1;
	int c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
		{
			// idStrings are equal until end point
			return 0;
		}

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
			{
				c1 -= ('a' - 'A');
			}

			if (c2 >= 'a' && c2 <= 'z')
			{
				c2 -= ('a' - 'A');
			}

			if (c1 < c2)
			{
				// strings less than
				return -1;
			}
			else if (c1 > c2)
			{
				// strings greater than
				return 1;
			}
		}
	}
	while (c1);

	// strings are equal
	return 0;
}

int idStr::icmp
(
	const char* s1,
	const char* s2
)
{
	int c1;
	int c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
			{
				c1 -= ('a' - 'A');
			}

			if (c2 >= 'a' && c2 <= 'z')
			{
				c2 -= ('a' - 'A');
			}

			if (c1 < c2)
			{
				// strings less than
				return -1;
			}
			else if (c1 > c2)
			{
				// strings greater than
				return 1;
			}
		}
	}
	while (c1);

	// strings are equal
	return 0;
}

int idStr::cmpn
(
	const char* s1,
	const char* s2,
	int n
)
{
	int c1;
	int c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
		{
			// strings are equal until end point
			return 0;
		}

		if (c1 < c2)
		{
			// strings less than
			return -1;
		}
		else if (c1 > c2)
		{
			// strings greater than
			return 1;
		}
	}
	while (c1);

	// strings are equal
	return 0;
}

int idStr::cmp
(
	const char* s1,
	const char* s2
)
{
	int c1;
	int c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (c1 < c2)
		{
			// strings less than
			return -1;
		}
		else if (c1 > c2)
		{
			// strings greater than
			return 1;
		}
	}
	while (c1);

	// strings are equal
	return 0;
}

/*
============
IsNumeric

Checks a string to see if it contains only numerical values.
============
*/
bool idStr::isNumeric
(
	const char* str
)
{
	int len;
	int i;
	bool dot;

	if (*str == '-')
	{
		str++;
	}

	dot = false;
	len = String::Length(str);
	for (i = 0; i < len; i++)
	{
		if (!isdigit(str[i]))
		{
			if ((str[i] == '.') && !dot)
			{
				dot = true;
				continue;
			}
			return false;
		}
	}

	return true;
}

idStr operator+
(
	const idStr& a,
	const float b
)
{
	char text[20];

	idStr result(a);

	sprintf(text, "%f", b);
	result.append(text);

	return result;
}

idStr operator+
(
	const idStr& a,
	const int b
)
{
	char text[20];

	idStr result(a);

	sprintf(text, "%d", b);
	result.append(text);

	return result;
}

idStr operator+
(
	const idStr& a,
	const unsigned b
)
{
	char text[20];

	idStr result(a);

	sprintf(text, "%u", b);
	result.append(text);

	return result;
}

idStr& idStr::operator+=
(
	const float a
)
{
	char text[20];

	sprintf(text, "%f", a);
	append(text);

	return *this;
}

idStr& idStr::operator+=
(
	const int a
)
{
	char text[20];

	sprintf(text, "%d", a);
	append(text);

	return *this;
}

idStr& idStr::operator+=
(
	const unsigned a
)
{
	char text[20];

	sprintf(text, "%u", a);
	append(text);

	return *this;
}

void idStr::CapLength
(
	int newlen
)
{
	assert(m_data);

	if (length() <= newlen)
	{
		return;
	}

	EnsureDataWritable();

	m_data->data[newlen] = 0;
	m_data->len = newlen;
}

void idStr::EnsureDataWritable
(
	void
)
{
	assert(m_data);
	strdata* olddata;
	int len;

	if (!m_data->refcount)
	{
		return;
	}

	olddata = m_data;
	len = length();

	m_data = new strdata;

	EnsureAlloced(len + 1, false);
	String::NCpy(m_data->data, olddata->data, len + 1);
	m_data->len = len;

	olddata->DelRef();
}

void idStr::EnsureAlloced(int amount, bool keepold)
{

	if (!m_data)
	{
		m_data = new strdata();
	}

	// Now, let's make sure it's writable
	EnsureDataWritable();

	char* newbuffer;
	bool wasalloced = (m_data->alloced != 0);

	if (amount < m_data->alloced)
	{
		return;
	}

	assert(amount);
	if (amount == 1)
	{
		m_data->alloced = 1;
	}
	else
	{
		int newsize, mod;
		mod = amount % STR_ALLOC_GRAN;
		if (!mod)
		{
			newsize = amount;
		}
		else
		{
			newsize = amount + STR_ALLOC_GRAN - mod;
		}
		m_data->alloced = newsize;
	}

	newbuffer = new char[m_data->alloced];
	if (wasalloced && keepold)
	{
		String::Cpy(newbuffer, m_data->data);
	}

	if (m_data->data)
	{
		delete [] m_data->data;
	}
	m_data->data = newbuffer;
}

void idStr::BackSlashesToSlashes
(
	void
)
{
	int i;

	EnsureDataWritable();

	for (i = 0; i < m_data->len; i++)
	{
		if (m_data->data[i] == '\\')
		{
			m_data->data[i] = '/';
		}
	}
}

void idStr::snprintf
(
	char* dst,
	int size,
	const char* fmt,
	...
)
{
	char buffer[0x10000];
	int len;
	va_list argptr;

	va_start(argptr,fmt);
	len = vsprintf(buffer,fmt,argptr);
	va_end(argptr);

	assert(len < size);

	String::NCpy(dst, buffer, size - 1);
}
