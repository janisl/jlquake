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
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"
#include "../../common/message_utils.h"
#include "../public.h"

//	Called when the player is totally leaving the server, either willingly
// or unwillingly.  This is NOT called if the entire server is quiting
// or crashing.
void SVQ2_DropClient( client_t* drop ) {
	// add the disconnect
	drop->netchan.message.WriteByte( q2svc_disconnect );

	if ( drop->state == CS_ACTIVE ) {
		// call the prog function for removing a client
		// this will remove the body, among other things
		ge->ClientDisconnect( drop->q2_edict );
	}

	if ( drop->q2_downloadData ) {
		FS_FreeFile( drop->q2_downloadData );
		drop->q2_downloadData = NULL;
	}

	drop->state = CS_ZOMBIE;		// become free in a few seconds
	drop->name[ 0 ] = 0;
}

static void SVQ2_BeginDemoserver() {
	char name[ MAX_OSPATH ];
	String::Sprintf( name, sizeof ( name ), "demos/%s", sv.name );
	FS_FOpenFileRead( name, &sv.q2_demofile, true );
	if ( !sv.q2_demofile ) {
		common->Error( "Couldn't open %s\n", name );
	}
}

//	Sends the first message from the server to a connected client.
// This will be sent on the initial connection and upon each server load.
static void SVQ2_New_f( client_t* client ) {
	common->DPrintf( "New() from %s\n", client->name );

	if ( client->state != CS_CONNECTED ) {
		common->Printf( "New not valid -- already spawned\n" );
		return;
	}

	// demo servers just dump the file message
	if ( sv.state == SS_DEMO ) {
		SVQ2_BeginDemoserver();
		return;
	}

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	const char* gamedir = Cvar_VariableString( "gamedir" );

	// send the serverdata
	client->netchan.message.WriteByte( q2svc_serverdata );
	client->netchan.message.WriteLong( Q2PROTOCOL_VERSION );
	client->netchan.message.WriteLong( svs.spawncount );
	client->netchan.message.WriteByte( sv.q2_attractloop );
	client->netchan.message.WriteString2( gamedir );

	int playernum;
	if ( sv.state == SS_CINEMATIC || sv.state == SS_PIC ) {
		playernum = -1;
	} else {
		playernum = client - svs.clients;
	}
	client->netchan.message.WriteShort( playernum );

	// send full levelname
	client->netchan.message.WriteString2( sv.q2_configstrings[ Q2CS_NAME ] );

	//
	// game server
	//
	if ( sv.state == SS_GAME ) {
		// set up the entity for the client
		q2edict_t* ent = Q2_EDICT_NUM( playernum + 1 );
		ent->s.number = playernum + 1;
		client->q2_edict = ent;
		Com_Memset( &client->q2_lastUsercmd, 0, sizeof ( client->q2_lastUsercmd ) );

		// begin fetching configstrings
		client->netchan.message.WriteByte( q2svc_stufftext );
		client->netchan.message.WriteString2( va( "cmd configstrings %i 0\n",svs.spawncount ) );
	}

}

