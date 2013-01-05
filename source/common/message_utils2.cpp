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
#include "Common.h"

void MSGQ2_WriteDeltaUsercmd(QMsg* buf, q2usercmd_t* from, q2usercmd_t* cmd)
{
	int bits = 0;
	if (cmd->angles[0] != from->angles[0])
	{
		bits |= Q2CM_ANGLE1;
	}
	if (cmd->angles[1] != from->angles[1])
	{
		bits |= Q2CM_ANGLE2;
	}
	if (cmd->angles[2] != from->angles[2])
	{
		bits |= Q2CM_ANGLE3;
	}
	if (cmd->forwardmove != from->forwardmove)
	{
		bits |= Q2CM_FORWARD;
	}
	if (cmd->sidemove != from->sidemove)
	{
		bits |= Q2CM_SIDE;
	}
	if (cmd->upmove != from->upmove)
	{
		bits |= Q2CM_UP;
	}
	if (cmd->buttons != from->buttons)
	{
		bits |= Q2CM_BUTTONS;
	}
	if (cmd->impulse != from->impulse)
	{
		bits |= Q2CM_IMPULSE;
	}

	buf->WriteByte(bits);

	if (bits & Q2CM_ANGLE1)
	{
		buf->WriteShort(cmd->angles[0]);
	}
	if (bits & Q2CM_ANGLE2)
	{
		buf->WriteShort(cmd->angles[1]);
	}
	if (bits & Q2CM_ANGLE3)
	{
		buf->WriteShort(cmd->angles[2]);
	}

	if (bits & Q2CM_FORWARD)
	{
		buf->WriteShort(cmd->forwardmove);
	}
	if (bits & Q2CM_SIDE)
	{
		buf->WriteShort(cmd->sidemove);
	}
	if (bits & Q2CM_UP)
	{
		buf->WriteShort(cmd->upmove);
	}

	if (bits & Q2CM_BUTTONS)
	{
		buf->WriteByte(cmd->buttons);
	}
	if (bits & Q2CM_IMPULSE)
	{
		buf->WriteByte(cmd->impulse);
	}

	buf->WriteByte(cmd->msec);
	buf->WriteByte(cmd->lightlevel);
}

