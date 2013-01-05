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

#include "event_queue.h"
#include "Common.h"
#include "common_defs.h"
#include "strings.h"
#include "command_buffer.h"
#include "command_line_args.h"
#include "network_channel.h"
#include "system.h"
#include "../client/public.h"
#include "../server/public.h"

/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

#define MAX_QUED_EVENTS     256
#define MASK_QUED_EVENTS    (MAX_QUED_EVENTS - 1)

#define MAX_PUSHED_EVENTS               1024

struct sysEvent_t
{
	int evTime;
	sysEventType_t evType;
	int evValue;
	int evValue2;
	int evPtrLength;				// bytes of data pointed to by evPtr, for journaling
	void* evPtr;					// this must be manually freed if not NULL
};

Cvar* com_journal;
fileHandle_t com_journalFile;			// events are written here
fileHandle_t com_journalDataFile;		// config files are written here

static sysEvent_t eventQue[MAX_QUED_EVENTS];
static int eventHead;
static int eventTail;

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

void Com_InitJournaling()
{
	Com_StartupVariable("journal");
	com_journal = Cvar_Get("journal", "0", CVAR_INIT);
	if (!com_journal->integer)
	{
		return;
	}

	if (com_journal->integer == 1)
	{
		common->Printf("Journaling events\n");
		com_journalFile = FS_FOpenFileWrite("journal.dat");
		com_journalDataFile = FS_FOpenFileWrite("journaldata.dat");
	}
	else if (com_journal->integer == 2)
	{
		common->Printf("Replaying journaled events\n");
		FS_FOpenFileRead("journal.dat", &com_journalFile, true);
		FS_FOpenFileRead("journaldata.dat", &com_journalDataFile, true);
	}

	if (!com_journalFile || !com_journalDataFile)
	{
		Cvar_Set("com_journal", "0");
		com_journalFile = 0;
		com_journalDataFile = 0;
		common->Printf("Couldn't open journal files\n");
	}
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

static sysEvent_t Com_GetEvent()
{
	if (com_pushedEventsHead > com_pushedEventsTail)
	{
		com_pushedEventsTail++;
		return com_pushedEvents[(com_pushedEventsTail - 1) & (MAX_PUSHED_EVENTS - 1)];
	}
	return Com_GetRealEvent();
}

static void Com_RunAndTimeServerPacket(netadr_t* evFrom, QMsg* buf)
{
	int t1, t2, msec;

	t1 = 0;

	if (com_speeds->integer)
	{
		t1 = Sys_Milliseconds();
	}

	SVT3_PacketEvent(*evFrom, buf);

	if (com_speeds->integer)
	{
		t2 = Sys_Milliseconds();
		msec = t2 - t1;
		if (com_speeds->integer == 3)
		{
			common->Printf("SVT3_PacketEvent time: %i\n", msec);
		}
	}
}

//	Returns last event time
int Com_EventLoop()
{
	netadr_t evFrom;
	byte bufData[MAX_MSGLEN];
	QMsg buf;

	buf.Init(bufData, sizeof(bufData));

	while (1)
	{
		sysEvent_t ev = Com_GetEvent();

		// if no more events are available
		if (ev.evType == SE_NONE)
		{
			if (GGameType & GAME_Tech3)
			{
				// manually send packet events for the loopback channel
				while (NET_GetLoopPacket(NS_CLIENT, &evFrom, &buf))
				{
					CLT3_PacketEvent(evFrom, &buf);
				}

				while (NET_GetLoopPacket(NS_SERVER, &evFrom, &buf))
				{
					// if the server just shut down, flush the events
					if (com_sv_running->integer)
					{
						Com_RunAndTimeServerPacket(&evFrom, &buf);
					}
				}
			}

			return ev.evTime;
		}

		switch (ev.evType)
		{
		default:
			common->FatalError("Com_EventLoop: bad event type %i", ev.evType);
			break;
		case SE_NONE:
			break;
		case SE_KEY:
			CL_KeyEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CHAR:
			CL_CharEvent(ev.evValue);
			break;
		case SE_MOUSE:
			CL_MouseEvent(ev.evValue, ev.evValue2);
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent(ev.evValue, ev.evValue2);
			break;
		case SE_CONSOLE:
			Cbuf_AddText((char*)ev.evPtr);
			Cbuf_AddText("\n");
			break;
		case SE_PACKET:
			if (GGameType & GAME_Tech3)
			{
				// this cvar allows simulation of connections that
				// drop a lot of packets.  Note that loopback connections
				// don't go through here at all.
				if (com_dropsim->value > 0)
				{
					static int seed;

					if (Q_random(&seed) < com_dropsim->value)
					{
						break;		// drop this packet
					}
				}

				evFrom = *(netadr_t*)ev.evPtr;
				buf.cursize = ev.evPtrLength - sizeof(evFrom);

				// we must copy the contents of the message out, because
				// the event buffers are only large enough to hold the
				// exact payload, but channel messages need to be large
				// enough to hold fragment reassembly
				if ((unsigned)buf.cursize > (unsigned)buf.maxsize)
				{
					common->Printf("Com_EventLoop: oversize packet\n");
					continue;
				}
				Com_Memcpy(buf._data, (byte*)((netadr_t*)ev.evPtr + 1), buf.cursize);
				if (com_sv_running->integer)
				{
					Com_RunAndTimeServerPacket(&evFrom, &buf);
				}
				else
				{
					CLT3_PacketEvent(evFrom, &buf);
				}
			}
			break;
		}

		// free any block data
		if (ev.evPtr)
		{
			Mem_Free(ev.evPtr);
		}
	}

	return 0;	// never reached
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
