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

#include "../../client.h"
#include "local.h"
#include "../../../server/public.h"

// first 12 bytes of the data are always:
//  long serverId;
//  long messageAcknowledge;
//  long reliableAcknowledge;
static void CLT3_Netchan_Encode(QMsg* msg)
{
	if (msg->cursize <= Q3CL_ENCODE_START)
	{
		return;
	}

	int srdc = msg->readcount;
	int sbit = msg->bit;
	int soob = msg->oob;

	msg->bit = 0;
	msg->readcount = 0;
	msg->oob = 0;

	int serverId = msg->ReadLong();
	int messageAcknowledge = msg->ReadLong();
	int reliableAcknowledge = msg->ReadLong();

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	byte* string = (byte*)clc.q3_serverCommands[reliableAcknowledge & ((GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF) - 1)];
	int index = 0;

	byte key = clc.q3_challenge ^ serverId ^ messageAcknowledge;
	for (int i = Q3CL_ENCODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last received now acknowledged server command
		if (!string[index])
		{
			index = 0;
		}
		if (string[index] > 127 || string[index] == '%')
		{
			key ^= '.' << (i & 1);
		}
		else
		{
			key ^= string[index] << (i & 1);
		}
		index++;
		// encode the data with this key
		*(msg->_data + i) = (*(msg->_data + i)) ^ key;
	}
}

// first four bytes of the data are always:
//  long reliableAcknowledge;
static void CLT3_Netchan_Decode(QMsg* msg)
{
	int srdc = msg->readcount;
	int sbit = msg->bit;
	int soob = msg->oob;

	msg->oob = 0;

	int reliableAcknowledge = msg->ReadLong();

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	byte* string = (byte*)clc.q3_reliableCommands[reliableAcknowledge & ((GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF) - 1)];
	int index = 0;
	// xor the client challenge with the netchan sequence number (need something that changes every message)
	byte key = clc.q3_challenge ^ LittleLong(*(unsigned*)msg->_data);
	for (int i = msg->readcount + Q3CL_DECODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last sent and with this message acknowledged client command
		if (!string[index])
		{
			index = 0;
		}
		if (string[index] > 127 || string[index] == '%')
		{
			key ^= '.' << (i & 1);
		}
		else
		{
			key ^= string[index] << (i & 1);
		}
		index++;
		// decode the data with this key
		*(msg->_data + i) = *(msg->_data + i) ^ key;
	}
}

static void CLET_WriteBinaryMessage(QMsg* msg)
{
	if (!clc.et_binaryMessageLength)
	{
		return;
	}

	msg->Uncompressed();

	if ((msg->cursize + clc.et_binaryMessageLength) >= msg->maxsize)
	{
		clc.et_binaryMessageOverflowed = true;
		return;
	}

	msg->WriteData(clc.et_binaryMessage, clc.et_binaryMessageLength);
	clc.et_binaryMessageLength = 0;
	clc.et_binaryMessageOverflowed = false;
}

void CLT3_Netchan_TransmitNextFragment(netchan_t* chan)
{
	Netchan_TransmitNextFragment(chan);
}

void CLT3_Netchan_Transmit(netchan_t* chan, QMsg* msg)
{
	msg->WriteByte(q3clc_EOF);
	if (GGameType & GAME_ET)
	{
		CLET_WriteBinaryMessage(msg);
	}

	if (!(GGameType & GAME_WolfSP) && (!(GGameType & GAME_ET) || !SVET_GameIsSinglePlayer()))
	{
		CLT3_Netchan_Encode(msg);
	}
	Netchan_Transmit(chan, msg->cursize, msg->_data);
}

bool CLT3_Netchan_Process(netchan_t* chan, QMsg* msg)
{
	int ret = Netchan_Process(chan, msg);
	if (!ret)
	{
		return false;
	}
	if (!(GGameType & GAME_WolfSP) && (!(GGameType & GAME_ET) || !SVET_GameIsSinglePlayer()))
	{
		CLT3_Netchan_Decode(msg);
	}
	return true;
}
