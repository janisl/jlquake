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

//need to rewrite this

#ifndef __UTIL_STR_H__
#define __UTIL_STR_H__

#include "../../common/strings.h"

class strdata
{
public:
	strdata() : len(0), refcount(0), data(NULL), alloced(0)
	{
	}
	~strdata()
	{
		if (data)
		{
			delete [] data;
		}
	}

	void AddRef() { refcount++; }
	bool DelRef()	// True if killed
	{
		refcount--;
		if (refcount < 0)
		{
			delete this;
			return true;
		}

		return false;
	}

	int len;
	int refcount;
	char* data;
	int alloced;
};

class idStr
{
protected:
	strdata* m_data;
	void EnsureAlloced(int, bool keepold = true);
	void EnsureDataWritable();

public:
	~idStr();
	idStr();
	idStr(const char* text);
	idStr(const idStr& string);
	int length(void) const;
	const char* c_str(void) const;

	void operator=(const idStr& text);
	void operator=(const char* text);
};

inline idStr::~idStr()
{
	if (m_data)
	{
		m_data->DelRef();
		m_data = NULL;
	}
}

inline idStr::idStr() : m_data(NULL)
{
	EnsureAlloced(1);
	m_data->data[0] = 0;
}

inline idStr::idStr
(
	const char* text
) : m_data(NULL)
{
	int len;

	assert(text);

	if (text)
	{
		len = String::Length(text);
		EnsureAlloced(len + 1);
		String::Cpy(m_data->data, text);
		m_data->len = len;
	}
	else
	{
		EnsureAlloced(1);
		m_data->data[0] = 0;
		m_data->len = 0;
	}
}

inline idStr::idStr
(
	const idStr& text
) : m_data(NULL)
{
	m_data = text.m_data;
	m_data->AddRef();
}

inline int idStr::length(void) const
{
	return (m_data != NULL) ? m_data->len : 0;
}

inline const char* idStr::c_str(void) const
{
	assert(m_data);

	return m_data->data;
}

inline void idStr::operator=
(
	const idStr& text
)
{
	// adding the reference before deleting our current reference prevents
	// us from deleting our string if we are copying from ourself
	text.m_data->AddRef();
	m_data->DelRef();
	m_data = text.m_data;
}

inline void idStr::operator=
(
	const char* text
)
{
	int len;

	assert(text);

	if (!text)
	{
		// safe behaviour if NULL
		EnsureAlloced(1, false);
		m_data->data[0] = 0;
		m_data->len = 0;
		return;
	}

	if (!m_data)
	{
		len = String::Length(text);
		EnsureAlloced(len + 1, false);
		String::Cpy(m_data->data, text);
		m_data->len = len;
		return;
	}

	if (text == m_data->data)
	{
		return;	// Copying same thing.  Punt.

	}
	// If we alias and I don't do this, I could corrupt other strings...  This
	// will get called with EnsureAlloced anyway
	EnsureDataWritable();

	// Now we need to check if we're aliasing..
	if (text >= m_data->data && text <= m_data->data + m_data->len)
	{
		// Great, we're aliasing.  We're copying from inside ourselves.
		// This means that I don't have to ensure that anything is alloced,
		// though I'll assert just in case.
		int diff = text - m_data->data;
		int i;

		assert(String::Length(text) < (unsigned)m_data->len);

		for (i = 0; text[i]; i++)
		{
			m_data->data[i] = text[i];
		}

		m_data->data[i] = 0;

		m_data->len -= diff;

		return;
	}

	len = String::Length(text);
	EnsureAlloced(len + 1, false);
	String::Cpy(m_data->data, text);
	m_data->len = len;
}

#endif
