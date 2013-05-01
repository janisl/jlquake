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

#include "local.h"
#include "../et/local.h"
#include "../public.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"

// first four bytes of the data are always:
//	long reliableAcknowledge;
static void SVT3_Netchan_Encode( client_t* client, QMsg* msg, const char* commandString ) {
	if ( msg->cursize < Q3SV_ENCODE_START ) {
		return;
	}

	const byte* string = reinterpret_cast<const byte*>( commandString );
	int index = 0;
	// xor the client challenge with the netchan sequence number
	byte key = client->challenge ^ client->netchan.outgoingSequence;
	for ( int i = Q3SV_ENCODE_START; i < msg->cursize; i++ ) {
		// modify the key with the last received and with this message acknowledged client command
		if ( !string[ index ] ) {
			index = 0;
		}
		if ( string[ index ] > 127 || string[ index ] == '%' ) {
			key ^= '.' << ( i & 1 );
		} else {
			key ^= string[ index ] << ( i & 1 );
		}
		index++;
		// encode the data with this key
		*( msg->_data + i ) = *( msg->_data + i ) ^ key;
	}
}

// first 12 bytes of the data are always:
//	long serverId;
//	long messageAcknowledge;
//	long reliableAcknowledge;
static void SVT3_Netchan_Decode( client_t* client, QMsg* msg ) {
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

	const byte* string = reinterpret_cast<byte*>( client->q3_reliableCommands[ reliableAcknowledge &
																			   ( ( GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF ) - 1 ) ] );
	int index = 0;

	byte key = client->challenge ^ serverId ^ messageAcknowledge;
	for ( int i = msg->readcount + Q3SV_DECODE_START; i < msg->cursize; i++ ) {
		// modify the key with the last sent and acknowledged server command
		if ( !string[ index ] ) {
			index = 0;
		}
		if ( string[ index ] > 127 || string[ index ] == '%' ) {
			key ^= '.' << ( i & 1 );
		} else {
			key ^= string[ index ] << ( i & 1 );
		}
		index++;
		// decode the data with this key
		*( msg->_data + i ) = *( msg->_data + i ) ^ key;
	}
}

void SVT3_Netchan_TransmitNextFragment( client_t* client ) {
	Netchan_TransmitNextFragment( &client->netchan );
	if ( GGameType & GAME_WolfSP ) {
		return;
	}
	while ( !client->netchan.unsentFragments && client->q3_netchan_start_queue ) {
		// make sure the netchan queue has been properly initialized (you never know)
		if ( !( GGameType & GAME_ET ) && !client->q3_netchan_end_queue ) {
			common->Error( "netchan queue is not properly initialized in SVT3_Netchan_TransmitNextFragment\n" );
		}
		// the last fragment was transmitted, check wether we have queued messages
		common->DPrintf( "#462 Netchan_TransmitNextFragment: popping a queued message for transmit\n" );
		netchan_buffer_t* netbuf = client->q3_netchan_start_queue;

		// pop from queue
		client->q3_netchan_start_queue = netbuf->next;
		if ( !client->q3_netchan_start_queue ) {
			common->DPrintf( "#462 Netchan_TransmitNextFragment: emptied queue\n" );
			if ( GGameType & GAME_ET ) {
				client->et_netchan_end_queue = NULL;
			} else {
				client->q3_netchan_end_queue = &client->q3_netchan_start_queue;
			}
		} else {
			common->DPrintf( "#462 Netchan_TransmitNextFragment: remaining queued message\n" );
		}

		if ( !( GGameType & GAME_ET ) || !SVET_GameIsSinglePlayer() ) {
			SVT3_Netchan_Encode( client, &netbuf->msg, GGameType & GAME_ET ? netbuf->lastClientCommandString : client->q3_lastClientCommandString );
		}
		Netchan_Transmit( &client->netchan, netbuf->msg.cursize, netbuf->msg._data );

		Mem_Free( netbuf );
		if ( !( GGameType & GAME_ET ) ) {
			break;
		}
	}
}

static void SVET_WriteBinaryMessage( QMsg* msg, client_t* cl ) {
	if ( !cl->et_binaryMessageLength ) {
		return;
	}

	msg->Uncompressed();

	if ( ( msg->cursize + cl->et_binaryMessageLength ) >= msg->maxsize ) {
		cl->et_binaryMessageOverflowed = true;
		return;
	}

	msg->WriteData( cl->et_binaryMessage, cl->et_binaryMessageLength );
	cl->et_binaryMessageLength = 0;
	cl->et_binaryMessageOverflowed = false;
}

//	if there are some unsent fragments (which may happen if the snapshots
// and the gamestate are fragmenting, and collide on send for instance)
// then buffer them and make sure they get sent in correct order
void SVT3_Netchan_Transmit( client_t* client, QMsg* msg ) {
	msg->WriteByte( q3svc_EOF );
	if ( GGameType & GAME_ET ) {
		SVET_WriteBinaryMessage( msg, client );
	}

	if ( !( GGameType & GAME_WolfSP ) && client->netchan.unsentFragments ) {
		netchan_buffer_t* netbuf;
		common->DPrintf( "#462 SVT3_Netchan_Transmit: unsent fragments, stacked\n" );
		netbuf = ( netchan_buffer_t* )Mem_Alloc( sizeof ( netchan_buffer_t ) );

		// store the msg, we can't store it encoded, as the encoding depends on stuff we still have to finish sending
		netbuf->msg.Copy( netbuf->msgBuffer, sizeof ( netbuf->msgBuffer ), *msg );
		netbuf->next = NULL;
		if ( GGameType & GAME_ET ) {
			// copy the command, since the command number used for encryption is
			// already compressed in the buffer, and receiving a new command would
			// otherwise lose the proper encryption key
			String::Cpy( netbuf->lastClientCommandString, client->q3_lastClientCommandString );
		}

		// insert it in the queue, the message will be encoded and sent later
		if ( GGameType & GAME_ET ) {
			if ( !client->q3_netchan_start_queue ) {
				client->q3_netchan_start_queue = netbuf;
			} else {
				client->et_netchan_end_queue->next = netbuf;
			}
			client->et_netchan_end_queue = netbuf;
		} else {
			*client->q3_netchan_end_queue = netbuf;
			client->q3_netchan_end_queue = &( *client->q3_netchan_end_queue )->next;
		}

		// emit the next fragment of the current message for now
		Netchan_TransmitNextFragment( &client->netchan );
	} else {
		if ( !( GGameType & GAME_WolfSP ) && ( !( GGameType & GAME_ET ) || !SVET_GameIsSinglePlayer() ) ) {
			SVT3_Netchan_Encode( client, msg, client->q3_lastClientCommandString );
		}
		Netchan_Transmit( &client->netchan, msg->cursize, msg->_data );
	}
}

bool SVT3_Netchan_Process( client_t* client, QMsg* msg ) {
	int ret = Netchan_Process( &client->netchan, msg );
	if ( !ret ) {
		return false;
	}
	if ( !( GGameType & GAME_WolfSP ) && ( !( GGameType & GAME_ET ) || !SVET_GameIsSinglePlayer() ) ) {
		SVT3_Netchan_Decode( client, msg );
	}
	return true;
}
