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

#ifndef __UTIL_LIST_H__
#define __UTIL_LIST_H__

template<class type>
class idList
{
private:
	int m_num;
	int m_size;
	int m_granularity;
	type* m_list;

public:
	idList(int granularity = 16);
	~idList();
	void Clear(void);
	int Num(void);
	void Resize(int size);
	type operator[](int index) const;
	type& operator[](int index);
	int Append(type const& obj);
};

template<class type>
inline idList<type>::idList(int granularity)
{
	assert(granularity > 0);

	m_list          = NULL;
	m_granularity   = granularity;
	Clear();
}

template<class type>
inline idList<type>::~idList()
{
	Clear();
}

template<class type>
inline void idList<type>::Clear(void)
{
	if (m_list)
	{
		delete[] m_list;
	}

	m_list  = NULL;
	m_num   = 0;
	m_size  = 0;
}

template<class type>
inline int idList<type>::Num(void)
{
	return m_num;
}

template<class type>
inline void idList<type>::Resize(int size)
{
	type* temp;
	int i;

	assert(size > 0);

	if (size <= 0)
	{
		Clear();
		return;
	}

	temp    = m_list;
	m_size  = size;
	if (m_size < m_num)
	{
		m_num = m_size;
	}

	m_list = new type[m_size];
	for (i = 0; i < m_num; i++)
	{
		m_list[i] = temp[i];
	}

	if (temp)
	{
		delete[] temp;
	}
}

template<class type>
inline type idList<type>::operator[](int index) const
{
	assert(index >= 0);
	assert(index < m_num);

	return m_list[index];
}

template<class type>
inline type& idList<type>::operator[](int index)
{
	assert(index >= 0);
	assert(index < m_num);

	return m_list[index];
}

template<class type>
inline int idList<type>::Append(type const& obj)
{
	if (!m_list)
	{
		Resize(m_granularity);
	}

	if (m_num == m_size)
	{
		Resize(m_size + m_granularity);
	}

	m_list[m_num] = obj;
	m_num++;

	return m_num - 1;
}

#endif	/* !__UTIL_LIST_H__ */
