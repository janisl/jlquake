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

sysEvent_t eventQue[MAX_QUED_EVENTS];
int eventHead;
int eventTail;

static byte sys_packetReceived[MAX_MSGLEN];

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

sysEvent_t Sys_SharedGetEvent()
{
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
