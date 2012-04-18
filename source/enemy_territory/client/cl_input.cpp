/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

int old_com_frameTime;

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

extern kbutton_t in_left;
extern kbutton_t in_right;
extern kbutton_t in_forward;
extern kbutton_t in_back;
extern kbutton_t in_lookup;
extern kbutton_t in_lookdown;
extern kbutton_t in_moveleft;
extern kbutton_t in_moveright;
extern kbutton_t in_strafe;
extern kbutton_t in_speed;
extern kbutton_t in_up;
extern kbutton_t in_down;
extern kbutton_t in_buttons[16];
extern bool in_mlooking;

// Arnout: doubleTap button mapping
static kbutton_t* dtmapping[] = {
	NULL,                 // ETDT_NONE
	&in_moveleft,        // ETDT_MOVELEFT
	&in_moveright,       // ETDT_MOVERIGHT
	&in_forward,         // ETDT_FORWARD
	&in_back,            // ETDT_BACK
	&in_buttons[12],       // ETDT_LEANLEFT
	&in_buttons[13],       // ETDT_LEANRIGHT
	&in_up               // ETDT_UP
};

void IN_Notebook( void ) {
	//if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
	//VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NOTEBOOK);	// startup notebook
	//}
}

void IN_Help( void ) {
	if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_HELP );        // startup help system
	}
}

void IN_CenterViewWMP()
{
}

//==========================================================================

Cvar  *cl_recoilPitch;

Cvar  *cl_bypassMouseInput;       // NERVE - SMF

Cvar  *cl_doubletapdelay;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles( void ) {
	float speed;

	if ( in_speed.active ) {
		speed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		speed = 0.001 * cls.frametime;
	}

	if ( !in_strafe.active ) {
		cl.viewangles[YAW] -= speed * cl_yawspeed->value * CL_KeyState( &in_right );
		cl.viewangles[YAW] += speed * cl_yawspeed->value * CL_KeyState( &in_left );
	}

	cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * CL_KeyState( &in_lookup );
	cl.viewangles[PITCH] += speed * cl_pitchspeed->value * CL_KeyState( &in_lookdown );
}

/*
================
CL_KeyMove

Sets the etusercmd_t based on key states
================
*/
void CL_KeyMove( etusercmd_t *cmd ) {
	int movespeed;
	int forward, side, up;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 127;
		cmd->buttons &= ~Q3BUTTON_WALKING;
	} else {
		cmd->buttons |= Q3BUTTON_WALKING;
		movespeed = 64;
	}

	forward = 0;
	side = 0;
	up = 0;
	if ( in_strafe.active ) {
		side += movespeed * CL_KeyState( &in_right );
		side -= movespeed * CL_KeyState( &in_left );
	}

	side += movespeed * CL_KeyState( &in_moveright );
	side -= movespeed * CL_KeyState( &in_moveleft );

//----(SA)	added
	if ( cmd->buttons & BUTTON_ACTIVATE ) {
		if ( side > 0 ) {
			cmd->wbuttons |= WBUTTON_LEANRIGHT;
		} else if ( side < 0 ) {
			cmd->wbuttons |= WBUTTON_LEANLEFT;
		}

		side = 0;   // disallow the strafe when holding 'activate'
	}
