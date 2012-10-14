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

//	Dumps the current net message, prefixed by the length
void CLQ2_WriteDemoMessage(QMsg* msg, int headerBytes)
{
	int len, swlen;

	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);
	FS_Write(msg->_data + headerBytes, len, clc.demofile);
}

//	stop recording a demo
void CLQ2_Stop_f()
{
	if (!clc.demorecording)
	{
		common->Printf("Not recording a demo.\n");
		return;
	}

	// finish up
	int len = -1;
	FS_Write(&len, 4, clc.demofile);
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	clc.demorecording = false;
	common->Printf("Stopped demo.\n");
}

//	Begins recording a demo from the current position
void CLQ2_Record_f()
{
	char name[MAX_OSPATH];
	byte buf_data[MAX_MSGLEN_Q2];
	QMsg buf;
	int i;
	int len;
	q2entity_state_t* ent;
	q2entity_state_t nullstate;

	if (Cmd_Argc() != 2)
	{
		common->Printf("record <demoname>\n");
		return;
	}

	if (clc.demorecording)
	{
		common->Printf("Already recording.\n");
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	String::Sprintf(name, sizeof(name), "demos/%s.dm2", Cmd_Argv(1));

	common->Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = true;

	// don't start saving messages until a non-delta compressed message is received
	cls.q2_demowaiting = true;

	//
	// write out messages to hold the startup information
	//
	buf.InitOOB(buf_data, sizeof(buf_data));

	// send the serverdata
	buf.WriteByte(q2svc_serverdata);
	buf.WriteLong(Q2PROTOCOL_VERSION);
	buf.WriteLong(0x10000 + cl.servercount);
	buf.WriteByte(1);	// demos are always attract loops
	buf.WriteString2(cl.q2_gamedir);
	buf.WriteShort(cl.playernum);

	buf.WriteString2(cl.q2_configstrings[Q2CS_NAME]);

	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS_Q2; i++)
	{
		if (cl.q2_configstrings[i][0])
		{
			if (buf.cursize + String::Length(cl.q2_configstrings[i]) + 32 > buf.maxsize)
			{	// write it out
				len = LittleLong(buf.cursize);
				FS_Write(&len, 4, clc.demofile);
				FS_Write(buf._data, buf.cursize, clc.demofile);
				buf.cursize = 0;
			}

			buf.WriteByte(q2svc_configstring);
			buf.WriteShort(i);
			buf.WriteString2(cl.q2_configstrings[i]);
		}

	}

	// baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_EDICTS_Q2; i++)
	{
		ent = &clq2_entities[i].baseline;
		if (!ent->modelindex)
		{
			continue;
		}

		if (buf.cursize + 64 > buf.maxsize)
		{	// write it out
			len = LittleLong(buf.cursize);
			FS_Write(&len, 4, clc.demofile);
			FS_Write(buf._data, buf.cursize, clc.demofile);
			buf.cursize = 0;
		}

		buf.WriteByte(q2svc_spawnbaseline);
		MSGQ2_WriteDeltaEntity(&nullstate, &clq2_entities[i].baseline, &buf, true, true);
	}

	buf.WriteByte(q2svc_stufftext);
	buf.WriteString2("precache\n");

	// write it to the demo file

	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(buf._data, buf.cursize, clc.demofile);

	// the rest of the demo file will be individual frames
}
