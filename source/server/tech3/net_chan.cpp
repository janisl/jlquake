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

#include "../server.h"
#include "local.h"
//#include "../quake3/local.h"
//#include "../wolfsp/local.h"
//#include "../wolfmp/local.h"
//#include "../et/local.h"

// first four bytes of the data are always:
//	long reliableAcknowledge;
void SVT3_Netchan_Encode(client_t* client, QMsg* msg, const char* commandString)
{
	if (msg->cursize < Q3SV_ENCODE_START)
	{
		return;
	}

	const byte* string = reinterpret_cast<const byte*>(commandString);
	int index = 0;
	// xor the client challenge with the netchan sequence number
	byte key = client->challenge ^ client->netchan.outgoingSequence;
	for (int i = Q3SV_ENCODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last received and with this message acknowledged client command
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
		*(msg->_data + i) = *(msg->_data + i) ^ key;
	}
}

// first 12 bytes of the data are always:
//	long serverId;
//	long messageAcknowledge;
//	long reliableAcknowledge;
void SVT3_Netchan_Decode(client_t* client, QMsg* msg)
{
	int srdc = msg->readcount;
	int sbit = msg->bit;
	int soob = msg->oob;

	msg->oob = 0;

	int serverId = msg->ReadLong();
	int messageAcknowledge = msg->ReadLong();
	int reliableAcknowledge = msg->ReadLong();

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	const byte* string = reinterpret_cast<byte*>(client->q3_reliableCommands[reliableAcknowledge &
		((GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF) - 1)]);
	int index = 0;

	byte key = client->challenge ^ serverId ^ messageAcknowledge;
	for (int i = msg->readcount + Q3SV_DECODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last sent and acknowledged server command
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
