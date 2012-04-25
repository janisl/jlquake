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

#include "../../common/qcommon.h"
#include "util_str.h"

static const int STR_ALLOC_GRAN = 20;

void idStr::EnsureDataWritable
(
	void
)
{
	qassert(m_data);
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

	qassert(amount);
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
