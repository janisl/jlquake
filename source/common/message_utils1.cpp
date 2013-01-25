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

#include "message_utils.h"

void MSGQW_WriteDeltaUsercmd( QMsg* buf, qwusercmd_t* from, qwusercmd_t* cmd ) {
	//
	// send the movement message
	//
	int bits = 0;
	if ( cmd->angles[ 0 ] != from->angles[ 0 ] ) {
		bits |= QWCM_ANGLE1;
	}
	if ( cmd->angles[ 1 ] != from->angles[ 1 ] ) {
		bits |= QWCM_ANGLE2;
	}
	if ( cmd->angles[ 2 ] != from->angles[ 2 ] ) {
		bits |= QWCM_ANGLE3;
	}
	if ( cmd->forwardmove != from->forwardmove ) {
		bits |= QWCM_FORWARD;
	}
	if ( cmd->sidemove != from->sidemove ) {
		bits |= QWCM_SIDE;
	}
	if ( cmd->upmove != from->upmove ) {
		bits |= QWCM_UP;
	}
	if ( cmd->buttons != from->buttons ) {
		bits |= QWCM_BUTTONS;
	}
	if ( cmd->impulse != from->impulse ) {
		bits |= QWCM_IMPULSE;
	}

	buf->WriteByte( bits );

	if ( bits & QWCM_ANGLE1 ) {
		buf->WriteAngle16( cmd->angles[ 0 ] );
	}
	if ( bits & QWCM_ANGLE2 ) {
		buf->WriteAngle16( cmd->angles[ 1 ] );
	}
	if ( bits & QWCM_ANGLE3 ) {
		buf->WriteAngle16( cmd->angles[ 2 ] );
	}

	if ( bits & QWCM_FORWARD ) {
		buf->WriteShort( cmd->forwardmove );
	}
	if ( bits & QWCM_SIDE ) {
		buf->WriteShort( cmd->sidemove );
	}
	if ( bits & QWCM_UP ) {
		buf->WriteShort( cmd->upmove );
	}

	if ( bits & QWCM_BUTTONS ) {
		buf->WriteByte( cmd->buttons );
	}
	if ( bits & QWCM_IMPULSE ) {
		buf->WriteByte( cmd->impulse );
	}
	buf->WriteByte( cmd->msec );
}

void MSGQW_ReadDeltaUsercmd( QMsg* buf, qwusercmd_t* from, qwusercmd_t* move ) {
	Com_Memcpy( move, from, sizeof ( *move ) );

	int bits = buf->ReadByte();

// read current angles
	if ( bits & QWCM_ANGLE1 ) {
		move->angles[ 0 ] = buf->ReadAngle16();
	}
	if ( bits & QWCM_ANGLE2 ) {
		move->angles[ 1 ] = buf->ReadAngle16();
	}
	if ( bits & QWCM_ANGLE3 ) {
		move->angles[ 2 ] = buf->ReadAngle16();
	}

// read movement
	if ( bits & QWCM_FORWARD ) {
		move->forwardmove = buf->ReadShort();
	}
	if ( bits & QWCM_SIDE ) {
		move->sidemove = buf->ReadShort();
	}
	if ( bits & QWCM_UP ) {
		move->upmove = buf->ReadShort();
	}

// read buttons
	if ( bits & QWCM_BUTTONS ) {
		move->buttons = buf->ReadByte();
	}

	if ( bits & QWCM_IMPULSE ) {
		move->impulse = buf->ReadByte();
	}

// read time to run command
	move->msec = buf->ReadByte();
}

void MSGHW_WriteUsercmd( QMsg* buf, hwusercmd_t* cmd, bool long_msg ) {
	//
	// send the movement message
	//
	int bits = 0;
	if ( cmd->angles[ 0 ] ) {
		bits |= HWCM_ANGLE1;
	}
	if ( cmd->angles[ 2 ] ) {
		bits |= HWCM_ANGLE3;
	}
	if ( cmd->forwardmove ) {
		bits |= HWCM_FORWARD;
	}
	if ( cmd->sidemove ) {
		bits |= HWCM_SIDE;
	}
	if ( cmd->upmove ) {
		bits |= HWCM_UP;
	}
	if ( cmd->buttons ) {
		bits |= HWCM_BUTTONS;
	}
	if ( cmd->impulse ) {
		bits |= HWCM_IMPULSE;
	}
	if ( cmd->msec ) {
		bits |= HWCM_MSEC;
	}

	buf->WriteByte( bits );
	if ( long_msg ) {
		buf->WriteByte( cmd->light_level );
	}

	if ( bits & HWCM_ANGLE1 ) {
		buf->WriteAngle16( cmd->angles[ 0 ] );
	}
	buf->WriteAngle16( cmd->angles[ 1 ] );
	if ( bits & HWCM_ANGLE3 ) {
		buf->WriteAngle16( cmd->angles[ 2 ] );
	}

	if ( bits & HWCM_FORWARD ) {
		buf->WriteChar( ( int )( cmd->forwardmove * 0.25 ) );
	}
	if ( bits & HWCM_SIDE ) {
		buf->WriteChar( ( int )( cmd->sidemove * 0.25 ) );
	}
	if ( bits & HWCM_UP ) {
		buf->WriteChar( ( int )( cmd->upmove * 0.25 ) );
	}

	if ( bits & HWCM_BUTTONS ) {
		buf->WriteByte( cmd->buttons );
	}
	if ( bits & HWCM_IMPULSE ) {
		buf->WriteByte( cmd->impulse );
	}
	if ( bits & HWCM_MSEC ) {
		buf->WriteByte( cmd->msec );
	}
}

void MSGHW_ReadUsercmd( QMsg* sb, hwusercmd_t* move, bool long_msg ) {
	int bits;

	Com_Memset( move, 0, sizeof ( *move ) );

	bits = sb->ReadByte();
	if ( long_msg ) {
		move->light_level = sb->ReadByte();
	} else   {
		move->light_level = 0;
	}

	// read current angles
	if ( bits & HWCM_ANGLE1 ) {
		move->angles[ 0 ] = sb->ReadAngle16();
	} else   {
		move->angles[ 0 ] = 0;
	}
	move->angles[ 1 ] = sb->ReadAngle16();
	if ( bits & HWCM_ANGLE3 ) {
		move->angles[ 2 ] = sb->ReadAngle16();
	} else   {
		move->angles[ 2 ] = 0;
	}

	// read movement
	if ( bits & HWCM_FORWARD ) {
		move->forwardmove = sb->ReadChar() * 4;
	}
	if ( bits & HWCM_SIDE ) {
		move->sidemove = sb->ReadChar() * 4;
	}
	if ( bits & HWCM_UP ) {
		move->upmove = sb->ReadChar() * 4;
	}

	// read buttons
	if ( bits & HWCM_BUTTONS ) {
		move->buttons = sb->ReadByte();
	} else   {
		move->buttons = 0;
	}

	if ( bits & HWCM_IMPULSE ) {
		move->impulse = sb->ReadByte();
	} else   {
		move->impulse = 0;
	}

	// read time to run command
	if ( bits & HWCM_MSEC ) {
		move->msec = sb->ReadByte();
	} else   {
		move->msec = 0;
	}
}
