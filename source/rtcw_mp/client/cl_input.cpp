/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

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

void IN_CenterViewWMP(void)
{
	qboolean ok = true;
	if (cgvm)
	{
		ok = VM_Call(cgvm, CG_CHECKCENTERVIEW);
	}
	if (ok)
	{
		cl.viewangles[PITCH] = -SHORT2ANGLE(cl.wm_snap.ps.delta_angles[PITCH]);
	}
}

void IN_Notebook(void)
{
	//if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
	//VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NOTEBOOK);	// startup notebook
	//}
}

void IN_Help(void)
{
	if (cls.state == CA_ACTIVE && !clc.demoplaying)
	{
		VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_HELP);			// startup help system
	}
}


//==========================================================================

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent(int dx, int dy, int time)
{
	if (in_keyCatchers & KEYCATCH_UI)
	{

		// NERVE - SMF - if we just want to pass it along to game
		if (cl_bypassMouseInput->integer == 1)
		{
			cl.mouseDx[cl.mouseIndex] += dx;
			cl.mouseDy[cl.mouseIndex] += dy;
		}
		else
		{
			VM_Call(uivm, UI_MOUSE_EVENT, dx, dy);
		}

	}
	else if (in_keyCatchers & KEYCATCH_CGAME)
	{
		VM_Call(cgvm, CG_MOUSE_EVENT, dx, dy);
	}
	else
	{
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
=================
CL_CreateNewCommands

Create a new wmusercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands(void)
{
	// no need to create usercmds until we have a gamestate
	if (cls.state < CA_PRIMED)
	{
		return;
	}

	// generate a command for this frame
	cl.q3_cmdNumber++;
	int cmdNum = cl.q3_cmdNumber & CMD_MASK_Q3;
	in_usercmd_t inCmd = CL_CreateCmd();
	wmusercmd_t cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.buttons = inCmd.buttons & 0xff;
	cmd.wbuttons = inCmd.buttons >> 8;
	cmd.forwardmove = inCmd.forwardmove;
	cmd.rightmove = inCmd.sidemove;
	cmd.upmove = inCmd.upmove;
	cmd.wolfkick = inCmd.kick;
	cmd.weapon = inCmd.weapon;
	for (int i = 0; i < 3; i++)
	{
		cmd.angles[i] = inCmd.angles[i];
	}
	cmd.serverTime = inCmd.serverTime;
	cmd.holdable = inCmd.holdable;
	cmd.mpSetup = inCmd.mpSetup;
	cmd.identClient = inCmd.identClient;
	cl.wm_cmds[cmdNum] = cmd;
}

/*
=================
CL_ReadyToSendPacket

Returns false if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
qboolean CL_ReadyToSendPacket(void)
{
	int oldPacketNum;
	int delta;

	// don't send anything if playing back a demo
	if (clc.demoplaying || cls.state == CA_CINEMATIC)
	{
		return false;
	}

	// If we are downloading, we send no less than 50ms between packets
	if (*clc.downloadTempName &&
		cls.realtime - clc.q3_lastPacketSentTime < 50)
	{
		return false;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if (cls.state != CA_ACTIVE &&
		cls.state != CA_PRIMED &&
		!*clc.downloadTempName &&
		cls.realtime - clc.q3_lastPacketSentTime < 1000)
	{
		return false;
	}

	// send every frame for loopbacks
	if (clc.netchan.remoteAddress.type == NA_LOOPBACK)
	{
		return true;
	}

	// send every frame for LAN
	if (SOCK_IsLANAddress(clc.netchan.remoteAddress))
	{
		return true;
	}

	// check for exceeding cl_maxpackets
	if (cl_maxpackets->integer < 15)
	{
		Cvar_Set("cl_maxpackets", "15");
	}
	else if (cl_maxpackets->integer > 100)
	{
		Cvar_Set("cl_maxpackets", "100");
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK_Q3;
	delta = cls.realtime -  cl.q3_outPackets[oldPacketNum].p_realtime;
	if (delta < 1000 / cl_maxpackets->integer)
	{
		// the accumulated commands will go out in the next packet
		return false;
	}

	return true;
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
void CL_WritePacket(void)
{
	QMsg buf;
	byte data[MAX_MSGLEN_WOLF];
	int i, j;
	wmusercmd_t* cmd, * oldcmd;
	wmusercmd_t nullcmd;
	int packetNum;
	int oldPacketNum;
	int count, key;

	// don't send anything if playing back a demo
	if (clc.demoplaying || cls.state == CA_CINEMATIC)
	{
		return;
	}

	memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;

	buf.Init(data, sizeof(data));

	buf.Bitstream();
	// write the current serverId so the server
	// can tell if this is from the current gameState
	buf.WriteLong(cl.q3_serverId);

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	buf.WriteLong(clc.q3_serverMessageSequence);

	// write the last reliable message we received
	buf.WriteLong(clc.q3_serverCommandSequence);

	// write any unacknowledged clientCommands
	// NOTE TTimo: if you verbose this, you will see that there are quite a few duplicates
	// typically several unacknowledged cp or userinfo commands stacked up
	for (i = clc.q3_reliableAcknowledge + 1; i <= clc.q3_reliableSequence; i++)
	{
		buf.WriteByte(q3clc_clientCommand);
		buf.WriteLong(i);
		buf.WriteString(clc.q3_reliableCommands[i & (MAX_RELIABLE_COMMANDS_WOLF - 1)]);
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if (cl_packetdup->integer < 0)
	{
		Cvar_Set("cl_packetdup", "0");
	}
	else if (cl_packetdup->integer > 5)
	{
		Cvar_Set("cl_packetdup", "5");
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1 - cl_packetdup->integer) & PACKET_MASK_Q3;
	count = cl.q3_cmdNumber - cl.q3_outPackets[oldPacketNum].p_cmdNumber;
	if (count > MAX_PACKET_USERCMDS)
	{
		count = MAX_PACKET_USERCMDS;
		common->Printf("MAX_PACKET_USERCMDS\n");
	}
	if (count >= 1)
	{
		if (cl_showSend->integer)
		{
			common->Printf("(%i)", count);
		}

		// begin a client move command
		if (cl_nodelta->integer || !cl.wm_snap.valid || clc.q3_demowaiting ||
			clc.q3_serverMessageSequence != cl.wm_snap.messageNum)
		{
			buf.WriteByte(q3clc_moveNoDelta);
		}
		else
		{
			buf.WriteByte(q3clc_move);
		}

		// write the command count
		buf.WriteByte(count);

		// use the checksum feed in the key
		key = clc.q3_checksumFeed;
		// also use the message acknowledge
		key ^= clc.q3_serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(clc.q3_serverCommands[clc.q3_serverCommandSequence & (MAX_RELIABLE_COMMANDS_WOLF - 1)], 32);

		// write all the commands, including the predicted command
		for (i = 0; i < count; i++)
		{
			j = (cl.q3_cmdNumber - count + i + 1) & CMD_MASK_Q3;
			cmd = &cl.wm_cmds[j];
			MSGWM_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}
	}

	//
	// deliver the message
	//
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK_Q3;
	cl.q3_outPackets[packetNum].p_realtime = cls.realtime;
	cl.q3_outPackets[packetNum].p_serverTime = oldcmd->serverTime;
	cl.q3_outPackets[packetNum].p_cmdNumber = cl.q3_cmdNumber;
	clc.q3_lastPacketSentTime = cls.realtime;

	if (cl_showSend->integer)
	{
		common->Printf("%i ", buf.cursize);
	}
	CL_Netchan_Transmit(&clc.netchan, &buf);

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	// TTimo: this causes a packet burst, which is bad karma for winsock
	// added a WARNING message, we'll see if there are legit situations where this happens
	while (clc.netchan.unsentFragments)
	{
		if (cl_showSend->integer)
		{
			common->Printf("WARNING: unsent fragments (not supposed to happen!)\n");
		}
		CL_Netchan_TransmitNextFragment(&clc.netchan);
	}
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd(void)
{
	// don't send any message if not connected
	if (cls.state < CA_CONNECTED)
	{
		return;
	}

	// don't send commands if paused
	if (com_sv_running->integer && sv_paused->integer && cl_paused->integer)
	{
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if (!CL_ReadyToSendPacket())
	{
		if (cl_showSend->integer)
		{
			common->Printf(". ");
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
void CL_InitInput(void)
{
	CL_InitInputCommon();

	//Cmd_AddCommand ("notebook",IN_Notebook);
	Cmd_AddCommand("help",IN_Help);

	cl_nodelta = Cvar_Get("cl_nodelta", "0", 0);
}