//----(SA)	end

	up += movespeed * CL_KeyState( &in_up );
	up -= movespeed * CL_KeyState( &in_down );

	forward += movespeed * CL_KeyState( &in_forward );
	forward -= movespeed * CL_KeyState( &in_back );

	// fretn - moved this to bg_pmove.c
	//if (!(cl.et_snap.ps.persistant[PERS_HWEAPON_USE]))
	//{
	cmd->forwardmove = ClampChar( forward );
	cmd->rightmove = ClampChar( side );
	cmd->upmove = ClampChar( up );
	//}

	// Arnout: double tap
	cmd->doubleTap = ETDT_NONE; // reset
	if ( com_frameTime - cl.et_doubleTap.lastdoubleTap > cl_doubletapdelay->integer + 150 + cls.frametime ) {   // double tap only once every 500 msecs (add
																											 // frametime for low(-ish) fps situations)
		int i;
		qboolean key_down;

		for ( i = 1; i < ETDT_NUM; i++ ) {
			key_down = dtmapping[i]->active || dtmapping[i]->wasPressed;

			if ( key_down && !cl.et_doubleTap.pressedTime[i] ) {
				cl.et_doubleTap.pressedTime[i] = com_frameTime;
			} else if ( !key_down && !cl.et_doubleTap.releasedTime[i]
						&& ( com_frameTime - cl.et_doubleTap.pressedTime[i] ) < ( cl_doubletapdelay->integer + cls.frametime ) ) {
				cl.et_doubleTap.releasedTime[i] = com_frameTime;
			} else if ( key_down && ( com_frameTime - cl.et_doubleTap.pressedTime[i] ) < ( cl_doubletapdelay->integer + cls.frametime )
						&& ( com_frameTime - cl.et_doubleTap.releasedTime[i] ) < ( cl_doubletapdelay->integer + cls.frametime ) ) {
				cl.et_doubleTap.pressedTime[i] = cl.et_doubleTap.releasedTime[i] = 0;
				cmd->doubleTap = i;
				cl.et_doubleTap.lastdoubleTap = com_frameTime;
			} else if ( !key_down && ( cl.et_doubleTap.pressedTime[i] || cl.et_doubleTap.releasedTime[i] ) ) {
				if ( com_frameTime - cl.et_doubleTap.pressedTime[i] >= ( cl_doubletapdelay->integer + cls.frametime ) ) {
					cl.et_doubleTap.pressedTime[i] = cl.et_doubleTap.releasedTime[i] = 0;
				}
			}
		}
	}
}

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent( int dx, int dy, int time ) {
	if ( in_keyCatchers & KEYCATCH_UI ) {

		// NERVE - SMF - if we just want to pass it along to game
		if ( cl_bypassMouseInput->integer == 1 ) {
			cl.mouseDx[cl.mouseIndex] += dx;
			cl.mouseDy[cl.mouseIndex] += dy;
		} else {
			VM_Call( uivm, UI_MOUSE_EVENT, dx, dy );
		}

	} else if ( in_keyCatchers & KEYCATCH_CGAME ) {
		if ( cl_bypassMouseInput->integer == 1 ) {
			cl.mouseDx[cl.mouseIndex] += dx;
			cl.mouseDy[cl.mouseIndex] += dy;
		} else {
			VM_Call( cgvm, CG_MOUSE_EVENT, dx, dy );
		}
	} else {
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
=================
CL_JoystickMove
=================
*/
void CL_JoystickMove( etusercmd_t *cmd ) {
	int movespeed;
	float anglespeed;

	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 2;
	} else {
		movespeed = 1;
		cmd->buttons |= Q3BUTTON_WALKING;
	}

	if ( in_speed.active ) {
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		anglespeed = 0.001 * cls.frametime;
	}

#ifdef __MACOS__
	cmd->rightmove = ClampChar( cmd->rightmove + cl.joystickAxis[AXIS_SIDE] );
#else
	if ( !in_strafe.active ) {
		cl.viewangles[YAW] += anglespeed * cl_yawspeed->value * cl.joystickAxis[AXIS_SIDE];
	} else {
		cmd->rightmove = ClampChar( cmd->rightmove + cl.joystickAxis[AXIS_SIDE] );
	}
#endif
	if ( in_mlooking ) {
		cl.viewangles[PITCH] += anglespeed * cl_pitchspeed->value * cl.joystickAxis[AXIS_FORWARD];
	} else {
		cmd->forwardmove = ClampChar( cmd->forwardmove + cl.joystickAxis[AXIS_FORWARD] );
	}

	cmd->upmove = ClampChar( cmd->upmove + cl.joystickAxis[AXIS_UP] );
}

/*
=================
CL_MouseMove
=================
*/
void CL_MouseMove( etusercmd_t *cmd ) {
	float mx, my;
	float accelSensitivity;
	float rate;

	// allow mouse smoothing
	if ( m_filter->integer ) {
		mx = ( cl.mouseDx[0] + cl.mouseDx[1] ) * 0.5;
		my = ( cl.mouseDy[0] + cl.mouseDy[1] ) * 0.5;
	} else {
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}
	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	rate = sqrt( mx * mx + my * my ) / (float)frame_msec;
	accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;

	// scale by FOV
	accelSensitivity *= cl.q3_cgameSensitivity;

/*	NERVE - SMF - this has moved to CG_CalcFov to fix zoomed-in/out transition movement bug
	if ( cl.et_snap.ps.stats[STAT_ZOOMED_VIEW] ) {
		if(cl.et_snap.ps.weapon == WP_SNIPERRIFLE) {
			accelSensitivity *= 0.1;
		}
		else if(cl.et_snap.ps.weapon == WP_SNOOPERSCOPE) {
			accelSensitivity *= 0.2;
		}
	}
*/
	if ( rate && cl_showMouseRate->integer ) {
		Com_Printf( "%f : %f\n", rate, accelSensitivity );
	}

// Ridah, experimenting with a slow tracking gun

	// Rafael - mg42
	if ( cl.et_snap.ps.persistant[PERS_HWEAPON_USE] ) {
		mx *= 2.5; //(accelSensitivity * 0.1);
		my *= 2; //(accelSensitivity * 0.075);
	} else
	{
		mx *= accelSensitivity;
		my *= accelSensitivity;
	}

	if ( !mx && !my ) {
		return;
	}

	// add mouse X/Y movement to cmd
	if ( in_strafe.active ) {
		cmd->rightmove = ClampChar( cmd->rightmove + m_side->value * mx );
	} else {
		cl.viewangles[YAW] -= m_yaw->value * mx;
	}

	if ( ( in_mlooking || cl_freelook->integer ) && !in_strafe.active ) {
		cl.viewangles[PITCH] += m_pitch->value * my;
	} else {
		cmd->forwardmove = ClampChar( cmd->forwardmove - m_forward->value * my );
	}
}


/*
==============
CL_CmdButtons
==============
*/
void CL_CmdButtons( etusercmd_t *cmd ) {
	int i;

	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//
	for ( i = 0 ; i < 7 ; i++ ) {
		if ( in_buttons[i].active || in_buttons[i].wasPressed ) {
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = qfalse;
	}

	for ( i = 0 ; i < 8 ; i++ ) {     // Arnout: this was i < 7, but there are 8 wbuttons
		if ( in_buttons[8 + i].active || in_buttons[8 + i].wasPressed ) {
			cmd->wbuttons |= 1 << i;
		}
		in_buttons[8 + i].wasPressed = qfalse;
	}

	if ( in_keyCatchers && !cl_bypassMouseInput->integer ) {
		cmd->buttons |= Q3BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	if ( anykeydown && ( !in_keyCatchers || cl_bypassMouseInput->integer ) ) {
		cmd->buttons |= ETBUTTON_ANY;
	}

	// Arnout: clear 'waspressed' from double tap buttons
	for ( i = 1; i < ETDT_NUM; i++ ) {
		dtmapping[i]->wasPressed = qfalse;
	}
}


/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove( etusercmd_t *cmd ) {
	int i;

	// copy the state that the cgame is currently sending
	cmd->weapon = cl.q3_cgameUserCmdValue;

	cmd->flags = cl.et_cgameFlags;

	cmd->identClient = cl.wm_cgameMpIdentClient;   // NERVE - SMF

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	for ( i = 0 ; i < 3 ; i++ ) {
		cmd->angles[i] = ANGLE2SHORT( cl.viewangles[i] );
	}
}


/*
=================
CL_CreateCmd
=================
*/
etusercmd_t CL_CreateCmd( void ) {
	etusercmd_t cmd;
	vec3_t oldAngles;
	float recoilAdd;

	VectorCopy( cl.viewangles, oldAngles );

	// keyboard angle adjustment
	CL_AdjustAngles();

	memset( &cmd, 0, sizeof( cmd ) );

	CL_CmdButtons( &cmd );

	// get basic movement from keyboard
	CL_KeyMove( &cmd );

	// get basic movement from mouse
	CL_MouseMove( &cmd );

	// get basic movement from joystick
	CL_JoystickMove( &cmd );

	// check to make sure the angles haven't wrapped
	if ( cl.viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
	} else if ( oldAngles[PITCH] - cl.viewangles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
	}

	// RF, set the kickAngles so aiming is effected
	recoilAdd = cl_recoilPitch->value;
	if ( Q_fabs( cl.viewangles[PITCH] + recoilAdd ) < 40 ) {
		cl.viewangles[PITCH] += recoilAdd;
	}
	// the recoilPitch has been used, so clear it out
	cl_recoilPitch->value = 0;

	// store out the final values
	CL_FinishMove( &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( abs( cl.viewangles[YAW] - oldAngles[YAW] ), 0 );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( abs( cl.viewangles[PITCH] - oldAngles[PITCH] ), 0 );
		}
	}

	return cmd;
}


/*
=================
CL_CreateNewCommands

Create a new etusercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands( void ) {
	etusercmd_t   *cmd;
	int cmdNum;

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
	cl.et_cmds[cmdNum] = CL_CreateCmd();
	cmd = &cl.et_cmds[cmdNum];
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
	int oldPacketNum;
	int delta;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ( *cls.et_downloadTempName &&
		 cls.realtime - clc.q3_lastPacketSentTime < 50 ) {
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if ( cls.state != CA_ACTIVE &&
		 cls.state != CA_PRIMED &&
		 !*cls.et_downloadTempName &&
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
	} else if ( cl_maxpackets->integer > 100 ) {
		Cvar_Set( "cl_maxpackets", "100" );
	}
	oldPacketNum = ( clc.netchan.outgoingSequence - 1 ) & PACKET_MASK_Q3;
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
	QMsg buf;
	byte data[MAX_MSGLEN_WOLF];
	int i, j;
	etusercmd_t   *cmd, *oldcmd;
	etusercmd_t nullcmd;
	int packetNum;
	int oldPacketNum;
	int count, key;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return;
	}

	memset( &nullcmd, 0, sizeof( nullcmd ) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof( data ) );

	buf.Bitstream();
	// write the current serverId so the server
	// can tell if this is from the current gameState
	buf.WriteLong( cl.q3_serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	buf.WriteLong( clc.q3_serverMessageSequence );

	// write the last reliable message we received
	buf.WriteLong( clc.q3_serverCommandSequence );

	// write any unacknowledged clientCommands
	// NOTE TTimo: if you verbose this, you will see that there are quite a few duplicates
	// typically several unacknowledged cp or userinfo commands stacked up
	for ( i = clc.q3_reliableAcknowledge + 1 ; i <= clc.q3_reliableSequence ; i++ ) {
		buf.WriteByte( q3clc_clientCommand );
		buf.WriteLong( i );
		buf.WriteString( clc.q3_reliableCommands[ i & ( MAX_RELIABLE_COMMANDS_ET - 1 ) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = ( clc.netchan.outgoingSequence - 1 - cl_packetdup->integer ) & PACKET_MASK_Q3;
	count = cl.q3_cmdNumber - cl.q3_outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf( "MAX_PACKET_USERCMDS\n" );
	}
	if ( count >= 1 ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl.et_snap.valid || clc.q3_demowaiting
			 || clc.q3_serverMessageSequence != cl.et_snap.messageNum ) {
			buf.WriteByte( q3clc_moveNoDelta );
		} else {
			buf.WriteByte( q3clc_move );
		}

		// write the command count
		buf.WriteByte( count );

		// use the checksum feed in the key
		key = clc.q3_checksumFeed;
		// also use the message acknowledge
		key ^= clc.q3_serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey( clc.q3_serverCommands[ clc.q3_serverCommandSequence & ( MAX_RELIABLE_COMMANDS_ET - 1 ) ], 32 );

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = ( cl.q3_cmdNumber - count + i + 1 ) & CMD_MASK_Q3;
			cmd = &cl.et_cmds[j];
			MSG_WriteDeltaUsercmdKey( &buf, key, oldcmd, cmd );
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
	CL_Netchan_Transmit( &clc.netchan, &buf );

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	// TTimo: this causes a packet burst, which is bad karma for winsock
	// added a WARNING message, we'll see if there are legit situations where this happens
	while ( clc.netchan.unsentFragments ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "WARNING: unsent fragments (not supposed to happen!)\n" );
		}
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

	//Cmd_AddCommand ("notebook",IN_Notebook);
	Cmd_AddCommand( "help",IN_Help );

	cl_nodelta = Cvar_Get( "cl_nodelta", "0", 0 );
	cl_debugMove = Cvar_Get( "cl_debugMove", "0", 0 );
}
