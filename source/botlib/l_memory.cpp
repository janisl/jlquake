/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
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

/*****************************************************************************
 * name:		l_memory.c
 *
 * desc:		memory allocation
 *
 * $Archive: /MissionPack/code/botlib/l_memory.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "botlib.h"
#include "l_log.h"
#include "be_interface.h"

#define MEM_ID      0x12345678l
#define HUNK_ID     0x87654321l

int allocatedmemory;
int totalmemorysize;
int numblocks;

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void* GetMemory(unsigned long size)
{
	void* ptr;
	unsigned long int* memid;

	ptr = botimport.GetMemory(size + sizeof(unsigned long int));
	if (!ptr)
	{
		return NULL;
	}
	memid = (unsigned long int*)ptr;
	*memid = MEM_ID;
	return (unsigned long int*)((char*)ptr + sizeof(unsigned long int));
}	//end of the function GetMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void* GetClearedMemory(unsigned long size)
{
	void* ptr;
	ptr = GetMemory(size);
	Com_Memset(ptr, 0, size);
	return ptr;
}	//end of the function GetClearedMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void* GetHunkMemory(unsigned long size)
{
	void* ptr;
	unsigned long int* memid;

	ptr = botimport.HunkAlloc(size + sizeof(unsigned long int));
	if (!ptr)
	{
		return NULL;
	}
	memid = (unsigned long int*)ptr;
	*memid = HUNK_ID;
	return (unsigned long int*)((char*)ptr + sizeof(unsigned long int));
}	//end of the function GetHunkMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void* GetClearedHunkMemory(unsigned long size)
{
	void* ptr;
	ptr = GetHunkMemory(size);
	Com_Memset(ptr, 0, size);
	return ptr;
}	//end of the function GetClearedHunkMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void FreeMemory(void* ptr)
{
	unsigned long int* memid;

	memid = (unsigned long int*)((char*)ptr - sizeof(unsigned long int));

	if (*memid == MEM_ID)
	{
		botimport.FreeMemory(memid);
	}	//end if
}	//end of the function FreeMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AvailableMemory(void)
{
	return botimport.AvailableMemory();
}	//end of the function AvailableMemory