//	Writes part of a packetentities message.
// Can delta from either a baseline or a previous packet_entity
void MSGQ2_WriteDeltaEntity(q2entity_state_t* from, q2entity_state_t* to, QMsg* msg, bool force, bool newentity)
{
	if (!to->number)
	{
		common->FatalError("Unset entity number");
	}
	if (to->number >= MAX_EDICTS_Q2)
	{
		common->FatalError("Entity number >= MAX_EDICTS_Q2");
	}

	// send an update
	int bits = 0;

	if (to->number >= 256)
	{
		bits |= Q2U_NUMBER16;		// number8 is implicit otherwise

	}
	if (to->origin[0] != from->origin[0])
	{
		bits |= Q2U_ORIGIN1;
	}
	if (to->origin[1] != from->origin[1])
	{
		bits |= Q2U_ORIGIN2;
	}
	if (to->origin[2] != from->origin[2])
	{
		bits |= Q2U_ORIGIN3;
	}

	if (to->angles[0] != from->angles[0])
	{
		bits |= Q2U_ANGLE1;
	}
	if (to->angles[1] != from->angles[1])
	{
		bits |= Q2U_ANGLE2;
	}
	if (to->angles[2] != from->angles[2])
	{
		bits |= Q2U_ANGLE3;
	}

	if (to->skinnum != from->skinnum)
	{
		if ((unsigned)to->skinnum < 256)
		{
			bits |= Q2U_SKIN8;
		}
		else if ((unsigned)to->skinnum < 0x10000)
		{
			bits |= Q2U_SKIN16;
		}
		else
		{
			bits |= (Q2U_SKIN8 | Q2U_SKIN16);
		}
	}

	if (to->frame != from->frame)
	{
		if (to->frame < 256)
		{
			bits |= Q2U_FRAME8;
		}
		else
		{
			bits |= Q2U_FRAME16;
		}
	}

	if (to->effects != from->effects)
	{
		if (to->effects < 256)
		{
			bits |= Q2U_EFFECTS8;
		}
		else if (to->effects < 0x8000)
		{
			bits |= Q2U_EFFECTS16;
		}
		else
		{
			bits |= Q2U_EFFECTS8 | Q2U_EFFECTS16;
		}
	}

	if (to->renderfx != from->renderfx)
	{
		if (to->renderfx < 256)
		{
			bits |= Q2U_RENDERFX8;
		}
		else if (to->renderfx < 0x8000)
		{
			bits |= Q2U_RENDERFX16;
		}
		else
		{
			bits |= Q2U_RENDERFX8 | Q2U_RENDERFX16;
		}
	}

	if (to->solid != from->solid)
	{
		bits |= Q2U_SOLID;
	}

	// event is not delta compressed, just 0 compressed
	if (to->event)
	{
		bits |= Q2U_EVENT;
	}

	if (to->modelindex != from->modelindex)
	{
		bits |= Q2U_MODEL;
	}
	if (to->modelindex2 != from->modelindex2)
	{
		bits |= Q2U_MODEL2;
	}
	if (to->modelindex3 != from->modelindex3)
	{
		bits |= Q2U_MODEL3;
	}
	if (to->modelindex4 != from->modelindex4)
	{
		bits |= Q2U_MODEL4;
	}

	if (to->sound != from->sound)
	{
		bits |= Q2U_SOUND;
	}

	if (newentity || (to->renderfx & Q2RF_BEAM))
	{
		bits |= Q2U_OLDORIGIN;
	}

	//
	// write the message
	//
	if (!bits && !force)
	{
		return;		// nothing to send!

	}
	//----------

	if (bits & 0xff000000)
	{
		bits |= Q2U_MOREBITS3 | Q2U_MOREBITS2 | Q2U_MOREBITS1;
	}
	else if (bits & 0x00ff0000)
	{
		bits |= Q2U_MOREBITS2 | Q2U_MOREBITS1;
	}
	else if (bits & 0x0000ff00)
	{
		bits |= Q2U_MOREBITS1;
	}

	msg->WriteByte(bits & 255);

	if (bits & 0xff000000)
	{
		msg->WriteByte((bits >> 8) & 255);
		msg->WriteByte((bits >> 16) & 255);
		msg->WriteByte((bits >> 24) & 255);
	}
	else if (bits & 0x00ff0000)
	{
		msg->WriteByte((bits >> 8) & 255);
		msg->WriteByte((bits >> 16) & 255);
	}
	else if (bits & 0x0000ff00)
	{
		msg->WriteByte((bits >> 8) & 255);
	}

	//----------

	if (bits & Q2U_NUMBER16)
	{
		msg->WriteShort(to->number);
	}
	else
	{
		msg->WriteByte(to->number);
	}

	if (bits & Q2U_MODEL)
	{
		msg->WriteByte(to->modelindex);
	}
	if (bits & Q2U_MODEL2)
	{
		msg->WriteByte(to->modelindex2);
	}
	if (bits & Q2U_MODEL3)
	{
		msg->WriteByte(to->modelindex3);
	}
	if (bits & Q2U_MODEL4)
	{
		msg->WriteByte(to->modelindex4);
	}

	if (bits & Q2U_FRAME8)
	{
		msg->WriteByte(to->frame);
	}
	if (bits & Q2U_FRAME16)
	{
		msg->WriteShort(to->frame);
	}

	if ((bits & Q2U_SKIN8) && (bits & Q2U_SKIN16))		//used for laser colors
	{
		msg->WriteLong(to->skinnum);
	}
	else if (bits & Q2U_SKIN8)
	{
		msg->WriteByte(to->skinnum);
	}
	else if (bits & Q2U_SKIN16)
	{
		msg->WriteShort(to->skinnum);
	}


	if ((bits & (Q2U_EFFECTS8 | Q2U_EFFECTS16)) == (Q2U_EFFECTS8 | Q2U_EFFECTS16))
	{
		msg->WriteLong(to->effects);
	}
	else if (bits & Q2U_EFFECTS8)
	{
		msg->WriteByte(to->effects);
	}
	else if (bits & Q2U_EFFECTS16)
	{
		msg->WriteShort(to->effects);
	}

	if ((bits & (Q2U_RENDERFX8 | Q2U_RENDERFX16)) == (Q2U_RENDERFX8 | Q2U_RENDERFX16))
	{
		msg->WriteLong(to->renderfx);
	}
	else if (bits & Q2U_RENDERFX8)
	{
		msg->WriteByte(to->renderfx);
	}
	else if (bits & Q2U_RENDERFX16)
	{
		msg->WriteShort(to->renderfx);
	}

	if (bits & Q2U_ORIGIN1)
	{
		msg->WriteCoord(to->origin[0]);
	}
	if (bits & Q2U_ORIGIN2)
	{
		msg->WriteCoord(to->origin[1]);
	}
	if (bits & Q2U_ORIGIN3)
	{
		msg->WriteCoord(to->origin[2]);
	}

	if (bits & Q2U_ANGLE1)
	{
		msg->WriteAngle(to->angles[0]);
	}
	if (bits & Q2U_ANGLE2)
	{
		msg->WriteAngle(to->angles[1]);
	}
	if (bits & Q2U_ANGLE3)
	{
		msg->WriteAngle(to->angles[2]);
	}

	if (bits & Q2U_OLDORIGIN)
	{
		msg->WriteCoord(to->old_origin[0]);
		msg->WriteCoord(to->old_origin[1]);
		msg->WriteCoord(to->old_origin[2]);
	}

	if (bits & Q2U_SOUND)
	{
		msg->WriteByte(to->sound);
	}
	if (bits & Q2U_EVENT)
	{
		msg->WriteByte(to->event);
	}
	if (bits & Q2U_SOLID)
	{
		msg->WriteShort(to->solid);
	}
}

void MSGQ2_ReadDeltaUsercmd(QMsg* msg_read, q2usercmd_t* from, q2usercmd_t* move)
{
	Com_Memcpy(move, from, sizeof(*move));

	int bits = msg_read->ReadByte();

	// read current angles
	if (bits & Q2CM_ANGLE1)
	{
		move->angles[0] = msg_read->ReadShort();
	}
	if (bits & Q2CM_ANGLE2)
	{
		move->angles[1] = msg_read->ReadShort();
	}
	if (bits & Q2CM_ANGLE3)
	{
		move->angles[2] = msg_read->ReadShort();
	}

	// read movement
	if (bits & Q2CM_FORWARD)
	{
		move->forwardmove = msg_read->ReadShort();
	}
	if (bits & Q2CM_SIDE)
	{
		move->sidemove = msg_read->ReadShort();
	}
	if (bits & Q2CM_UP)
	{
		move->upmove = msg_read->ReadShort();
	}

	// read buttons
	if (bits & Q2CM_BUTTONS)
	{
		move->buttons = msg_read->ReadByte();
	}

	if (bits & Q2CM_IMPULSE)
	{
		move->impulse = msg_read->ReadByte();
	}

	// read time to run command
	move->msec = msg_read->ReadByte();

	// read the light level
	move->lightlevel = msg_read->ReadByte();
}
