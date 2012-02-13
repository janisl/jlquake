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
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

int			old_com_frameTime;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/



//==========================================================================

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent( int dx, int dy, int time ) {
	if ( in_keyCatchers & KEYCATCH_UI ) {
		VM_Call( uivm, UI_MOUSE_EVENT, dx, dy );
	} else if (in_keyCatchers & KEYCATCH_CGAME) {
		VM_Call (cgvm, CG_MOUSE_EVENT, dx, dy);
	} else {
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove( q3usercmd_t *cmd ) {
	int		i;

	// copy the state that the cgame is currently sending
	cmd->weapon = cl.q3_cgameUserCmdValue;

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	for (i=0 ; i<3 ; i++) {
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
	}
}


/*
=================
CL_CreateCmd
=================
*/
q3usercmd_t CL_CreateCmd( void ) {
	q3usercmd_t	cmd;
	vec3_t		oldAngles;

	VectorCopy( cl.viewangles, oldAngles );

	Com_Memset( &cmd, 0, sizeof( cmd ) );

	in_usercmd_t inCmd = CL_CreateCmdCommon();
	cmd.forwardmove = ClampChar(inCmd.forwardmove);
	cmd.rightmove = ClampChar(inCmd.sidemove);
	cmd.upmove = ClampChar(inCmd.upmove);
	cmd.buttons = inCmd.buttons;

	// store out the final values
	CL_FinishMove( &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( abs(cl.viewangles[YAW] - oldAngles[YAW]), 0 );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( abs(cl.viewangles[PITCH] - oldAngles[PITCH]), 0 );
		}
	}

	return cmd;
}


/*
=================
CL_CreateNewCommands

Create a new q3usercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands( void ) {
	q3usercmd_t	*cmd;
	int			cmdNum;

	// no need to create usercmds until we have a gamestate
	if ( cls.state < CA_PRIMED ) {
		return;
	}

	frame_msec = com_frameTime - old_com_frameTime;

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if ( frame_msec > 200 ) {
		frame_msec = 200;
	}
	old_com_frameTime = com_frameTime;


	// generate a command for this frame
	cl.q3_cmdNumber++;
	cmdNum = cl.q3_cmdNumber & CMD_MASK_Q3;
	cl.q3_cmds[cmdNum] = CL_CreateCmd ();
	cmd = &cl.q3_cmds[cmdNum];
}

/*
=================
CL_ReadyToSendPacket

Returns qfalse if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
qboolean CL_ReadyToSendPacket( void ) {
	int		oldPacketNum;
	int		delta;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ( *clc.downloadTempName &&
		cls.realtime - clc.q3_lastPacketSentTime < 50 ) {
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if ( cls.state != CA_ACTIVE && 
		cls.state != CA_PRIMED && 
		!*clc.downloadTempName &&
		cls.realtime - clc.q3_lastPacketSentTime < 1000 ) {
		return qfalse;
	}

	// send every frame for loopbacks
	if ( clc.netchan.remoteAddress.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// send every frame for LAN
	if ( SOCK_IsLANAddress( clc.netchan.remoteAddress ) ) {
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if ( cl_maxpackets->integer < 15 ) {
		Cvar_Set( "cl_maxpackets", "15" );
	} else if ( cl_maxpackets->integer > 125 ) {
		Cvar_Set( "cl_maxpackets", "125" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK_Q3;
	delta = cls.realtime -  cl.q3_outPackets[ oldPacketNum ].p_realtime;
	if ( delta < 1000 / cl_maxpackets->integer ) {
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	q3clc_move or q3clc_moveNoDelta
1	command count
<count * usercmds>

===================
*/
void CL_WritePacket( void ) {
	QMsg		buf;
	byte		data[MAX_MSGLEN_Q3];
	int			i, j;
	q3usercmd_t	*cmd, *oldcmd;
	q3usercmd_t	nullcmd;
	int			packetNum;
	int			oldPacketNum;
	int			count, key;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return;
	}

	Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof(data) );

	buf.Bitstream();
	// write the current serverId so the server
	// can tell if this is from the current gameState
	buf.WriteLong(cl.q3_serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	buf.WriteLong(clc.q3_serverMessageSequence );

	// write the last reliable message we received
	buf.WriteLong(clc.q3_serverCommandSequence );

	// write any unacknowledged clientCommands
	for ( i = clc.q3_reliableAcknowledge + 1 ; i <= clc.q3_reliableSequence ; i++ ) {
		buf.WriteByte(q3clc_clientCommand );
		buf.WriteLong(i );
		buf.WriteString(clc.q3_reliableCommands[ i & (MAX_RELIABLE_COMMANDS_Q3-1) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1 - cl_packetdup->integer) & PACKET_MASK_Q3;
	count = cl.q3_cmdNumber - cl.q3_outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}
	if ( count >= 1 ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl.q3_snap.valid || clc.q3_demowaiting
			|| clc.q3_serverMessageSequence != cl.q3_snap.messageNum ) {
			buf.WriteByte(q3clc_moveNoDelta);
		} else {
			buf.WriteByte(q3clc_move);
		}

		// write the command count
		buf.WriteByte(count );

		// use the checksum feed in the key
		key = clc.q3_checksumFeed;
		// also use the message acknowledge
		key ^= clc.q3_serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(clc.q3_serverCommands[ clc.q3_serverCommandSequence & (MAX_RELIABLE_COMMANDS_Q3-1) ], 32);

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = (cl.q3_cmdNumber - count + i + 1) & CMD_MASK_Q3;
			cmd = &cl.q3_cmds[j];
			MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}
	}

	//
	// deliver the message
	//
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK_Q3;
	cl.q3_outPackets[ packetNum ].p_realtime = cls.realtime;
	cl.q3_outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.q3_outPackets[ packetNum ].p_cmdNumber = cl.q3_cmdNumber;
	clc.q3_lastPacketSentTime = cls.realtime;

	if ( cl_showSend->integer ) {
		Com_Printf( "%i ", buf.cursize );
	}

	CL_Netchan_Transmit (&clc.netchan, &buf);	

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	// TTimo: this causes a packet burst, which is bad karma for winsock
	// added a WARNING message, we'll see if there are legit situations where this happens
	while ( clc.netchan.unsentFragments ) {
		Com_DPrintf( "WARNING: #462 unsent fragments (not supposed to happen!)\n" );
		CL_Netchan_TransmitNextFragment( &clc.netchan );
	}
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd( void ) {
	// don't send any message if not connected
	if ( cls.state < CA_CONNECTED ) {
		return;
	}

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		return;
	}

	CL_WritePacket();
}

/*
============
CL_InitInput
============
*/
void CL_InitInput( void ) {
	CL_InitInputCommon();

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get ("cl_debugMove", "0", 0);
}
