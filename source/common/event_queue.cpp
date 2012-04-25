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

// HEADER FILES ------------------------------------------------------------

#include "qcommon.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

sysEvent_t eventQue[MAX_QUED_EVENTS];
int eventHead;
int eventTail;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Sys_QueEvent
//
//	A time of 0 will get the current time
//	Ptr should either be null, or point to a block of data that can
// be freed by the game later.
//
//==========================================================================

void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void* ptr)
{
	sysEvent_t* ev;

	ev = &eventQue[eventHead & MASK_QUED_EVENTS];
	if (eventHead - eventTail >= MAX_QUED_EVENTS)
	{
		Log::write("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if (ev->evPtr)
		{
			Mem_Free(ev->evPtr);
		}
		eventTail++;
	}

	eventHead++;

	if (time == 0)
	{
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

//==========================================================================
//
//	Sys_SharedGetEvent
//
//==========================================================================

sysEvent_t Sys_SharedGetEvent()
{
	// return if we have data
	if (eventHead > eventTail)
	{
		eventTail++;
		return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
	}

	// create an empty event to return

	sysEvent_t ev;
	Com_Memset(&ev, 0, sizeof(ev));
	//	Windows uses timeGetTime();
	ev.evTime = Sys_Milliseconds();

	return ev;
}
