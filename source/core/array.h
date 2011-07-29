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
//**	Dynamic array template.
//**
//**************************************************************************

#ifdef MEM_DEBUG_NEW
#undef new
#endif

enum EArrayNew { E_ArrayNew };

inline void* operator new(size_t, void* Ptr, EArrayNew, EArrayNew)
{
	return Ptr;
}

template<class T> class QArray
{
public:
	QArray()
	: ArrData(0)
	{
		Clear();
	}
	QArray(const QArray<T>& Other)
	: ArrData(0)
	{
		*this = Other;
	}
	~QArray()
	{
		Clear();
	}

	void Clear()
	{
		if (ArrData)
		{
			for (int i = 0; i < ArrSize; i++)
			{
				ArrData[i].~T();
			}
			Mem_Free(ArrData);
		}
		ArrData = NULL;
		ArrNum = 0;
		ArrSize = 0;
	}
	int Num() const
	{
		return ArrNum;
	}
	int NumAllocated() const
	{
		return ArrSize;
	}
	T* Ptr()
	{
		return ArrData;
	}
	const T* Ptr() const
	{
		return ArrData;
	}

	void Resize(int NewSize)
	{
		qassert(NewSize >= 0);

		if (NewSize <= 0)
		{
			Clear();
			return;
		}

		if (NewSize == ArrSize)
		{
			return;
		}
		T* OldData = ArrData;
		int OldSize = ArrSize;
		ArrSize = NewSize;
		if (ArrNum > NewSize)
		{
			ArrNum = NewSize;
		}

		ArrData = (T*)Mem_Alloc(ArrSize * sizeof(T));
		for (int i = 0; i < ArrSize; i++)
		{
			new(&ArrData[i], E_ArrayNew, E_ArrayNew) T;
		}
		for (int i = 0; i < ArrNum; i++)
		{
			ArrData[i] = OldData[i];
		}

		if (OldData)
		{
			for (int i = 0; i < OldSize; i++)
			{
				OldData[i].~T();
			}
			Mem_Free(OldData);
		}
	}
	void SetNum(int NewNum, bool bResize = true)
	{
		qassert(NewNum >= 0);
		if (bResize || NewNum > ArrSize)
		{
			Resize(NewNum);
		}
		ArrNum = NewNum;
	}
	void SetNumWithReserve(int NewNum)
	{
		qassert(NewNum >= 0);
		if (NewNum > ArrSize)
		{
			Resize(NewNum + NewNum * 3 / 8 + 32);
		}
		ArrNum = NewNum;
	}
	void Condense()
	{
		Resize(ArrNum);
	}

	QArray<T>& operator=(const QArray<T>& Other)
	{
		Clear();

		ArrNum = Other.ArrNum;
		ArrSize = Other.ArrSize;
		if (ArrSize)
		{
			ArrData = (T*)Mem_Alloc(ArrSize * sizeof(T));
			for (int i = 0; i < ArrSize; i++)
			{
				new(&ArrData[i], E_ArrayNew, E_ArrayNew) T;
			}
			for (int i = 0; i < ArrNum; i++)
			{
				ArrData[i] = Other.ArrData[i];
			}
		}
		return *this;
	}
	T& operator[](int Index)
	{
		qassert(Index >= 0);
		qassert(Index < ArrNum);
		return ArrData[Index];
	}
	const T& operator[](int Index) const
	{
		qassert(Index >= 0);
		qassert(Index < ArrNum);
		return ArrData[Index];
	}

	void Insert(int Index, const T& Item)
	{
		if (ArrNum == ArrSize)
		{
			Resize(ArrSize + ArrSize * 3 / 8 + 32);
		}
		ArrNum++;
		for (int i = ArrNum - 1; i > Index; i--)
		{
			ArrData[i] = ArrData[i - 1];
		}
		ArrData[Index] = Item;
	}
	int Append(const T& Item)
	{
		if (ArrNum == ArrSize)
		{
			Resize(ArrSize + ArrSize * 3 / 8 + 32);
		}
		ArrData[ArrNum] = Item;
		ArrNum++;
		return ArrNum - 1;
	}
	T& Alloc()
	{
		if (ArrNum == ArrSize)
		{
			Resize(ArrSize + ArrSize * 3 / 8 + 32);
		}

		return ArrData[ArrNum++];
	}
	bool RemoveIndex(int Index)
	{
		qassert(ArrData != NULL);
		qassert(Index >= 0);
		qassert(Index < ArrNum);
	
		if (Index < 0 || Index >= ArrNum)
		{
			return false;
		}

		ArrNum--;
		for (int i = Index; i < ArrNum; i++)
		{
			ArrData[i] = ArrData[i + 1];
		}

		return true;
	}
	void Remove(const T& Item)
	{
		for (int i = 0; i < ArrNum; i++)
		{
			if (ArrData[i] == Item)
			{
				RemoveIndex(i--);
			}
		}
	}

private:
	int		ArrNum;
	int		ArrSize;
	T*		ArrData;
};

#ifdef MEM_DEBUG_NEW
#define new MEM_DEBUG_NEW
#endif
