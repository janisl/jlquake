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
//**
//**	Memory Allocation.
//**
//**************************************************************************

//#define MEM_DEBUG		1

void Mem_Shutdown();

#ifdef MEM_DEBUG

void* Mem_Alloc(int size, const char* FileName, int LineNumber);
void* Mem_ClearedAlloc(int size, const char* FileName, int LineNumber);
void Mem_Free(void* ptr, const char* FileName, int LineNumber);

inline void* operator new(size_t Size, const char* FileName, int LineNumber)
{
	return Mem_Alloc(Size, FileName, LineNumber);
}

inline void operator delete(void* Ptr, const char* FileName, int LineNumber)
{
	Mem_Free(Ptr, FileName, LineNumber);
}

inline void* operator new[](size_t Size, const char* FileName, int LineNumber)
{
	return Mem_Alloc(Size, FileName, LineNumber);
}

inline void operator delete[](void* Ptr, const char* FileName, int LineNumber)
{
	Mem_Free(Ptr, FileName, LineNumber);
}

inline void* operator new(size_t Size)
{
	return Mem_Alloc(Size, "", 0);
}

inline void operator delete(void* Ptr)
{
	Mem_Free(Ptr, "", 0);
}

inline void* operator new[](size_t Size)
{
	return Mem_Alloc(Size, "", 0);
}

inline void operator delete[](void* Ptr)
{
	Mem_Free(Ptr, "", 0);
}

#define Mem_Alloc(size)				Mem_Alloc(size, __FILE__, __LINE__)
#define Mem_ClearedAlloc(size)		Mem_ClearedAlloc(size, __FILE__, __LINE__)
#define Mem_Free(ptr)				Mem_Free(ptr, __FILE__, __LINE__)

#define MEM_DEBUG_NEW				new(__FILE__, __LINE__)
#undef new
#define new							MEM_DEBUG_NEW

#else

void* Mem_Alloc(int size);
void* Mem_ClearedAlloc(int size);
void Mem_Free(void* ptr);

inline void* operator new(size_t Size)
{
	return Mem_Alloc(Size);
}

inline void operator delete(void* Ptr)
{
	Mem_Free(Ptr);
}

inline void* operator new[](size_t Size)
{
	return Mem_Alloc(Size);
}

inline void operator delete[](void* Ptr)
{
	Mem_Free(Ptr);
}

#endif
