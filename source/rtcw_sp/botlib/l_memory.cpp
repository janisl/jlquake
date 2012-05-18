/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		l_memory.c
 *
 * desc:		memory allocation
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
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
	memset(ptr, 0, size);
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
	memset(ptr, 0, size);
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