static void SVQ2_Configstrings_f( client_t* client ) {
	common->DPrintf( "Configstrings() from %s\n", client->name );

	if ( client->state != CS_CONNECTED ) {
		common->Printf( "configstrings not valid -- already spawned\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( String::Atoi( Cmd_Argv( 1 ) ) != svs.spawncount ) {
		common->Printf( "SV_Configstrings_f from different level\n" );
		SVQ2_New_f( client );
		return;
	}

	int start = String::Atoi( Cmd_Argv( 2 ) );

	// write a packet full of data

	while ( client->netchan.message.cursize < MAX_MSGLEN_Q2 / 2 &&
			start < MAX_CONFIGSTRINGS_Q2 ) {
		if ( sv.q2_configstrings[ start ][ 0 ] ) {
			client->netchan.message.WriteByte( q2svc_configstring );
			client->netchan.message.WriteShort( start );
			client->netchan.message.WriteString2( sv.q2_configstrings[ start ] );
		}
		start++;
	}

	// send next command

	if ( start == MAX_CONFIGSTRINGS_Q2 ) {
		client->netchan.message.WriteByte( q2svc_stufftext );
		client->netchan.message.WriteString2( va( "cmd baselines %i 0\n",svs.spawncount ) );
	} else {
		client->netchan.message.WriteByte( q2svc_stufftext );
		client->netchan.message.WriteString2( va( "cmd configstrings %i %i\n",svs.spawncount, start ) );
	}
}

static void SVQ2_Baselines_f( client_t* client ) {
	common->DPrintf( "Baselines() from %s\n", client->name );

	if ( client->state != CS_CONNECTED ) {
		common->Printf( "baselines not valid -- already spawned\n" );
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( String::Atoi( Cmd_Argv( 1 ) ) != svs.spawncount ) {
		common->Printf( "SV_Baselines_f from different level\n" );
		SVQ2_New_f( client );
		return;
	}

	int start = String::Atoi( Cmd_Argv( 2 ) );

	q2entity_state_t nullstate;
	Com_Memset( &nullstate, 0, sizeof ( nullstate ) );

	// write a packet full of data

	while ( client->netchan.message.cursize <  MAX_MSGLEN_Q2 / 2 &&
			start < MAX_EDICTS_Q2 ) {
		q2entity_state_t* base = &sv.q2_baselines[ start ];
		if ( base->modelindex || base->sound || base->effects ) {
			client->netchan.message.WriteByte( q2svc_spawnbaseline );
			MSGQ2_WriteDeltaEntity( &nullstate, base, &client->netchan.message, true, true );
		}
		start++;
	}

	// send next command

	if ( start == MAX_EDICTS_Q2 ) {
		client->netchan.message.WriteByte( q2svc_stufftext );
		client->netchan.message.WriteString2( va( "precache %i\n", svs.spawncount ) );
	} else {
		client->netchan.message.WriteByte( q2svc_stufftext );
		client->netchan.message.WriteString2( va( "cmd baselines %i %i\n",svs.spawncount, start ) );
	}
}

static void SVQ2_Begin_f( client_t* client ) {
	common->DPrintf( "Begin() from %s\n", client->name );

	// handle the case of a level changing while a client was connecting
	if ( String::Atoi( Cmd_Argv( 1 ) ) != svs.spawncount ) {
		common->Printf( "SV_Begin_f from different level\n" );
		SVQ2_New_f( client );
		return;
	}

	client->state = CS_ACTIVE;

	// call the game begin function
	ge->ClientBegin( client->q2_edict );

	Cbuf_InsertFromDefer();
}

static void SVQ2_NextDownload_f( client_t* client ) {
	if ( !client->q2_downloadData ) {
		return;
	}

	int r = client->downloadSize - client->downloadCount;
	if ( r > 1024 ) {
		r = 1024;
	}

	client->netchan.message.WriteByte( q2svc_download );
	client->netchan.message.WriteShort( r );

	client->downloadCount += r;
	int size = client->downloadSize;
	if ( !size ) {
		size = 1;
	}
	int percent = client->downloadCount * 100 / size;
	client->netchan.message.WriteByte( percent );
	client->netchan.message.WriteData(
		client->q2_downloadData + client->downloadCount - r, r );

	if ( client->downloadCount != client->downloadSize ) {
		return;
	}

	FS_FreeFile( client->q2_downloadData );
	client->q2_downloadData = NULL;
}

static void SVQ2_BeginDownload_f( client_t* client ) {
	const char* name = Cmd_Argv( 1 );

	int offset = 0;
	if ( Cmd_Argc() > 2 ) {
		offset = String::Atoi( Cmd_Argv( 2 ) );	// downloaded offset

	}
	// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check
	if ( strstr( name, ".." ) || !allow_download->value
		// leading dot is no good
		 || *name == '.'
		// leading slash bad as well, must be in subdir
		 || *name == '/'
		// next up, skin check
		 || ( String::NCmp( name, "players/", 6 ) == 0 && !allow_download_players->value )
		// now models
		 || ( String::NCmp( name, "models/", 6 ) == 0 && !allow_download_models->value )
		// now sounds
		 || ( String::NCmp( name, "sound/", 6 ) == 0 && !allow_download_sounds->value )
		// now maps (note special case for maps, must not be in pak)
		 || ( String::NCmp( name, "maps/", 6 ) == 0 && !allow_download_maps->value )
		// MUST be in a subdirectory
		 || !strstr( name, "/" ) ) {// don't allow anything with .. path
		client->netchan.message.WriteByte( q2svc_download );
		client->netchan.message.WriteShort( -1 );
		client->netchan.message.WriteByte( 0 );
		return;
	}


	if ( client->q2_downloadData ) {
		FS_FreeFile( client->q2_downloadData );
	}

	client->downloadSize = FS_ReadFile( name, ( void** )&client->q2_downloadData );
	client->downloadCount = offset;

	if ( offset > client->downloadSize ) {
		client->downloadCount = client->downloadSize;
	}

	if ( !client->q2_downloadData
		// special check for maps, if it came from a pak file, don't allow
		// download  ZOID
		 || ( String::NCmp( name, "maps/", 5 ) == 0 && FS_FileIsInPAK( name, NULL ) == 1 ) ) {
		common->DPrintf( "Couldn't download %s to %s\n", name, client->name );
		if ( client->q2_downloadData ) {
			FS_FreeFile( client->q2_downloadData );
			client->q2_downloadData = NULL;
		}

		client->netchan.message.WriteByte( q2svc_download );
		client->netchan.message.WriteShort( -1 );
		client->netchan.message.WriteByte( 0 );
		return;
	}

	SVQ2_NextDownload_f( client );
	common->DPrintf( "Downloading %s to %s\n", name, client->name );
}

//	The client is going to disconnect, so remove the connection immediately
static void SVQ2_Disconnect_f( client_t* client ) {
	SVQ2_DropClient( client );
}

//	Dumps the serverinfo info string
static void SVQ2_ShowServerinfo_f( client_t* client ) {
	Info_Print( Cvar_InfoString( CVAR_SERVERINFO, MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
			MAX_INFO_VALUE_Q2, true, false ) );
}

void SVQ2_Nextserver() {
	//ZOID, SS_PIC can be nextserver'd in coop mode
	if ( sv.state == SS_GAME || ( sv.state == SS_PIC && !Cvar_VariableValue( "coop" ) ) ) {
		return;		// can't nextserver while playing a normal game

	}
	svs.spawncount++;	// make sure another doesn't sneak in
	const char* v = Cvar_VariableString( "nextserver" );
	if ( !v[ 0 ] ) {
		Cbuf_AddText( "killserver\n" );
	} else {
		Cbuf_AddText( const_cast<char*>( v ) );
		Cbuf_AddText( "\n" );
	}
	Cvar_SetLatched( "nextserver","" );
}

//	A cinematic has completed or been aborted by a client, so move
// to the next server,
static void SVQ2_Nextserver_f( client_t* client ) {
	if ( String::Atoi( Cmd_Argv( 1 ) ) != svs.spawncount ) {
		common->DPrintf( "Nextserver() from wrong level, from %s\n", client->name );
		return;		// leftover from last server
	}

	common->DPrintf( "Nextserver() from %s\n", client->name );

	SVQ2_Nextserver();
}

static void SVQ2_ClientThink( client_t* cl, q2usercmd_t* cmd ) {
	cl->q2_commandMsec -= cmd->msec;

	if ( cl->q2_commandMsec < 0 && svq2_enforcetime->value ) {
		common->DPrintf( "commandMsec underflow from %s\n", cl->name );
		return;
	}

	ge->ClientThink( cl->q2_edict, cmd );
}

//	Pull specific info from a newly changed userinfo string
// into a more C freindly form.
void SVQ2_UserinfoChanged( client_t* cl ) {
	// call prog code to allow overrides
	ge->ClientUserinfoChanged( cl->q2_edict, cl->userinfo );

	// name for C code
	String::NCpy( cl->name, Info_ValueForKey( cl->userinfo, "name" ), sizeof ( cl->name ) - 1 );
	// mask off high bit
	for ( int i = 0; i < ( int )sizeof ( cl->name ); i++ ) {
		cl->name[ i ] &= 127;
	}

	// rate command
	const char* val = Info_ValueForKey( cl->userinfo, "rate" );
	if ( String::Length( val ) ) {
		int i = String::Atoi( val );
		cl->rate = i;
		if ( cl->rate < 100 ) {
			cl->rate = 100;
		}
		if ( cl->rate > 15000 ) {
			cl->rate = 15000;
		}
	} else {
		cl->rate = 5000;
	}

	// msg command
	val = Info_ValueForKey( cl->userinfo, "msg" );
	if ( String::Length( val ) ) {
		cl->messagelevel = String::Atoi( val );
	}
}

static void SVQ2_ParseUserInfo( client_t* cl, QMsg& message ) {
	String::NCpy( cl->userinfo, message.ReadString2(), MAX_INFO_STRING_Q2 - 1 );
	SVQ2_UserinfoChanged( cl );
}

ucmd_t q2_ucmds[] =
{
	// auto issued
	{"new", SVQ2_New_f},
	{"configstrings", SVQ2_Configstrings_f},
	{"baselines", SVQ2_Baselines_f},
	{"begin", SVQ2_Begin_f},

	{"nextserver", SVQ2_Nextserver_f},

	{"disconnect", SVQ2_Disconnect_f},

	// issued by hand at client consoles
	{"info", SVQ2_ShowServerinfo_f},

	{"download", SVQ2_BeginDownload_f},
	{"nextdl", SVQ2_NextDownload_f},

	{NULL, NULL}
};

static bool SVQ2_ParseMove( client_t* cl, QMsg& message, bool& move_issued ) {
	if ( move_issued ) {
		// someone is trying to cheat...
		return false;
	}
	move_issued = true;

	int checksumIndex = message.readcount;
	int checksum = message.ReadByte();
	int lastframe = message.ReadLong();

	if ( lastframe != cl->q2_lastframe ) {
		cl->q2_lastframe = lastframe;
		if ( cl->q2_lastframe > 0 ) {
			cl->q2_frame_latency[ cl->q2_lastframe & ( LATENCY_COUNTS - 1 ) ] =
				svs.realtime - cl->q2_frames[ cl->q2_lastframe & UPDATE_MASK_Q2 ].senttime;
		}
	}

	q2usercmd_t nullcmd;
	Com_Memset( &nullcmd, 0, sizeof ( nullcmd ) );
	q2usercmd_t oldest, oldcmd, newcmd;
	MSGQ2_ReadDeltaUsercmd( &message, &nullcmd, &oldest );
	MSGQ2_ReadDeltaUsercmd( &message, &oldest, &oldcmd );
	MSGQ2_ReadDeltaUsercmd( &message, &oldcmd, &newcmd );

	if ( cl->state != CS_ACTIVE ) {
		cl->q2_lastframe = -1;
		return true;
	}

	// if the checksum fails, ignore the rest of the packet
	int calculatedChecksum = COMQ2_BlockSequenceCRCByte(
		message._data + checksumIndex + 1,
		message.readcount - checksumIndex - 1,
		cl->netchan.incomingSequence );

	if ( calculatedChecksum != checksum ) {
		common->DPrintf( "Failed command checksum for %s (%d != %d)/%d\n",
			cl->name, calculatedChecksum, checksum,
			cl->netchan.incomingSequence );
		return false;
	}

	if ( !sv_paused->value ) {
		int net_drop = cl->netchan.dropped;
		if ( net_drop < 20 ) {
			while ( net_drop > 2 ) {
				SVQ2_ClientThink( cl, &cl->q2_lastUsercmd );

				net_drop--;
			}
			if ( net_drop > 1 ) {
				SVQ2_ClientThink( cl, &oldest );
			}

			if ( net_drop > 0 ) {
				SVQ2_ClientThink( cl, &oldcmd );
			}

		}
		SVQ2_ClientThink( cl, &newcmd );
	}

	cl->q2_lastUsercmd = newcmd;
	return true;
}

static bool SVQ2_ParseStringCommand( client_t* cl, QMsg& message, int& stringCmdCount ) {
	enum { MAX_STRINGCMDS = 8 };

	const char* s = message.ReadString2();

	// malicious users may try using too many string commands
	if ( ++stringCmdCount < MAX_STRINGCMDS ) {
		SV_ExecuteClientCommand( cl, s, true, false );
	}

	if ( cl->state == CS_ZOMBIE ) {
		return false;	// disconnect command
	}
	return true;
}

//	The current net_message is parsed for the given client
void SVQ2_ExecuteClientMessage( client_t* cl, QMsg& message ) {
	// only allow one move command
	bool move_issued = false;
	int stringCmdCount = 0;

	while ( 1 ) {
		if ( message.readcount > message.cursize ) {
			common->Printf( "SV_ReadClientMessage: badread\n" );
			SVQ2_DropClient( cl );
			return;
		}

		int c = message.ReadByte();
		if ( c == -1 ) {
			break;
		}

		switch ( c ) {
		default:
			common->Printf( "SV_ReadClientMessage: unknown command char\n" );
			SVQ2_DropClient( cl );
			return;
		case q2clc_nop:
			break;
		case q2clc_userinfo:
			SVQ2_ParseUserInfo( cl, message );
			break;
		case q2clc_move:
			if ( !SVQ2_ParseMove( cl, message, move_issued ) ) {
				return;
			}
			break;
		case q2clc_stringcmd:
			if ( !SVQ2_ParseStringCommand( cl, message, stringCmdCount ) ) {
				return;
			}
			break;
		}
	}
}
