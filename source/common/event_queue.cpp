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

#include "qcommon.h"
#include "../client/public.h"

/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

#define MAX_QUED_EVENTS     256
#define MASK_QUED_EVENTS    (MAX_QUED_EVENTS - 1)

static sysEvent_t eventQue[MAX_QUED_EVENTS];
static int eventHead;
static int eventTail;

#define MAX_PUSHED_EVENTS               1024

static int com_pushedEventsHead = 0;
static int com_pushedEventsTail = 0;
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

static byte sys_packetReceived[MAX_MSGLEN];

void Com_InitEventQueue()
{
	// clear queues
	Com_Memset(&eventQue[0], 0, MAX_QUED_EVENTS * sizeof(sysEvent_t));

	// clear the static buffer array
	// this requires SE_NONE to be accepted as a valid but NOP event
	Com_Memset(com_pushedEvents, 0, sizeof(com_pushedEvents));
	// reset counters while we are at it
	// beware: GetEvent might still return an SE_NONE from the buffer
	com_pushedEventsHead = 0;
	com_pushedEventsTail = 0;
}

//	A time of 0 will get the current time
//	Ptr should either be null, or point to a block of data that can
// be freed by the game later.
void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void* ptr)
{
	sysEvent_t* ev;

	ev = &eventQue[eventHead & MASK_QUED_EVENTS];
	if (eventHead - eventTail >= MAX_QUED_EVENTS)
	{
		common->Printf("Sys_QueEvent: overflow\n");
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

static sysEvent_t Sys_GetEvent()
{
	// return if we have data
	if (eventHead > eventTail)
	{
		eventTail++;
		return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
	}

	// pump the message loop
	Sys_MessageLoop();
	Sys_SendKeyEvents();

	// check for console commands
	const char* s = Sys_ConsoleInput();
	if (s)
	{
		int len = String::Length(s) + 1;
		char* b = (char*)Mem_Alloc(len);
		String::NCpyZ(b, s, len);
		Sys_QueEvent(0, SE_CONSOLE, 0, 0, len, b);
	}

	if (GGameType & GAME_Tech3)
	{
		// check for network packets
		QMsg netmsg;
		netmsg.Init(sys_packetReceived, sizeof(sys_packetReceived));
		netadr_t adr;
		if (NET_GetUdpPacket(NS_SERVER, &adr, &netmsg))
		{
			// copy out to a seperate buffer for qeueing
			int len = sizeof(netadr_t) + netmsg.cursize;
			netadr_t* buf = (netadr_t*)Mem_Alloc(len);
			*buf = adr;
			Com_Memcpy(buf + 1, netmsg._data, netmsg.cursize);
			Sys_QueEvent(0, SE_PACKET, 0, 0, len, buf);
		}
	}

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

static sysEvent_t Com_GetRealEvent()
{
	sysEvent_t ev;

	// either get an event from the system or the journal file
	if (com_journal && com_journal->integer == 2)
	{
		int r = FS_Read(&ev, sizeof(ev), com_journalFile);
		if (r != sizeof(ev))
		{
			common->FatalError("Error reading from journal file");
		}
		if (ev.evPtrLength)
		{
			ev.evPtr = Mem_Alloc(ev.evPtrLength);
			r = FS_Read(ev.evPtr, ev.evPtrLength, com_journalFile);
			if (r != ev.evPtrLength)
			{
				common->FatalError("Error reading from journal file");
			}
		}
	}
	else
	{
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if (com_journal && com_journal->integer == 1)
		{
			int r = FS_Write(&ev, sizeof(ev), com_journalFile);
			if (r != sizeof(ev))
			{
				common->FatalError("Error writing to journal file");
			}
			if (ev.evPtrLength)
			{
				r = FS_Write(ev.evPtr, ev.evPtrLength, com_journalFile);
				if (r != ev.evPtrLength)
				{
					common->FatalError("Error writing to journal file");
				}
			}
		}
	}

	return ev;
}

static void Com_PushEvent(sysEvent_t* event)
{
	static bool printedWarning = false;

	sysEvent_t* ev = &com_pushedEvents[com_pushedEventsHead & (MAX_PUSHED_EVENTS - 1)];

	if (com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS)
	{
		// don't print the warning constantly, or it can give time for more...
		if (!printedWarning)
		{
			printedWarning = true;
			common->Printf("WARNING: Com_PushEvent overflow\n");
		}

		if (ev->evPtr)
		{
			Mem_Free(ev->evPtr);
		}
		com_pushedEventsTail++;
	}
	else
	{
		printedWarning = false;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

sysEvent_t Com_GetEvent()
{
	if (com_pushedEventsHead > com_pushedEventsTail)
	{
		com_pushedEventsTail++;
		return com_pushedEvents[(com_pushedEventsTail - 1) & (MAX_PUSHED_EVENTS - 1)];
	}
	return Com_GetRealEvent();
}

//	Can be used for profiling, but will be journaled accurately
int Com_Milliseconds()
{
	// get events and push them until we get a null event with the current time
	sysEvent_t ev;
	do
	{
		ev = Com_GetRealEvent();
		if (ev.evType != SE_NONE)
		{
			Com_PushEvent(&ev);
		}
	}
	while (ev.evType != SE_NONE);

	return ev.evTime;
}
